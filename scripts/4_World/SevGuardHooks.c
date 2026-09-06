modded class WeaponManager
{
	override bool CanFire(Weapon_Base wpn)
	{
		if (SevEntryGuard.Blocks(m_player)) return false;
		return super.CanFire(wpn);
	}
}

modded class DayZPlayerMeleeFightLogic_LightHeavy
{
	override bool CanFight()
	{
		if (SevEntryGuard.Blocks(m_Player)) return false;
		return super.CanFight();
	}
}

modded class ActionBase
{
	override bool Can(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (SevEntryGuard.Blocks(player) || SevEntryGuard.BlocksObject(item)) return false;
		if (target && (SevEntryGuard.BlocksObject(target.GetObject()) || SevEntryGuard.BlocksObject(target.GetParent()))) return false;
		return super.Can(player, target, item);
	}
}
