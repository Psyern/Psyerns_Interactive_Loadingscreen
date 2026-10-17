// Psyerns Interactive Loadingscreen - parallax scene
//
// Builds the configured layers as ImageWidgets under a full-screen frame and moves
// them every frame against the (smoothed) mouse position. Each layer moves by its
// depth: far layers barely move, near layers move a lot - that difference sells the 3D.

class PSY_ILS_Layer
{
	ImageWidget m_Widget;
	PSY_ILS_LayerData m_Data;
	//! extra size on each side so moving/rolling never reveals an edge (pixels)
	float m_MarginX;
	float m_MarginY;
}

class PSY_ILS_Particle
{
	ImageWidget m_Widget;
	PSY_ILS_LayerData m_Data;
	//! normalized screen position, 0..1 (wraps slightly outside)
	float m_X;
	float m_Y;
	float m_Depth;
	float m_Size;
	float m_Phase;
}

class PSY_ILS_Scene
{
	//! sibling sort offset so the scene always renders behind the vanilla widgets
	protected static const int SORT_BUMP = 10;

	protected Widget m_Root;
	protected PSY_ILS_ConfigData m_Config;
	protected ref array<ref PSY_ILS_Layer> m_Layers;
	protected ref array<ref PSY_ILS_Particle> m_Particles;

	protected ImageWidget m_CursorLight;
	protected PSY_ILS_LayerData m_CursorLightData;
	protected float m_LightX;
	protected float m_LightY;
	protected bool m_LightPlaced;
	protected bool m_Announced;
	protected float m_DiagLogTimer;

	protected float m_Time;
	protected float m_ScreenW;
	protected float m_ScreenH;

	//! smoothed camera offset, -1..1 on both axes
	protected float m_CamX;
	protected float m_CamY;
	//! raw (unsmoothed) target, -1..1
	protected float m_TargetX;
	protected float m_TargetY;

	protected int m_LastMouseX;
	protected int m_LastMouseY;
	protected float m_IdleTime;
	protected float m_IdleWeight;

	//! Creates the scene as first (backmost) child of parent. Returns null when disabled or on error.
	static PSY_ILS_Scene Create(WorkspaceWidget workspace, Widget parent, array<ref PSY_ILS_LayerData> layers)
	{
		PSY_ILS_ConfigData config = PSY_ILS_Config.Get();
		if (!config || !config.Enabled || !layers || layers.Count() == 0)
			return null;

		if (!workspace || !parent)
			return null;

		PSY_ILS_Scene scene = new PSY_ILS_Scene(workspace, parent, config, layers);
		if (!scene.IsValid())
			return null;

		return scene;
	}

	void PSY_ILS_Scene(WorkspaceWidget workspace, Widget parent, PSY_ILS_ConfigData config, array<ref PSY_ILS_LayerData> layers)
	{
		m_Config = config;
		m_Layers = new array<ref PSY_ILS_Layer>();
		m_Particles = new array<ref PSY_ILS_Particle>();
		m_IdleTime = 0;
		m_IdleWeight = 0;

		m_Root = workspace.CreateWidgets(PSY_ILS_Const.LAYOUT_SCENE, parent);
		if (!m_Root)
		{
			Print("[PSY_ILS] Could not create " + PSY_ILS_Const.LAYOUT_SCENE);
			return;
		}

		PushBehindSiblings();

		int sort = 0;
		foreach (PSY_ILS_LayerData data : layers)
		{
			if (data.Type == PSY_ILS_Const.TYPE_PARTICLES)
			{
				sort = BuildParticles(workspace, data, sort);
			}
			else if (data.Type == PSY_ILS_Const.TYPE_CURSORLIGHT)
			{
				if (!m_CursorLight)
				{
					m_CursorLight = CreateImage(workspace, data, sort);
					m_CursorLightData = data;
					sort++;
				}
			}
			else
			{
				ImageWidget image = CreateImage(workspace, data, sort);
				if (image)
				{
					PSY_ILS_Layer layer = new PSY_ILS_Layer();
					layer.m_Widget = image;
					layer.m_Data = data;
					m_Layers.Insert(layer);
				}
				sort++;
			}
		}

		GetMousePos(m_LastMouseX, m_LastMouseY);
	}

	void ~PSY_ILS_Scene()
	{
		if (m_Root)
			m_Root.Unlink();
	}

	bool IsValid()
	{
		return m_Root != null;
	}

	Widget GetRoot()
	{
		return m_Root;
	}

	//! Call every frame while the screen is visible.
	void Update(float timeslice)
	{
		if (!m_Root)
			return;

		float w;
		float h;
		m_Root.GetScreenSize(w, h);
		if (w <= 1 || h <= 1)
		{
			// the tree is not laid out yet (first engine load): fall back to the render resolution
			int screenW;
			int screenH;
			GetScreenSize(screenW, screenH);
			w = screenW;
			h = screenH;
		}

		if (w <= 1 || h <= 1)
			return;

		if (!m_Announced)
		{
			m_Announced = true;
			PrintFormat("[PSY_ILS] Scene active %1x%2, %3 layers, %4 particles, cursor light %5", w, h, m_Layers.Count(), m_Particles.Count(), m_CursorLight != null);
		}

		if (w != m_ScreenW || h != m_ScreenH)
			OnResize(w, h);

		// loading hitches can deliver huge timeslices, keep the smoothing stable
		float dt = Math.Clamp(timeslice, 0.0, 0.1);
		m_Time += dt;

		UpdateCamera(dt);
		UpdateLayers();
		UpdateParticles(dt);
		UpdateCursorLight(dt);

		#ifdef DIAG_DEVELOPER
		m_DiagLogTimer += dt;
		if (m_DiagLogTimer >= 5.0)
		{
			m_DiagLogTimer = 0;
			PrintFormat("[PSY_ILS] tick t=%1 cam=%2/%3 idle=%4", m_Time, m_CamX, m_CamY, m_IdleWeight);
		}
		#endif
	}

	protected void OnResize(float w, float h)
	{
		m_ScreenW = w;
		m_ScreenH = h;

		if (!m_LightPlaced)
		{
			m_LightX = w * 0.5;
			m_LightY = h * 0.5;
			m_LightPlaced = true;
		}

		foreach (PSY_ILS_Layer layer : m_Layers)
		{
			float depth = layer.m_Data.Depth;
			float roll = m_Config.MaxRollDeg * depth * Math.DEG2RAD;
			float rollSin = Math.AbsFloat(Math.Sin(roll));

			layer.m_MarginX = 0;
			layer.m_MarginY = 0;
			if (!layer.m_Data.NoOverscan && (depth > 0 || layer.m_Data.SwayAmplitude > 0))
			{
				// shift + sway + what a rotated rectangle needs to still cover the screen, plus 2% safety
				layer.m_MarginX = m_Config.MaxShift * w * depth + layer.m_Data.SwayAmplitude * w + h * 0.5 * rollSin + w * 0.02;
				layer.m_MarginY = m_Config.MaxShiftVertical * h * depth + w * 0.5 * rollSin + h * 0.02;
			}
		}
	}

	protected void UpdateCamera(float dt)
	{
		int mx;
		int my;
		GetMousePos(mx, my);

		if (mx != m_LastMouseX || my != m_LastMouseY)
		{
			m_LastMouseX = mx;
			m_LastMouseY = my;
			m_IdleTime = 0;
		}
		else
		{
			m_IdleTime += dt;
		}

		float rootX;
		float rootY;
		m_Root.GetScreenPos(rootX, rootY);

		float mouseX = Math.Clamp(((mx - rootX) / m_ScreenW) * 2.0 - 1.0, -1.0, 1.0);
		float mouseY = Math.Clamp(((my - rootY) / m_ScreenH) * 2.0 - 1.0, -1.0, 1.0);

		// no input for a while (or no cursor during a blocking load): let the camera wander on its own
		float idleTarget = 0.0;
		if (m_IdleTime >= m_Config.IdleDriftAfter)
			idleTarget = 1.0;

		m_IdleWeight += (idleTarget - m_IdleWeight) * Math.Clamp(dt * 1.5, 0.0, 1.0);

		float driftT = m_Time * m_Config.IdleDriftSpeed;
		float driftX = Math.Sin(driftT) * m_Config.IdleDriftStrength;
		float driftY = Math.Sin(driftT * 1.37 + 1.3) * m_Config.IdleDriftStrength * 0.6;

		m_TargetX = mouseX * (1.0 - m_IdleWeight) + driftX * m_IdleWeight;
		m_TargetY = mouseY * (1.0 - m_IdleWeight) + driftY * m_IdleWeight;

		float k = Math.Clamp(dt * m_Config.MouseSmoothing, 0.0, 1.0);
		m_CamX += (m_TargetX - m_CamX) * k;
		m_CamY += (m_TargetY - m_CamY) * k;
	}

	protected void UpdateLayers()
	{
		float breath = Math.Sin(m_Time * m_Config.BreathingSpeed);

		foreach (PSY_ILS_Layer layer : m_Layers)
		{
			if (!layer.m_Widget)
				continue;

			PSY_ILS_LayerData data = layer.m_Data;
			float depth = data.Depth;
			float scale = 1.0 + m_Config.BreathingScale * breath * depth;

			float lw = (m_ScreenW + layer.m_MarginX * 2.0) * scale;
			float lh = (m_ScreenH + layer.m_MarginY * 2.0) * scale;

			float sway = 0;
			if (data.SwayAmplitude > 0)
				sway = Math.Sin(m_Time * data.SwaySpeed) * data.SwayAmplitude * m_ScreenW;

			// layers move against the mouse: looking right reveals more of the right side
			float px = (m_ScreenW - lw) * 0.5 - m_CamX * m_Config.MaxShift * m_ScreenW * depth + sway;
			float py = (m_ScreenH - lh) * 0.5 - m_CamY * m_Config.MaxShiftVertical * m_ScreenH * depth;

			layer.m_Widget.SetPos(px, py);
			layer.m_Widget.SetSize(lw, lh);

			if (depth > 0 && m_Config.MaxRollDeg > 0)
				layer.m_Widget.SetRotation(0, 0, m_CamX * m_Config.MaxRollDeg * depth);
		}
	}

	protected void UpdateParticles(float dt)
	{
		float sizeScale = m_ScreenH / 1080.0;

		foreach (PSY_ILS_Particle p : m_Particles)
		{
			if (!p.m_Widget)
				continue;

			PSY_ILS_LayerData data = p.m_Data;

			p.m_Y -= data.RiseSpeed * p.m_Depth * dt;
			p.m_X += Math.Sin(m_Time * 0.6 + p.m_Phase) * 0.01 * p.m_Depth * dt;

			if (p.m_Y < -0.05)
			{
				p.m_Y = 1.05;
				p.m_X = Math.RandomFloatInclusive(-0.05, 1.05);
			}

			if (p.m_X < -0.1)
				p.m_X += 1.2;
			else if (p.m_X > 1.1)
				p.m_X -= 1.2;

			float size = p.m_Size * sizeScale;
			float px = p.m_X * m_ScreenW - m_CamX * m_Config.MaxShift * m_ScreenW * p.m_Depth - size * 0.5;
			float py = p.m_Y * m_ScreenH - m_CamY * m_Config.MaxShiftVertical * m_ScreenH * p.m_Depth - size * 0.5;

			p.m_Widget.SetPos(px, py);
			p.m_Widget.SetSize(size, size);

			float flicker = 0.65 + 0.35 * Math.Sin(m_Time * 1.7 + p.m_Phase * 3.0);
			p.m_Widget.SetAlpha(data.Alpha * flicker);
		}
	}

	protected void UpdateCursorLight(float dt)
	{
		if (!m_CursorLight)
			return;

		float targetX = (m_TargetX + 1.0) * 0.5 * m_ScreenW;
		float targetY = (m_TargetY + 1.0) * 0.5 * m_ScreenH;

		float k = Math.Clamp(dt * m_CursorLightData.Smoothing, 0.0, 1.0);
		m_LightX += (targetX - m_LightX) * k;
		m_LightY += (targetY - m_LightY) * k;

		float size = m_CursorLightData.Size * m_ScreenH;
		m_CursorLight.SetPos(m_LightX - size * 0.5, m_LightY - size * 0.5);
		m_CursorLight.SetSize(size, size);

		// dim the light while the camera is drifting on its own
		m_CursorLight.SetAlpha(m_CursorLightData.Alpha * (1.0 - m_IdleWeight * 0.5));
	}

	protected ImageWidget CreateImage(WorkspaceWidget workspace, PSY_ILS_LayerData data, int sort)
	{
		string layout = PSY_ILS_Const.LAYOUT_IMAGE;
		if (data.Additive)
			layout = PSY_ILS_Const.LAYOUT_IMAGE_ADDITIVE;

		ImageWidget image = ImageWidget.Cast(workspace.CreateWidgets(layout, m_Root));
		if (!image)
		{
			Print("[PSY_ILS] Could not create " + layout);
			return null;
		}

		if (!image.LoadImageFile(0, data.Texture))
			Print("[PSY_ILS] Could not load texture " + data.Texture);

		image.SetImage(0);
		image.SetAlpha(data.Alpha);
		image.SetSort(sort);
		return image;
	}

	protected int BuildParticles(WorkspaceWidget workspace, PSY_ILS_LayerData data, int sort)
	{
		for (int i = 0; i < data.Count; i++)
		{
			ImageWidget image = CreateImage(workspace, data, sort);
			if (!image)
				break;

			PSY_ILS_Particle p = new PSY_ILS_Particle();
			p.m_Widget = image;
			p.m_Data = data;
			p.m_X = Math.RandomFloatInclusive(-0.05, 1.05);
			p.m_Y = Math.RandomFloatInclusive(0.0, 1.05);
			p.m_Depth = Math.RandomFloatInclusive(data.MinDepth, data.MaxDepth);
			p.m_Size = Math.RandomFloatInclusive(data.MinSize, data.MaxSize);
			p.m_Phase = Math.RandomFloatInclusive(0.0, Math.PI2);
			m_Particles.Insert(p);
		}

		return sort + 1;
	}

	//! Gives every vanilla sibling a higher sort so the scene renders first (= behind).
	protected void PushBehindSiblings()
	{
		Widget parent = m_Root.GetParent();
		if (!parent)
			return;

		// collect first: changing the sort may reorder the sibling chain while we walk it
		array<Widget> siblings = new array<Widget>();
		Widget child = parent.GetChildren();
		while (child)
		{
			if (child != m_Root)
				siblings.Insert(child);
			child = child.GetSibling();
		}

		foreach (Widget sibling : siblings)
		{
			sibling.SetSort(sibling.GetSort() + SORT_BUMP);
		}

		m_Root.SetSort(0);
	}
}
