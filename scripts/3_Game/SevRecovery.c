enum SevRecoveryDecision
{
	UNTOUCHED,
	NEEDS_RETURN,
	DONE,
	BLOCK
}

class SevRecovery
{
	static int Decide(bool recordValid, bool tokensMatch, bool mutationProven, bool returnProven, bool untouchedProven)
	{
		if (!recordValid || !tokensMatch)
			return SevRecoveryDecision.BLOCK;
		if (untouchedProven && (mutationProven || returnProven))
			return SevRecoveryDecision.BLOCK;
		if (returnProven && !mutationProven)
			return SevRecoveryDecision.BLOCK;
		if (mutationProven && returnProven)
			return SevRecoveryDecision.DONE;
		if (mutationProven)
			return SevRecoveryDecision.NEEDS_RETURN;
		if (untouchedProven)
			return SevRecoveryDecision.UNTOUCHED;
		return SevRecoveryDecision.BLOCK;
	}
}
