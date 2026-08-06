# PlayableConfigurable

A KenshiLib/RE_Kenshi preload plugin. Makes every race and subrace
configurable and playable from an in-game window, re-evaluated against
whatever mods are active on each launch - no static patch to regenerate
when the mod list changes.

## Architecture

Loads as a **preload** plugin (`RE_Kenshi.json` declares `PreloadPlugins`),
not postload:

- RE_Kenshi's postload phase only loads plugins for mods listed in
  `ou->activeMods`. Its preload phase loads every *available* mod's plugin
  regardless of enablement (`ou->availabelModsOrderedList`), so
  `ModEnabled()` reads `data\mods.cfg` itself at startup and stays fully
  inert (no hooks, no work) if `PlayableConfigurable.mod` isn't in the
  active list.
- Preload timing is also what lets `Apply()` run before the engine builds
  the `RaceGroupData`/`RaceData` caches the character editor actually
  reads - a postload plugin would edit the data too late for those caches
  to reflect it.

### Visibility model

A race is selectable in the creator when three things are all true:
`playable` bool, an editor-limits xml file, and membership in >=1
`RACE_GROUP`. An optional fourth gate applies per-start: `NEW_GAME_STARTOFF`
records can carry a `force race` list restricting that specific start to
certain races (vanilla example: Fishman is playable/has limits/has a
group, but no start's allow-list includes it) - `@clearForceRace` clears
these lists.

- **Enable**: `playable = true`; borrow an editor-limits xml via
  `animalLimitsFile` if missing (only when `allowAnimals` is set); ensure
  group membership, creating a plugin-owned `PCG::<sid>` group only if the
  race has no group at all.
- **Disable**: `playable = false` only. Group membership is left untouched
  - `RaceData::raceGroup`/`isRelatedRace(...)` are gameplay-facing (not
  editor-only) fields, and `Item::canEquip(RaceData*, bool)` takes a race
  directly, so removing a group a mod authored for other reasons could
  plausibly affect equipment-fit/relatedness checks on real NPCs.

Both actions no-op if the race is already in the desired state - a fresh
install with no config produces `0 enabled, 0 disabled`.

### Hooks

| Hook | Purpose |
|---|---|
| `GameDataManager::postProcessingTheDatas` | `Apply()` runs before the original, so edits land before the engine's caches are built. Also the re-apply point for later reloads (save import, mod-list change). |
| `TitleScreen::_CONSTRUCTOR` | Builds the RACES button + window the first time the title screen exists. |
| `TitleScreen::_NV_show` | Mirrors the RACES button's visibility onto the title screen's own show/hide, so it only appears on the main menu. |

### Scope

`RaceData` - the runtime object gameplay actually consults - has no
`playable` field, so NPCs, animals, factions, combat, and existing saves
are untouched. Only character creation and the character editor read the
gated data.

## Config file

`PlayableConfigurable.config.txt`, plain text:

```
on|<stringId>|<name>|<note>
@allowAnimals = true|false
@animalLimitsFile = <path>
@clearForceRace = true|false
@lang = <locale code, e.g. ru_RU>
```

Location: `mods\PlayableConfigurable\` if that folder exists (manual/dev
install), else `%LOCALAPPDATA%\kenshi\` (Workshop installs, since the
Steam-managed mod folder gets revalidated/wiped on update).

A race absent from the config defaults to `Orig` - its own pristine
visibility as captured the first time `Apply` saw it (i.e. whatever the
currently loaded mods themselves assert). Races no longer in the current
load order are kept as inert `#~`-prefixed lines and reactivate
automatically if their mod returns.

## Localization

The plugin's own UI (RACES button, window title, tier dividers, presets,
tooltips, option toggles, language controls) supports **12 languages**,
stored in one `STR[L_COUNT][T_COUNT]` table and one `LANGS[L_COUNT]` array
of `{code, full, testcp}` structs.

**9 are Kenshi's own official languages** (matches FCS's Translation Mode
list and the `locale/` folders Kenshi ships): English, Russian, Spanish,
Chinese Simplified, German, French, Japanese, Korean, Portuguese (Brazil).
**3 are opportunistic extras** with no Kenshi-shipped locale at all:
Ukrainian, Polish, Chinese Traditional - included because community
translation mods commonly ship fonts for them, and the font-safety check
below doesn't care whether coverage came from Kenshi or a mod.

### Language selection

- `g_lang` (currently displayed) is detected once at startup from
  `language=` in Kenshi's own `settings.cfg`, via `DetectLang()` ->
  `LangByCode()` (exact match first, 3-char prefix fallback - needed
  because `zh_CN`/`zh_TW` share a prefix but are different scripts).
- A button (`optL`) cycles `g_lang` through all 12 unconditionally on
  click, persisting the choice to `@lang`. Its caption is
  `LANGS[g_lang].full` - a fixed "English name | native name" pair, not
  itself translated, so the label stays identifiable regardless of what
  `g_lang` currently is.
- `g_autoLang` is a second, independent index: Kenshi's own detected
  client language, set once at startup and never changed by
  `@lang`/cycling. It exists only so the font-warning banner (below) can
  speak a language guaranteed to actually render.

### Font-safety check

Kenshi loads exactly one Cyrillic/CJK-capable font override at a time,
matching whatever `language=` Kenshi itself is set to
(`locale/<code>/gui/fonts/kenshi_fonts.xml`); `en_GB` is the only locale
with no override (bare ASCII). Picking a `g_lang` other than Kenshi's own
can render as blank glyphs.

`FontHas(cp)` checks this directly rather than trusting the language
code: it looks up whatever font resource is *actually* registered right
now (`MyGUI::FontManager::getByName("Kenshi_StandardFont_Medium")` ->
`ResourceTrueTypeFont::getCodePointRanges()`), so it's correct regardless
of whether coverage came from Kenshi's own locale or a translation mod
redefining the same resource name. `LANGS[i].testcp` is one representative
codepoint per language, each verified against the real declared `<Code
range>` in every `locale/<code>/gui/fonts/kenshi_fonts.xml` - chosen to be
a character the language actually needs elsewhere in its own strings, not
just any non-ASCII character from its own name (e.g. German's `testcp` is
`ä`, not a letter from "Deutsch," which has none).

`Draw()` checks `FontHas(LANGS[g_lang].testcp)` every redraw. If it
fails, `langWarn` (a full-width row below the language button) shows
`STR[g_autoLang][T_FONT_WARN]` - the warning translated into Kenshi's
*own* client language, since that's the one language `FontHas` can never
fail for. Hovering it shows a fixed, deliberately untranslated English
explanation (`OnFontWarnTip`) as a universal fallback. The font check
gates the warning only, never selection - `optL` always offers all 12
regardless of what's currently renderable.

### Data table

- `STR[L_COUNT][T_COUNT]`: one row per language, looked up via `T(id)`
  (current `g_lang`) and formatted via `Fmt(id, arg)` for `%s`/`%d`
  templates.
- `LANGS[L_COUNT]`: `{code, full, testcp}` per language - single source
  of truth for `LangByCode`, `DetectLang`, `OnLang`, `SyncLangBtn`, and
  the font check.
- Adding a language = one `STR[]` row + one `LANGS[]` entry, no other
  code changes.
- Non-ASCII table entries are `\xHH`-escaped UTF-8 (not raw literal
  characters), since this toolchain doesn't reliably treat a BOM-less
  source file as UTF-8. Every escape run is isolated in its own adjacent
  string-literal segment (`"\xC3\xA9" "buts"`, not `"\xA9buts"`) - `\x`
  in C/C++ greedily consumes any following hex-digit character, so an
  escape immediately followed by a literal `b`/`c`/`d`/`f` etc. merges
  into one oversized, uncompilable escape otherwise.
- Terminology for shared concepts (e.g. "Race") is cross-checked against
  Kenshi's own shipped translation catalogs
  (`locale/<code>/LC_MESSAGES/main.po`).

### Race/mod name translation

Race and category names (`e.name`/`c.name`) are read straight from
`GameData::name` and not translated by this plugin. `fcs.def` flags
`RACE`/`RACE_GROUP` as `TRANSLATE: ALL`, and Kenshi's shipped
`locale/<code>/gamedata.po` catalogs have matching entries keyed by record
ID, so vanilla/base-game names should already reflect Kenshi's own
localization by the time this plugin reads them (inferred from catalog
structure, not directly confirmed in-game). Third-party mod races only
translate if that mod shipped its own catalog. Mod/file names shown in
tooltips (e.g. `small_changes_otto.mod`) are literal identifiers and
intentionally stay untranslated - only the wrapping template text
("Mod: ", "Vanilla (", "Ungrouped: ") is.

### Buffer safety

Any fixed-size `sprintf_s` buffer fed by mod-provided text (category
names, origin filenames) is a crash vector, disproportionately for
non-Latin scripts (3 UTF-8 bytes/char for CJK vs. 1 for Latin) - MSVC's
`_s` family terminates the process on overflow rather than truncating.
`Draw()`'s category row avoids this by building only the numeric suffix
into a fixed buffer, then concatenating the unbounded mod name onto it as
a `std::string` (no fixed limit) - the same pattern the race-row branch
and tooltip text already use. `Fmt()`'s buffer is 2048 bytes, sized for a
worst-case 255-character NTFS filename component in CJK (~765 UTF-8
bytes).

Tooltip sizing uses `Glyphs()` (counts UTF-8 lead bytes) rather than
`std::string::size()` (raw bytes), since one visual character can be 2-3
bytes.

## Build environment (one-time)

1. VC++ 2010 x64 platform toolset. Visual Studio 2010 Ultimate ISO from
   archive.org (hash-verified against the published MD5/SHA1 before
   mounting), installing only Visual C++ -> X64 Compilers and Tools.
   Required by RE_Kenshi/KenshiLib.
2. KenshiLib SDK: `BFrizzleFoShizzle/KenshiLib_Examples_deps` (Boost 1.60
   + Ogre + MyGUI headers, prebuilt libs) plus the KenshiLib release zip
   (`KenshiReclaimer/KenshiLib`) for `KenshiLib.lib` and `Include/`.
   Staged under `sdk/` next to this project; `Directory.Build.props`
   resolves `KENSHILIB_DIR` / `BOOST_INCLUDE_PATH`.

```
msbuild PlayableConfigurable.vcxproj /p:Configuration=Release /p:Platform=x64
```

## Shipping / Deploy

`dist/PlayableConfigurable/` holds exactly what users get: a **0-record
stub `.mod`** (the carrier so Kenshi lists the folder), `RE_Kenshi.json`
declaring `PreloadPlugins`, the DLL, `README.txt`, and `COPYING.txt`
(GPLv3, required - KenshiLib linkage). Source itself is not bundled
inline - it's at
https://github.com/RongSongRangeria/PlayableConfigurable (linked from
`README.txt`), which is what this repository is. To deploy manually,
copy into `...\Kenshi\mods\PlayableConfigurable\`:

- `PlayableConfigurable.dll`
- `PlayableConfigurable.mod`
- `RE_Kenshi.json` - **must be written without a UTF-8 BOM.** RE_Kenshi's
  JSON parser rejects a BOM with `Invalid value` and the plugin silently
  never loads. `[System.IO.File]::WriteAllText(path, json, (New-Object
  System.Text.UTF8Encoding($false)))` in PowerShell; most editors need an
  explicit "UTF-8 without BOM" save option.

Config is written by the in-game window at
`mods\PlayableConfigurable\PlayableConfigurable.config.txt` if that folder
exists, else `%LOCALAPPDATA%\kenshi\PlayableConfigurable.config.txt`.

Requires RE_Kenshi with KenshiLib. `PreloadPlugins` (what this plugin
uses) was introduced in v0.3.0 - built and tested against v0.3.4.

**Before uploading, diff the live install's `.mod` against `dist/`'s.**
Running the generator, or any test that touches the live
`mods\PlayableConfigurable\` folder, silently overwrites the shipping
0-record stub with a real generated one. FCS uploads whatever is in the
live install folder, not `dist/`, so a stale stub here ships
broken/frozen data to every subscriber without any build error to catch
it.

## Verified behavior

- Fresh install, no config: `0 enabled, 0 disabled` - a true no-op
  against whatever the loaded mods themselves mark playable.
- Toggling in-game applies live, no restart, confirmed by reading the
  actual character creator (not just the log) before and after a toggle.
- Disabling the mod in `mods.cfg` leaves the plugin fully inert (confirmed
  by log: it loads, checks, and returns before touching anything).
- `Unlock forced starts`: turning it on takes effect immediately; turning
  it back off needs a relaunch, since the original per-start restriction
  is only ever cleared, never restored from memory.

## Caveats

- GPLv3: KenshiLib linkage makes the plugin GPLv3. Publishing the DLL
  requires publishing this source under GPLv3 (see `COPYING.txt` and the
  License section of the user-facing `README.txt`). Private use has no
  obligation.

---

## License

GPLv3. This plugin links KenshiLib (GPLv3), so publishing it requires
publishing this source under GPLv3 too - that's what this repository is.
`dist/PlayableConfigurable/COPYING.txt` ships the full license text
alongside the DLL. Built against
[RE_Kenshi](https://github.com/BFrizzleFoShizzle/RE_Kenshi).

---

## Alias key

`PlayableConfigurable.cpp` uses short aliases throughout. Every one is
defined at the top of the file and listed here.

### Type aliases

| Alias | Expands to | What it is |
|---|---|---|
| `S` | `std::string` | string |
| `SS` | `std::set<S>` | set of stringIds |
| `SBM` | `std::map<S, bool>` | stringId -> flag map (config states, expand memo) |
| `SIM` | `std::map<S, int>` | stringId -> index map |
| `GD` | `GameData` | one loaded game record (RACE, RACE_GROUP, ...) |
| `GDM` | `GameDataManager` | the live record container (`ou->gamedata`) |
| `RV` | `const Ogre::vector<GameDataReference>::type` | a record's reference list (e.g. a group's `races`) |
| `TS` | `TitleScreen` | Kenshi's main-menu screen |
| `W` | `MyGUI::Widget` | base widget |
| `WP` | `MyGUI::WidgetPtr` | widget pointer (event handler argument) |
| `WN` | `MyGUI::Window` | the config window |
| `B` | `MyGUI::Button` | every clickable row/button, including the language cycle button |
| `SV` | `MyGUI::ScrollView` | the scrolling list container |
| `TXB` | `MyGUI::TextBox` | plain non-interactive text (tier dividers, `langLabel`, `langWarn`) - a different C++ type from `B`, so it needs its own pool |
| `CL` | `MyGUI::Colour` | text colour |
| `AL` | `MyGUI::Align` | widget alignment |
| `IC` | `MyGUI::IntCoord` | pixel rect |
| `TT` | `MyGUI::ToolTipInfo` | hover event payload |

### Macros

| Alias | Meaning |
|---|---|
| `DG(f)` | `MyGUI::newDelegate(f)` - binds a handler to a widget event |
| `SK` | `"Kenshi_Button1"` - the only button skin used |
| `TAG` | `"[PlayableConfigurable] "` - prefixed once, inside `Emit`, instead of at every call site |
| `COUNTOF(a)` | array element count (`sizeof(a)/sizeof(*(a))`, cast to `int`) |
| `HOOK(fn, detour, orig, msg)` | install a KenshiLib hook, `Err(msg)` if it fails |
| `TIP_GUARD(info)` | shared early-return prefix for tooltip handlers (`OnTip`/`OnLangTip`/`OnFontWarnTip`) - hide-on-`Hide`, bail on anything but `Show` |

### Globals

| Alias | What it holds |
|---|---|
| `g` | the loaded config (`Cfg`) |
| `g_loaded` / `g_busy` | config-loaded flag / re-entrancy guard |
| `R` | all races (`std::vector<Race>`), name-sorted |
| `K` | display categories (`std::vector<Cat>`), tier-then-name sorted |
| `D` | flattened visible rows (`std::vector<Row>`) |
| `OpenMemo` | which categories are expanded, keyed by group sid |
| `Orig` | race sid -> its pristine visibility (`playable && limits && group`), captured the first time `Apply` sees it |
| `wnd`, `launch`, `sv`, `tip` | window, RACES button, scroll view, tooltip |
| `optA`, `optF`, `optL` | the Animals / Forced-starts / Language toggle buttons |
| `langLabel` | static text next to `optL` - always the translated word "Language" |
| `langWarn` | full-width row below the language row - hidden unless the *selected* language's font isn't loaded, in which case it shows the warning in `g_autoLang` (Kenshi's own client language, not necessarily the selected one) |
| `g_lang` | index into `STR[][]`/`LANGS[]` for the UI's currently active (user-selected) language |
| `g_autoLang` | index into `STR[][]`/`LANGS[]` for Kenshi's own detected client language - set once at startup, never changed by `@lang`/`OnLang`; exists solely so `langWarn` can speak a language that's guaranteed to actually render |
| `poolE`, `poolR` | pooled expander and row buttons |
| `poolD` | pooled divider text widgets (`Kenshi_TextboxStandardText` skin - a real Kenshi label skin, not a button) |

### Structs and functions

| Alias | Role |
|---|---|
| `Cfg` | config file contents |
| `Rows` | RAII wrapper around a `getDataOfType` query (frees the lektor buffer) |
| `Race`, `Cat`, `Row` | one race / one display category (carries a `tier`: 0 Race Groups, 1 Modded Groups, 2 Uncategorized) / one visible list row (`kind`: 0 divider, 1 header, 2 race) |
| `Lang` | one language's `{code, full, testcp}` - `LANGS[L_COUNT]` is the single source of truth `LangByCode`/`DetectLang`/`OnLang`/`SyncLangBtn`/`Draw` all read from |
| `Get`, `GetB`, `GetF`, `SetB`, `SetF` | generic map read; bool/file field read and write on a record |
| `Trim`, `Bool` | string trim, string->bool |
| `Emit`, `Log`, `Err` | shared `TAG`-prefixed formatter; `Log`/`Err` are the variadic wrappers around `DebugLog`/`ErrorLog` |
| `CfgPath`, `LoadCfg`, `SaveCfg` | config file location, read, write |
| `Apply` | the whole gate-enforcement pass over live data |
| `OrgFile`, `Van`, `Org` | origin filename, is-vanilla-origin test, provenance label |
| `InList` | linear membership test shared by `Van` and `VanillaPlayable` |
| `VanillaPlayable` | is this sid one of vanilla's own 8 selectable races (`VANILLA_PLAYABLE`) - different question from `Van` (origin) |
| `Scan`, `Flat`, `Ensure`, `Draw`, `Commit` | rebuild data, flatten to rows, grow widget pool, repaint, save+apply+repaint |
| `Idx` | widget name -> display row index |
| `Tail` | suffix test on a stringId |
| `Rebuild` | re-runs the engine's post-load processing so cache-backed views pick up live edits |
| `Mk`, `SetOpt` | button factory, option-button repaint |
| `RowSet` | templated coord+caption+colour triad, shared by `B` and `TXB` row widgets |
| `HideRow` | hides a display slot's expander/row/divider widgets together |

---

## Source notes

`PlayableConfigurable.cpp` is comment-free; this section is the reference
documentation, keyed to the identifier it describes.

### `Cfg::states`
race stringId -> desired state (true = playable).

### `Rows`
Game-filled lektor buffers are malloc-owned and NOT freed by lektor's
destructor - the caller must release `stuff` (RaceChange_Extension
precedent: *"release matches.stuff before returning ... otherwise a
repeated dialogue action can leak memory"*). `Rows` is the RAII wrapper
that guarantees this for every query.

### `CfgPath`
Config lives in the game-relative mod folder (synced there by
`Play-Kenshi.ps1` / deployed next to the DLL); falls back to
`%LOCALAPPDATA%\kenshi\`.

### `LoadCfg`
Race lines are `state | stringId | name | note`.

### `Apply`
- Opens with a recursion guard and an `ou` null-check (it also runs at
  preload, where game data is still empty - a harmless no-op).
- Builds `grouped` (the set of race sids currently in >=1 RACE_GROUP)
  with one pass over `RACE_GROUP` records.
- Per race: `vis = playable && limits && group` is the current
  visibility; `Orig` remembers it the first time it's seen. Skips
  instantly if the desired state already matches `vis`.
- Enable: set `playable = true`, borrow an editor-limits xml if missing,
  and create/reuse `PCG::<sid>` ONLY if the race has no group at all yet.
  A race with no limits is skipped unless `allowAnimals`.
- Disable: set `playable = false`. That's it - group membership is left
  alone (see "Visibility model" above for why).
- The schema default for an absent `playable` is TRUE (per `fcs.def`).
- Runtime group ids are deterministic and never serialized to a .mod
  file.
- Finally, optionally lifts per-start creator locks.

### `PostProcess_hook`
Runs `Apply()` BEFORE the original post-processing, so our edits are in
place when the engine builds `RaceData` / `RaceGroupData` - the caches the
character editor enumerates. This is why the plugin must load as a
**preload** plugin. Re-applying on later reloads (save import, mod-list
change) is idempotent.

### UI section
In-game config window (title-screen MyGUI, KillButton pattern). Only
skins proven on this install are used: `Kenshi_WindowCX`,
`Kenshi_Button1`, `Kenshi_ScrollView` - all three are ResourceLayout
*templates* in `data\gui\templates\kenshi_templates.xml`, which MyGUI
accepts as skin names.

Rows live in a `Kenshi_ScrollView` canvas (pixel coords), so the list
scrolls instead of paging - a 169-member group is browsable in one place.
Every toggle saves the config file AND re-applies to live GameData
immediately.

### `C_ON` / `C_OFF` / `C_MIX` / `C_WARN`
Native-Kenshi palette: warm white for active, dim grey for inactive, a
middle tone for partially-enabled groups, and a warm red reserved for
`langWarn`'s font-not-loaded message.

### `Race::origin`
Provenance label derived from the stringID suffix.

### `Cat`
Display category = a real RACE_GROUP record (Human, Hive, Skeleton...);
our own synthetic per-race groups are filtered out of the view. `mem`
holds indices into the race list.

### `Row`
`cat` indexes the category list; `race` indexes the race list, and is -1
for headers.

### `OpenMemo`
group sid -> expanded, survives rebuilds.

### `VANILLA`
Base-game data files. A record whose stringID ends in one of these was
defined by vanilla Kenshi (incl. the Newland-era base mods), not by a
workshop/local mod.

### `Org`
`"17-gamedata.quack"` -> `"Vanilla (gamedata.quack)"`;
`"123-Some Mod.mod"` -> `"Mod: Some Mod.mod"`.

### `SaveCfg`
Everything currently listed gets an explicit line; entries for races
whose mods are absent right now are preserved so they keep their state
when the mod returns. (Generator-written notes are regenerated by the
generator; the plugin writes empty notes.)

### `Scan`
Category membership is read straight from each RACE_GROUP's live `races`
list - safe to do directly since `Apply` never removes a race from a
group, so a disabled race is always still listed under its real category.
Hidden from the view: our synthetic `PCG::` groups and generated-mod
groups (`*-PlayableConfigurable.mod`).

Races in no RACE_GROUP at all (no engine signal exists to relate them -
see `isRelatedRace` below) are bucketed by origin instead of dumped into
one flat list: all vanilla-origin ones share an `"Ungrouped: Vanilla"`
category, and every other mod gets its own `"Ungrouped: <file>"` category
(`sid`-prefixed `::vanilla` / `::mod:<file>` so they never collide with a
real group's sid).

Every `Cat` also gets a `tier` (0 = the group RECORD's own origin is
vanilla, 1 = a mod defined the group, 2 = the synthetic ungrouped buckets
- always tier 2, since being ungrouped is what put them there), and
`ByCat` sorts tier first. `Flat()` inserts a non-interactive divider `Row`
(`kind = 0`) whenever the tier changes, so the window reads as three
sections: Race Groups, Modded Groups, Uncategorized.

Dividers render as plain centered text (`Kenshi_TextboxStandardText`, a
real `MyGUI::TextBox` skin lifted from Kenshi's own layouts), not a
button: no border, no hover highlight, no click handler wired. `TextBox`
and `Button` are different C++ types in MyGUI, so dividers get their own
`poolD` rather than reusing `poolR`.

### `isRelatedRace` (not used)
`RaceData::raceGroup` and `RaceData::isRelatedRace(...)` exist in the SDK
headers but are empirically dead at every point this plugin can run:
`raceGroup` is null on every race (including known-grouped ones) even
after `postProcessingTheDatas`, and `isRelatedRace` returns false for
every distinct pair tested (e.g. Greenlander/Scorchlander despite sharing
the Human group). This data is evidently built lazily by the
character-editor UI itself when opened, which a plugin cannot trigger.

### `Idx`
Display index parsed from widget names `PCRow<i>` / `PCExp<i>` (5-char
prefixes); with the scroll view each widget maps 1:1 to a display row.

### `OnPreset`
One handler for all four preset buttons, keyed by the digit in its widget
name:

| key | button | rule |
|---|---|---|
| 0 | ENABLE ALL | every race on |
| 1 | DISABLE ALL | every race off |
| 2 | DEFAULT ALL | `Orig` - each race's pristine visibility as the currently loaded mods (and vanilla) themselves assert it, captured before this plugin edits anything. |
| 3 | VANILLA ONLY | `VanillaPlayable(sid)` - exactly the 8 races vanilla itself makes selectable (`VANILLA_PLAYABLE`, hardcoded), regardless of what any loaded mod did to those records' current group/limits. Deliberately NOT `Orig`-based: a mod can patch a vanilla race's group without touching its own file, which would silently drop it from an `Orig`-driven vanilla filter even though vanilla always considered it playable. |

`VANILLA_PLAYABLE` is a fixed 8-sid table (Greenlander, Scorchlander,
Shek, Skeleton, Fishman, Hive Prince, Hive Soldier Drone, Hive Worker
Drone) - Kenshi's own vanilla-selectable set, the same game data on every
install.

### `BuildUi`
The title screen can be constructed repeatedly (quit to menu); if MyGUI
still holds our widgets we reuse them, and only rebuild if it wiped them.

Layout order: 2x2 preset block, then the scrolling list (Kenshi's own
ScrollView template with its styled vertical scrollbar), then the two
option toggles, then the language row and warning row.

The provenance tooltip sits on the `ToolTip` layer, which has
`Pick=false`, so it never steals the mouse.

### `Ensure`
Row widgets are pooled and reused: created once on demand, then only
repositioned/recaptioned. The widget name encodes the display index.

### `Title_hook`
Title screen constructor hook: UI thread, MyGUI ready - safe to build
here.

### `TitleShow_hook`
Main-menu-only visibility: the RACES button mirrors the title screen's
own show/hide, so it disappears the moment a game starts or loads and
returns on quit-to-menu. The window and tooltip are force-closed on hide.

### `startPlugin`
Runs at preload, so it only installs hooks and loads config; the UI is
built later by the title-screen hook, and the real work happens in
`PostProcess_hook` during data load. Installs three hooks (each degrades
gracefully with an `Err(...)` if it fails), builds the UI immediately if
the title screen already exists, and applies once at startup.
