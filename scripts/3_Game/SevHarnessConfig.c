class SevHarnessConfig
{
	int SchemaVersion = 1;
	bool Enabled = false;
	ref array<string> AdminIds = new array<string>();
	ref array<float> Center = new array<float>();
	ref array<float> Fallback = new array<float>();
	int ReadySeconds = 120;
	int MinimumPlayers = 2;
	int MaximumPlayers = 8;
	int PreparationTimeoutSeconds = 30;
	int CountdownSeconds = 10;
	int DemonstrationSeconds = 15;
	int CombatQuietSeconds = 60;
	int CoordinatorTickMs = 250;
	float SpawnRadiusMeters = 50;
	float MinimumSeparationMeters = 10;
	int SpawnAttemptsPerPlayer = 32;
	int CandidateChecksPerTick = 8;
	int ClientStateRefreshSeconds = 1;
	float ReturnSearchRadiusMeters = 25;
	ref array<string> HarnessKit = {"TShirt_White", "Jeans_Blue", "AthleticShoes_Black"};
	ref array<string> ReturnClothing = {"TShirt_White", "Jeans_Blue", "AthleticShoes_Black"};

	static bool Between(float value, float low, float high)
	{
		return value >= low && value <= high;
	}

	static bool ValidPosition(array<float> coordinates)
	{
		if (!coordinates || coordinates.Count() != 3) return false;
		return Between(coordinates[0], 0.01, 100000) && Between(coordinates[2], 0.01, 100000) && Between(coordinates[1], -1000, 10000);
	}

	static bool ValidSteamId(string identity)
	{
		if (identity.Length() != 17 || identity.Substring(0, 7) != "7656119") return false;
		for (int index = 0; index < identity.Length(); index++)
		{
			if ("0123456789".IndexOf(identity.Substring(index, 1)) < 0) return false;
		}
		return true;
	}

	static bool ValidKit(array<string> kit)
	{
		// The rehearsal has only the specified three clothing slots, no cargo.
		return kit && kit.Count() == 3 && kit[0] == "TShirt_White" && kit[1] == "Jeans_Blue" && kit[2] == "AthleticShoes_Black";
	}

	bool Validate()
	{
		if (SchemaVersion != 1 || !ValidPosition(Center) || !ValidPosition(Fallback)) return false;
		if (!AdminIds || AdminIds.Count() < 1 || AdminIds.Count() > 32) return false;
		foreach (string identity : AdminIds)
		{
			if (!ValidSteamId(identity)) return false;
		}
		if (!Between(ReadySeconds, 30, 600) || !Between(MinimumPlayers, 2, 8) || !Between(MaximumPlayers, MinimumPlayers, 8)) return false;
		if (!Between(PreparationTimeoutSeconds, 5, 120) || !Between(CountdownSeconds, 3, 60) || !Between(DemonstrationSeconds, 5, 60)) return false;
		if (!Between(CombatQuietSeconds, 30, 300) || CoordinatorTickMs != 250 || ClientStateRefreshSeconds != 1) return false;
		if (!Between(SpawnRadiusMeters, 20, 200) || !Between(MinimumSeparationMeters, 5, 50) || !Between(ReturnSearchRadiusMeters, 5, 100)) return false;
		if (SpawnAttemptsPerPlayer != 32 || CandidateChecksPerTick != 8) return false;
		return ValidKit(HarnessKit) && ValidKit(ReturnClothing);
	}

	bool ValidateLoaded()
	{
		if (!GetGame().IsServer() || !Validate()) return false;
		int worldSize = GetGame().GetWorld().GetWorldSize();
		if (Center[0] >= worldSize || Center[2] >= worldSize || Fallback[0] >= worldSize || Fallback[2] >= worldSize) return false;
		foreach (string classname : HarnessKit)
		{
			if (!GetGame().ConfigIsExisting("CfgVehicles " + classname)) return false;
		}
		return true;
	}

	bool MayOffer() { return Enabled && ValidateLoaded(); }
	// No native safety/persistence trial can be inferred from a config flag.
	static bool MayDestroyInventory() { return false; }

	bool IsAdmin(PlayerIdentity identity)
	{
		if (!GetGame().IsServer() || !identity || !MayOffer()) return false;
		return AdminIds.Find(identity.GetPlainId()) >= 0;
	}

	static bool Load(out SevHarnessConfig config, string path = "$profile:SentinelEvents/config.json")
	{
		config = null;
		if (!GetGame().IsServer()) return false;
		FileHandle handle = OpenFile(path, FileMode.READ);
		if (!handle) return false;
		string data;
		int count = ReadFile(handle, data, 16385);
		CloseFile(handle);
		if (count < 2 || count > 16384 || data.Length() != count) return false;
		SevHarnessConfig candidate = new SevHarnessConfig();
		string error;
		if (!JsonFileLoader<SevHarnessConfig>.LoadData(data, candidate, error) || !candidate.ValidateLoaded()) return false;
		config = candidate;
		return true;
	}
}
