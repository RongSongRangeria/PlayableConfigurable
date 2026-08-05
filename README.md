# PlayableConfigurable

Makes races and subraces playable in Kenshi's character creator, adapting at
every launch to whatever mods are loaded. 
Think of it like a configurable patch to make every race and animal playable

## Why preload

RE_Kenshi's postload phase only loads plugins whose mod is enabled 
(`ou->activeMods`), but its preload phase loads every *available* mod's 
plugin regardless of enablement (`ou->availabelModsOrderedList`). 
Being preload is also what lets `Apply()` run before the engine builds 
the `RaceGroupData`/`RaceData` caches the character editor reads
postload plugins land too late
Since preload doesn't respect the enabled/disabled state,
`ModEnabled()` reads `data\mods.cfg` itself at startup and 
stays fully inert if the mod isn't listed.

**Visibility = `playable`, editor-limits & race-group**

optional gate sits on top: `NEW_GAME_STARTOFF` records can carry a
`force race` list restricting a specific start to certain races (vanilla:
Fishman is playable, has limits, and has its own group, but no start's
allow-list includes it). `@clearForceRace` deals with this.

- **Enable** = `playable = true`, an editor-limits xml (borrowed via
  `animalLimitsFile` when `allowAnimals` is set), and group membership -
  creating a plugin-owned `PCG::<sid>` group only if the race has no group at
  all yet.
- **Disable** = `playable = false`.
  `RaceData::raceGroup` and `RaceData::isRelatedRace(...)` are gameplay-facing fields/methods (not
  editor-only), and `Item::canEquip(RaceData*, bool)` takes a race directly,
  so a group a mod authored for reasons other than the creator could
  plausibly feed equipment-fit or relatedness checks for real NPCs.

Both actions no-op instantly if the race is already in the desired
visibility state - confirmed on a fresh install with no config:
`0 enabled, 0 disabled`.

## Scope

Only character creation and the character editor are affected. `RaceData` -
the runtime object gameplay consults - has no `playable` field, so NPCs,
animals, factions, combat and existing saves SHOULD be untouched.

## Hooks

- `GameDataManager::postProcessingTheDatas` - `Apply()` runs BEFORE the
  original, so edits are in place before the engine's caches are built. Also
  the re-apply point for later reloads (save import, mod-list change).
- `TitleScreen::_CONSTRUCTOR` - builds the RACES button and window the first
  time the title screen exists.
- `TitleScreen::_NV_show` - mirrors the RACES button's visibility onto the
  title screen's own show/hide, so it only appears on the main menu.

## Config

`PlayableConfigurable.config.txt`: lines `on|<stringId>|name|note`,
`@allowAnimals`, `@animalLimitsFile`, `@clearForceRace`, `@lang` (a
`locale/`-style code like `ru_RU`; empty/unrecognized = auto-detect from
Kenshi's own `settings.cfg` every boot). Located at
`mods\PlayableConfigurable\` when that exists (manual/dev install), otherwise
`%LOCALAPPDATA%\kenshi\` Steam-managed workshop folders,
get revalidated on update. Races absent from the current launch shouldn't appear, 
mod should only show loaded races from modlist with other configs remaining
inert `#~` lines and returns when their mod does.

**A race with no setting defaults to the game's own choice** 
(`Orig`, captured before the plugin modifies anything), and the apply pass skips any
race whose visibility already matches, a fresh install SHOULD change nothing:
 `0 enabled, 0 disabled`. `DEFAULT ALL` should restore that state

## Localization

The plugin's own UI (RACES button, window title, tier dividers, preset
buttons, tooltips, option toggles) supports all 9 languages Kenshi itself
ships: English, Russian, Spanish, Chinese, German, French, Japanese,
Korean, and Portuguese (Brazil) - confirmed against FCS's own Translation
Mode language list, which matches the `locale/` folders Kenshi ships.
Detected once at startup by reading `language=` out of Kenshi's own
`settings.cfg` (same file the game's own Options menu writes) - any other
value falls back to English. A real dropdown at the bottom of the window
(`optL`, a `MyGUI::ComboBox` built from Kenshi's own `Kenshi_ComboBox`
template - the same "reuse Kenshi's own skins" approach as `Kenshi_Button1`/
`Kenshi_ScrollView` elsewhere in this file, `setComboModeDrop(true)` so it's
list-only, not free-typed) lets the player override the auto-detected
language, next to a static label (`langLabel`) showing the word
"Language" translated. Items are `LANG_FULL[L_COUNT]` - a fixed, *not*
per-language-translated "English name | native name" pair (`"Russian | 
Русский"`, `"Japanese | 日本語"`, etc.) so every entry stays identifiable
regardless of whatever language is currently active - the whole point of a
language picker is being readable before you've picked the right one.
Picking an entry fires `eventComboAccept` (`OnLang(CB*, size_t)`) and
persists the choice to `@lang` in the config file, same as every other
setting.

**Popup-list corruption bug found + fixed:** the hint tooltip was
originally attached to `optL` itself (the combo). First real screenshot
after shipping the ComboBox showed the popup list rendering as visual
noise even though the closed combo displayed its selection cleanly -
strong sign the two were fighting over the same space rather than a font
problem (the closed state proved the font itself was fine). Root cause:
Kenshi's `Kenshi_ComboBox` template puts its popup `ListBox` on layer
`"Popup"`, while our tooltip widget is on layer `"ToolTip"` - if `ToolTip`
renders above `Popup` (plausible; tooltips are usually top-most), hovering
the combo to open its list would also fire our tooltip, drawing a large
caveat-text box on top of the list it was covering. Fixed by moving
`setNeedToolTip`/`eventToolTip` off `optL` and onto the static `langLabel`
instead - same information, but the label never opens a competing popup so
there's nothing for the tooltip to collide with.

**Font caveat (why the override isn't risk-free):** Kenshi ships a
Cyrillic/CJK-capable font override per locale (`locale/<code>/gui/fonts/
kenshi_fonts.xml`, swapping in fonts with the needed Unicode `Codes` ranges
for the standard `Kenshi_StandardFont_*` resources our widgets reuse), but
**only one** such override is ever loaded - whichever one matches Kenshi's
own `language=` setting at boot. `en_GB` is the only locale with *no*
override at all (bare ASCII + narrow punctuation). So text renders
correctly for exactly two cases: whatever language Kenshi itself is
running in (that font is already loaded), and English (a subset of every
font here). Overriding to anything else may show blank glyphs for that
language's special characters, depending on what Kenshi's own client
loaded - there's no way to fix this from a plugin without bundling and
hooking in our own fonts, which was deliberately out of scope. `optL` has
a tooltip (`T_LANG_HINT`) saying exactly this, in whatever language is
currently active.

All 9 languages live in one `STR[L_COUNT][T_COUNT]` table (`L_EN/L_RU/L_ES/
L_ZH/L_DE/L_FR/L_JA/L_KO/L_PT`), looked up via `T(id)` and formatted via
`Fmt(id, arg)` for the handful of `%s`/`%d` templates. Adding a language
means adding one row to the table and one prefix check in `DetectLang()` -
no other code changes. Terminology for shared concepts (e.g. "Race") was
cross-checked against Kenshi's own shipped translation catalogs
(`locale/<code>/LC_MESSAGES/main.po`) to stay consistent with the base
game's vocabulary rather than inventing our own.

**Race/group names are not translated by us, but likely already are by
Kenshi itself.** `fcs.def` flags both `RACE` and `RACE_GROUP` as
`TRANSLATE: ALL`, and Kenshi's shipped `locale/<code>/gamedata.po` catalogs
have matching entries keyed by record ID (e.g. `msgid "Fishman"` ->
`msgstr "Рыболюд"` in `ru_RU`). `e.name`/`c.name` are read straight from
`GameData::name` (`Scan()`) - never inline during our hook, only later on
window-open/commit, well after Kenshi's own data-loading pipeline has
already run - so for vanilla/base-game races this should already reflect
whatever Kenshi itself localized, for free. Not yet directly confirmed
in-game (blocked by the launcher dialog issue below before reaching a race
name in a non-English session) - inferred from the catalog evidence, not
observed. Third-party mod races only translate if that mod's own author
shipped a catalog for it - outside our control either way. Mod/file names
shown in tooltips (e.g. `small_changes_otto.mod`) are literal identifiers
and correctly stay untranslated; only the wrapping template text around
them ("Mod: ", "Vanilla (", "Ungrouped: ") is ours.

**Tooltip sizing bug found + fixed:** both tooltips (`OnTip`, `OnLangTip`)
originally sized their box off `text.size()` - the *byte* length of a
`std::string`. That's fine for ASCII but wildly wrong for UTF-8 text where
one visual character can be 2-3 bytes (a Chinese/Korean sentence would
massively over-size its tooltip box). Fixed with `Glyphs()`, which counts
UTF-8 lead bytes (`(b & 0xC0) != 0x80`) instead of raw bytes - not
pixel-perfect for wide CJK glyphs, but far closer than counting bytes.

**VS2010 `\x` escape gotcha:** non-ASCII UI text is written as `\xHH`
byte-escaped UTF-8 (not raw literal characters) because this toolchain
doesn't reliably treat a BOM-less source file as UTF-8. That escaping has
its own trap: `\x` consumes *any* number of following hex-digit characters
(`0-9a-fA-F`), so an escape immediately followed by a literal letter like
`b`/`c`/`d`/`f` silently merges into one oversized escape and fails to
compile (`C2022: too big for character`) - e.g. `"\xA9buts"` reads as
`\xA9B` + `uts`, not `é` + `buts`. Every escape run must be its own
adjacent string-literal segment (`"\xC3\xA9" "buts"`, which the compiler
concatenates with zero runtime cost) so it's always immediately followed by
a closing quote, never a literal letter.

## Build environment (one-time)

1. VC++ 2010 x64 platform toolset.
   Visual Studio 2010 Ultimate ISO from archive.org (hash-verified against
   the published MD5/SHA1 before mounting), installing only Visual C++ ->
   X64 Compilers and Tools. Used due to the asks of RE_Kenshi/KenshiLib
2. KenshiLib SDK: `BFrizzleFoShizzle/KenshiLib_Examples_deps` (Boost 1.60 +
   Ogre + MyGUI headers, prebuilt libs) plus the KenshiLib release zip
   (`KenshiReclaimer/KenshiLib`) for `KenshiLib.lib` and `Include/`. Staged
   under `sdk/` next to this project; `Directory.Build.props` resolves
   `KENSHILIB_DIR` / `BOOST_INCLUDE_PATH`

```
msbuild PlayableConfigurable.vcxproj /p:Configuration=Release /p:Platform=x64
```

## Shipping / Deploy

`dist/PlayableConfigurable/` holds exactly what users get: a **0-record stub
`.mod`** (the carrier so Kenshi lists the folder), `RE_Kenshi.json` declaring
`PreloadPlugins`, the DLL, `README.txt`, and `COPYING.txt` (GPLv3, required -
KenshiLib linkage). Source itself is no longer bundled inline - it's at
https://github.com/RongSongRangeria/PlayableConfigurable (linked from
`README.txt`), which is what the repo this file lives in actually is. To
deploy manually, copy into `...\Kenshi\mods\PlayableConfigurable\`:

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

Requires RE_Kenshi with KenshiLib. `PreloadPlugins` (what this plugin uses)
was introduced in v0.3.0. So theoretically it would work with V0.3.0, but it was built with V0.3.4

## Verified behavior

- Fresh install, no config: `0 enabled, 0 disabled` - a true no-op against
  whatever the loaded mods themselves mark playable.
- Toggling in-game applies live, no restart, confirmed by reading the actual
  character creator (not just the log) before and after a toggle.
- Disabling the mod in `mods.cfg` leaves the plugin fully inert (confirmed by
  log: it loads, checks, and returns before touching anything).
- `Unlock forced starts`: turning it on takes effect immediately; turning it
  back off needs a relaunch, since the original per-start restriction is only
  ever cleared, never restored from memory.

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

`PlayableConfigurable.cpp` uses short aliases throughout. Every one is defined at
the top of the file and listed here.

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
| `B` | `MyGUI::Button` | every clickable row/button |
| `CB` | `MyGUI::ComboBox` | the language dropdown - built from Kenshi's own `Kenshi_ComboBox` template |
| `SV` | `MyGUI::ScrollView` | the scrolling list container |
| `TXB` | `MyGUI::TextBox` | plain non-interactive text (tier dividers) - a different C++ type from `B`, so it needs its own pool |
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

### Globals

| Alias | What it holds |
|---|---|
| `g` | the loaded config (`Cfg`) |
| `g_loaded` / `g_busy` | config-loaded flag / re-entrancy guard |
| `R` | all races (`std::vector<Race>`), name-sorted |
| `K` | display categories (`std::vector<Cat>`), tier-then-name sorted |
| `TIER_NAMES` | the 3 divider labels, indexed by `Cat::tier` / a divider `Row::cat` |
| `D` | flattened visible rows (`std::vector<Row>`) |
| `OpenMemo` | which categories are expanded, keyed by group sid |
| `Orig` | race sid -> its pristine visibility (`playable && limits && group`), captured the first time `Apply` sees it |
| `wnd`, `launch`, `sv`, `tip` | window, RACES button, scroll view, tooltip |
| `optA`, `optF` | the Animals / Forced-starts toggle buttons |
| `poolE`, `poolR` | pooled expander and row buttons |
| `poolD` | pooled divider text widgets (`Kenshi_TextboxStandardText` skin - a real Kenshi label skin, not a button) |

### Structs and functions

| Alias | Role |
|---|---|
| `Cfg` | config file contents |
| `Rows` | RAII wrapper around a `getDataOfType` query (frees the lektor buffer) |
| `Race`, `Cat`, `Row` | one race / one display category (carries a `tier`: 0 Race Groups, 1 Modded Groups, 2 Uncategorized) / one visible list row (`kind`: 0 divider, 1 header, 2 race) |
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

`PlayableConfigurable.cpp` is comment-free in source order, keyed to
the identifier it described, is documentation

### `Cfg::states`
race stringId -> desired state (true = playable).

### `Rows`
Game-filled lektor buffers are malloc-owned and NOT freed by lektor's destructor -
the caller must release `stuff` (RaceChange_Extension precedent: *"release
matches.stuff before returning ... otherwise a repeated dialogue action can leak
memory"*). `Rows` is the RAII wrapper that guarantees this for every query.

### `CfgPath`
Config lives in the game-relative mod folder (synced there by `Play-Kenshi.ps1` /
deployed next to the DLL); falls back to the DLL's own directory.

### `LoadCfg`
Race lines are `state | stringId | name | note`.

### `Apply`
- Opens with a recursion guard and an `ou` null-check (it also runs at preload,
  where game data is still empty - a harmless no-op).
- Builds `grouped` (the set of race sids currently in >=1 RACE_GROUP) with one
  pass over `RACE_GROUP` records.
- Per race: `vis = playable && limits && group` is the current visibility;
  `Orig` remembers it the first time it's seen. Skips instantly if the desired
  state already matches `vis`.
- Enable: set `playable = true`, borrow an editor-limits xml if missing, and
  create/reuse `PCG::<sid>` ONLY if the race has no group at all yet. A race
  with no limits is skipped unless `allowAnimals`.
- Disable: set `playable = false`. That's it - group membership is left alone
  (see "Visibility model" above for why).
- The schema default for an absent `playable` is TRUE (per `fcs.def`).
- Runtime group ids are deterministic and never serialized to a .mod file.
- Finally, optionally lifts per-start creator locks.

### `PostProcess_hook`
Runs `Apply()` BEFORE the original post-processing, so our edits are in place
when the engine builds `RaceData` / `RaceGroupData` - the caches the character
editor enumerates. This is why the plugin must load as a **preload** plugin.
Re-applying on later reloads (save import, mod-list change) is idempotent.

### UI section
In-game config window (title-screen MyGUI, KillButton pattern). Only skins proven
on this install are used: `Kenshi_WindowCX`, `Kenshi_Button1`, `Kenshi_ScrollView` -
all three are ResourceLayout *templates* in
`data\gui\templates\kenshi_templates.xml`, which MyGUI accepts as skin names.

Rows live in a `Kenshi_ScrollView` canvas (pixel coords), so the list scrolls
instead of paging - a 169-member group is browsable in one place. Every toggle
saves the config file AND re-applies to live GameData immediately.

### `C_ON` / `C_OFF` / `C_MIX`
Native-Kenshi palette: warm white for active, dim grey for inactive, and a middle
tone for partially-enabled groups.

### `Race::origin`
Provenance label derived from the stringID suffix.

### `Cat`
Display category = a real RACE_GROUP record (Human, Hive, Skeleton...); our own
synthetic per-race groups are filtered out of the view. `mem` holds indices into
the race list.

### `Row`
`cat` indexes the category list; `race` indexes the race list, and is -1 for
headers.

### `OpenMemo`
group sid -> expanded, survives rebuilds.

### `VANILLA`
Base-game data files. A record whose stringID ends in one of these was defined by
vanilla Kenshi (incl. the Newland-era base mods), not by a workshop/local mod.

### `Org`
`"17-gamedata.quack"` -> `"Vanilla (gamedata.quack)"`;
`"123-Some Mod.mod"` -> `"Mod: Some Mod.mod"`.

### `SaveCfg`
Everything currently listed gets an explicit line; entries for races whose mods
are absent right now are preserved so they keep their state when the mod returns.
(Generator-written notes are regenerated by the generator; the plugin writes
empty notes.)

### `Scan`
Category membership is read straight from each RACE_GROUP's live `races` list -
safe to do directly now that `Apply` never removes a race from a group, so a
disabled race is always still listed under its real category. Hidden from the
view: our synthetic `PCG::` groups and generated-mod groups
(`*-PlayableConfigurable.mod`).

Races in no RACE_GROUP at all (no engine signal exists to relate them - see
`isRelatedRace` below) are bucketed by origin instead of dumped into one flat
list: all vanilla-origin ones share an `"Ungrouped: Vanilla"` category, and
every other mod gets its own `"Ungrouped: <file>"` category (`sid`-prefixed
`::vanilla` / `::mod:<file>` so they never collide with a real group's sid).
One mod contributing few ungrouped races produces a small category - expected,
not a bug.

Every `Cat` also gets a `tier` (0 = the group RECORD's own origin is vanilla,
1 = a mod defined the group, 2 = the synthetic ungrouped buckets - always tier
2, since being ungrouped is what put them there), and `ByCat` sorts tier first.
`Flat()` inserts a non-interactive divider `Row` (`kind = 0`) whenever the tier
changes, labelled from `TIER_NAMES`, so the window reads as three sections:
Race Groups, Modded Groups, Uncategorized.

Dividers render as plain centered text (`Kenshi_TextboxStandardText`, a real
`MyGUI::TextBox` skin lifted from Kenshi's own layouts - used 22 places
game-side, e.g. its Main Menu labels), not a button: no border, no hover
highlight, no click handler wired. `TextBox` and `Button` are different C++
types in MyGUI, so dividers get their own `poolD` rather than reusing `poolR`.

### `isRelatedRace` (considered, not used)
`RaceData::raceGroup` and `RaceData::isRelatedRace(...)` exist in the SDK
headers but were empirically dead at every point this plugin can run: a
diagnostic probe (built, deployed, logged, then reverted) found `raceGroup`
null on every race - including known-grouped ones - even well after the
engine's own postProcessingTheDatas, and `isRelatedRace` false for every
distinct pair tested (Greenlander/Scorchlander despite sharing the Human
group). That data is evidently built lazily by the character-editor UI itself
when opened, which a plugin cannot trigger. Do not reach for it again without
new evidence.

### `Idx`
Display index parsed from widget names `PCRow<i>` / `PCExp<i>` (5-char prefixes);
with the scroll view each widget maps 1:1 to a display row.

### `OnPreset`
One handler for all four preset buttons, keyed by the digit in its widget name:

| key | button | rule |
|---|---|---|
| 0 | ENABLE ALL | every race on |
| 1 | DISABLE ALL | every race off |
| 2 | DEFAULT ALL | `Orig` - each race's pristine visibility as the CURRENTLY loaded mods (and vanilla) themselves assert it, captured before this plugin edits anything. This is "the mods' playables": whatever a mod marks playable+grouped stays on, same as an unlisted race's default. |
| 3 | VANILLA ONLY | `VanillaPlayable(sid)` - exactly the 8 races vanilla itself makes selectable (`VANILLA_PLAYABLE`, hardcoded; see below), regardless of what any loaded mod did to those records' current group/limits. Deliberately NOT `Orig`-based: a mod can patch a vanilla race's group without touching its own file, which would silently drop it from an `Orig`-driven vanilla filter even though vanilla always considered it playable. |

`VANILLA_PLAYABLE` is a fixed 8-sid table (Greenlander, Scorchlander, Shek,
Skeleton, Fishman, Hive Prince, Hive Soldier Drone, Hive Worker Drone) -
Kenshi's own vanilla-selectable set, the same game data on every install, so
hardcoding it is safe. Verified twice: parsing the 4 base files alone
(`Get-VanillaBaseline.ps1`) and by the user reading the creator on a genuine
0-mod install in-game (only Fishman wasn't independently confirmed there,
because the vanilla start used carries its own `force race` list that excludes
it - a start-level restriction, not a race-level one; `@clearForceRace` is the
separate lever for that).

### `BuildUi`
The title screen can be constructed repeatedly (quit to menu); if MyGUI still
holds our widgets we reuse them, and only rebuild if it wiped them.

Layout order: 2x2 preset block, then the scrolling list (Kenshi's own ScrollView
template with its styled vertical scrollbar), then the two option toggles.

The provenance tooltip sits on the `ToolTip` layer, which has `Pick=false`, so it
never steals the mouse.

### `Ensure`
Row widgets are pooled and reused: created once on demand, then only
repositioned/recaptioned. The widget name encodes the display index.

### `Title_hook`
Title screen constructor hook: UI thread, MyGUI ready - safe to build here.

### `TitleShow_hook`
Main-menu-only visibility: the RACES button mirrors the title screen's own
show/hide, so it disappears the moment a game starts or loads and returns on
quit-to-menu. The window and tooltip are force-closed on hide.

### `startPlugin`
Runs at preload, so it only installs hooks and loads config; the UI is built
later by the title-screen hook, and the real work happens in `PostProcess_hook`
during data load. Installs three hooks (each degrades gracefully with an
`Err(...)` if it fails), builds the UI immediately when the title screen
already exists - postload runs after its construction - and applies once at
startup, since postload plugins start with `ou->gamedata` fully merged.
