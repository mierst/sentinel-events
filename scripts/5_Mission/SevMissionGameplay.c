modded class MissionGameplay
{
	protected bool m_SevInputProbe;
	protected PlayerBase m_SevInputPlayer;
	protected string m_SevInputToken;
	protected ref array<string> m_SevInputTokens = new array<string>();

	// Local diagnostic entry only; no RPC. Caller must establish these shared
	// groups are unused and ensure no other writer changes them during the trial.
	bool SevBeginInputDiagnostic(PlayerBase player, string token, bool knownUnusedBaseline)
	{
		if (GetGame().IsServer() || !knownUnusedBaseline || !player || GetGame().GetPlayer() != player || !player.SevGuardPresentation() || !SevSessionStore.ValidId(token)) return false;
		if (m_SevInputProbe) return m_SevInputPlayer == player && m_SevInputToken == token;
		if (m_SevInputTokens.Count() >= 256 || m_SevInputTokens.Find(token) >= 0) return false;
		array<string> groups = {"movement", "aiming", "menu"};
		foreach (string group : groups)
		{
			if (m_ActiveInputExcludeGroups && m_ActiveInputExcludeGroups.Find(group) >= 0) return false;
		}
		m_SevInputProbe = true;
		m_SevInputPlayer = player;
		m_SevInputToken = token;
		m_SevInputTokens.Insert(token);
		AddActiveInputExcludes(groups);
		return true;
	}

	bool SevEndInputDiagnostic(PlayerBase player, string token, bool baselineStillExclusive)
	{
		if (GetGame().IsServer() || !baselineStillExclusive || player != m_SevInputPlayer || token != m_SevInputToken || token == "") return false;
		if (!m_SevInputProbe) return true;
		RemoveActiveInputExcludes({"movement", "aiming", "menu"});
		m_SevInputProbe = false;
		return true;
	}
}
