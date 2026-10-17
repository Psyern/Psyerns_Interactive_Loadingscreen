// Psyerns Interactive Loadingscreen - hooks into the vanilla screens
//
// LoadingScreen  : the engine loading screen (game start, joining, map load)
// LoginTimeBase  : "Loading in X seconds" countdown after connecting
// LoginQueueBase : server queue position screen
// In all three the parallax scene replaces the vanilla "ImageBackground"/"Background"
// image and sits behind every vanilla widget (progress bar, hints, buttons stay usable).

modded class LoadingScreen
{
	protected ref PSY_ILS_Scene m_PSY_ILS_Scene;
	protected float m_PSY_ILS_LastTick;

	void LoadingScreen(DayZGame game)
	{
		PSY_ILS_ConfigData config = PSY_ILS_Config.Get();
		if (!config.Enabled)
			return;

		PSY_ILS_HideVanillaBranding();

		if (!config.OnLoadingScreen)
			return;

		Widget parent = m_WidgetRoot.FindAnyWidget("PanelWidget1");
		if (!parent)
			parent = m_WidgetRoot;

		m_PSY_ILS_Scene = PSY_ILS_Scene.Create(game.GetLoadingWorkspace(), parent, config.Layers);
		PSY_ILS_HideVanillaBackground();
	}

	override void Show()
	{
		super.Show();

		// vanilla Show() turns the background image and the logo back on every time
		PSY_ILS_HideVanillaBackground();
		PSY_ILS_HideVanillaBranding();
		PSY_ILS_Tick();
	}

	override void Inc()
	{
		super.Inc();
		PSY_ILS_Tick();
	}

	override void SetProgress(float val)
	{
		super.SetProgress(val);
		PSY_ILS_Tick();
	}

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		PSY_ILS_Tick();
	}

	//! The first engine load is synchronous and never calls OnUpdate, only the progress
	//! callbacks (Inc/SetProgress) - so every entry point ticks, timed by the game clock.
	protected void PSY_ILS_Tick()
	{
		if (!m_PSY_ILS_Scene || !m_DayZGame)
			return;

		float now = m_DayZGame.GetTickTime();
		float dt = now - m_PSY_ILS_LastTick;
		if (m_PSY_ILS_LastTick <= 0 || dt < 0)
			dt = 0;

		m_PSY_ILS_LastTick = now;
		m_PSY_ILS_Scene.Update(dt);
	}

	protected void PSY_ILS_HideVanillaBranding()
	{
		PSY_ILS_ConfigData config = PSY_ILS_Config.Get();
		if (!config.Enabled)
			return;

		if (config.HideDayZLogo)
		{
			if (m_ImageLogoMid)
				m_ImageLogoMid.Show(false);
			if (m_ImageLogoCorner)
				m_ImageLogoCorner.Show(false);
		}

		if (config.HideModdedWarning && m_ModdedWarning)
			m_ModdedWarning.Show(false);
	}

	protected void PSY_ILS_HideVanillaBackground()
	{
		if (!m_PSY_ILS_Scene)
			return;

		if (m_ImageWidgetBackground)
			m_ImageWidgetBackground.Show(false);
	}
}

modded class LoginScreenBase
{
	protected ref PSY_ILS_Scene m_PSY_ILS_Scene;

	protected void PSY_ILS_AttachScene(array<ref PSY_ILS_LayerData> layers)
	{
		if (!layoutRoot)
			return;

		// LoginTimeStatic/LoginQueueStatic run Init() in their constructor and the UIManager runs it
		// again when the menu is shown, which builds a second layoutRoot. A scene still sitting on
		// the first root would tick invisibly while the new root shows the vanilla background.
		if (m_PSY_ILS_Scene)
		{
			Widget sceneRoot = m_PSY_ILS_Scene.GetRoot();
			if (sceneRoot && sceneRoot.GetParent() == layoutRoot)
				return;

			#ifdef DIAG_DEVELOPER
			PrintFormat("[PSY_ILS] %1: layoutRoot was rebuilt, scene moved to the new root", ClassName());
			#endif
			m_PSY_ILS_Scene = null;
		}

		PSY_ILS_ConfigData config = PSY_ILS_Config.Get();
		if (!config.Enabled || !config.OnLoginScreens)
			return;

		m_PSY_ILS_Scene = PSY_ILS_Scene.Create(g_Game.GetWorkspace(), layoutRoot, layers);
		if (!m_PSY_ILS_Scene)
			return;

		Widget background = layoutRoot.FindAnyWidget("Background");
		if (background)
			background.Show(false);
	}

	override void Update(float timeslice)
	{
		super.Update(timeslice);

		// super may have closed the menu (Leave), the scene handles a dead root itself
		if (m_PSY_ILS_Scene)
			m_PSY_ILS_Scene.Update(timeslice);
	}
}

modded class LoginTimeBase
{
	override Widget Init()
	{
		layoutRoot = super.Init();
		PSY_ILS_AttachScene(PSY_ILS_Config.Get().GetLoginTimeLayers());
		return layoutRoot;
	}
}

modded class LoginQueueBase
{
	override Widget Init()
	{
		layoutRoot = super.Init();
		PSY_ILS_AttachScene(PSY_ILS_Config.Get().GetLoginQueueLayers());
		return layoutRoot;
	}
}
