class CfgPatches
{
	class DayZ_Expansion_AI_Dynamic_scripts
	{
		requiredAddons[] = {"DayZExpansion_AI_Scripts"};
	};
};
class CfgMods
{
	class DayZ_Expansion_AI_Dynamic
	{
		action = "";
		hideName = 0;
		hidePicture = 0;
		name = "Spatial AI";
		credits = "DayZ Expansion and dolphin";
		author = "Dolphin";
		authorID = "";
		version = "0.1";
		extra = 0;
		type = "servermod";
		dependencies[] = {"Game","World","Mission"};
		class defs
		{
			class gameScriptModule
			{
				value = "";
				files[] = {"Dynamic-ai/Scripts/3_Game"};
			};
			class worldScriptModule
			{
				value = "";
				files[] = {"Dynamic-ai/Scripts/4_World"};
			};
			class missionScriptModule
			{
				value = "";
				files[] = {"SafeZone/scripts/Common","Dynamic-ai/Scripts/5_Mission"};
			};
		};
	};
};
