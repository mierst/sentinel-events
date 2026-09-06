class SevFixtureSurface : SevSpawnSurface
{
	int Calls;
	bool Safe;
	bool FallbackOnly;

	override bool Validate(vector candidate, out vector position)
	{
		Calls++;
		position = candidate;
		position[1] = 12;
		return Safe || (FallbackOnly && candidate[0] == 800);
	}
}

class SevFixtureHandPolicy : SevHandGuardPolicy
{
	bool Deny;
	bool LocationsForwarded;
	bool Called;
	override bool Blocks(Man actor, InventoryLocation src, InventoryLocation secondSrc, InventoryLocation dst, InventoryLocation secondDst)
	{
		Called = true;
		LocationsForwarded = src && secondSrc && dst && secondDst && src != secondSrc && src != dst && src != secondDst;
		return Deny;
	}
}

class SevFixtureHandEvent : HandEventBase
{
	ref InventoryLocation SecondSource = new InventoryLocation();
	ref InventoryLocation Destination = new InventoryLocation();
	ref InventoryLocation SecondDestination = new InventoryLocation();
	void SevFixtureHandEvent(Man p = null, InventoryLocation src = null) { m_Src = new InventoryLocation(); }
	override InventoryLocation GetSecondSrc() { return SecondSource; }
	override InventoryLocation GetDst() { return Destination; }
	override InventoryLocation GetSecondDst() { return SecondDestination; }
}

class SevGuardSpawnTests
{
	static void Check(string name, bool expected, bool got)
	{
		string result = "FAIL";
		if (expected == got) result = "PASS";
		Print("[SEV] fixture " + name + ": expected=" + expected.ToString() + " got=" + got.ToString() + " " + result);
	}

	static SevHarnessConfig Config()
	{
		SevHarnessConfig config = new SevHarnessConfig();
		config.AdminIds.Insert("76561198000000001");
		config.Center = {500, 12, 500};
		config.Fallback = {800, 12, 800};
		return config;
	}

	static void Register()
	{
		SevHarnessConfig config = new SevHarnessConfig();
		Check("config-disabled", false, config.Enabled);
		Check("config-missing", false, config.Validate());
		Check("config-defaults", true, config.ReadySeconds == 120 && config.MinimumPlayers == 2 && config.MaximumPlayers == 8 && config.PreparationTimeoutSeconds == 30 && config.CountdownSeconds == 10 && config.DemonstrationSeconds == 15 && config.CombatQuietSeconds == 60 && config.CoordinatorTickMs == 250 && config.SpawnRadiusMeters == 50 && config.MinimumSeparationMeters == 10 && config.SpawnAttemptsPerPlayer == 32 && config.CandidateChecksPerTick == 8 && config.ClientStateRefreshSeconds == 1 && config.ReturnSearchRadiusMeters == 25);
		Check("config-destruction-disabled", false, SevHarnessConfig.MayDestroyInventory());
		config = Config();
		Check("config-valid", true, config.Validate());
		config.ReadySeconds = 29;
		Check("config-ready-bound", false, config.Validate());
		config = Config(); config.MaximumPlayers = 9;
		Check("config-roster-bound", false, config.Validate());
		config = Config(); config.MinimumPlayers = 7; config.MaximumPlayers = 6;
		Check("config-roster-order", false, config.Validate());
		config = Config(); config.CoordinatorTickMs = 1;
		Check("config-fixed-tick", false, config.Validate());
		config = Config(); config.CandidateChecksPerTick = 32;
		Check("config-fixed-budget", false, config.Validate());
		config = Config(); config.SpawnAttemptsPerPlayer = 33;
		Check("config-fixed-attempts", false, config.Validate());
		config = Config(); config.AdminIds[0] = "hashed-id";
		Check("config-admin-format", false, config.Validate());
		config = Config(); config.Fallback.Clear();
		Check("config-fallback-required", false, config.Validate());
		config = Config(); config.HarnessKit.Insert("M4A1");
		Check("config-kit-bounded", false, config.Validate());
		config = Config(); config.ReadySeconds = 601;
		Check("config-ready-high", false, config.Validate());
		config = Config(); config.MinimumPlayers = 1;
		Check("config-minimum-low", false, config.Validate());
		config = Config(); config.PreparationTimeoutSeconds = 4;
		Check("config-preparation-low", false, config.Validate());
		config = Config(); config.PreparationTimeoutSeconds = 121;
		Check("config-preparation-high", false, config.Validate());
		config = Config(); config.CountdownSeconds = 2;
		Check("config-countdown-low", false, config.Validate());
		config = Config(); config.CountdownSeconds = 61;
		Check("config-countdown-high", false, config.Validate());
		config = Config(); config.DemonstrationSeconds = 4;
		Check("config-demonstration-low", false, config.Validate());
		config = Config(); config.DemonstrationSeconds = 61;
		Check("config-demonstration-high", false, config.Validate());
		config = Config(); config.CombatQuietSeconds = 29;
		Check("config-quiet-low", false, config.Validate());
		config = Config(); config.CombatQuietSeconds = 301;
		Check("config-quiet-high", false, config.Validate());
		config = Config(); config.SpawnRadiusMeters = 19;
		Check("config-radius-low", false, config.Validate());
		config = Config(); config.SpawnRadiusMeters = 201;
		Check("config-radius-high", false, config.Validate());
		config = Config(); config.MinimumSeparationMeters = 4;
		Check("config-separation-low", false, config.Validate());
		config = Config(); config.MinimumSeparationMeters = 51;
		Check("config-separation-high", false, config.Validate());
		config = Config(); config.ReturnSearchRadiusMeters = 4;
		Check("config-return-radius-low", false, config.Validate());
		config = Config(); config.ReturnSearchRadiusMeters = 101;
		Check("config-return-radius-high", false, config.Validate());
		config = Config(); config.ClientStateRefreshSeconds = 2;
		Check("config-fixed-refresh", false, config.Validate());
		config = Config(); config.SchemaVersion = 2;
		Check("config-schema", false, config.Validate());
		config = Config(); config.Center[0] = -1;
		Check("config-position-invalid", false, config.Validate());
		config = Config(); config.AdminIds.Clear();
		Check("config-admin-required", false, config.Validate());
		config = Config();
		Check("config-loaded-classes", true, config.ValidateLoaded());
		Check("config-default-offers-disabled", false, config.MayOffer());

		// Valid JSON with invalid settings: load must preserve exact file bytes.
		string configPath = "$profile:sev-invalid-config-fixture.json";
		FileHandle handle = OpenFile(configPath, FileMode.WRITE);
		string invalidData = "{\"Enabled\":true,\"ReadySeconds\":0}";
		FPrint(handle, invalidData);
		CloseFile(handle);
		SevHarnessConfig loadedConfig;
		Check("config-invalid-file-rejected", false, SevHarnessConfig.Load(loadedConfig, configPath));
		handle = OpenFile(configPath, FileMode.READ);
		string afterLoad;
		ReadFile(handle, afterLoad, 1024);
		CloseFile(handle);
		Check("config-invalid-file-preserved", true, afterLoad == invalidData && !loadedConfig);
		DeleteFile(configPath);

		SevGuardLease lease = new SevGuardLease();
		Check("guard-acquire", true, lease.Acquire("player-a", "token-a", true));
		Check("guard-duplicate", true, lease.Acquire("player-a", "token-a", false));
		Check("guard-preserves-damage", true, lease.PriorAllowDamage);
		Check("guard-other-token", false, lease.Acquire("player-a", "token-b", true));
		Check("guard-other-player", false, lease.Release("player-b", "token-a"));
		Check("guard-stale-release", false, lease.Release("player-a", "token-b"));
		Check("guard-held-after-stale", true, lease.Held);
		Check("guard-release", true, lease.Release("player-a", "token-a"));
		Check("guard-duplicate-release", true, lease.Release("player-a", "token-a"));
		Check("guard-reacquire-released-token", false, lease.Acquire("player-a", "token-a", true));
		lease = new SevGuardLease();
		lease.Acquire("player-a", "token-b", false);
		Check("guard-preserves-protection", false, lease.PriorAllowDamage);
		lease = new SevGuardLease();
		Check("guard-empty-token", false, lease.Acquire("player-a", "", true));

		array<vector> reserved = new array<vector>();
		vector center = "500 12 500";
		Check("spawn-dry-clear", true, SevSpawnPlanner.Accept(true, true, true, center, reserved, 10));
		Check("spawn-shoreline", false, SevSpawnPlanner.Accept(false, true, true, center, reserved, 10));
		Check("spawn-slope", false, SevSpawnPlanner.Accept(true, false, true, center, reserved, 10));
		Check("spawn-obstacle", false, SevSpawnPlanner.Accept(true, true, false, center, reserved, 10));
		reserved.Insert("509 12 500");
		Check("spawn-minimum-distance", false, SevSpawnPlanner.Accept(true, true, true, center, reserved, 10));
		reserved[0] = "510 12 500";
		Check("spawn-separation-boundary", true, SevSpawnPlanner.Accept(true, true, true, center, reserved, 10));
		vector sampled = SevSpawnPlanner.Sample(center, 20, 0.25, 0);
		Check("spawn-area-uniform", true, Math.AbsFloat(sampled[0] - 510) < 0.001 && sampled[2] == 500);
		SevFixtureSurface surface = new SevFixtureSurface();
		SevSpawnPlanner planner = new SevSpawnPlanner(surface);
		SevSpawnSearch search = planner.BeginArena(center, 0, reserved);
		Check("spawn-zero-radius", true, search.Status == SevSpawnStatus.INVALID);
		reserved.Clear();
		search = planner.BeginArena(center, 50, reserved);
		Check("spawn-pending", true, search.Status == SevSpawnStatus.PENDING);
		Check("spawn-zero-budget", true, planner.Advance(search, 0) == 0 && surface.Calls == 0);
		Check("spawn-eight-budget", true, planner.Advance(search, 100) == 8 && surface.Calls == 8 && search.Attempts == 8);
		Check("spawn-still-pending", true, search.Status == SevSpawnStatus.PENDING);
		planner.Advance(search, 8); planner.Advance(search, 8); planner.Advance(search, 8);
		Check("spawn-attempt-cap", true, search.Status == SevSpawnStatus.EXHAUSTED && search.Attempts == 32 && surface.Calls == 32);
		vector result;
		Check("spawn-no-unsafe-last", false, search.GetResult(result));
		Check("spawn-cap-no-retry", true, planner.Advance(search, 8) == 0 && surface.Calls == 32);
		surface.Calls = 0; surface.Safe = true;
		search = planner.BeginArena(center, 50, reserved);
		planner.Advance(search, 8);
		Check("spawn-ready", true, search.GetResult(result) && result[1] == 12 && search.Attempts == 1);
		Check("spawn-ready-no-retry", true, planner.Advance(search, 8) == 0 && surface.Calls == 1);
		surface.Safe = false; surface.Calls = 0;
		search = planner.BeginReturn(center, "800 12 800");
		planner.Advance(search, 8); planner.Advance(search, 8); planner.Advance(search, 8); planner.Advance(search, 8);
		Check("spawn-fallback-failure", true, search.Status == SevSpawnStatus.EXHAUSTED && surface.Calls == 32 && !search.GetResult(result));
		surface.Calls = 0; surface.FallbackOnly = true;
		search = planner.BeginReturn(center, "800 12 800");
		planner.Advance(search, 8); planner.Advance(search, 8); planner.Advance(search, 8); planner.Advance(search, 8);
		Check("spawn-fallback-reserved-attempt", true, search.GetResult(result) && result[0] == 800 && search.Attempts == 32);
		surface.Calls = 0; surface.FallbackOnly = false;
		array<ref SevSpawnSearch> searches = new array<ref SevSpawnSearch>();
		searches.Insert(planner.BeginArena(center, 50, reserved));
		searches.Insert(planner.BeginArena(center, 50, reserved));
		Check("spawn-shared-tick-budget", true, planner.AdvanceTick(searches) == 8 && surface.Calls == 8);
		surface.Calls = 0;
		for (int requestIndex = 2; requestIndex < 16; requestIndex++) searches.Insert(planner.BeginArena(center, 50, reserved));
		Check("spawn-sixteen-shared-requests", true, planner.AdvanceTick(searches) == 8 && surface.Calls == 8);
		search = planner.BeginReturn("0 0 0", "800 12 800");
		Check("spawn-invalid-origin", true, search.Status == SevSpawnStatus.INVALID && !search.GetResult(result));
		search = planner.BeginReturn(center, "0 0 0");
		Check("spawn-invalid-fallback", true, search.Status == SevSpawnStatus.INVALID && !search.GetResult(result));
		SevMovementBaseline baseline = new SevMovementBaseline();
		Check("guard-unknown-movement-baseline", false, baseline.IsValid());
		baseline.Known = true;
		Check("guard-known-movement-baseline", true, baseline.IsValid());
		baseline.SpeedType = HumanInputControllerOverrideType.ONE_FRAME;
		Check("guard-transient-baseline-rejected", false, baseline.IsValid());
		Check("guard-movement-not-proven", false, SevEntryGuard.MovementProven());
		SevHandGuardPolicy savedHandler = SevHandGuardPolicy.Handler;
		SevFixtureHandPolicy handPolicy = new SevFixtureHandPolicy();
		SevHandGuardPolicy.Handler = handPolicy;
		SevFixtureHandEvent handEvent = new SevFixtureHandEvent();
		InventoryValidation validation = new InventoryValidation();
		validation.m_Mode = InventoryMode.JUNCTURE;
		handPolicy.Deny = true;
		Check("guard-hands-juncture-denied", false, handEvent.CanPerformEventEx(validation));
		Check("guard-hands-all-locations", true, handPolicy.LocationsForwarded);
		validation.m_Mode = InventoryMode.SERVER;
		Check("guard-hands-server-path-preserved", true, handEvent.CanPerformEventEx(validation));
		validation.m_Mode = InventoryMode.JUNCTURE;
		validation.m_IsRemote = true;
		Check("guard-hands-remote-path-preserved", true, handEvent.CanPerformEventEx(validation));
		validation.m_IsRemote = false;
		handPolicy.Deny = false;
		Check("guard-hands-unguarded-super", true, handEvent.CanPerformEventEx(validation));
		// Exercise the actual native-script subtype, whose acknowledged-completion
		// branch returns before HandEventBase. No inventory event is performed.
		HandEventTake takeEvent = new HandEventTake(null, new InventoryLocation());
		validation.m_IsJuncture = true;
		handPolicy.Deny = true;
		handPolicy.Called = false;
		Check("guard-take-completion-denied", false, takeEvent.CanPerformEventEx(validation));
		Check("guard-take-completion-policy-called", true, handPolicy.Called);
		validation.m_Mode = InventoryMode.SERVER;
		Check("guard-take-completion-server-preserved", true, takeEvent.CanPerformEventEx(validation));
		validation.m_Mode = InventoryMode.JUNCTURE;
		validation.m_IsRemote = true;
		Check("guard-take-completion-remote-preserved", true, takeEvent.CanPerformEventEx(validation));
		validation.m_IsRemote = false;
		handPolicy.Deny = false;
		Check("guard-take-completion-unguarded-super", true, takeEvent.CanPerformEventEx(validation));
		SevHandGuardPolicy.Handler = savedHandler;
		Print("[SEV] GuardSpawn fixtures complete");
	}
}
