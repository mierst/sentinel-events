modded class ChatInputMenu
{
	protected ref Timer m_SevCommandClose;
	protected bool m_SevCommandSubmitted;
	protected bool m_SevOpenReadyAfterClose;
	override bool OnChange(Widget w, int x, int y, bool finished)
	{
		EditBoxWidget input = EditBoxWidget.Cast(w);
		if (!finished || !input || input.GetName() != "InputEditBoxWidget") return super.OnChange(w, x, y, finished);
		string text = input.GetText();
		int command;
		if (text == "/event rehearse") command = 1;
		else if (text == "/event cancel") command = 2;
		else if (text != "/event ready") return super.OnChange(w, x, y, finished);
		if (m_SevCommandSubmitted) return true;
		m_SevCommandSubmitted = true;
		MissionGameplay mission = MissionGameplay.Cast(GetGame().GetMission());
		if (mission && command > 0) mission.SevSendAdmin(command);
		m_SevOpenReadyAfterClose = command == 0;
		m_SevCommandClose = new Timer(CALL_CATEGORY_GUI);
		m_SevCommandClose.Run(0.1, this, "SevCloseCommand");
		return true;
	}
	void SevCloseCommand()
	{
		if (m_SevOpenReadyAfterClose)
		{
			MissionGameplay mission = MissionGameplay.Cast(GetGame().GetMission());
			if (mission) GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(mission.SevOpenReady, 200, false);
		}
		Close();
	}
}
