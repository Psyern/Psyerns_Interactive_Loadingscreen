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
		if (!config.Enabled || !config.OnLoadingScreen)
			return;

		Widget parent = m_WidgetRoot.FindAnyWidget("PanelWidget1");
		if (!parent)
			parent = m_WidgetRoot;

		m_PSY_ILS_Scene = PSY_ILS_Scene.Create(game.GetLoadingWorkspace(), parent);
		PSY_ILS_HideVanillaBackground();
	}

	override void Show()
	{
		super.Show();

		// vanilla Show() turns the background image back on every time
		PSY_ILS_HideVanillaBackground();
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

	protected void PSY_ILS_AttachScene()
	{
		if (!layoutRoot || m_PSY_ILS_Scene)
			return;

		PSY_ILS_ConfigData config = PSY_ILS_Config.Get();
		if (!config.Enabled || !config.OnLoginScreens)
			return;

		m_PSY_ILS_Scene = PSY_ILS_Scene.Create(g_Game.GetWorkspace(), layoutRoot);
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
		PSY_ILS_AttachScene();
		return layoutRoot;
	}
}

modded class LoginQueueBase
{
	override Widget Init()
	{
		layoutRoot = super.Init();
		PSY_ILS_AttachScene();
		return layoutRoot;
	}
}
