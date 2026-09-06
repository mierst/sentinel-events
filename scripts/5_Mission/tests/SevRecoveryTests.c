class SevRecoveryTests
{
	static void Register()
	{
		Check("recovery-invalid", SevRecoveryDecision.BLOCK, SevRecovery.Decide(false, true, true, false, false));
		Check("recovery-token-mismatch", SevRecoveryDecision.BLOCK, SevRecovery.Decide(true, false, true, false, false));
		Check("recovery-missing-proof", SevRecoveryDecision.BLOCK, SevRecovery.Decide(true, true, false, false, false));
		Check("recovery-untouched", SevRecoveryDecision.UNTOUCHED, SevRecovery.Decide(true, true, false, false, true));
		Check("recovery-needs-return", SevRecoveryDecision.NEEDS_RETURN, SevRecovery.Decide(true, true, true, false, false));
		Check("recovery-done", SevRecoveryDecision.DONE, SevRecovery.Decide(true, true, true, true, false));
		Check("recovery-contradictory", SevRecoveryDecision.BLOCK, SevRecovery.Decide(true, true, true, false, true));
		Check("recovery-return-without-mutation", SevRecoveryDecision.BLOCK, SevRecovery.Decide(true, true, false, true, false));
		Check("recovery-return-and-untouched", SevRecoveryDecision.BLOCK, SevRecovery.Decide(true, true, false, true, true));
		Print("[SEV] Recovery fixtures complete");
	}

	static void Check(string name, int expected, int got)
	{
		string result = "FAIL";
		if (expected == got)
			result = "PASS";
		Print("[SEV] fixture " + name + ": expected=" + expected.ToString() + " got=" + got.ToString() + " " + result);
	}
}
