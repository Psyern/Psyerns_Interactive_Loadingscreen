# Psyerns Interactive Loadingscreen

Interactive, multi-layer parallax loading screen for DayZ. Every layer moves with the mouse
according to its depth, so the flat picture reads as a 3D scene – plus rising embers,
a cursor-following light, idle camera drift and a slow "breathing" zoom.

![Default scene](Psyerns_Interactive_Loadingscreen_Doks/previews/psy_ils_composite.png)

- Replaces the background of the engine loading screen, the login countdown and the queue screen
- Vanilla progress bar, hints and buttons stay on top and keep working
- Fully data-driven: layers, depths, particles and light in one JSON
- Client-only, `3_Game` only, no dependencies, no RPCs
- Ships with a procedural default scene; bring your own artwork for server branding

**Documentation (German):** [Psyerns_Interactive_Loadingscreen_Doks](Psyerns_Interactive_Loadingscreen_Doks/README.md)

| Folder | Content |
|---|---|
| `Psyerns_Interactive_Loadingscreen/` | PBO source, prefix `Psyerns_Interactive_Loadingscreen` |
| `Psyerns_Interactive_Loadingscreen_Doks/` | Documentation, previews, texture generator, build script |

Load it with `-mod=` (client content), not `-serverMod`.
