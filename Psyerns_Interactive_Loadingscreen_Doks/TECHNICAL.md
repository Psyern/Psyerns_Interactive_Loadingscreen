# Technik

## Einstiegspunkte (alle `3_Game`)

| Hook | Vanilla-Stelle | Was passiert |
|---|---|---|
| `modded class LoadingScreen` – Konstruktor | `DayZGame.c`, `new LoadingScreen(this)` beim Spielstart | Szene als erstes Kind von `PanelWidget1` im Loading-Workspace bauen, `ImageBackground` ausblenden |
| `LoadingScreen.Show()` | wird bei jedem Ladevorgang aufgerufen und schaltet `ImageBackground` wieder ein | nach `super` wieder ausblenden |
| `LoadingScreen.OnUpdate(timeslice)` | `DayZGame.OnUpdate`, solange `IsLoading()` | Szene animieren |
| `LoadingScreen.Inc()` / `SetProgress()` | Progress-Callbacks der Engine | Szene ticken (siehe unten) |
| `LoginTimeBase.Init()` / `LoginQueueBase.Init()` | Login-Countdown / Queue (auch die `*Static`-Varianten bei `-connect`) | Szene in `layoutRoot`, `Background` ausblenden |
| `LoginScreenBase.Update(timeslice)` | UIManager bzw. `DayZGame.OnUpdate` für die Static-Varianten | Szene animieren |

Alle Einstiegspunkte rufen `PSY_ILS_Tick()` auf, das die Zeit selbst über `GetTickTime()` misst.
Riesige Frame-Zeiten nach einem Hänger werden auf 0.1 s begrenzt, damit die Glättung nicht springt.

### Verhalten je Ladephase (im Client getestet, DayZ 1.30 Diag)

| Phase | Script pro Frame? | Szene |
|---|---|---|
| Erster Spielstart (Engine lädt synchron) | **nein** – der Ladebalken läuft nativ über `ProgressAsync`, Script bekommt nur `Inc()` | vollständig gerendertes **Standbild** (Layout beim ersten Tick) |
| Server-Join / Map-Load / Login-Countdown | ja (`OnUpdate` / `Update`) | voll animiert: Maus-Parallax, Idle-Drift, Partikel, Licht |

Das Standbild beim allerersten Start ist eine Engine-Grenze und betrifft jeden Loadingscreen-Mod.
Mit `-doScriptLogs` in DayZDiag schreibt die Szene alle 5 s `[PSY_ILS] tick t=… cam=…/… idle=…`
(nur unter `DIAG_DEVELOPER`, im Retail-Spiel nicht kompiliert).

## Z-Reihenfolge

Die Szene wird als Kind des Vanilla-Panels erzeugt. Damit sie **hinter** allen Vanilla-Widgets liegt,
erhöht `PSY_ILS_Scene.PushBehindSiblings()` die `sort`-Werte aller Geschwister um 10 (relative Reihenfolge
bleibt erhalten) und setzt die eigene auf 0. Die Geschwister werden vorher in ein Array gesammelt, weil
`SetSort` die Geschwisterkette umsortieren kann. Innerhalb der Szene bekommt jede Ebene `sort` = Index.

## Parallax-Mathematik

```
maus      = Cursor relativ zur Szene, normiert auf -1..1
ziel      = mix(maus, lissajous_drift, idleWeight)
kamera   += (ziel - kamera) * clamp(dt * MouseSmoothing)

pro Ebene (Tiefe d):
  rand_x  = MaxShift*W*d + Sway*W + H/2*|sin(roll_max)| + 2 % W
  rand_y  = MaxShiftVertical*H*d + W/2*|sin(roll_max)| + 2 % H
  größe   = (Bildschirm + 2*rand) * (1 + BreathingScale*sin(t)*d)
  pos     = zentriert - kamera * MaxShift*W*d + sway
  rotation (roll) = kamera_x * MaxRollDeg * d   (Pivot 0.5/0.5 aus dem Layout)
```

Der Rand wird bei jeder Auflösungsänderung neu berechnet, sodass nie ein Bildrand sichtbar wird.

## Konfiguration laden

`PSY_ILS_Config.Get()` lädt einmal und cached: zuerst `$profile:Psyerns_Interactive_Loadingscreen/psy_ils_config.json`,
dann die gepackte `Psyerns_Interactive_Loadingscreen/Data/psy_ils_config.json` (`JsonFileLoader.LoadFile`).
`Validate()` klemmt alle Werte in sichere Bereiche, verwirft Ebenen ohne Textur und behandelt unbekannte Typen als `image`.

**Falle (im Client verifiziert):** `JsonSerializer` übernimmt bei Array-Elementen die Member-Initialisierer
(`float Alpha = 1.0;`) **nicht** – ein im JSON fehlender Schlüssel kommt als `0` / `""` an. Ohne `"Alpha"` war eine
Ebene deshalb unsichtbar. `ApplyLayerDefaults()` setzt darum für alle Werte, bei denen 0 keinen Sinn ergibt,
den Standard ein (`Alpha`, `Count`, `MinSize`, `MaxSize`, `MaxDepth`, `RiseSpeed`, `Size`, `Smoothing`, leerer `Type`).

## Texturen

UI-`.edds` müssen Power-of-Two sein. Der Generator (`tools/psy_ils_generate_textures.py`) enthält einen
eigenen Enfusion-EDDS-Encoder (BGRA8, LZ4-Blöcke in 64-KB-Chunks, Mips kleinste zuerst).

## Kompatibilität

- Keine Abhängigkeiten, nur `DZ_Data`/`DZ_Scripts`.
- Expansion/Namalsk-Loadingscreens tauschen das (bei uns ausgeblendete) Hintergrundbild – beide Mods laufen nebeneinander,
  sichtbar ist die Parallax-Szene. Logos, Hints und Ladebalken anderer Mods bleiben im Vordergrund.
- Kein `GetGame()` (DayZ 1.29+), keine 1.30-only-APIs.
