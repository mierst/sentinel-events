modded class MissionGameplay
{
	protected ref SevReadySnapshot m_SevReadyState;
	protected ref SevClientRevision m_SevReadyRevision = new SevClientRevision();
	protected ref SevReadyMenu m_SevReadyMenu;
	protected float m_SevReadyReceived;
	protected bool m_SevReadyDismissed;
	protected bool m_SevReadyPending;

	override void OnInit()
	{
		super.OnInit();
		SevMissionReadySink sink = new SevMissionReadySink();
		sink.Owner = this;
		SevReadySink.Current = sink;
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(SevReadyRefresh, 1000, true);
	}

	override void OnMissionFinish()
	{
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(SevReadyRefresh);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(SevOpenReady);
		SevReadySink.Current = null;
		if (m_SevReadyMenu) m_SevReadyMenu.Close();
		m_SevReadyState = null;
		super.OnMissionFinish();
	}

	void SevReceiveReady(SevReadySnapshot snapshot)
	{
		if (!snapshot || !m_SevReadyRevision.Apply(snapshot.RunId, snapshot.Generation, snapshot.Revision, snapshot.Phase)) return;
		if (!m_SevReadyState || snapshot.Generation != m_SevReadyState.Generation)
		{
			m_SevReadyDismissed = false;
			m_SevReadyPending = true;
		}
		m_SevReadyState = snapshot;
		m_SevReadyReceived = SevCoordinator.Now();
		SevReadyRefresh();
	}

	void SevReadyRefresh()
	{
		if (!m_SevReadyState) return;
		if (m_SevReadyMenu) m_SevReadyMenu.Present(m_SevReadyState, SevReadyFresh());
		else if (m_SevReadyPending && !m_SevReadyDismissed) SevOpenReady();
	}
	bool SevReadyFresh()
	{
		return m_SevReadyState && SevCoordinator.Now() - m_SevReadyReceived < 3;
	}
	void SevOpenReady()
	{
		if (!SevReadyFresh() || m_SevReadyState.Phase != SevReadyPhase.READY || m_SevReadyState.Remaining <= 0) return;
		if (m_SevReadyMenu || GetGame().GetUIManager().GetMenu() || GetGame().GetUIManager().IsModalVisible()) return;
		m_SevReadyDismissed = false;
		m_SevReadyPending = false;
		m_SevReadyMenu = new SevReadyMenu();
		GetGame().GetUIManager().ShowScriptedMenu(m_SevReadyMenu, null);
		m_SevReadyMenu.Present(m_SevReadyState, true);
	}
	void SevDismissReady(SevReadyMenu menu)
	{
		if (menu != m_SevReadyMenu) return;
		m_SevReadyDismissed = true;
		m_SevReadyPending = false;
		m_SevReadyMenu = null;
	}
	bool SevSendReady(bool accept)
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player || !SevReadyFresh() || m_SevReadyState.Phase != SevReadyPhase.READY || m_SevReadyState.Remaining <= 0) return false;
		if (accept && player.IsInVehicle()) return false;
		ScriptRPC rpc = new ScriptRPC();
		rpc.Write(SevNet.SCHEMA); rpc.Write(m_SevReadyState.RunId);
		rpc.Write(m_SevReadyState.OfferRevision); rpc.Write(accept);
		rpc.Send(player, SevNet.READY_RESPONSE, true);
		return true;
	}
	void SevSendAdmin(int command)
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player || (command != 1 && command != 2)) return;
		ScriptRPC rpc = new ScriptRPC();
		rpc.Write(SevNet.SCHEMA); rpc.Write(command);
		rpc.Send(player, SevNet.ADMIN_REQUEST, true);
	}
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

class SevMissionReadySink : SevReadySink
{
	MissionGameplay Owner;
	override void Receive(SevReadySnapshot snapshot)
	{
		if (Owner) Owner.SevReceiveReady(snapshot);
	}
}
