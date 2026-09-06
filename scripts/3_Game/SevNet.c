class SevNet
{
	// Vanilla scripts/3_Game/Enums/ERPCs.c plus this standalone package inspected:
	// 734210-734213 have no collision in that set, not a globally reserved range.
	static const int OFFER = 734210;
	static const int READY_RESPONSE = 734211;
	static const int STATE = 734212;
	static const int ADMIN_REQUEST = 734213;
	static const int SCHEMA = 1;
	static bool Owned(int opcode) { return opcode >= OFFER && opcode <= ADMIN_REQUEST; }
	static bool Direction(int opcode, bool server, bool senderPresent)
	{
		if (server) return senderPresent && (opcode == READY_RESPONSE || opcode == ADMIN_REQUEST);
		return !senderPresent && (opcode == OFFER || opcode == STATE);
	}
	static bool Envelope(int schema, string runId, int revision)
	{
		return schema == SCHEMA && runId.Length() <= 64 && SevSessionStore.ValidId(runId) && revision > 0 && revision < 1000000000;
	}
}

class SevRequestBudget
{
	protected float m_Tokens = 4;
	protected float m_Last;
	protected bool m_Started;
	bool Take(float now)
	{
		if (!(now >= 0) || (m_Started && now < m_Last)) return false;
		if (m_Started) m_Tokens = Math.Min(4, m_Tokens + (now - m_Last) * 2);
		m_Started = true;
		m_Last = now;
		if (m_Tokens < 1) return false;
		m_Tokens = m_Tokens - 1;
		return true;
	}
}

enum SevReadyPhase { IDLE, READY, PREFLIGHT, REPORT, CANCELLED }
enum SevReadyStatus { OFFERED, ACCEPTED, DECLINED, EXCLUDED }
enum SevReadyReason { NONE, RECOVERY_UNKNOWN, RECOVERY_PENDING, QUIET, VEHICLE, DEAD, DISCONNECTED, RESTRAINED, UNCONSCIOUS, MINIMUM, SPAWN, TIMEOUT, CANCELLED, FULL }

// Pure authoritative run decisions. Only the World coordinator supplies identity,
// observed eligibility and time. Offer revision remains stable through refreshes.
class SevReadyRun
{
	string RunId;
	int OfferRevision;
	int Revision;
	int Phase;
	float Deadline;
	ref array<string> Accepted = new array<string>();
	bool Start(bool admin, string runId, float now, int duration)
	{
		if (!admin || Phase == SevReadyPhase.READY || Phase == SevReadyPhase.PREFLIGHT) return false;
		if (!SevNet.Envelope(1, runId, 1) || !(now >= 0) || duration < 30 || duration > 600) return false;
		RunId = runId;
		OfferRevision = 1;
		Revision = 1;
		Deadline = now + duration;
		Phase = SevReadyPhase.READY;
		Accepted.Clear();
		return true;
	}
	bool Respond(string identity, string runId, int revision, bool accept, float now, SevEligibility eligibility)
	{
		if (!SevSessionStore.ValidId(identity, true) || runId != RunId || revision != OfferRevision || Phase != SevReadyPhase.READY || !(now >= 0 && now < Deadline)) return false;
		int index = Accepted.Find(identity);
		if (!accept)
		{
			if (index >= 0) { Accepted.Remove(index); Revision++; }
			return true;
		}
		if (!SevAdmission.CanAccept(eligibility, true, true)) return false;
		if (index >= 0) return true;
		if (Accepted.Count() >= 8) return false;
		Accepted.Insert(identity);
		Revision++;
		return true;
	}
	bool Close(float now, int eligibleCount, int minimum)
	{
		if (Phase != SevReadyPhase.READY || !(now >= Deadline)) return false;
		Revision++;
		Phase = SevReadyPhase.REPORT;
		if (eligibleCount > Accepted.Count() || !SevAdmission.CanPrepare(eligibleCount, minimum, true, true)) return false;
		Phase = SevReadyPhase.PREFLIGHT;
		return true;
	}
	bool Cancel(bool admin)
	{
		if (!admin || Phase == SevReadyPhase.IDLE) return false;
		if (Phase == SevReadyPhase.CANCELLED) return true;
		Phase = SevReadyPhase.CANCELLED;
		Revision++;
		return true;
	}
	bool CurrentCallback(string runId, int revision)
	{
		return Phase == SevReadyPhase.PREFLIGHT && runId == RunId && revision == Revision;
	}
}

class SevClientRevision
{
	protected string m_Run;
	protected int m_Generation;
	protected int m_Revision;
	protected int m_Phase;
	bool Apply(string runId, int generation, int revision, int phase)
	{
		if (!SevNet.Envelope(1, runId, revision) || generation < 1 || generation < m_Generation || phase < SevReadyPhase.READY || phase > SevReadyPhase.CANCELLED) return false;
		if (generation == m_Generation)
		{
			if (runId != m_Run || revision < m_Revision || phase < m_Phase) return false;
			if (revision == m_Revision && phase != m_Phase) return false;
		}
		m_Run = runId;
		m_Generation = generation;
		m_Revision = revision;
		m_Phase = phase;
		return true;
	}
}

// Fixed scalar envelope; no nested arrays, arbitrary identity, gear or text.
// ParamsReadContext allocates the run string before script can check Length.
// Script bounds reject it immediately but cannot establish a native byte cap.
class SevReadySnapshot
{
	int Schema = 1;
	string RunId;
	int Generation;
	int Revision;
	int OfferRevision;
	int Phase;
	float Deadline;
	float Remaining;
	int Accepted;
	int Status;
	int Reason;
	bool Read(ParamsReadContext ctx)
	{
		if (!ctx.Read(Schema) || Schema != 1) return false;
		if (!ctx.Read(RunId) || RunId.Length() > 64) return false;
		if (!ctx.Read(Generation) || !ctx.Read(Revision) || !ctx.Read(OfferRevision) || !ctx.Read(Phase)) return false;
		if (!ctx.Read(Deadline) || !ctx.Read(Remaining) || !ctx.Read(Accepted) || !ctx.Read(Status) || !ctx.Read(Reason)) return false;
		return SevNet.Envelope(Schema, RunId, Revision) && Generation > 0 && Generation < 1000000000 && OfferRevision == 1 && Phase >= SevReadyPhase.READY && Phase <= SevReadyPhase.CANCELLED && Deadline >= 0 && Remaining >= 0 && Remaining <= 600 && Accepted >= 0 && Accepted <= 8 && Status >= 0 && Status <= SevReadyStatus.EXCLUDED && Reason >= 0 && Reason <= SevReadyReason.FULL;
	}
	void Write(ScriptRPC rpc)
	{
		rpc.Write(Schema); rpc.Write(RunId); rpc.Write(Generation); rpc.Write(Revision);
		rpc.Write(OfferRevision); rpc.Write(Phase); rpc.Write(Deadline); rpc.Write(Remaining);
		rpc.Write(Accepted); rpc.Write(Status); rpc.Write(Reason);
	}
}

// Lower-layer bridge; the mission owns and clears the client presentation sink.
class SevReadySink
{
	static ref SevReadySink Current;
	void Receive(SevReadySnapshot snapshot) {}
}
