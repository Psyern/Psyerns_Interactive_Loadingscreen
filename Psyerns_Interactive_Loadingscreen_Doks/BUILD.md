# Bauen

## PBO packen

Voraussetzung: DayZ Tools (AddonBuilder).

```powershell
powershell -ExecutionPolicy Bypass -File Psyerns_Interactive_Loadingscreen_Doks\tools\build_pbo.ps1
# optional signieren:
powershell -ExecutionPolicy Bypass -File Psyerns_Interactive_Loadingscreen_Doks\tools\build_pbo.ps1 -SignKey "C:\Keys\Psyern.biprivatekey"
```

Ergebnis: `@Psyerns_Interactive_Loadingscreen/Addons/Psyerns_Interactive_Loadingscreen.pbo` + `mod.cpp`
(der Ordner ist in `.gitignore`).

Manuell im AddonBuilder:
- Quelle: `Psyerns_Interactive_Loadingscreen/`
- **Prefix: `Psyerns_Interactive_Loadingscreen`** – alle Pfade in Scripts und JSON hängen daran
- „Binarize" aus (pack only), Include-Liste: `tools/include.txt`
  (`*.c;*.cpp;*.layout;*.edds;*.json;*.csv;*.xml;*.imageset`) – sonst fehlen JSON/Layouts im PBO

## Texturen neu generieren

Die Standard-Szene ist prozedural erzeugt und jederzeit reproduzierbar:

```
python -m pip install pillow lz4 numpy
python Psyerns_Interactive_Loadingscreen_Doks/tools/psy_ils_generate_textures.py
```

Schreibt alle `.edds` nach `Psyerns_Interactive_Loadingscreen/gui/textures/` und PNG-Vorschauen nach
`Psyerns_Interactive_Loadingscreen_Doks/previews/`. Farben, Motive und Seed (`SEED`) stehen oben im Skript.

## Compile-Test ohne Client

Da der Mod nur `3_Game` nutzt, kompiliert ihn auch ein dedizierter Server:

```
DayZServer_x64.exe -config=serverDZ.cfg -port=2402 -profiles=<temp> -mod=<pfad>\@Psyerns_Interactive_Loadingscreen -dologs
```

Im `<temp>/script_*.log` dürfen keine `SCRIPT (E)`-Zeilen auftauchen. (Auf dem Server wird kein
Loadingscreen gebaut – die Optik prüft man im Client.)
