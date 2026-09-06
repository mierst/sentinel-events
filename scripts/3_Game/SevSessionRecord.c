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
	int SchemaVersion;
	string RunId;
	ref array<string> PlayerIds = new array<string>();
	string Integrity;
}
