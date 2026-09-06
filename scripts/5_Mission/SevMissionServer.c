modded class MissionServer
{
	protected ref SevHarnessConfig m_SevHarnessConfig;

	override void OnInit()
	{
		super.OnInit();
		// Read-only load; absent/invalid operator settings never get overwritten.
		bool configValid = SevHarnessConfig.Load(m_SevHarnessConfig);
		Print("[SEV] harness config-valid=" + configValid.ToString() + " destructive-entry=false");
		SevAdmissionTests.Register();
		SevRecoveryTests.Register();
		SevSessionStoreTests.Register();
		SevGuardSpawnTests.Register();
		SevReadyTests.Register();
		SevCoordinator.Current = new SevCoordinator(m_SevHarnessConfig);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SevCoordinator.Current.TimerTick, 250, true);
	}

	override void OnMissionFinish()
	{
		if (SevCoordinator.Current) GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(SevCoordinator.Current.TimerTick);
		SevCoordinator.Current = null;
		super.OnMissionFinish();
	}
}
