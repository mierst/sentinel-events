// Compatibility data for the withdrawn movement diagnostic and format fixtures.
// A supplied baseline does not authorize any native controller operation.
class SevMovementBaseline
{
	bool Known;
	bool Disabled;
	int SpeedType;
	float Speed;
	int AngleType;
	float Angle;

	bool IsValid()
	{
		return Known && (SpeedType == HumanInputControllerOverrideType.DISABLED || SpeedType == HumanInputControllerOverrideType.ENABLED) && (AngleType == HumanInputControllerOverrideType.DISABLED || AngleType == HumanInputControllerOverrideType.ENABLED) && SevHarnessConfig.Between(Speed, 0, 3) && SevHarnessConfig.Between(Angle, -180, 180);
	}
}

class SevGuardState
{
	PlayerBase Player;
	ref SevGuardLease Lease;
	vector Anchor;
}

class SevWorldHandGuardPolicy : SevHandGuardPolicy
{
	override bool Blocks(Man actor, InventoryLocation src, InventoryLocation secondSrc, InventoryLocation dst, InventoryLocation secondDst)
	{
		return SevEntryGuard.BlocksObject(actor) || SevEntryGuard.BlocksLocation(src) || SevEntryGuard.BlocksLocation(secondSrc) || SevEntryGuard.BlocksLocation(dst) || SevEntryGuard.BlocksLocation(secondDst);
	}
}

class SevEntryGuard
{
	protected static ref map<string, ref SevGuardState> s_SevTokens = new map<string, ref SevGuardState>();
	protected static ref map<string, ref SevGuardState> s_SevPlayers = new map<string, ref SevGuardState>();

	static bool Acquire(PlayerBase player, string token)
	{
		if (!GetGame().IsServer() || !player || !player.GetIdentity() || !SevSessionStore.ValidId(token)) return false;
		string identity = player.GetIdentity().GetId();
		SevGuardState previous;
		if (s_SevTokens.Find(token, previous))
			return previous.Player == player && previous.Lease.Acquire(identity, token, player.GetAllowDamage());
		if (s_SevPlayers.Contains(identity) || s_SevPlayers.Count() >= 8 || s_SevTokens.Count() >= 256) return false;
		SevGuardState state = new SevGuardState();
		state.Lease = new SevGuardLease();
		if (!state.Lease.Acquire(identity, token, player.GetAllowDamage())) return false;
		if (!SevHandGuardPolicy.Handler) SevHandGuardPolicy.Handler = new SevWorldHandGuardPolicy();
		state.Player = player;
		state.Anchor = player.GetPosition();
		s_SevTokens.Insert(token, state);
		s_SevPlayers.Insert(identity, state);
		// Registry first; reject new requests before interrupting existing work.
		player.SevSetGuardPresentation(true);
		player.SetAllowDamage(false);
		if (player.GetActionManager()) player.GetActionManager().EndOrInterruptCurrentAction();
		if (player.GetThrowing()) player.GetThrowing().CancelThrowing(player.GetInputController(), player.GetCommandModifier_Weapons());
		return true;
	}

	static bool Release(PlayerBase player, string token)
	{
		if (!GetGame().IsServer() || !player) return false;
		SevGuardState state;
		if (!s_SevTokens.Find(token, state) || state.Player != player) return false;
		if (!state.Lease.Held) return true;
		player.SetAllowDamage(state.Lease.PriorAllowDamage);
		player.SevSetGuardPresentation(false);
		state.Lease.Release(state.Lease.Identity, token);
		s_SevPlayers.Remove(state.Lease.Identity);
		return true;
	}

	static bool IsHeld(string token)
	{
		if (!GetGame().IsServer()) return false;
		SevGuardState state;
		return s_SevTokens.Find(token, state) && state.Lease.Held;
	}

	static bool Blocks(PlayerBase player)
	{
		if (!player) return false;
		if (!GetGame().IsServer()) return player.SevGuardPresentation();
		SevGuardState state;
		return s_SevPlayers.Find(player.SevGuardIdentity(), state) && state.Player == player && state.Lease.Held;
	}

	static bool BlocksObject(Object object)
	{
		if (!object) return false;
		PlayerBase player = PlayerBase.Cast(object);
		EntityAI entity = EntityAI.Cast(object);
		if (!player && entity) player = PlayerBase.Cast(entity.GetHierarchyRootPlayer());
		return Blocks(player);
	}

	static bool BlocksLocation(InventoryLocation location)
	{
		return location && (BlocksObject(location.GetParent()) || BlocksObject(location.GetItem()));
	}

	static bool BeginMovementDiagnostic(PlayerBase player, string token, SevMovementBaseline baseline)
	{
		// Withdrawn after live uncontrolled-movement failure. Preserve callers but
		// never access the player/controller, install overrides, or claim a freeze.
		return false;
	}

	static bool MeasureDisplacement(PlayerBase player, string token, out float meters)
	{
		meters = 0;
		if (!GetGame().IsServer()) return false;
		SevGuardState state;
		if (!s_SevTokens.Find(token, state) || !player || state.Player != player || !state.Lease.Held) return false;
		meters = vector.Distance(state.Anchor, player.GetPosition());
		return true;
	}

	static bool MovementProven() { return false; }
}
