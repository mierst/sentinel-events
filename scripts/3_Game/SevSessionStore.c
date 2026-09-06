class SevSessionStore
{
	static const int MAX_GENERATIONS = 32;
	static const int MAX_BYTES = 16384;
	static const int LEGACY_MANIFEST_SCHEMA = 1;
	static const int CURRENT_MANIFEST_SCHEMA = 2;
	static const string ROOT = "$profile:SentinelEvents/runs/";

	static bool HasObjectEnvelope(string text)
	{
		if (text.Length() < 2) return false;
		return text.Substring(0, 1) == "{" && text.Substring(text.Length() - 1, 1) == "}";
	}

	// Hex preserves case and is collision-free for the validated ASCII alphabet.
	static string Encode(string value)
	{
		string digits = "0123456789abcdef";
		string encoded;
		for (int index = 0; index < value.Length(); index++)
		{
			int code = value.Substring(index, 1).ToAscii();
			int high = code / 16;
			encoded += digits.Substring(high, 1) + digits.Substring(code % 16, 1);
		}
		return encoded;
	}

	static bool ValidId(string value, bool player = false)
	{
		if (value.Length() < 1 || value.Length() > 64)
			return false;
		string alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-";
		if (player)
			alphabet += "+/=";
		for (int index = 0; index < value.Length(); index++)
		{
			if (alphabet.IndexOf(value.Substring(index, 1)) < 0)
				return false;
		}
		return true;
	}

	// Accidental-corruption detector, NOT authentication or durable storage proof.
	static string Fingerprint(string value)
	{
		int first = 1;
		int second = 0;
		for (int index = 0; index < value.Length(); index++)
		{
			first = (first + value.Substring(index, 1).ToAscii()) % 65521;
			second = (second + first) % 65521;
		}
		return value.Length().ToString() + "-" + first.ToString() + "-" + second.ToString();
	}

	static bool ValidVector(vector value, float limit)
	{
		for (int axis = 0; axis < 3; axis++)
		{
			// Positive finite range check also rejects NaN/infinity.
			if (!(value[axis] >= -limit && value[axis] <= limit))
				return false;
		}
		return true;
	}

	static int PhaseOrder(string phase)
	{
		if (phase == "INTENT_RECORDED") return 1;
		if (phase == "MUTATING") return 2;
		if (phase == "PREPARED") return 3;
		if (phase == "RETURNING") return 4;
		if (phase == "RETURNED") return 5;
		return 0;
	}

	static bool ValidRecord(SevSessionRecord record)
	{
		if (!record || record.SchemaVersion != 1)
			return false;
		if (record.Sequence < 1 || record.Sequence > MAX_GENERATIONS)
			return false;
		if (!ValidId(record.RunId) || !ValidId(record.PlayerId, true) || !ValidId(record.Token))
			return false;
		if (!ValidVector(record.Origin, 100000) || record.Origin == vector.Zero)
			return false;
		if (!ValidVector(record.ReturnPosition, 100000) || record.ReturnPosition == vector.Zero)
			return false;
		if (!ValidVector(record.OriginOrientation, 360))
			return false;
		int phase = PhaseOrder(record.Phase);
		if (phase == 0)
			return false;
		if (phase <= 2 && (record.MutationReceipt != "" || record.ReturnReceipt != ""))
			return false;
		if (phase >= 3 && !ValidId(record.MutationReceipt))
			return false;
		if (phase < 5 && record.ReturnReceipt != "")
			return false;
		if (phase == 5 && !ValidId(record.ReturnReceipt))
			return false;
		if (record.Integrity.Length() > 64 || record.PreviousIntegrity.Length() > 64)
			return false;
		return true;
	}

	private bool Fail(string code, out string error)
	{
		error = code;
		return false;
	}

	private bool ValidManifest(SevSessionManifest manifest)
	{
		if (!manifest || !ValidId(manifest.RunId))
			return false;
		if (manifest.SchemaVersion != LEGACY_MANIFEST_SCHEMA && manifest.SchemaVersion != CURRENT_MANIFEST_SCHEMA)
			return false;
		if (!manifest.PlayerIds || manifest.PlayerIds.Count() < 1 || manifest.PlayerIds.Count() > 8)
			return false;
		if (manifest.Integrity.Length() > 64)
			return false;
		for (int index = 0; index < manifest.PlayerIds.Count(); index++)
		{
			string identity = manifest.PlayerIds[index];
			if (!ValidId(identity, true))
				return false;
			for (int previous = 0; previous < index; previous++)
			{
				if (manifest.PlayerIds[previous] == identity)
					return false;
			}
		}
		return true;
	}

	private bool SessionDirectory(SevSessionManifest manifest, string playerId, out string directory, out string error)
	{
		directory = "";
		if (!ValidManifest(manifest)) return Fail("manifest-invalid", error);
		int memberIndex = -1;
		for (int index = 0; index < manifest.PlayerIds.Count(); index++)
		{
			// Exact identity comparison; never normalize case or accept a slot
			// from a request. The immutable validated order owns the namespace.
			if (manifest.PlayerIds[index] == playerId)
			{
				memberIndex = index;
				break;
			}
		}
		if (memberIndex < 0) return Fail("member-unknown", error);
		directory = ROOT + Encode(manifest.RunId) + "/";
		if (manifest.SchemaVersion == LEGACY_MANIFEST_SCHEMA)
			directory += Encode(playerId);
		else
			directory += "p" + memberIndex.ToString();
		// There is no alternate-path fallback or implicit migration. Native
		// directory/file failures still block unusually long profile/run paths.
		return true;
	}

	private bool ManifestText(SevSessionManifest manifest, out string text, out string error)
	{
		string saved = manifest.Integrity;
		manifest.Integrity = "";
		string payload;
		bool made = JsonFileLoader<SevSessionManifest>.MakeData(manifest, payload, error, false);
		manifest.Integrity = saved;
		if (!made) return Fail("manifest-serialize", error);
		manifest.Integrity = Fingerprint(payload);
		return JsonFileLoader<SevSessionManifest>.MakeData(manifest, text, error, false);
	}

	private bool RecordText(SevSessionRecord record, out string text, out string error)
	{
		string saved = record.Integrity;
		record.Integrity = "";
		string payload;
		bool made = JsonFileLoader<SevSessionRecord>.MakeData(record, payload, error, false);
		record.Integrity = saved;
		if (!made) return Fail("record-serialize", error);
		record.Integrity = Fingerprint(payload);
		return JsonFileLoader<SevSessionRecord>.MakeData(record, text, error, false);
	}

	private bool ReadBounded(string path, out string text, out string error)
	{
		text = "";
		FileHandle handle = OpenFile(path, FileMode.READ);
		if (!handle) return Fail("file-missing-or-unreadable", error);
		int count = ReadFile(handle, text, MAX_BYTES + 1);
		CloseFile(handle);
		if (count < 1 || count > MAX_BYTES || text.Length() != count)
			return Fail("file-size-or-encoding", error);
		// Generated records are single canonical root objects. Do not send a
		// torn write to the native parser, which logs SCRIPT(E) on truncation.
		if (!HasObjectEnvelope(text)) return Fail("file-incomplete-envelope", error);
		return true;
	}

	private bool WriteFresh(string path, string text, out string error)
	{
		if (text.Length() < 1 || text.Length() > MAX_BYTES)
			return Fail("file-size", error);
		if (FileExist(path)) return Fail("file-already-exists", error);
		FileHandle handle = OpenFile(path, FileMode.WRITE);
		if (!handle) return Fail("file-create", error);
		FPrint(handle, text);
		CloseFile(handle);
		string readback;
		if (!ReadBounded(path, readback, error)) return false;
		if (readback != text) return Fail("readback-mismatch", error);
		return true;
	}

	bool WriteManifest(SevSessionManifest manifest, out string error)
	{
		error = "";
		if (!ValidManifest(manifest)) return Fail("manifest-invalid", error);
		if (manifest.SchemaVersion != CURRENT_MANIFEST_SCHEMA) return Fail("manifest-write-version", error);
		MakeDirectory("$profile:SentinelEvents");
		MakeDirectory("$profile:SentinelEvents/runs");
		string directory = ROOT + Encode(manifest.RunId);
		// A pre-existing run is never silently adopted, even without a manifest.
		if (FileExist(directory)) return Fail("run-already-exists", error);
		if (!MakeDirectory(directory)) return Fail("run-create", error);
		string text;
		if (!ManifestText(manifest, text, error)) return false;
		if (!WriteFresh(directory + "/manifest.json", text, error)) return false;
		SevSessionManifest loaded;
		return ReadManifest(manifest.RunId, loaded, error);
	}

	private bool ReadManifest(string runId, out SevSessionManifest manifest, out string error)
	{
		manifest = null;
		if (!ValidId(runId)) return Fail("run-invalid", error);
		string text;
		if (!ReadBounded(ROOT + Encode(runId) + "/manifest.json", text, error)) return false;
		SevSessionManifest candidate = new SevSessionManifest();
		string parseError;
		if (!JsonFileLoader<SevSessionManifest>.LoadData(text, candidate, parseError)) return Fail("manifest-json", error);
		if (!ValidManifest(candidate) || candidate.RunId != runId) return Fail("manifest-fields", error);
		string supplied = candidate.Integrity;
		string canonical;
		if (!ManifestText(candidate, canonical, error)) return false;
		if (supplied != candidate.Integrity || canonical != text) return Fail("manifest-integrity-or-schema", error);
		manifest = candidate;
		return true;
	}

	private bool SameSession(SevSessionRecord previous, SevSessionRecord next)
	{
		if (previous.Token != next.Token || previous.RunId != next.RunId || previous.PlayerId != next.PlayerId)
			return false;
		if (previous.Origin != next.Origin || previous.OriginOrientation != next.OriginOrientation || previous.ReturnPosition != next.ReturnPosition)
			return false;
		if (PhaseOrder(next.Phase) < PhaseOrder(previous.Phase) || previous.Phase == "RETURNED")
			return false;
		if (previous.MutationReceipt != "" && previous.MutationReceipt != next.MutationReceipt)
			return false;
		return true;
	}

	private bool ReadSession(SevSessionManifest manifest, string playerId, out SevSessionRecord record, out string error)
	{
		record = null;
		string directory;
		if (!SessionDirectory(manifest, playerId, directory, error)) return false;
		string runId = manifest.RunId;
		string name;
		FileAttr attributes;
		FindFileHandle search = FindFile(directory + "/*", name, attributes, FindFileFlags.DIRECTORIES);
		if (!search) return Fail("session-missing", error);
		int count = 0;
		int highest = 0;
		bool invalid = false;
		bool more = true;
		while (more)
		{
			if (name != "." && name != ".." && name != "")
			{
				count++;
				int found = 0;
				for (int sequence = 1; sequence <= MAX_GENERATIONS; sequence++)
				{
					if (name == "g" + sequence.ToString() + ".json") found = sequence;
				}
				if (found == 0 || count > MAX_GENERATIONS)
				{
					invalid = true;
					break;
				}
				if (found > highest) highest = found;
			}
			more = FindNextFile(search, name, attributes);
		}
		CloseFindFile(search);
		if (invalid || count == 0 || count != highest) return Fail("generation-set", error);
		SevSessionRecord previous;
		for (int current = 1; current <= highest; current++)
		{
			string text;
			if (!ReadBounded(directory + "/g" + current.ToString() + ".json", text, error)) return false;
			SevSessionRecord candidate = new SevSessionRecord();
			string parseError;
			if (!JsonFileLoader<SevSessionRecord>.LoadData(text, candidate, parseError)) return Fail("generation-json", error);
			if (!ValidRecord(candidate) || candidate.Sequence != current || candidate.RunId != runId || candidate.PlayerId != playerId)
				return Fail("generation-fields", error);
			string supplied = candidate.Integrity;
			string canonical;
			if (!RecordText(candidate, canonical, error)) return false;
			if (supplied != candidate.Integrity || canonical != text) return Fail("generation-integrity-or-schema", error);
			if (current == 1)
			{
				if (candidate.PreviousIntegrity != "" || candidate.Phase != "INTENT_RECORDED") return Fail("first-generation", error);
			}
			else
			{
				if (!SameSession(previous, candidate) || candidate.PreviousIntegrity != previous.Integrity) return Fail("generation-chain", error);
			}
			previous = candidate;
		}
		record = previous;
		return true;
	}

	bool WriteNext(SevSessionRecord record, out string error)
	{
		error = "";
		if (!ValidRecord(record)) return Fail("record-invalid", error);
		SevSessionManifest manifest;
		if (!ReadManifest(record.RunId, manifest, error)) return false;
		string directory;
		if (!SessionDirectory(manifest, record.PlayerId, directory, error)) return false;
		if (record.Sequence == 1)
		{
			if (record.Phase != "INTENT_RECORDED" || FileExist(directory)) return Fail("first-generation-conflict", error);
			if (!MakeDirectory(directory)) return Fail("session-create", error);
			record.PreviousIntegrity = "";
		}
		else
		{
			SevSessionRecord previous;
			if (!LoadLatest(record.RunId, record.PlayerId, previous, error)) return false;
			if (record.Sequence != previous.Sequence + 1 || !SameSession(previous, record)) return Fail("next-generation-conflict", error);
			record.PreviousIntegrity = previous.Integrity;
		}
		string text;
		if (!RecordText(record, text, error)) return false;
		if (SevPersistenceProbe.Hit("before-generation-write")) return Fail("diagnostic-before-write", error);
		string path = directory + "/g" + record.Sequence.ToString() + ".json";
		if (SevPersistenceProbe.Hit("partial-generation"))
		{
			WriteFresh(path, "{", error);
			return Fail("diagnostic-partial-write", error);
		}
		if (!WriteFresh(path, text, error)) return false;
		SevSessionRecord readback;
		if (!ReadSession(manifest, record.PlayerId, readback, error)) return false;
		if (SevPersistenceProbe.Hit("after-generation-readback")) return Fail("diagnostic-after-readback", error);
		return true;
	}

	bool LoadLatest(string runId, string playerId, out SevSessionRecord record, out string error)
	{
		record = null;
		error = "";
		if (!ValidId(runId) || !ValidId(playerId, true)) return Fail("identity-invalid", error);
		SevSessionManifest manifest;
		if (!ReadManifest(runId, manifest, error)) return false;
		if (manifest.PlayerIds.Find(playerId) < 0) return Fail("member-unknown", error);
		SevSessionRecord selected;
		foreach (string member : manifest.PlayerIds)
		{
			SevSessionRecord candidate;
			if (!ReadSession(manifest, member, candidate, error)) return false;
			if (member == playerId) selected = candidate;
		}
		record = selected;
		return true;
	}
}
