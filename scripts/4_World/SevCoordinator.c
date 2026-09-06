class SevReadyMember
{
	string Identity;
	PlayerBase Character;
	PlayerBase Recipient;
	int Status;
	int Reason;
}

// No recovery clear proof exists yet. The observation must remain UNKNOWN even
// for absent markers; no journal scan, save or guessed untouched state is used.
class SevReadyObservation
{
	SevEligibility Observe(PlayerBase player, string identity, int quietSeconds, out int reason)
	{
		SevEligibility eligibility = new SevEligibility();
		reason = SevReadyReason.DISCONNECTED;
		if (!player || !player.GetIdentity() || player.GetIdentity().GetId() != identity || !player.SevReadyConnected()) return eligibility;
		eligibility.Connected = true;
		eligibility.Alive = player.IsAlive();
		eligibility.InVehicle = player.IsInVehicle();
		eligibility.Unconscious = player.IsUnconscious();
		eligibility.Restrained = player.IsRestrained();
		eligibility.QuietSatisfied = player.SevReadyQuiet(quietSeconds);
		eligibility.HasPendingSession = true;
		reason = player.SevReadyRecoveryReason();
		if (!eligibility.Alive) reason = SevReadyReason.DEAD;
		else if (eligibility.InVehicle) reason = SevReadyReason.VEHICLE;
		else if (eligibility.Unconscious) reason = SevReadyReason.UNCONSCIOUS;
		else if (eligibility.Restrained) reason = SevReadyReason.RESTRAINED;
		else if (!eligibility.QuietSatisfied) reason = SevReadyReason.QUIET;
		return eligibility;
	}
}

class SevCoordinator
{
	static ref SevCoordinator Current;
	protected ref SevHarnessConfig m_Config;
	protected ref SevReadyRun m_Run = new SevReadyRun();
	protected ref SevReadyObservation m_Observation = new SevReadyObservation();
	protected ref array<ref SevReadyMember> m_Members = new array<ref SevReadyMember>();
	protected ref array<ref SevReadyMember> m_Roster = new array<ref SevReadyMember>();
	protected ref SevSpawnPlanner m_Planner;
	protected ref array<ref SevSpawnSearch> m_Searches = new array<ref SevSpawnSearch>();
	protected ref array<vector> m_Reserved = new array<vector>();
	protected int m_Generation;
	protected int m_BootNonce;
	protected int m_PreflightMember;
	protected float m_NextRefresh;
	protected float m_PreflightDeadline;
	protected string m_SearchRun;
	protected int m_SearchRevision;
	protected int m_ReportReason;

	void SevCoordinator(SevHarnessConfig config)
	{
		m_Config = config;
		m_BootNonce = Math.RandomInt(1, 1000000000);
		if (config) m_Planner = new SevSpawnPlanner(new SevNativeSpawnSurface(), config.MinimumSeparationMeters, config.ReturnSearchRadiusMeters);
	}
	static float Now() { return GetGame().GetTime() * 0.001; }
	void TimerTick() { Tick(Now()); }
	protected SevReadyMember Find(string identity)
	{
		foreach (SevReadyMember member : m_Members)
		{
			if (member.Identity == identity) return member;
		}
		return null;
	}
	void StartRehearsal(PlayerIdentity sender)
	{
		if (!GetGame().IsServer() || !m_Config || !m_Config.IsAdmin(sender) || m_Generation >= 999999998) return;
		if (m_Run.Phase == SevReadyPhase.READY || m_Run.Phase == SevReadyPhase.PREFLIGHT) return;
		array<Man> players = new array<Man>();
		GetGame().GetPlayers(players);
		// Native GetPlayers materializes its connected list before this script cap.
		// No offer allocation or per-player work proceeds above the private cap.
		if (players.Count() > 64) return;
		float now = Now();
		m_Generation++;
		string runId = "ready-" + m_BootNonce.ToString() + "-" + m_Generation.ToString() + "-" + GetGame().GetTime().ToString();
		if (!m_Run.Start(true, runId, now, m_Config.ReadySeconds)) return;
		m_Members.Clear(); m_Roster.Clear(); m_Searches.Clear(); m_Reserved.Clear();
		m_ReportReason = SevReadyReason.NONE;
		foreach (Man man : players)
		{
			PlayerBase player = PlayerBase.Cast(man);
			if (!player || !player.GetIdentity() || !SevSessionStore.ValidId(player.GetIdentity().GetId(), true)) continue;
			SevReadyMember member = new SevReadyMember();
			member.Identity = player.GetIdentity().GetId();
			member.Character = player;
			member.Recipient = player;
			m_Members.Insert(member);
		}
		PushAll(true);
		m_NextRefresh = now + 1;
	}
	void Respond(PlayerIdentity sender, string runId, int revision, bool accept)
	{
		if (!GetGame().IsServer() || !sender || !m_Config) return;
		SevReadyMember member = Find(sender.GetId());
		float now = Now();
		if (!member || !member.Character || member.Character.GetIdentity() != sender) return;
		if (runId != m_Run.RunId || revision != m_Run.OfferRevision || m_Run.Phase != SevReadyPhase.READY || now >= m_Run.Deadline) return;
		int reason;
		SevEligibility eligibility = m_Observation.Observe(member.Character, member.Identity, m_Config.CombatQuietSeconds, reason);
		if (accept && m_Run.Accepted.Find(member.Identity) < 0 && m_Run.Accepted.Count() >= m_Config.MaximumPlayers)
		{
			Exclude(member, SevReadyReason.FULL);
		}
		else if (m_Run.Respond(member.Identity, runId, revision, accept, now, eligibility))
		{
			int status = SevReadyStatus.DECLINED;
			if (accept) status = SevReadyStatus.ACCEPTED;
			if (member.Status != status || member.Reason != SevReadyReason.NONE) m_Run.Revision++;
			member.Status = status;
			member.Reason = SevReadyReason.NONE;
		}
		else Exclude(member, reason);
		PushAll(false);
	}
	protected void Exclude(SevReadyMember member, int reason)
	{
		int index = m_Run.Accepted.Find(member.Identity);
		if (index >= 0) m_Run.Accepted.Remove(index);
		if (member.Status != SevReadyStatus.EXCLUDED || member.Reason != reason) m_Run.Revision++;
		member.Status = SevReadyStatus.EXCLUDED;
		member.Reason = reason;
	}
	void Cancel(PlayerIdentity sender)
	{
		if (!GetGame().IsServer() || !m_Config || !m_Config.IsAdmin(sender)) return;
		if (!m_Run.Cancel(true)) return;
		m_Searches.Clear();
		m_ReportReason = SevReadyReason.CANCELLED;
		PushAll(false);
	}
	void Reconnect(PlayerBase player)
	{
		if (!GetGame().IsServer() || !player || !player.GetIdentity()) return;
		SevReadyMember member = Find(player.GetIdentity().GetId());
		if (!member) return;
		// Never substitute a new character into an accepted/fixed roster.
		MemberConnectionChanged(member);
		member.Recipient = player;
		Push(member, player, false);
	}
	void Disconnect(PlayerBase player)
	{
		if (!GetGame().IsServer()) return;
		foreach (SevReadyMember member : m_Members)
		{
			if (member.Character == player) { MemberConnectionChanged(member); return; }
		}
		MemberConnectionChanged(null);
	}
	protected void MemberConnectionChanged(SevReadyMember member)
	{
		if (!member) return;
		Exclude(member, SevReadyReason.DISCONNECTED);
		// The frozen roster owns preflight validity. A declined/offered player's
		// presentation update cannot cancel another character's location checks.
		if (m_Run.Phase == SevReadyPhase.PREFLIGHT && m_Roster.Find(member) >= 0) Finish(SevReadyReason.DISCONNECTED);
	}
	protected void CloseReady(float now)
	{
		m_Roster.Clear();
		foreach (SevReadyMember member : m_Members)
		{
			if (member.Status != SevReadyStatus.ACCEPTED) continue;
			int reason;
			SevEligibility eligibility = m_Observation.Observe(member.Character, member.Identity, m_Config.CombatQuietSeconds, reason);
			if (SevAdmission.CanAccept(eligibility, true, true)) m_Roster.Insert(member);
			else Exclude(member, reason);
		}
		if (!m_Run.Close(now, m_Roster.Count(), m_Config.MinimumPlayers))
		{
			m_ReportReason = SevReadyReason.MINIMUM;
			PushAll(false);
			return;
		}
		m_PreflightMember = 0;
		m_PreflightDeadline = now + m_Config.PreparationTimeoutSeconds;
		m_SearchRun = m_Run.RunId;
		m_SearchRevision = m_Run.CallbackRevision();
		BeginMemberSearch();
		PushAll(false);
	}
	protected void BeginMemberSearch()
	{
		m_Searches.Clear();
		SevReadyMember member = m_Roster[m_PreflightMember];
		vector center = Vector(m_Config.Center[0], m_Config.Center[1], m_Config.Center[2]);
		vector fallback = Vector(m_Config.Fallback[0], m_Config.Fallback[1], m_Config.Fallback[2]);
		m_Searches.Insert(m_Planner.BeginArena(center, m_Config.SpawnRadiusMeters, m_Reserved));
		m_Searches.Insert(m_Planner.BeginReturn(member.Character.GetPosition(), fallback));
	}
	protected void AdvancePreflight(float now)
	{
		if (!m_Run.CurrentCallback(m_SearchRun, m_SearchRevision)) { Finish(SevReadyReason.DISCONNECTED); return; }
		if (now >= m_PreflightDeadline) { Finish(SevReadyReason.TIMEOUT); return; }
		foreach (SevReadyMember member : m_Roster)
		{
			int reason;
			SevEligibility eligibility = m_Observation.Observe(member.Character, member.Identity, m_Config.CombatQuietSeconds, reason);
			if (!SevAdmission.CanAccept(eligibility, true, true)) { Exclude(member, reason); Finish(reason); return; }
		}
		// Exactly one shared eight-candidate call per 250 ms tick. Resolve one
		// arena at a time so subsequent requests include all earlier reservations.
		m_Planner.AdvanceTick(m_Searches);
		foreach (SevSpawnSearch search : m_Searches)
		{
			if (search.Status == SevSpawnStatus.INVALID || search.Status == SevSpawnStatus.EXHAUSTED) { Finish(SevReadyReason.SPAWN); return; }
			if (search.Status == SevSpawnStatus.PENDING) return;
		}
		vector position;
		if (!m_Searches[0].GetResult(position)) { Finish(SevReadyReason.SPAWN); return; }
		m_Reserved.Insert(position);
		m_PreflightMember++;
		if (m_PreflightMember == m_Roster.Count()) { Finish(SevReadyReason.NONE); return; }
		BeginMemberSearch();
	}
	protected void Finish(int reason)
	{
		m_Run.Phase = SevReadyPhase.REPORT;
		m_Run.Revision++;
		m_ReportReason = reason;
		m_Searches.Clear();
		Print("[SEV] ready preflight-report reason=" + reason.ToString() + " roster=" + m_Roster.Count().ToString() + " destructive-entry=false");
		PushAll(false);
	}
	void Tick(float nowSeconds)
	{
		if (!GetGame().IsServer() || !m_Config || m_Run.Phase == SevReadyPhase.IDLE) return;
		if (m_Run.Phase == SevReadyPhase.READY && nowSeconds >= m_Run.Deadline) CloseReady(nowSeconds);
		else if (m_Run.Phase == SevReadyPhase.PREFLIGHT) AdvancePreflight(nowSeconds);
		if (nowSeconds >= m_NextRefresh)
		{
			PushAll(false);
			m_NextRefresh = nowSeconds + 1;
		}
	}
	protected void PushAll(bool offer)
	{
		foreach (SevReadyMember member : m_Members) Push(member, member.Recipient, offer);
	}
	protected void Push(SevReadyMember member, PlayerBase player, bool offer)
	{
		if (!player || !player.GetIdentity() || player.GetIdentity().GetId() != member.Identity || !player.SevReadyConnected()) return;
		SevReadySnapshot snapshot = new SevReadySnapshot();
		snapshot.RunId = m_Run.RunId; snapshot.Generation = m_Generation;
		snapshot.Revision = m_Run.Revision; snapshot.OfferRevision = m_Run.OfferRevision;
		snapshot.Phase = m_Run.Phase; snapshot.Deadline = m_Run.Deadline;
		snapshot.Remaining = Math.Max(0, m_Run.Deadline - Now());
		snapshot.Accepted = m_Run.Accepted.Count(); snapshot.Status = member.Status;
		snapshot.Reason = member.Reason;
		if (snapshot.Reason == SevReadyReason.NONE) snapshot.Reason = m_ReportReason;
		ScriptRPC rpc = new ScriptRPC();
		snapshot.Write(rpc);
		int opcode = SevNet.STATE;
		if (offer) opcode = SevNet.OFFER;
		rpc.Send(player, opcode, true, player.GetIdentity());
	}
}
