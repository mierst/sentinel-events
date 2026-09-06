modded class MissionServer
{
	override void OnInit()
	{
		super.OnInit();
		SevAdmissionTests.Register();
		SevRecoveryTests.Register();
		SevSessionStoreTests.Register();
	}
}
