class SevEligibility
{
	bool Connected;
	bool Alive;
	bool Unconscious;
	bool Restrained;
	bool InVehicle;
	bool QuietSatisfied;
	bool HasPendingSession;
}

class SevAdmission
{
	static bool CanAccept(SevEligibility eligibility, bool currentOffer, bool beforeDeadline)
	{
		if (!eligibility || !currentOffer || !beforeDeadline)
		{
			return false;
		}

		return eligibility.Connected && eligibility.Alive && !eligibility.Unconscious && !eligibility.Restrained && !eligibility.InVehicle && eligibility.QuietSatisfied && !eligibility.HasPendingSession;
	}

	static bool CanPrepare(int eligibleCount, int minimumCount, bool arenaValid, bool kitsValid)
	{
		return minimumCount >= 2 && eligibleCount >= minimumCount && arenaValid && kitsValid;
	}
}
