class CfgPatches
{
	class Psyerns_Interactive_Loadingscreen
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data", "DZ_Scripts", "DZ_Sounds_Effects"};
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

// Main menu music (Deadmans Echo preset). Only played when the JSON lists the SoundSet in
// "MenuMusicSoundSets" - defining it here alone changes nothing.
class CfgSoundShaders
{
	class PSY_ILS_DME_Menu_SoundShader
	{
		samples[] =
		{
			{"Psyerns_Interactive_Loadingscreen\sound\psy_ils_dme_menu1", 1},
			{"Psyerns_Interactive_Loadingscreen\sound\psy_ils_dme_menu2", 1},
			{"Psyerns_Interactive_Loadingscreen\sound\psy_ils_dme_menu3", 1}
		};
		volume = 0.5;
	};
};

class CfgSoundSets
{
	class PSY_ILS_DME_Menu_SoundSet
	{
		soundShaders[] = {"PSY_ILS_DME_Menu_SoundShader"};
		volumeFactor = 1;
		frequencyFactor = 1;
		spatial = 0;
	};
};
