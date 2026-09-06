class SevPersistenceProbe
{
	// Compile-time opt-in. Change only in a dedicated diagnostic source build.
	static const bool ENABLED = false;
	static const string FAULT_POINT = "";

	static bool Hit(string point)
	{
		if (!ENABLED) return false;
		Print("[SEV] persistence probe boundary=" + point);
		return FAULT_POINT == point;
	}
}
