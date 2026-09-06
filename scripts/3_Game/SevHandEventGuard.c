// World installs the participant registry policy; Game cannot refer to PlayerBase.
class SevHandGuardPolicy
{
	static ref SevHandGuardPolicy Handler;
	bool Blocks(Man actor, InventoryLocation src, InventoryLocation secondSrc, InventoryLocation dst, InventoryLocation secondDst) { return false; }
}

modded class HandEventBase
{
	override bool CanPerformEventEx(InventoryValidation validation)
	{
		// Native ProcessInputData assigns JUNCTURE, not from the client payload.
		if (GetGame().IsServer() && validation && !validation.m_IsRemote && validation.m_Mode == InventoryMode.JUNCTURE && SevHandGuardPolicy.Handler)
		{
			if (SevHandGuardPolicy.Handler.Blocks(m_Player, GetSrc(), GetSecondSrc(), GetDst(), GetSecondDst())) return false;
		}
		return super.CanPerformEventEx(validation);
	}
}
