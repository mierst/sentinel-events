// World installs the participant registry policy; Game cannot refer to PlayerBase.
class SevHandGuardPolicy
{
	static ref SevHandGuardPolicy Handler;
	bool Blocks(Man actor, InventoryLocation src, InventoryLocation secondSrc, InventoryLocation dst, InventoryLocation secondDst) { return false; }

	static bool MustCheck(InventoryValidation validation)
	{
		// ProcessInputData assigns this mode on the server, not from client data.
		return GetGame().IsServer() && validation && !validation.m_IsRemote && validation.m_Mode == InventoryMode.JUNCTURE && Handler;
	}
}

modded class HandEventBase
{
	override bool CanPerformEventEx(InventoryValidation validation)
	{
		if (SevHandGuardPolicy.MustCheck(validation))
		{
			if (SevHandGuardPolicy.Handler.Blocks(m_Player, GetSrc(), GetSecondSrc(), GetDst(), GetSecondDst())) return false;
		}
		return super.CanPerformEventEx(validation);
	}
}

modded class HandEventTake
{
	override bool CanPerformEventEx(InventoryValidation validation)
	{
		// Vanilla acknowledged takes return before HandEventBase. Recheck this
		// completion against the current guard before preserving that super path.
		if (SevHandGuardPolicy.MustCheck(validation) && validation.m_IsJuncture)
		{
			// Take has one source; its destination is the actor's hands. Checking
			// actor covers that destination without allocating a native location.
			if (SevHandGuardPolicy.Handler.Blocks(m_Player, GetSrc(), null, null, null)) return false;
		}
		return super.CanPerformEventEx(validation);
	}
}
