// Psyerns Interactive Loadingscreen - configuration
//
// The default config ships inside the PBO (Data/psy_ils_config.json).
// For testing a client can drop a copy into
// $profile:Psyerns_Interactive_Loadingscreen/psy_ils_config.json, which wins over the packed one.

class PSY_ILS_Const
{
	static const string CONFIG_PACKED = "Psyerns_Interactive_Loadingscreen/Data/psy_ils_config.json";
	static const string CONFIG_PROFILE = "$profile:Psyerns_Interactive_Loadingscreen/psy_ils_config.json";

	static const string LAYOUT_SCENE = "Psyerns_Interactive_Loadingscreen/gui/layouts/psy_ils_scene.layout";
	static const string LAYOUT_IMAGE = "Psyerns_Interactive_Loadingscreen/gui/layouts/psy_ils_image.layout";
	static const string LAYOUT_IMAGE_ADDITIVE = "Psyerns_Interactive_Loadingscreen/gui/layouts/psy_ils_image_additive.layout";

	static const string TYPE_IMAGE = "image";
	static const string TYPE_PARTICLES = "particles";
	static const string TYPE_CURSORLIGHT = "cursorlight";

	static const int MAX_PARTICLES_PER_LAYER = 200;
}

//! One entry of the "Layers" array. Which fields matter depends on Type.
class PSY_ILS_LayerData
{
	// all types
	string Type = "image";
	string Texture;
	float Alpha = 1.0;
	bool Additive = false;

	// image: 0 = static (vignette/overlay), 1 = foreground, > 1 = even closer than the foreground
	float Depth = 0.5;
	//! horizontal sine sway, fraction of the screen width
	float SwayAmplitude = 0.0;
	//! sway speed in radians per second
	float SwaySpeed = 0.0;
	//! true: the layer keeps exactly screen size and may reveal its (transparent) edges while moving.
	//! For text/logo layers whose content sits close to the border and must never be cropped.
	bool NoOverscan = false;

	// particles
	int Count = 20;
	//! particle size in pixels at 1080p, scaled with the screen height
	float MinSize = 4.0;
	float MaxSize = 12.0;
	float MinDepth = 0.3;
	float MaxDepth = 1.0;
	//! rise speed in screen heights per second at depth 1
	float RiseSpeed = 0.02;

	// cursorlight
	//! diameter as fraction of the screen height
	float Size = 0.7;
	//! how fast the light follows the cursor (higher = snappier)
	float Smoothing = 7.0;
}

class PSY_ILS_ConfigData
{
	int ConfigVersion = 1;
	bool Enabled = true;
	bool OnLoadingScreen = true;
	bool OnLoginScreens = true;
	//! hide the DayZ logo and the "modified version" note on the engine loading screen
	bool HideDayZLogo = false;
	bool HideModdedWarning = false;

	//! horizontal shift of a depth 1 layer, fraction of the screen width
	float MaxShift = 0.04;
	//! vertical shift of a depth 1 layer, fraction of the screen height
	float MaxShiftVertical = 0.025;
	//! roll of a depth 1 layer at full mouse deflection, degrees
	float MaxRollDeg = 0.6;
	//! how fast the camera follows the mouse (higher = snappier)
	float MouseSmoothing = 4.0;
	//! seconds without mouse movement before the camera drifts on its own
	float IdleDriftAfter = 2.5;
	float IdleDriftStrength = 0.45;
	float IdleDriftSpeed = 0.25;
	//! slow zoom pulse, scaled by layer depth
	float BreathingScale = 0.012;
	float BreathingSpeed = 0.35;

	//! engine loading screen (and fallback for the two login screens)
	ref array<ref PSY_ILS_LayerData> Layers;
	//! "Loading in X seconds" countdown; empty = use Layers
	ref array<ref PSY_ILS_LayerData> LoginTimeLayers;
	//! server queue screen; empty = use Layers
	ref array<ref PSY_ILS_LayerData> LoginQueueLayers;

	//! SoundSets that replace the main menu music (CfgSoundSets names); empty = vanilla music
	ref array<string> MenuMusicSoundSets;

	void PSY_ILS_ConfigData()
	{
		Layers = new array<ref PSY_ILS_LayerData>();
		LoginTimeLayers = new array<ref PSY_ILS_LayerData>();
		LoginQueueLayers = new array<ref PSY_ILS_LayerData>();
		MenuMusicSoundSets = new array<string>();
	}

	array<ref PSY_ILS_LayerData> GetLoginTimeLayers()
	{
		if (LoginTimeLayers.Count() > 0)
			return LoginTimeLayers;
		return Layers;
	}

	array<ref PSY_ILS_LayerData> GetLoginQueueLayers()
	{
		if (LoginQueueLayers.Count() > 0)
			return LoginQueueLayers;
		return Layers;
	}

	void Validate()
	{
		MaxShift = Math.Clamp(MaxShift, 0.0, 0.25);
		MaxShiftVertical = Math.Clamp(MaxShiftVertical, 0.0, 0.25);
		MaxRollDeg = Math.Clamp(MaxRollDeg, 0.0, 10.0);
		if (MouseSmoothing <= 0)
			MouseSmoothing = 4.0;
		MouseSmoothing = Math.Clamp(MouseSmoothing, 0.1, 60.0);
		IdleDriftAfter = Math.Max(IdleDriftAfter, 0.0);
		IdleDriftStrength = Math.Clamp(IdleDriftStrength, 0.0, 1.0);
		IdleDriftSpeed = Math.Clamp(IdleDriftSpeed, 0.0, 10.0);
		BreathingScale = Math.Clamp(BreathingScale, 0.0, 0.2);
		BreathingSpeed = Math.Clamp(BreathingSpeed, 0.0, 10.0);

		// arrays missing in the JSON arrive as null
		if (!Layers)
			Layers = new array<ref PSY_ILS_LayerData>();
		if (!LoginTimeLayers)
			LoginTimeLayers = new array<ref PSY_ILS_LayerData>();
		if (!LoginQueueLayers)
			LoginQueueLayers = new array<ref PSY_ILS_LayerData>();
		if (!MenuMusicSoundSets)
			MenuMusicSoundSets = new array<string>();

		ValidateLayers(Layers, "Layers");
		ValidateLayers(LoginTimeLayers, "LoginTimeLayers");
		ValidateLayers(LoginQueueLayers, "LoginQueueLayers");
	}

	protected void ValidateLayers(array<ref PSY_ILS_LayerData> layers, string arrayName)
	{
		for (int i = layers.Count() - 1; i >= 0; i--)
		{
			PSY_ILS_LayerData layer = layers.Get(i);
			if (!layer || layer.Texture == string.Empty)
			{
				PrintFormat("[PSY_ILS] %1[%2] has no texture, skipped", arrayName, i);
				layers.Remove(i);
				continue;
			}

			// JsonSerializer does NOT apply the member initializers to array elements: a key missing
			// in the JSON arrives as 0 / "" (field-verified: layers without "Alpha" were invisible).
			// So 0 means "not set" for every value where 0 makes no sense.
			ApplyLayerDefaults(layer);

			layer.Type.ToLower();
			if (layer.Type != PSY_ILS_Const.TYPE_IMAGE && layer.Type != PSY_ILS_Const.TYPE_PARTICLES && layer.Type != PSY_ILS_Const.TYPE_CURSORLIGHT)
			{
				PrintFormat("[PSY_ILS] %1[%2] has unknown Type '%3', treated as image", arrayName, i, layer.Type);
				layer.Type = PSY_ILS_Const.TYPE_IMAGE;
			}

			layer.Alpha = Math.Clamp(layer.Alpha, 0.0, 1.0);
			layer.Depth = Math.Clamp(layer.Depth, 0.0, 3.0);
			layer.SwayAmplitude = Math.Clamp(layer.SwayAmplitude, 0.0, 0.25);
			if (layer.Count < 0)
				layer.Count = 0;
			if (layer.Count > PSY_ILS_Const.MAX_PARTICLES_PER_LAYER)
				layer.Count = PSY_ILS_Const.MAX_PARTICLES_PER_LAYER;
			layer.MinSize = Math.Max(layer.MinSize, 1.0);
			layer.MaxSize = Math.Max(layer.MaxSize, layer.MinSize);
			layer.MinDepth = Math.Clamp(layer.MinDepth, 0.0, 3.0);
			layer.MaxDepth = Math.Clamp(layer.MaxDepth, layer.MinDepth, 3.0);
			layer.Size = Math.Clamp(layer.Size, 0.01, 4.0);
			layer.Smoothing = Math.Clamp(layer.Smoothing, 0.1, 60.0);
		}
	}

	protected void ApplyLayerDefaults(PSY_ILS_LayerData layer)
	{
		if (layer.Type == string.Empty)
			layer.Type = PSY_ILS_Const.TYPE_IMAGE;
		if (layer.Alpha <= 0)
			layer.Alpha = 1.0;
		if (layer.Count <= 0)
			layer.Count = 20;
		if (layer.MinSize <= 0)
			layer.MinSize = 4.0;
		if (layer.MaxSize <= 0)
			layer.MaxSize = 12.0;
		if (layer.MaxDepth <= 0)
			layer.MaxDepth = 1.0;
		if (layer.RiseSpeed <= 0)
			layer.RiseSpeed = 0.02;
		if (layer.Size <= 0)
			layer.Size = 0.7;
		if (layer.Smoothing <= 0)
			layer.Smoothing = 7.0;
	}
}

class PSY_ILS_Config
{
	protected static ref PSY_ILS_ConfigData s_Data;

	static PSY_ILS_ConfigData Get()
	{
		if (s_Data)
			return s_Data;

		s_Data = Load(PSY_ILS_Const.CONFIG_PROFILE);
		if (!s_Data)
			s_Data = Load(PSY_ILS_Const.CONFIG_PACKED);

		if (!s_Data)
		{
			Print("[PSY_ILS] No config found, interactive loading screen disabled");
			s_Data = new PSY_ILS_ConfigData();
			s_Data.Enabled = false;
		}

		s_Data.Validate();
		return s_Data;
	}

	protected static PSY_ILS_ConfigData Load(string path)
	{
		if (!FileExist(path))
			return null;

		PSY_ILS_ConfigData data = new PSY_ILS_ConfigData();
		string error;
		if (!JsonFileLoader<PSY_ILS_ConfigData>.LoadFile(path, data, error))
		{
			PrintFormat("[PSY_ILS] Failed to load %1: %2", path, error);
			return null;
		}

		PrintFormat("[PSY_ILS] Config loaded from %1", path);
		return data;
	}
}
