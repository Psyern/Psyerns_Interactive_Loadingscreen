class CfgPatches
{
	class Psyerns_Interactive_Loadingscreen
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data", "DZ_Scripts"};
	};
};

class CfgMods
{
	class Psyerns_Interactive_Loadingscreen
	{
		dir = "Psyerns_Interactive_Loadingscreen";
		picture = "";
		action = "https://github.com/Psyern/Psyerns_Interactive_Loadingscreen";
		hideName = 1;
		hidePicture = 1;
		name = "Psyerns Interactive Loadingscreen";
		credits = "Psyern";
		author = "Psyern";
		authorID = "";
		version = "1.0.0";
		extra = 0;
		type = "mod";

		dependencies[] = {"Game"};

		class defs
		{
			class gameScriptModule
			{
				value = "";
				files[] = {"Psyerns_Interactive_Loadingscreen/Scripts/3_Game"};
			};
		};
	};
};
