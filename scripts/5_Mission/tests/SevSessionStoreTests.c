class SevSessionStoreTests
{
	static void Register()
	{
		// Synthetic identities only. Fixture files live below an isolated test run.
		string runId = "fixture-" + Math.RandomInt(100000, 999999).ToString();
		SevSessionStore store = new SevSessionStore();
		SevSessionManifest manifest = new SevSessionManifest();
		manifest.SchemaVersion = 2;
		manifest.RunId = runId;
		manifest.PlayerIds.Insert("SyntheticA");
		manifest.PlayerIds.Insert("SyntheticB");
		string error;
		SevRecoveryTests.Check("store-manifest", 1, store.WriteManifest(manifest, error));
		SevRecoveryTests.Check("store-manifest-immutable", 0, store.WriteManifest(manifest, error));
		SevSessionRecord first = Intent(runId, "SyntheticA");
		SevRecoveryTests.Check("store-first-write", 1, store.WriteNext(first, error));
		SevSessionRecord loaded;
		SevRecoveryTests.Check("store-missing-member", 0, store.LoadLatest(runId, "SyntheticA", loaded, error));
		SevSessionRecord second = Intent(runId, "SyntheticB");
		SevRecoveryTests.Check("store-second-member", 1, store.WriteNext(second, error));
		SevRecoveryTests.Check("store-roundtrip", 1, store.LoadLatest(runId, "SyntheticA", loaded, error));
		bool fieldsMatch = false;
		if (loaded)
			fieldsMatch = loaded.Sequence == 1 && loaded.Token == "synthetic-token" && loaded.Origin == "100 20 100" && loaded.PlayerId == "SyntheticA";
		SevRecoveryTests.Check("store-fields", 1, fieldsMatch);
		first.Sequence = 2;
		first.Phase = "PREPARED";
		first.MutationReceipt = "synthetic-mutation";
		SevRecoveryTests.Check("store-next-generation", 1, store.WriteNext(first, error));
		SevRecoveryTests.Check("store-latest", 1, store.LoadLatest(runId, "SyntheticA", loaded, error));
		bool latestMatch = false;
		if (loaded)
			latestMatch = loaded.Sequence == 2 && loaded.Phase == "PREPARED";
		SevRecoveryTests.Check("store-latest-fields", 1, latestMatch);
		SevRecoveryTests.Check("store-repeat-sequence", 0, store.WriteNext(first, error));
		first.Sequence = 3;
		first.Token = "different-token";
		SevRecoveryTests.Check("store-token-change", 0, store.WriteNext(first, error));
		SevRecoveryTests.Check("store-unsafe-run", 0, store.LoadLatest("../escape", "SyntheticA", loaded, error));
		SevRecoveryTests.Check("store-unknown-member", 0, store.LoadLatest(runId, "SyntheticC", loaded, error));
		SevRecoveryTests.Check("store-path-encoding", 1, SevSessionStore.Encode("A/a+") == "412f612b");
		string directory = "$profile:SentinelEvents/runs/" + SevSessionStore.Encode(runId) + "/p0";
		SevRecoveryTests.Check("store-keeps-previous", 1, FileExist(directory + "/g1.json"));
		WriteSynthetic(directory + "/g3.json", "{");
		SevRecoveryTests.Check("store-truncated-newer", 0, store.LoadLatest(runId, "SyntheticA", loaded, error));
		SevRecoveryTests.Check("store-failure-clears-output", 1, loaded == null && error != "");
		DeleteFile(directory + "/g3.json");
		string oversized;
		for (int index = 0; index < 16385; index++)
			oversized += "x";
		WriteSynthetic(directory + "/g3.json", oversized);
		SevRecoveryTests.Check("store-oversized", 0, store.LoadLatest(runId, "SyntheticA", loaded, error));
		DeleteFile(directory + "/g3.json");
		WriteSynthetic(directory + "/g4.json", "{}");
		SevRecoveryTests.Check("store-generation-gap", 0, store.LoadLatest(runId, "SyntheticA", loaded, error));
		DeleteFile(directory + "/g4.json");
		CopyFile(directory + "/g2.json", directory + "/g3.json");
		SevRecoveryTests.Check("store-contradictory-generation", 0, store.LoadLatest(runId, "SyntheticA", loaded, error));
		DeleteFile(directory + "/g3.json");
		SevRecoveryTests.Check("store-valid-after-fault-removal", 1, store.LoadLatest(runId, "SyntheticA", loaded, error));
		first.SchemaVersion = 2;
		first.Token = "synthetic-token";
		SevRecoveryTests.Check("store-unknown-schema", 0, store.WriteNext(first, error));
		first.SchemaVersion = 1;
		first.Origin = "0 0 0";
		SevRecoveryTests.Check("store-invalid-position", 0, store.WriteNext(first, error));
		string otherDirectory = "$profile:SentinelEvents/runs/" + SevSessionStore.Encode(runId) + "/p1";
		DeleteFile(otherDirectory + "/g1.json");
		SevRecoveryTests.Check("store-lost-member", 0, store.LoadLatest(runId, "SyntheticA", loaded, error));
		CheckLayouts();
		Print("[SEV] Store fixtures complete");
	}

	private static void CheckLayouts()
	{
		// Both IDs are synthetic, 44 characters, and differ only in letter case.
		// Run under a long profile root to reproduce the original native failure.
		string upper = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
		string lower = "aAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
		string suffix = Math.RandomInt(100000, 999999).ToString();
		string runId = "layout-fixture-" + suffix;
		SevSessionStore store = new SevSessionStore();
		SevSessionManifest manifest = new SevSessionManifest();
		manifest.SchemaVersion = 2;
		manifest.RunId = runId;
		manifest.PlayerIds.Insert(upper);
		manifest.PlayerIds.Insert(lower);
		string error;
		SevRecoveryTests.Check("store-layout-manifest", 1, store.WriteManifest(manifest, error));
		SevSessionRecord first = Intent(runId, upper);
		SevSessionRecord second = Intent(runId, lower);
		SevRecoveryTests.Check("store-layout-long-first", 1, store.WriteNext(first, error));
		SevRecoveryTests.Check("store-layout-case-second", 1, store.WriteNext(second, error));
		SevSessionRecord loaded;
		SevRecoveryTests.Check("store-layout-long-roundtrip", 1, store.LoadLatest(runId, upper, loaded, error));
		bool upperMatches = false;
		if (loaded) upperMatches = loaded.PlayerId == upper;
		SevRecoveryTests.Check("store-layout-first-identity", 1, upperMatches);
		SevRecoveryTests.Check("store-layout-case-roundtrip", 1, store.LoadLatest(runId, lower, loaded, error));
		bool lowerMatches = false;
		if (loaded) lowerMatches = loaded.PlayerId == lower;
		SevRecoveryTests.Check("store-layout-second-identity", 1, lowerMatches);
		string directory = "$profile:SentinelEvents/runs/" + SevSessionStore.Encode(runId);
		SevRecoveryTests.Check("store-layout-slot-paths", 1, FileExist(directory + "/p0/g1.json") && FileExist(directory + "/p1/g1.json"));
		DeleteFile(directory + "/p0/g1.json");
		SevRecoveryTests.Check("store-layout-wrong-slot-copy", 1, CopyFile(directory + "/p1/g1.json", directory + "/p0/g1.json"));
		SevRecoveryTests.Check("store-layout-wrong-slot-load", 0, store.LoadLatest(runId, upper, loaded, error));
		SevRecoveryTests.Check("store-layout-wrong-slot-clears", 1, loaded == null && error == "generation-fields");
		manifest.RunId = "unknown-layout-" + suffix;
		manifest.SchemaVersion = 3;
		SevRecoveryTests.Check("store-layout-unknown-version", 0, store.WriteManifest(manifest, error));
		manifest.RunId = "legacy-denied-" + suffix;
		manifest.SchemaVersion = 1;
		SevRecoveryTests.Check("store-layout-new-v1-denied", 0, store.WriteManifest(manifest, error));

		string legacyRun = "legacy-" + suffix;
		WriteLegacyFixture(legacyRun);
		SevRecoveryTests.Check("store-layout-legacy-load", 1, store.LoadLatest(legacyRun, "LegacySynthetic", loaded, error));
		bool legacyMatches = false;
		if (loaded) legacyMatches = loaded.RunId == legacyRun && loaded.PlayerId == "LegacySynthetic" && loaded.Sequence == 1;
		SevRecoveryTests.Check("store-layout-legacy-identity", 1, legacyMatches);
		SevSessionRecord legacyNext = Intent(legacyRun, "LegacySynthetic");
		legacyNext.Sequence = 2;
		legacyNext.Phase = "PREPARED";
		legacyNext.MutationReceipt = "synthetic-mutation";
		SevRecoveryTests.Check("store-layout-legacy-next", 1, store.WriteNext(legacyNext, error));
		string legacyDirectory = "$profile:SentinelEvents/runs/" + SevSessionStore.Encode(legacyRun);
		string legacyMember = legacyDirectory + "/" + SevSessionStore.Encode("LegacySynthetic");
		SevRecoveryTests.Check("store-layout-legacy-no-migration", 1, FileExist(legacyMember + "/g1.json") && FileExist(legacyMember + "/g2.json") && !FileExist(legacyDirectory + "/p0"));
	}

	private static void WriteLegacyFixture(string runId)
	{
		// Arrange the published v1 shape without using the new-manifest writer.
		SevSessionManifest manifest = new SevSessionManifest();
		manifest.SchemaVersion = 1;
		manifest.RunId = runId;
		manifest.PlayerIds.Insert("LegacySynthetic");
		string text;
		string error;
		JsonFileLoader<SevSessionManifest>.MakeData(manifest, text, error, false);
		manifest.Integrity = SevSessionStore.Fingerprint(text);
		JsonFileLoader<SevSessionManifest>.MakeData(manifest, text, error, false);
		string directory = "$profile:SentinelEvents/runs/" + SevSessionStore.Encode(runId);
		MakeDirectory(directory);
		WriteSynthetic(directory + "/manifest.json", text);
		string memberDirectory = directory + "/" + SevSessionStore.Encode("LegacySynthetic");
		MakeDirectory(memberDirectory);
		SevSessionRecord record = Intent(runId, "LegacySynthetic");
		JsonFileLoader<SevSessionRecord>.MakeData(record, text, error, false);
		record.Integrity = SevSessionStore.Fingerprint(text);
		JsonFileLoader<SevSessionRecord>.MakeData(record, text, error, false);
		WriteSynthetic(memberDirectory + "/g1.json", text);
	}

	static SevSessionRecord Intent(string runId, string playerId)
	{
		SevSessionRecord record = new SevSessionRecord();
		record.SchemaVersion = 1;
		record.Sequence = 1;
		record.RunId = runId;
		record.PlayerId = playerId;
		record.Token = "synthetic-token";
		record.Phase = "INTENT_RECORDED";
		record.Origin = "100 20 100";
		record.OriginOrientation = "0 0 0";
		record.ReturnPosition = "100 20 100";
		return record;
	}

	private static void WriteSynthetic(string path, string content)
	{
		FileHandle handle = OpenFile(path, FileMode.WRITE);
		if (handle)
		{
			FPrint(handle, content);
			CloseFile(handle);
		}
	}
}
