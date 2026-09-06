class SevSessionRecord
{
	int SchemaVersion;
	int Sequence;
	string RunId;
	string PlayerId;
	string Token;
	string Phase;
	vector Origin;
	vector OriginOrientation;
	string MutationReceipt;
	string ReturnReceipt;
	vector ReturnPosition;
	string PreviousIntegrity;
	string Integrity;
}

class SevSessionManifest
{
	// v1 retains hex identity directories. v2 uses immutable member slots p0-p7.
	// Record/character schemas are independent and remain version 1.
	int SchemaVersion;
	string RunId;
	ref array<string> PlayerIds = new array<string>();
	string Integrity;
}
