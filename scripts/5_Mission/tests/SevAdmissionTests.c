class SevAdmissionTests
{
	static void Register()
	{
		SevEligibility eligibility = ValidEligibility();
		Report("eligible", true, SevAdmission.CanAccept(eligibility, true, true));

		eligibility = ValidEligibility();
		eligibility.InVehicle = true;
		Report("vehicle", false, SevAdmission.CanAccept(eligibility, true, true));

		eligibility = ValidEligibility();
		eligibility.Unconscious = true;
		Report("unconscious", false, SevAdmission.CanAccept(eligibility, true, true));

		eligibility = ValidEligibility();
		eligibility.Restrained = true;
		Report("restrained", false, SevAdmission.CanAccept(eligibility, true, true));

		eligibility = ValidEligibility();
		eligibility.QuietSatisfied = false;
		Report("recent-combat", false, SevAdmission.CanAccept(eligibility, true, true));

		eligibility = ValidEligibility();
		eligibility.HasPendingSession = true;
		Report("pending-session", false, SevAdmission.CanAccept(eligibility, true, true));

		eligibility = ValidEligibility();
		Report("stale-offer", false, SevAdmission.CanAccept(eligibility, false, true));

		eligibility = ValidEligibility();
		Report("deadline-equality", false, SevAdmission.CanAccept(eligibility, true, false));

		Report("below-minimum", false, SevAdmission.CanPrepare(1, 2, true, true));
		Report("invalid-arena", false, SevAdmission.CanPrepare(2, 2, false, true));
		Report("invalid-kit", false, SevAdmission.CanPrepare(2, 2, true, false));
	}

	private static SevEligibility ValidEligibility()
	{
		SevEligibility eligibility = new SevEligibility();
		eligibility.Connected = true;
		eligibility.Alive = true;
		eligibility.QuietSatisfied = true;
		return eligibility;
	}

	private static void Report(string name, bool expected, bool got)
	{
		string expectedValue = "false";
		string gotValue = "false";
		string result = "FAIL";

		if (expected)
		{
			expectedValue = "true";
		}

		if (got)
		{
			gotValue = "true";
		}

		if (expected == got)
		{
			result = "PASS";
		}

		Print("[SEV] fixture " + name + ": expected=" + expectedValue + " got=" + gotValue + " " + result);
	}
}
