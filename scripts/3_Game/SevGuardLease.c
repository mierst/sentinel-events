// Pure token/identity lease policy. Native side effects belong to SevEntryGuard.
class SevGuardLease
{
	string Identity;
	string Token;
	bool PriorAllowDamage;
	bool Held;

	bool Acquire(string identity, string token, bool priorAllowDamage)
	{
		if (!SevSessionStore.ValidId(identity, true) || !SevSessionStore.ValidId(token)) return false;
		if (Token != "") return Held && Identity == identity && Token == token;
		Identity = identity;
		Token = token;
		PriorAllowDamage = priorAllowDamage;
		Held = true;
		return true;
	}

	bool Release(string identity, string token)
	{
		if (Token == "" || Identity != identity || Token != token) return false;
		Held = false;
		return true;
	}
}
