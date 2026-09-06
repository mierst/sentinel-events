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
	}

	override void OnReconnect()
	{
		super.OnReconnect();
		SevConnectionDiagnostic("reconnect");
	}

	override void OnDisconnect()
	{
		SevCacheIdentity();
		super.OnDisconnect();
		SevConnectionDiagnostic("disconnect");
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
