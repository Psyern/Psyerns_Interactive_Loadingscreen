# Eigene Ebenen / Server-Branding

## 1. Artwork vorbereiten

- Pro Ebene ein **PNG mit Transparenz**. Alles, was hinter dieser Ebene sichtbar bleiben soll, ist transparent.
- Größe **Power-of-Two**, empfohlen **2048 × 1024** (wird auf den Bildschirm gestreckt).
  DayZ rendert Nicht-Power-of-Two-UI-Texturen als weiße/einfarbige Blöcke – daran scheitern die meisten eigenen Loadingscreens.
- Wichtige Motive nicht an den Rand legen: Ebenen werden je nach Tiefe bis zu ~8 % größer als der Bildschirm gezogen.
- Gute Aufteilung aus einem Foto/Gemälde: Himmel · Horizont · Mittelgrund · Vordergrund · optional Logo (`Depth` 0).

## 2. In .edds umwandeln

```
python -m pip install pillow lz4 numpy
python Psyerns_Interactive_Loadingscreen_Doks/tools/psy_ils_generate_textures.py --convert mein_himmel.png Psyerns_Interactive_Loadingscreen/gui/textures/mein_himmel.edds
```

Der Konverter schreibt das Enfusion-Format (BGRA8, LZ4-Mipmaps) und bricht bei Nicht-Power-of-Two-Größen ab.
`ImageToPAA.exe` aus den DayZ Tools ist dafür **nicht** geeignet – es schreibt PAA-Daten, auch wenn die Datei `.edds` heißt.

## 3. In die Konfiguration eintragen

```json
"Layers": [
	{ "Type": "image", "Texture": "Psyerns_Interactive_Loadingscreen/gui/textures/mein_himmel.edds", "Depth": 0.05 },
	{ "Type": "image", "Texture": "Psyerns_Interactive_Loadingscreen/gui/textures/meine_stadt.edds", "Depth": 0.5 },
	{ "Type": "image", "Texture": "Psyerns_Interactive_Loadingscreen/gui/textures/mein_vordergrund.edds", "Depth": 1.0 },
	{ "Type": "image", "Texture": "Psyerns_Interactive_Loadingscreen/gui/textures/mein_logo.edds", "Depth": 0.0 }
]
```

Faustregeln für die Tiefe:
- Himmel 0.02–0.08, Berge 0.1–0.2, Mittelgrund 0.3–0.6, Vordergrund 0.9–1.3
- Große Unterschiede zwischen benachbarten Ebenen = stärkerer 3D-Effekt, aber auch mehr „Pappaufsteller"-Look.
- Logos/Text mit `Depth` 0 bleiben fest stehen. Sollen sie sich mitbewegen, aber nie am Rand abgeschnitten werden: `"NoOverscan": true`.

Texturen dürfen auch aus einem **anderen** PBO kommen (z. B. ein eigenes Branding-Addon) –
der Pfad ist dann einfach dessen Prefix.

## Flaches Artwork automatisch zerlegen

Hast du nur ein fertiges Bild (Logo + Motiv in einem), zeigt `tools/psy_ils_build_dme_preset.py`, wie man es
per Skript in Ebenen zerlegt: Schrift per Farbmaske freistellen und im Hintergrund retuschieren, Randmotive als
nahe Ebene ausschneiden, unteren Bildteil als Boden. Das Skript ist auf das DME-Template zugeschnitten
(`TEXT_BOXES`, Bandbreiten) und lässt sich für andere Artworks anpassen.

## 4. Testen ohne Neupacken

JSON nach `<Client-Profilordner>/Psyerns_Interactive_Loadingscreen/psy_ils_config.json` kopieren,
DayZ starten, Ergebnis ansehen, anpassen. Texturen müssen dafür aber bereits in einem geladenen PBO liegen.

## 5. Ausliefern

Konfiguration nach `Psyerns_Interactive_Loadingscreen/Data/psy_ils_config.json` übernehmen, PBO bauen
([BUILD.md](BUILD.md)), signieren, `.bikey` auf den Server.
