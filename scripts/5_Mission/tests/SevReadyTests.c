class SevReadyTests
{
	static void Register()
	{
		Check("net-server-ready", true, SevNet.Direction(734211, true, true));
		Check("net-server-admin", true, SevNet.Direction(734213, true, true));
		Check("net-server-offer-denied", false, SevNet.Direction(734210, true, true));
		Check("net-server-state-denied", false, SevNet.Direction(734212, true, true));
		Check("net-server-missing-sender", false, SevNet.Direction(734211, true, false));
		Check("net-client-offer", true, SevNet.Direction(734210, false, false));
		Check("net-client-state", true, SevNet.Direction(734212, false, false));
		Check("net-client-request-denied", false, SevNet.Direction(734211, false, false));
		Check("net-client-sender-denied", false, SevNet.Direction(734210, false, true));
		Check("net-unknown-opcode", false, SevNet.Direction(734214, true, true));
		Check("net-envelope", true, SevNet.Envelope(1, "run-1", 1));
		Check("net-schema-denied", false, SevNet.Envelope(2, "run-1", 1));
		Check("net-empty-run", false, SevNet.Envelope(1, "", 1));
		Check("net-zero-revision", false, SevNet.Envelope(1, "run-1", 0));
		SevRequestBudget budget = new SevRequestBudget();
		Check("net-rate-first", true, budget.Take(100));
		budget.Take(100);
		budget.Take(100);
		Check("net-rate-burst-four", true, budget.Take(100));
		Check("net-rate-burst-five-denied", false, budget.Take(100));
		Check("net-rate-too-soon", false, budget.Take(100.49));
		Check("net-rate-refill", true, budget.Take(100.5));
		Check("net-rate-backwards", false, budget.Take(99));

		SevReadyRun run = new SevReadyRun();
		Check("ready-nonadmin-start", false, run.Start(false, "run-1", 100, 30));
		Check("ready-admin-start", true, run.Start(true, "run-1", 100, 30));
		Check("ready-active-start-denied", false, run.Start(true, "run-2", 100, 30));
		SevEligibility eligibility = Eligible();
		Check("ready-stale-run", false, run.Respond("a", "old", 1, true, 101, eligibility));
		Check("ready-stale-revision", false, run.Respond("a", "run-1", 2, true, 101, eligibility));
		Check("ready-valid-accept", true, run.Respond("a", "run-1", 1, true, 101, eligibility));
		Check("ready-duplicate-accept", true, run.Respond("a", "run-1", 1, true, 102, eligibility));
		Check("ready-duplicate-count", true, run.Accepted.Count() == 1);
		Check("ready-exact-deadline", false, run.Respond("b", "run-1", 1, true, 130, eligibility));
		eligibility.InVehicle = true;
		Check("ready-changed-eligibility", false, run.Respond("b", "run-1", 1, true, 103, eligibility));
		Check("ready-early-close", false, run.Close(129, 2, 2));
		Check("ready-minimum-after-change", false, run.Close(130, 1, 2));
		Check("ready-below-minimum-report", true, run.Phase == SevReadyPhase.REPORT);
		Check("ready-closed-response", false, run.Respond("b", "run-1", 1, true, 131, Eligible()));
		Check("ready-nonadmin-cancel", false, run.Cancel(false));
		Check("ready-cancel", true, run.Cancel(true));
		int revision = run.Revision;
		Check("ready-cancel-twice", true, run.Cancel(true));
		Check("ready-cancel-idempotent", true, run.Revision == revision);
		Check("ready-next-run", true, run.Start(true, "run-2", 140, 30));
		Check("ready-old-callback", false, run.CurrentCallback("run-1", 1));
		run.Respond("a", "run-2", 1, true, 141, Eligible());
		run.Respond("b", "run-2", 1, true, 141, Eligible());
		Check("ready-valid-preflight", true, run.Close(170, 2, 2));
		Check("ready-preflight-phase", true, run.Phase == SevReadyPhase.PREFLIGHT);
		Check("ready-current-callback", true, run.CurrentCallback("run-2", run.Revision));
		Check("ready-stale-callback-revision", false, run.CurrentCallback("run-2", run.Revision - 1));
		Check("ready-preflight-response-denied", false, run.Respond("c", "run-2", 1, true, 169, Eligible()));
		run.Cancel(true);
		Check("ready-cancelled-callback", false, run.CurrentCallback("run-2", run.Revision));
		run.Start(true, "run-3", 200, 30);
		run.Respond("a", "run-3", 1, true, 201, Eligible());
		Check("ready-decline-after-accept", true, run.Respond("a", "run-3", 1, false, 202, null));
		Check("ready-decline-removes-count", true, run.Accepted.Count() == 0);
		Check("ready-missing-eligibility", false, run.Respond("a", "run-3", 1, true, 203, null));
		eligibility = Eligible(); eligibility.HasPendingSession = true;
		Check("ready-recovery-blocked", false, run.Respond("a", "run-3", 1, true, 203, eligibility));
		for (int index = 0; index < 8; index++) run.Respond("member-" + index.ToString(), "run-3", 1, true, 204, Eligible());
		Check("ready-eight-member-cap", true, run.Accepted.Count() == 8);
		Check("ready-ninth-member-denied", false, run.Respond("ninth", "run-3", 1, true, 205, Eligible()));
		Check("ready-ninth-count-unchanged", true, run.Accepted.Count() == 8);

		SevClientRevision client = new SevClientRevision();
		Check("ready-client-first", true, client.Apply("run-1", 1, 1, SevReadyPhase.READY));
		Check("ready-client-transition", true, client.Apply("run-1", 1, 2, SevReadyPhase.CANCELLED));
		Check("ready-client-replayed-offer", false, client.Apply("run-1", 1, 1, SevReadyPhase.READY));
		Check("ready-client-next-run", true, client.Apply("run-2", 2, 1, SevReadyPhase.READY));
		Check("ready-client-old-run", false, client.Apply("run-1", 1, 9, SevReadyPhase.READY));
		Check("ready-client-equal-refresh", true, client.Apply("run-2", 2, 1, SevReadyPhase.READY));
		Check("ready-client-same-generation-other-run", false, client.Apply("other", 2, 2, SevReadyPhase.READY));
		Check("ready-client-same-revision-other-phase", false, client.Apply("run-2", 2, 1, SevReadyPhase.REPORT));
		Check("ready-client-zero-generation", false, client.Apply("run-3", 0, 1, SevReadyPhase.READY));
		Print("[SEV] Ready fixtures complete");
	}

	static SevEligibility Eligible()
	{
		SevEligibility eligibility = new SevEligibility();
		eligibility.Connected = true;
		eligibility.Alive = true;
		eligibility.QuietSatisfied = true;
		return eligibility;
	}

	static void Check(string name, bool expected, bool got)
	{
		string result = "FAIL";
		if (expected == got) result = "PASS";
		Print("[SEV] fixture " + name + ": expected=" + expected.ToString() + " got=" + got.ToString() + " " + result);
	}
}
