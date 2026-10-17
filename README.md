# Psyerns Interactive Loadingscreen

Interactive, multi-layer parallax loading screen for DayZ. Every layer moves with the mouse
according to its depth, so the flat picture reads as a 3D scene – plus rising embers,
a cursor-following light, idle camera drift and a slow "breathing" zoom.

![Deadmans Echo preset in game](Psyerns_Interactive_Loadingscreen_Doks/previews/psy_ils_dme_ingame_loading.png)

Replaces the old **FixedLoadingScreen** mod: the active config is the *Deadmans Echo* preset (the three DME artworks
split into parallax layers, one per screen, DayZ logo hidden, DME menu music). A neutral procedural dusk scene ships
as a preset in `Data/presets/`.

- Replaces the background of the engine loading screen, the login countdown and the queue screen
- Vanilla progress bar, hints and buttons stay on top and keep working
- Fully data-driven: layers, depths, particles and light in one JSON; separate scenes per screen
- Optional branding: hide the DayZ logo / modded note, replace the main menu music
- Client-only, `3_Game` only, no dependencies, no RPCs
- Ships with a procedural default scene; bring your own artwork for server branding

**Documentation (German):** [Psyerns_Interactive_Loadingscreen_Doks](Psyerns_Interactive_Loadingscreen_Doks/README.md)

| Folder | Content |
|---|---|
| `Psyerns_Interactive_Loadingscreen/` | PBO source, prefix `Psyerns_Interactive_Loadingscreen` |
| `Psyerns_Interactive_Loadingscreen_Doks/` | Documentation, previews, texture generator, build script |

Load it with `-mod=` (client content), not `-serverMod`.
