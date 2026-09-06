class CfgPatches
{
	class SentinelEvents
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data"};
	};
};

class CfgMods
{
	class SentinelEvents
	{
		dir = "SentinelEvents";
		name = "Sentinel Events";
		author = "Sentinel Events contributors";
		version = "0.0.1-pre";
		type = "mod";
		dependencies[] = {"Game", "World", "Mission"};

		class defs
		{
			class gameScriptModule
			{
				value = "";
				files[] = {"SentinelEvents/scripts/3_Game"};
			};

			class worldScriptModule
			{
				value = "";
				files[] = {"SentinelEvents/scripts/4_World"};
			};

			class missionScriptModule
			{
				value = "";
				files[] = {"SentinelEvents/scripts/5_Mission"};
			};
		};
	};
};

class CfgSentinelEvents
{
	enabled = 0;
};
