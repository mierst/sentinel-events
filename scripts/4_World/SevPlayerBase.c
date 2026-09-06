class SevCharacterMarker
{
	int SchemaVersion;
	string RunId;
	string PlayerId;
	string Token;
	string State;
	string MutationReceipt;
	string ReturnReceipt;

	bool IsValid()
	{
		if (SchemaVersion != 1 || !SevSessionStore.ValidId(RunId) || !SevSessionStore.ValidId(PlayerId, true) || !SevSessionStore.ValidId(Token))
			return false;
		if (State == "UNTOUCHED")
			return MutationReceipt == "" && ReturnReceipt == "";
		if (State == "MUTATED")
			return SevSessionStore.ValidId(MutationReceipt) && ReturnReceipt == "";
		if (State == "RETURNED")
			return SevSessionStore.ValidId(MutationReceipt) && SevSessionStore.ValidId(ReturnReceipt);
		return false;
	}
}

modded class PlayerBase
{
	protected string m_SevStableIdentity;
	protected string m_SevMarkerData;
	protected ref SevCharacterMarker m_SevMarker;
	protected bool m_SevGuardPresentation;
	protected bool m_SevReadyConnected;
	protected float m_SevQuietSince;
	protected ref SevRequestBudget m_SevRequestBudget = new SevRequestBudget();

	void PlayerBase()
	{
		RegisterNetSyncVariableBool("m_SevGuardPresentation");
	}

	void SevSetGuardPresentation(bool held)
	{
		if (!GetGame().IsServer()) return;
		SevCacheIdentity();
		m_SevGuardPresentation = held;
		SetSynchDirty();
	}

	bool SevGuardPresentation() { return m_SevGuardPresentation; }
	string SevGuardIdentity() { return m_SevStableIdentity; }

	override bool CanManipulateInventory()
	{
		if (SevEntryGuard.Blocks(this)) return false;
		return super.CanManipulateInventory();
	}

	override bool CanDropEntity(notnull EntityAI item)
	{
		if (SevEntryGuard.Blocks(this)) return false;
		return super.CanDropEntity(item);
	}

	override void OnStoreSave(ParamsWriteContext ctx)
	{
		SevPersistenceProbe.Hit("before-player-save");
		super.OnStoreSave(ctx);
		// Empty envelope is deliberately not evidence of untouched inventory.
		ctx.Write(m_SevMarkerData);
		SevPersistenceProbe.Hit("after-player-save");
	}

	override bool OnStoreLoad(ParamsReadContext ctx, int version)
	{
		m_SevMarker = null;
		m_SevMarkerData = "";
		if (!super.OnStoreLoad(ctx, version)) return false;
		string envelope;
		// Legacy/unrelated characters must preserve vanilla load success.
		if (!ctx.Read(envelope)) return true;
		if (envelope.Length() < 1 || envelope.Length() > 1024) return true;
		if (!SevSessionStore.HasObjectEnvelope(envelope)) return true;
		SevCharacterMarker candidate = new SevCharacterMarker();
		string error;
		if (!JsonFileLoader<SevCharacterMarker>.LoadData(envelope, candidate, error)) return true;
		if (!candidate.IsValid()) return true;
		string canonical;
		if (!JsonFileLoader<SevCharacterMarker>.MakeData(candidate, canonical, error, false)) return true;
		if (canonical != envelope) return true;
		m_SevMarker = candidate;
		m_SevMarkerData = envelope;
		SevPersistenceProbe.Hit("after-player-load");
		return true;
	}

	protected void SevCacheIdentity()
	{
		PlayerIdentity identity = GetIdentity();
		if (identity && SevSessionStore.ValidId(identity.GetId(), true))
			m_SevStableIdentity = identity.GetId();
	}

	protected void SevConnectionDiagnostic(string eventName)
	{
		if (!GetGame().IsServer()) return;
		SevCacheIdentity();
		bool cached = m_SevStableIdentity != "";
		bool marked = m_SevMarker != null;
		// No stable ID, entity ID, name, token, or receipt enters public logs.
		Print("[SEV] persistence lifecycle=" + eventName + " identity-cached=" + cached.ToString() + " marker-present=" + marked.ToString());
	}

	override void OnConnect()
	{
		super.OnConnect();
		SevConnectionDiagnostic("connect");
		SevBeginReadyObservation();
	}

	override void OnReconnect()
	{
		super.OnReconnect();
		SevConnectionDiagnostic("reconnect");
		SevBeginReadyObservation();
	}

	override void OnDisconnect()
	{
		SevCacheIdentity();
		m_SevReadyConnected = false;
		if (SevCoordinator.Current) SevCoordinator.Current.Disconnect(this);
		super.OnDisconnect();
		SevConnectionDiagnostic("disconnect");
	}

	protected void SevBeginReadyObservation()
	{
		if (!GetGame().IsServer()) return;
		m_SevReadyConnected = true;
		m_SevQuietSince = SevCoordinator.Now();
		if (SevCoordinator.Current) SevCoordinator.Current.Reconnect(this);
	}
	bool SevReadyConnected() { return m_SevReadyConnected; }
	bool SevReadyQuiet(int quietSeconds)
	{
		return GetGame().IsServer() && m_SevReadyConnected && SevCoordinator.Now() - m_SevQuietSince >= quietSeconds;
	}
	int SevReadyRecoveryReason()
	{
		if (m_SevMarker) return SevReadyReason.RECOVERY_PENDING;
		return SevReadyReason.RECOVERY_UNKNOWN;
	}
	void SevObserveCombat()
	{
		if (GetGame().IsServer()) m_SevQuietSince = SevCoordinator.Now();
	}
	// Observation only: preserve damage and record conservative victim quiet time.
	// Source hierarchy identifies held weapons/melee actors, but unattributed
	// projectiles/explosives do not prove complete outgoing combat observation.
	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
		if (!GetGame().IsServer()) return;
		SevObserveCombat();
		PlayerBase attacker;
		if (source) attacker = PlayerBase.Cast(source.GetHierarchyRootPlayer());
		if (attacker && attacker != this) attacker.SevObserveCombat();
	}

	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		if (!SevNet.Owned(rpc_type)) { super.OnRPC(sender, rpc_type, ctx); return; }
		bool server = GetGame().IsServer();
		if (!SevNet.Direction(rpc_type, server, sender != null)) return;
		if (!server)
		{
			if (GetGame().GetPlayer() != this || !SevReadySink.Current) return;
			SevReadySnapshot snapshot = new SevReadySnapshot();
			if (snapshot.Read(ctx)) SevReadySink.Current.Receive(snapshot);
			return;
		}
		// Authenticate the RPC target before any payload read or coordinator work.
		if (!m_SevReadyConnected || !GetIdentity() || GetIdentity().GetId() != sender.GetId() || !SevCoordinator.Current) return;
		if (!m_SevRequestBudget.Take(SevCoordinator.Now())) return;
		int schema;
		if (!ctx.Read(schema) || schema != SevNet.SCHEMA) return;
		if (rpc_type == SevNet.ADMIN_REQUEST)
		{
			int command;
			if (!ctx.Read(command)) return;
			if (command == 1) SevCoordinator.Current.StartRehearsal(sender);
			else if (command == 2) SevCoordinator.Current.Cancel(sender);
			return;
		}
		string runId;
		int revision;
		bool accept;
		if (!ctx.Read(runId) || runId.Length() > 64) return;
		if (!ctx.Read(revision) || !SevNet.Envelope(schema, runId, revision) || !ctx.Read(accept)) return;
		SevCoordinator.Current.Respond(sender, runId, revision, accept);
	}

	// Correlation only. This probe never authorizes destruction or applies guards.
	bool SevHasMatchingMarker(SevSessionRecord record)
	{
		if (!GetGame().IsServer() || !SevSessionStore.ValidRecord(record) || !m_SevMarker)
			return false;
		SevCacheIdentity();
		if (m_SevStableIdentity != record.PlayerId || !m_SevMarker.IsValid())
			return false;
		return m_SevMarker.RunId == record.RunId && m_SevMarker.PlayerId == record.PlayerId && m_SevMarker.Token == record.Token;
	}

	// Server-side diagnostic entry point, compile-time disabled in normal builds.
	// Simulated receipt stages do not represent inventory mutation or Hive proof.
	bool SevSetDiagnosticMarker(SevSessionRecord record, string state)
	{
		if (!SevPersistenceProbe.ENABLED || !GetGame().IsServer()) return false;
		if (!SevSessionStore.ValidRecord(record)) return false;
		SevCacheIdentity();
		if (m_SevStableIdentity != record.PlayerId) return false;
		SevCharacterMarker marker = new SevCharacterMarker();
		marker.SchemaVersion = 1;
		marker.RunId = record.RunId;
		marker.PlayerId = record.PlayerId;
		marker.Token = record.Token;
		marker.State = state;
		marker.MutationReceipt = record.MutationReceipt;
		marker.ReturnReceipt = record.ReturnReceipt;
		if (!marker.IsValid()) return false;
		if (state == "RETURNED") SevPersistenceProbe.Hit("before-return-receipt");
		string data;
		string error;
		if (!JsonFileLoader<SevCharacterMarker>.MakeData(marker, data, error, false)) return false;
		if (data.Length() > 1024) return false;
		m_SevMarker = marker;
		m_SevMarkerData = data;
		if (state == "RETURNED") SevPersistenceProbe.Hit("after-return-receipt");
		return true;
	}
}
