// Psyerns Interactive Loadingscreen - main menu music
//
// MenuMusicSoundSets in the config replaces the main menu playlist. Hooked in PreloadData()
// instead of RegisterTracksMenu(): map registries (e.g. DynamicMusicPlayerRegistryNasdara)
// override RegisterTracksMenu themselves and would bypass a base-class override.

modded class DynamicMusicPlayerRegistry
{
	override protected void PreloadData()
	{
		super.PreloadData();

		PSY_ILS_ConfigData config = PSY_ILS_Config.Get();
		if (!config.Enabled || config.MenuMusicSoundSets.Count() == 0)
			return;

		m_TracksMenu = new array<ref DynamicMusicTrackData>();
		foreach (string soundSet : config.MenuMusicSoundSets)
		{
			if (soundSet != string.Empty)
				RegisterTrackMenu(soundSet);
		}

		PrintFormat("[PSY_ILS] Menu music replaced: %1 SoundSet(s) on %2", m_TracksMenu.Count(), ClassName());
	}
}
