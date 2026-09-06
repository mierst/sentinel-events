class SevSessionStoreTests
{
	static void Register()
	{
		// Synthetic identities only. Fixture files live below an isolated test run.
		string runId = "fixture-" + Math.RandomInt(100000, 999999).ToString();
		SevSessionStore store = new SevSessionStore();
		SevSessionManifest manifest = new SevSessionManifest();
		manifest.SchemaVersion = 1;
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
		string directory = "$profile:SentinelEvents/runs/" + SevSessionStore.Encode(runId) + "/" + SevSessionStore.Encode("SyntheticA");
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
		string otherDirectory = "$profile:SentinelEvents/runs/" + SevSessionStore.Encode(runId) + "/" + SevSessionStore.Encode("SyntheticB");
		DeleteFile(otherDirectory + "/g1.json");
		SevRecoveryTests.Check("store-lost-member", 0, store.LoadLatest(runId, "SyntheticA", loaded, error));
		Print("[SEV] Store fixtures complete");
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
