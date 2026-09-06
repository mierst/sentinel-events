class SevReadyMenu : UIScriptedMenu
{
	protected TextWidget m_Title;
	protected MultilineTextWidget m_Status;
	protected TextWidget m_Deadline;
	protected ButtonWidget m_Accept;
	protected ButtonWidget m_Decline;
	protected ButtonWidget m_Close;
	protected string m_RequestRun;
	protected int m_RequestRevision;
	protected bool m_Waiting;
	protected float m_RequestSent;

	override Widget Init()
	{
		layoutRoot = GetGame().GetWorkspace().CreateWidgets("SentinelEvents/layouts/sev_ready.layout");
		m_Title = TextWidget.Cast(layoutRoot.FindAnyWidget("SevTitle"));
		m_Status = MultilineTextWidget.Cast(layoutRoot.FindAnyWidget("SevStatus"));
		m_Deadline = TextWidget.Cast(layoutRoot.FindAnyWidget("SevDeadline"));
		m_Accept = ButtonWidget.Cast(layoutRoot.FindAnyWidget("SevAccept"));
		m_Decline = ButtonWidget.Cast(layoutRoot.FindAnyWidget("SevDecline"));
		m_Close = ButtonWidget.Cast(layoutRoot.FindAnyWidget("SevClose"));
		return layoutRoot;
	}
	override void OnHide()
	{
		super.OnHide();
		MissionGameplay mission = MissionGameplay.Cast(GetGame().GetMission());
		if (mission) mission.SevDismissReady(this);
	}
	void Present(SevReadySnapshot snapshot, bool fresh)
	{
		if (!layoutRoot || !snapshot) return;
		if (snapshot.RunId != m_RequestRun || snapshot.Revision > m_RequestRevision || SevCoordinator.Now() - m_RequestSent >= 3) m_Waiting = false;
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		bool vehicle = player && player.IsInVehicle();
		bool open = fresh && snapshot.Phase == SevReadyPhase.READY && snapshot.Remaining > 0;
		m_Title.SetText("Sentinel Events - readiness rehearsal");
		m_Deadline.SetText("Server ready time remaining: " + Math.Ceil(snapshot.Remaining).ToString() + " s | Accepted: " + snapshot.Accepted.ToString());
		string status = "Choose Accept only after reading the warning. You can close this menu to stash first.";
		if (snapshot.Status == SevReadyStatus.ACCEPTED) status = "Server accepted. Waiting for ready to close; eligibility will be checked again.";
		if (snapshot.Status == SevReadyStatus.DECLINED) status = "Server recorded your decline. Your character is unchanged.";
		if (snapshot.Status == SevReadyStatus.EXCLUDED) status = "Excluded: " + Reason(snapshot.Reason);
		if (snapshot.Phase == SevReadyPhase.PREFLIGHT) status = "Checking the roster and safe locations. No equipment, health or position changes.";
		if (snapshot.Phase == SevReadyPhase.REPORT) status = "Preflight report: " + Reason(snapshot.Reason) + " No demonstration starts; your character is unchanged.";
		if (snapshot.Phase == SevReadyPhase.CANCELLED) status = "The administrator cancelled this rehearsal. Your character is unchanged.";
		if (m_Waiting) status = "Request sent. Waiting for the server; acceptance is not yet confirmed.";
		if (vehicle && open) status = "Exit the vehicle before accepting. " + status;
		if (!fresh) status = "Connection state is stale. Waiting for a current server response; acceptance is disabled.";
		m_Status.SetText(status);
		m_Accept.Enable(open && !vehicle && !m_Waiting && snapshot.Status != SevReadyStatus.ACCEPTED);
		m_Decline.Enable(open && !m_Waiting && snapshot.Status != SevReadyStatus.DECLINED);
		m_RequestRun = snapshot.RunId;
		m_RequestRevision = snapshot.Revision;
	}
	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (w == m_Close) { Close(); return true; }
		if (w == m_Accept || w == m_Decline)
		{
			MissionGameplay mission = MissionGameplay.Cast(GetGame().GetMission());
			if (mission && !m_Waiting && mission.SevSendReady(w == m_Accept))
			{
				m_Waiting = true;
				m_RequestSent = SevCoordinator.Now();
				m_Accept.Enable(false); m_Decline.Enable(false);
				m_Status.SetText("Request sent. Waiting for the server; acceptance is not yet confirmed.");
			}
			return true;
		}
		return super.OnClick(w, x, y, button);
	}
	static string Reason(int reason)
	{
		switch (reason)
		{
			case SevReadyReason.NONE: return "Read-only checks completed.";
			case SevReadyReason.RECOVERY_UNKNOWN: return "Recovery status is unknown; no bounded clear proof is available.";
			case SevReadyReason.RECOVERY_PENDING: return "A character marker needs recovery review.";
			case SevReadyReason.QUIET: return "Wait the full configured combat quiet interval after connection or observed damage.";
			case SevReadyReason.VEHICLE: return "Exit the vehicle; this rehearsal never extracts passengers.";
			case SevReadyReason.DEAD: return "The original character is no longer alive.";
			case SevReadyReason.DISCONNECTED: return "The original connected character changed or disconnected.";
			case SevReadyReason.RESTRAINED: return "The character is restrained.";
			case SevReadyReason.UNCONSCIOUS: return "The character is unconscious.";
			case SevReadyReason.MINIMUM: return "Too few eligible participants; at least two are required.";
			case SevReadyReason.SPAWN: return "No validated safe arena/return position was found within the search cap.";
			case SevReadyReason.TIMEOUT: return "The non-destructive location check timed out.";
			case SevReadyReason.CANCELLED: return "Cancelled by the administrator.";
			case SevReadyReason.FULL: return "The bounded roster is full.";
		}
		return "Unknown server reason; acceptance is unavailable.";
	}
}
