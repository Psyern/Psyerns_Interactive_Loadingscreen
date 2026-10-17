# Konfiguration

Die Standard-Konfiguration liegt im PBO: `Psyerns_Interactive_Loadingscreen/Data/psy_ils_config.json`.

Zum Ausprobieren ohne neu zu packen kann ein Client eine Kopie nach
`<Client-Profilordner>/Psyerns_Interactive_Loadingscreen/psy_ils_config.json` legen
(Profilordner = `-profiles=` des Clients, standardmäßig `Dokumente\DayZ`). Diese Datei hat Vorrang.
Für den Server-Einsatz gehört die endgültige Konfiguration ins PBO (siehe [CUSTOM_LAYERS.md](CUSTOM_LAYERS.md)).

## Presets

| Datei | Inhalt |
|---|---|
| `Data/psy_ils_config.json` | **aktiv** – Deadmans Echo (identisch mit dem Preset unten) |
| `Data/presets/psy_ils_deadmans_echo.json` | DME-Artworks als Parallax, eigenes Bild je Screen, DayZ-Logo aus, DME-Menümusik ([MIGRATION_FIXEDLOADINGSCREEN.md](MIGRATION_FIXEDLOADINGSCREEN.md)) |
| `Data/presets/psy_ils_dusk.json` | neutrale prozedurale Abenddämmerung, Vanilla-Logo und -Musik |

Preset wechseln: Datei nach `Data/psy_ils_config.json` kopieren und neu packen.

Im Client-Log (`script_*.log`) steht beim Start, welche Datei geladen wurde:
`[PSY_ILS] Config loaded from ...`. Fehlerhafte JSON wird dort mit Grund gemeldet;
fehlt jede Konfiguration, bleibt der Vanilla-Loadingscreen aktiv.

## Globale Optionen

| Schlüssel | Standard | Bereich | Bedeutung |
|---|---|---|---|
| `ConfigVersion` | 1 | – | Formatversion |
| `Enabled` | true | – | Hauptschalter |
| `OnLoadingScreen` | true | – | Szene im Engine-Loadingscreen |
| `OnLoginScreens` | true | – | Szene im Login-Countdown und in der Warteschlange |
| `HideDayZLogo` | false | – | DayZ-Logo im Engine-Loadingscreen ausblenden |
| `HideModdedWarning` | false | – | Hinweis „modifizierte Version des Spiels" ausblenden |
| `MenuMusicSoundSets` | [] | – | SoundSet-Namen (aus `CfgSoundSets`), die die Hauptmenü-Musik ersetzen; leer = Vanilla-Musik. Mitgeliefert: `PSY_ILS_DME_Menu_SoundSet` |
| `MaxShift` | 0.04 | 0–0.25 | Horizontale Verschiebung einer Ebene mit `Depth` 1, Anteil der Bildschirmbreite |
| `MaxShiftVertical` | 0.025 | 0–0.25 | Vertikale Verschiebung bei `Depth` 1, Anteil der Bildschirmhöhe |
| `MaxRollDeg` | 0.6 | 0–10 | Neigung (Grad) einer `Depth`-1-Ebene bei voller Mausauslenkung; 0 = aus |
| `MouseSmoothing` | 4.0 | 0.1–60 | Wie schnell die Kamera der Maus folgt (höher = direkter) |
| `IdleDriftAfter` | 2.5 | ≥ 0 | Sekunden ohne Mausbewegung bis zum automatischen Schwenk |
| `IdleDriftStrength` | 0.45 | 0–1 | Stärke des automatischen Schwenks |
| `IdleDriftSpeed` | 0.25 | 0–10 | Geschwindigkeit des automatischen Schwenks |
| `BreathingScale` | 0.012 | 0–0.2 | Zoom-Puls (× Tiefe); 0 = aus |
| `BreathingSpeed` | 0.35 | 0–10 | Geschwindigkeit des Zoom-Pulses |
| `Layers` | – | – | Ebenen des Engine-Loadingscreens, **von hinten nach vorne** |
| `LoginTimeLayers` | [] | – | Eigene Ebenen für den Login-Countdown; leer = `Layers` |
| `LoginQueueLayers` | [] | – | Eigene Ebenen für die Warteschlange; leer = `Layers` |

> **Am besten alle Schlüssel ausschreiben.** Fehlende Schlüssel kommen beim JSON-Laden als `0` an.
> Für Werte, bei denen 0 sinnlos ist (`Alpha`, `Count`, `MinSize`, `MaxSize`, `MaxDepth`, `RiseSpeed`, `Size`,
> `Smoothing`, `MouseSmoothing`), setzt der Mod dann den Standard ein. Bei Werten, wo 0 eine Bedeutung hat
> (`Depth`, `MaxShift`, `MaxRollDeg`, `SwayAmplitude` …), bleibt es bei 0 – z. B. fehlendes `Depth` = statische Ebene.
> Eine Ebene ganz ausblenden: aus dem Array entfernen (Alpha 0 wird als „nicht gesetzt" = 1 gewertet).

## Ebenen (`Layers`, `LoginTimeLayers`, `LoginQueueLayers`)

Alle drei Arrays haben dasselbe Format. Die Reihenfolge im Array ist die Zeichenreihenfolge: der erste Eintrag liegt ganz hinten.
Jeder Eintrag hat einen `Type`:

### `image` – Bildebene

| Schlüssel | Standard | Bedeutung |
|---|---|---|
| `Texture` | – | Pfad zur `.edds` (Pflicht) |
| `Depth` | 0.5 | 0 = bewegt sich nicht (Vignette, Logo-Overlay), 1 = Vordergrund, bis 3 = noch näher |
| `Alpha` | 1.0 | Deckkraft 0–1 |
| `Additive` | false | Additive Blendung (für Licht/Glow-Ebenen) |
| `SwayAmplitude` | 0 | Horizontales Pendeln, Anteil der Bildschirmbreite (0–0.25) |
| `SwaySpeed` | 0 | Pendelgeschwindigkeit in rad/s |
| `NoOverscan` | false | `true`: Ebene bleibt exakt bildschirmgroß (siehe unten) |

Die Ebene wird automatisch so viel größer als der Bildschirm gezogen, dass Verschiebung,
Pendeln und Neigung nie einen Rand freilegen. Ebenen mit `Depth` 0 und ohne Sway füllen
exakt den Bildschirm. Für Schrift- oder Logo-Ebenen mit transparentem Rand, deren Inhalt nah am
Bildrand liegt, verhindert `NoOverscan` das Abschneiden – die Ebene bewegt sich trotzdem.

### `particles` – Partikelschwarm

| Schlüssel | Standard | Bedeutung |
|---|---|---|
| `Texture` | – | Partikel-Textur (weicher Punkt) |
| `Count` | 20 | Anzahl (max. 200) |
| `MinSize` / `MaxSize` | 4 / 12 | Größe in Pixeln bei 1080p (skaliert mit der Auflösung) |
| `MinDepth` / `MaxDepth` | 0.3 / 1.0 | Zufällige Tiefe je Partikel (Parallax + Steiggeschwindigkeit) |
| `RiseSpeed` | 0.02 | Bildschirmhöhen pro Sekunde bei Tiefe 1 |
| `Alpha` | 1.0 | Maximale Deckkraft (flackert darunter) |
| `Additive` | false | Für Glut/Funken `true` |

### `cursorlight` – Licht am Cursor

| Schlüssel | Standard | Bedeutung |
|---|---|---|
| `Texture` | – | Radialer Verlauf |
| `Size` | 0.7 | Durchmesser, Anteil der Bildschirmhöhe |
| `Smoothing` | 7.0 | Wie schnell das Licht dem Cursor folgt |
| `Alpha` | 1.0 | Helligkeit |
| `Additive` | false | Für Licht `true` |

Nur der erste `cursorlight`-Eintrag wird verwendet.

## Szene „Deadmans Echo" (aktiv)

Je Screen dieselbe Struktur, nur das Artwork wechselt (DME1 Loadingscreen, DME2 Countdown, DME3 Queue):

| # | Typ | Inhalt | Depth |
|---|---|---|---|
| 0 | image | Artwork-Hintergrund (Schrift herausretuschiert) | 0.06 |
| 1 | image | Rauchband (pendelt) | 0.3 |
| 2 | particles | ferne Glut, 22 Stück | 0.25–0.45 |
| 3 | image | dunkler Boden | 0.5 |
| 4 | cursorlight | warmes Licht | – |
| 5 | image | Gesichter-Silhouetten | 0.8 |
| 6 | particles | nahe Glut, 16 Stück | 0.8–1.3 |
| 7 | image | „DEADMANS ECHO" + URL (`NoOverscan`) | 0.3 |
| 8 | image | Vignette | 0 |

## Szene „Dusk" (Preset)

![Dusk im Spiel](previews/psy_ils_ingame.png)

| # | Typ | Textur | Depth |
|---|---|---|---|
| 0 | image | Himmel mit Mond und Sternen | 0.05 |
| 1 | image | Bergketten | 0.15 |
| 2 | image | Wald-Hügel mit Funkmast | 0.3 |
| 3 | image | Nebelband (pendelt) | 0.42 |
| 4 | particles | ferne Glut, 24 Stück | 0.3–0.55 |
| 5 | image | Dorf mit Kirche, Strommasten, beleuchteten Fenstern | 0.6 |
| 6 | cursorlight | warmes Licht | – |
| 7 | particles | nahe Glut, 18 Stück | 0.8–1.3 |
| 8 | image | Vordergrund: Tannen, Gras | 1.0 |
| 9 | image | Vignette | 0 |
