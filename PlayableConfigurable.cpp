#include <Debug.h>
#include <core/Functions.h>
#include <kenshi/Enums.h>
#include <kenshi/GameData.h>
#include <kenshi/GameDataManager.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/gui/TitleScreen.h>

#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_Window.h>
#include <mygui/MyGUI_Button.h>
#include <mygui/MyGUI_Delegate.h>
#include <mygui/MyGUI_FontManager.h>
#include <mygui/MyGUI_ResourceTrueTypeFont.h>
#include <mygui/MyGUI_ScrollView.h>
#include <mygui/MyGUI_TextBox.h>
#include <mygui/MyGUI_WidgetToolTip.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace
{

typedef std::string S;
typedef std::set<S> SS;
typedef std::map<S, bool> SBM;
typedef std::map<S, int> SIM;
typedef GameData GD;
typedef GameDataManager GDM;
typedef const Ogre::vector<GameDataReference>::type RV;
typedef TitleScreen TS;
typedef MyGUI::Widget W;
typedef MyGUI::WidgetPtr WP;
typedef MyGUI::Window WN;
typedef MyGUI::Button B;
typedef MyGUI::ScrollView SV;
typedef MyGUI::TextBox TXB;
typedef MyGUI::Colour CL;
typedef MyGUI::Align AL;
typedef MyGUI::IntCoord IC;
typedef MyGUI::ToolTipInfo TT;

#define DG MyGUI::newDelegate
#define SK "Kenshi_Button1"
#define TAG "[PlayableConfigurable] "
#define COUNTOF(a) (int)(sizeof(a) / sizeof(*(a)))
#define HOOK(fn, detour, orig, msg) \
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(fn), detour, &orig)) Err(msg)
#define TIP_GUARD(info) \
    if (!tip) return; \
    if ((info).type == TT::Hide) { tip->setVisible(false); return; } \
    if ((info).type != TT::Show) return;

enum { ROW_H = 34, EXP_W = 38, INDENT = 30 };
const CL C_ON(0.95f, 0.93f, 0.86f), C_OFF(0.45f, 0.44f, 0.40f), C_MIX(0.72f, 0.70f, 0.64f), C_WARN(0.85f, 0.35f, 0.25f);
const char* const VANILLA[] = {
    "gamedata.quack", "gamedata.base", "dialogue.mod", "newwworld.mod", "rebirth.mod",
    "stick_people.mod", "chareditor.mod", "small_changes_otto.mod", "newwworld plus.mod"
};
const char* const VANILLA_PLAYABLE[] = {
    "17-gamedata.quack", "18019-gamedata.base", "5276-chareditor.mod", "17946-stick_people.mod",
    "55396-gamedata.base", "5346-stick_people.mod", "18961-gamedata.base", "18960-gamedata.base"
};

enum { L_EN, L_RU, L_ES, L_ZH, L_DE, L_FR, L_JA, L_KO, L_PT, L_UK, L_PL, L_ZHTW, L_COUNT };
enum {
    T_RACE_GROUPS, T_MODDED_GROUPS, T_UNCATEGORIZED,
    T_UNKNOWN_ORIGIN, T_VANILLA_PAREN, T_MOD_COLON,
    T_UNGROUPED_VANILLA, T_UNGROUPED_MOD, T_UNGROUPED_VANILLA_DESC, T_UNGROUPED_MOD_DESC,
    T_WIN_TITLE, T_CAT_COUNT, T_ON_TAG, T_OFF_TAG, T_ANIMAL_SUFFIX,
    T_RACES_BTN, T_ENABLE_ALL, T_DISABLE_ALL, T_DEFAULT_ALL, T_VANILLA_ONLY,
    T_ANIMALS_ON, T_ANIMALS_OFF, T_FORCE_ON, T_FORCE_OFF, T_LANGUAGE, T_LANG_HINT,
    T_COUNT
};
const char* const STR[L_COUNT][T_COUNT] = {
/* en_GB */ {
    "Race Groups", "Modded Groups", "Uncategorized",
    "unknown origin", "Vanilla (%s)", "Mod: %s",
    "Ungrouped: Vanilla", "Ungrouped: %s", "vanilla races with no race group", "mod \"%s\" races with no race group",
    "PlayableConfigurable  (%d/%d on)", "   (%d/%d on)", "[ ON ]  ", "[ off ]  ", "  (animal)",
    "RACES", "ENABLE ALL", "DISABLE ALL", "DEFAULT ALL", "VANILLA ONLY",
    "Animals allowed: ON", "Animals allowed: OFF", "Unlock forced starts: ON (restart to undo)", "Unlock forced starts: OFF",
    "Language", "Only your Kenshi language + English render reliably; others may show blanks"
},
/* ru_RU */ {
    "\xD0\x93\xD1\x80\xD1\x83\xD0\xBF\xD0\xBF\xD1\x8B \xD1\x80\xD0\xB0\xD1\x81", "\xD0\x93\xD1\x80\xD1\x83\xD0\xBF\xD0\xBF\xD1\x8B \xD0\xBC\xD0\xBE\xD0\xB4\xD0\xBE\xD0\xB2", "\xD0\x91\xD0\xB5\xD0\xB7 \xD0\xBA\xD0\xB0\xD1\x82\xD0\xB5\xD0\xB3\xD0\xBE\xD1\x80\xD0\xB8\xD0\xB8",
    "\xD0\xB8\xD1\x81\xD1\x82\xD0\xBE\xD1\x87\xD0\xBD\xD0\xB8\xD0\xBA \xD0\xBD\xD0\xB5\xD0\xB8\xD0\xB7\xD0\xB2\xD0\xB5\xD1\x81\xD1\x82\xD0\xB5\xD0\xBD", "\xD0\x92\xD0\xB0\xD0\xBD\xD0\xB8\xD0\xBB\xD1\x8C\xD0\xBD\xD0\xB0\xD1\x8F" " (%s)", "\xD0\x9C\xD0\xBE\xD0\xB4: %s",
    "\xD0\x91\xD0\xB5\xD0\xB7 \xD0\xB3\xD1\x80\xD1\x83\xD0\xBF\xD0\xBF\xD1\x8B: \xD0\xB2\xD0\xB0\xD0\xBD\xD0\xB8\xD0\xBB\xD1\x8C", "\xD0\x91\xD0\xB5\xD0\xB7 \xD0\xB3\xD1\x80\xD1\x83\xD0\xBF\xD0\xBF\xD1\x8B: %s", "\xD0\xB2\xD0\xB0\xD0\xBD\xD0\xB8\xD0\xBB\xD1\x8C\xD0\xBD\xD1\x8B\xD0\xB5 \xD1\x80\xD0\xB0\xD1\x81\xD1\x8B \xD0\xB1\xD0\xB5\xD0\xB7 \xD0\xB3\xD1\x80\xD1\x83\xD0\xBF\xD0\xBF\xD1\x8B", "\xD1\x80\xD0\xB0\xD1\x81\xD1\x8B \xD0\xBC\xD0\xBE\xD0\xB4\xD0\xB0 \"%s\" \xD0\xB1\xD0\xB5\xD0\xB7 \xD0\xB3\xD1\x80\xD1\x83\xD0\xBF\xD0\xBF\xD1\x8B",
    "PlayableConfigurable  (%d/%d \xD0\xB2\xD0\xBA\xD0\xBB)", "   (%d/%d \xD0\xB2\xD0\xBA\xD0\xBB)", "[ \xD0\x92\xD0\x9A\xD0\x9B ]  ", "[ \xD0\xB2\xD1\x8B\xD0\xBA\xD0\xBB ]  ", "  (\xD0\xB6\xD0\xB8\xD0\xB2\xD0\xBE\xD1\x82\xD0\xBD\xD0\xBE\xD0\xB5)",
    "\xD0\xA0\xD0\x90\xD0\xA1\xD0\xAB", "\xD0\x92\xD0\x9A\xD0\x9B\xD0\xAE\xD0\xA7\xD0\x98\xD0\xA2\xD0\xAC \xD0\x92\xD0\xA1\xD0\x95", "\xD0\x92\xD0\xAB\xD0\x9A\xD0\x9B\xD0\xAE\xD0\xA7\xD0\x98\xD0\xA2\xD0\xAC \xD0\x92\xD0\xA1\xD0\x95", "\xD0\x9F\xD0\x9E \xD0\xA3\xD0\x9C\xD0\x9E\xD0\x9B\xD0\xA7\xD0\x90\xD0\x9D\xD0\x98\xD0\xAE", "\xD0\xA2\xD0\x9E\xD0\x9B\xD0\xAC\xD0\x9A\xD0\x9E \xD0\x92\xD0\x90\xD0\x9D\xD0\x98\xD0\x9B\xD0\xAC",
    "\xD0\x96\xD0\xB8\xD0\xB2\xD0\xBE\xD1\x82\xD0\xBD\xD1\x8B\xD0\xB5 \xD1\x80\xD0\xB0\xD0\xB7\xD1\x80\xD0\xB5\xD1\x88\xD0\xB5\xD0\xBD\xD1\x8B: \xD0\x92\xD0\x9A\xD0\x9B", "\xD0\x96\xD0\xB8\xD0\xB2\xD0\xBE\xD1\x82\xD0\xBD\xD1\x8B\xD0\xB5 \xD1\x80\xD0\xB0\xD0\xB7\xD1\x80\xD0\xB5\xD1\x88\xD0\xB5\xD0\xBD\xD1\x8B: \xD0\x92\xD0\xAB\xD0\x9A\xD0\x9B",
    "\xD0\xA1\xD0\xBD\xD1\x8F\xD1\x82\xD1\x8C" " " "\xD0\xB7\xD0\xB0\xD0\xBF\xD1\x80\xD0\xB5\xD1\x82" " " "\xD1\x81\xD1\x82\xD0\xB0\xD1\x80\xD1\x82\xD0\xBE\xD0\xB2" ": " "\xD0\x92\xD0\x9A\xD0\x9B" " (" "\xD0\xB4\xD0\xBB\xD1\x8F" " " "\xD0\xBE\xD1\x82\xD0\xBC\xD0\xB5\xD0\xBD\xD1\x8B" " " "\xD0\xBD\xD1\x83\xD0\xB6\xD0\xB5\xD0\xBD" " " "\xD0\xBF\xD0\xB5\xD1\x80\xD0\xB5\xD0\xB7\xD0\xB0\xD0\xBF\xD1\x83\xD1\x81\xD0\xBA" ")",
    "\xD0\xA1\xD0\xBD\xD1\x8F\xD1\x82\xD1\x8C \xD0\xB7\xD0\xB0\xD0\xBF\xD1\x80\xD0\xB5\xD1\x82 \xD1\x81\xD1\x82\xD0\xB0\xD1\x80\xD1\x82\xD0\xBE\xD0\xB2: \xD0\x92\xD0\xAB\xD0\x9A\xD0\x9B",
    "\xD0\xAF\xD0\xB7\xD1\x8B\xD0\xBA",
    "\xD0\x9D\xD0\xB0\xD0\xB4\xD1\x91\xD0\xB6\xD0\xBD\xD0\xBE" " " "\xD0\xBE\xD1\x82\xD0\xBE\xD0\xB1\xD1\x80\xD0\xB0\xD0\xB6\xD0\xB0\xD1\x8E\xD1\x82\xD1\x81\xD1\x8F" " " "\xD1\x82\xD0\xBE\xD0\xBB\xD1\x8C\xD0\xBA\xD0\xBE" " " "\xD1\x8F\xD0\xB7\xD1\x8B\xD0\xBA" " " "\xD0\xB2\xD0\xB0\xD1\x88\xD0\xB5\xD0\xB9" " " "\xD0\xB8\xD0\xB3\xD1\x80\xD1\x8B" " Kenshi " "\xD0\xB8" " " "\xD0\xB0\xD0\xBD\xD0\xB3\xD0\xBB\xD0\xB8\xD0\xB9\xD1\x81\xD0\xBA\xD0\xB8\xD0\xB9" "; " "\xD0\xBE\xD1\x81\xD1\x82\xD0\xB0\xD0\xBB\xD1\x8C\xD0\xBD\xD1\x8B\xD0\xB5" " " "\xD0\xBC\xD0\xBE\xD0\xB3\xD1\x83\xD1\x82" " " "\xD0\xBF\xD0\xBE\xD0\xBA\xD0\xB0\xD0\xB7\xD1\x8B\xD0\xB2\xD0\xB0\xD1\x82\xD1\x8C" " " "\xD0\xBF\xD1\x80\xD0\xBE\xD0\xBF\xD1\x83\xD1\x81\xD0\xBA\xD0\xB8"
},
/* es_ES */ {
    "Grupos de raza", "Grupos de mods", "Sin categor" "\xC3\xAD" "a",
    "origen desconocido", "Vainilla (%s)", "Mod: %s",
    "Sin grupo: Vainilla", "Sin grupo: %s", "razas vainilla sin grupo de raza", "razas del mod \"%s\" sin grupo de raza",
    "PlayableConfigurable  (%d/%d activas)", "   (%d/%d activas)", "[ ON ]  ", "[ off ]  ", "  (animal)",
    "RAZAS", "ACTIVAR TODO", "DESACTIVAR TODO", "POR DEFECTO", "SOLO VAINILLA",
    "Animales permitidos: ON", "Animales permitidos: OFF", "Desbloquear inicios: ON (reiniciar para deshacer)", "Desbloquear inicios: OFF",
    "Idioma", "Solo el idioma de tu Kenshi y el ingl" "\xC3\xA9" "s se muestran de forma fiable; otros pueden mostrar espacios en blanco"
},
/* zh_CN */ {
    "\xE7\xA7\x8D\xE6\x97\x8F\xE5\x88\x86\xE7\xBB\x84", "\xE6\xA8\xA1\xE7\xBB\x84\xE5\x88\x86\xE7\xBB\x84", "\xE6\x9C\xAA\xE5\x88\x86\xE7\xB1\xBB",
    "\xE6\x9D\xA5\xE6\xBA\x90\xE6\x9C\xAA\xE7\x9F\xA5", "\xE5\x8E\x9F\xE7\x89\x88 (%s)", "\xE6\xA8\xA1\xE7\xBB\x84\xEF\xBC\x9A%s",
    "\xE6\x9C\xAA\xE5\x88\x86\xE7\xBB\x84\xEF\xBC\x9A\xE5\x8E\x9F\xE7\x89\x88", "\xE6\x9C\xAA\xE5\x88\x86\xE7\xBB\x84\xEF\xBC\x9A%s", "\xE6\x97\xA0\xE5\x88\x86\xE7\xBB\x84\xE7\x9A\x84\xE5\x8E\x9F\xE7\x89\x88\xE7\xA7\x8D\xE6\x97\x8F", "\xE6\x9D\xA5\xE8\x87\xAA\xE6\xA8\xA1\xE7\xBB\x84\xE2\x80\x9C%s\xE2\x80\x9D\xE7\x9A\x84\xE6\x97\xA0\xE5\x88\x86\xE7\xBB\x84\xE7\xA7\x8D\xE6\x97\x8F",
    "PlayableConfigurable  (%d/%d \xE5\xB7\xB2\xE5\x90\xAF\xE7\x94\xA8)", "   (%d/%d \xE5\xB7\xB2\xE5\x90\xAF\xE7\x94\xA8)", "[ \xE5\xBC\x80 ]  ", "[ \xE5\x85\xB3 ]  ", "  (\xE5\x8A\xA8\xE7\x89\xA9)",
    "\xE7\xA7\x8D\xE6\x97\x8F", "\xE5\x85\xA8\xE9\x83\xA8\xE5\x90\xAF\xE7\x94\xA8", "\xE5\x85\xA8\xE9\x83\xA8\xE7\xA6\x81\xE7\x94\xA8", "\xE6\x81\xA2\xE5\xA4\x8D\xE9\xBB\x98\xE8\xAE\xA4", "\xE4\xBB\x85\xE5\x8E\x9F\xE7\x89\x88",
    "\xE5\x85\x81\xE8\xAE\xB8\xE5\x8A\xA8\xE7\x89\xA9\xEF\xBC\x9A\xE5\xBC\x80", "\xE5\x85\x81\xE8\xAE\xB8\xE5\x8A\xA8\xE7\x89\xA9\xEF\xBC\x9A\xE5\x85\xB3",
    "\xE8\xA7\xA3\xE9\x99\xA4\xE5\x87\xBA\xE7\x94\x9F\xE9\x99\x90\xE5\x88\xB6\xEF\xBC\x9A\xE5\xBC\x80" "(" "\xE6\x92\xA4\xE9\x94\x80\xE9\x9C\x80\xE8\xA6\x81\xE9\x87\x8D\xE5\x90\xAF" ")",
    "\xE8\xA7\xA3\xE9\x99\xA4\xE5\x87\xBA\xE7\x94\x9F\xE9\x99\x90\xE5\x88\xB6\xEF\xBC\x9A\xE5\x85\xB3",
    "\xE8\xAF\xAD\xE8\xA8\x80",
    "\xE5\x8F\xAA\xE6\x9C\x89\xE4\xBD\xA0" " Kenshi " "\xE7\x9A\x84\xE8\xAF\xAD\xE8\xA8\x80\xE5\x92\x8C\xE8\x8B\xB1\xE8\xAF\xAD\xE8\x83\xBD\xE5\x8F\xAF\xE9\x9D\xA0\xE6\x98\xBE\xE7\xA4\xBA\xEF\xBC\x9B\xE5\x85\xB6\xE4\xBB\x96\xE8\xAF\xAD\xE8\xA8\x80\xE5\x8F\xAF\xE8\x83\xBD\xE5\x87\xBA\xE7\x8E\xB0\xE7\xA9\xBA\xE7\x99\xBD\xE5\xAD\x97\xE7\xAC\xA6"
},
/* de_DE */ {
    "Rassengruppen", "Mod-Gruppen", "Unkategorisiert",
    "unbekannte Herkunft", "Vanilla (%s)", "Mod: %s",
    "Ohne Gruppe: Vanilla", "Ohne Gruppe: %s", "Vanilla-Rassen ohne Rassengruppe", "Rassen des Mods \"%s\" ohne Rassengruppe",
    "PlayableConfigurable  (%d/%d an)", "   (%d/%d an)", "[ AN ]  ", "[ aus ]  ", "  (Tier)",
    "RASSEN", "ALLE AKTIVIEREN", "ALLE DEAKTIVIEREN", "STANDARD", "NUR VANILLA",
    "Tiere erlaubt: AN", "Tiere erlaubt: AUS",
    "Start-Beschr" "\xC3\xA4" "nkung aufheben: AN (Neustart zum R" "\xC3\xBC" "ckg" "\xC3\xA4" "ngigmachen)",
    "Start-Beschr" "\xC3\xA4" "nkung aufheben: AUS",
    "Sprache", "Nur die Sprache deines Kenshi-Clients und Englisch werden zuverl" "\xC3\xA4" "ssig angezeigt; andere k" "\xC3\xB6" "nnen leere Zeichen zeigen"
},
/* fr_FR */ {
    "Groupes de race", "Groupes de mods", "Non class" "\xC3\xA9",
    "origine inconnue", "Vanilla (%s)", "Mod : %s",
    "Sans groupe : Vanilla", "Sans groupe : %s", "races vanilla sans groupe de race", "races du mod \"%s\" sans groupe de race",
    "PlayableConfigurable  (%d/%d actives)", "   (%d/%d actives)", "[ ON ]  ", "[ off ]  ", "  (animal)",
    "RACES", "TOUT ACTIVER", "TOUT D" "\xC3\x89" "SACTIVER", "PAR D" "\xC3\x89" "FAUT", "VANILLA UNIQUEMENT",
    "Animaux autoris" "\xC3\xA9" "s : ON", "Animaux autoris" "\xC3\xA9" "s : OFF",
    "D" "\xC3\xA9" "bloquer les d" "\xC3\xA9" "buts forc" "\xC3\xA9" "s : ON (red" "\xC3\xA9" "marrer pour annuler)",
    "D" "\xC3\xA9" "bloquer les d" "\xC3\xA9" "buts forc" "\xC3\xA9" "s : OFF",
    "Langue", "Seules la langue de votre Kenshi et l'anglais s'affichent de mani" "\xC3\xA8" "re fiable ; les autres peuvent montrer des caract" "\xC3\xA8" "res vides"
},
/* ja_JP */ {
    "\xE7\xA8\xAE\xE6\x97\x8F\xE3\x82\xB0\xE3\x83\xAB\xE3\x83\xBC\xE3\x83\x97", "MOD" "\xE8\xBF\xBD\xE5\x8A\xA0\xE3\x82\xB0\xE3\x83\xAB\xE3\x83\xBC\xE3\x83\x97", "\xE6\x9C\xAA\xE5\x88\x86\xE9\xA1\x9E",
    "\xE5\x87\xBA\xE6\x89\x80\xE4\xB8\x8D\xE6\x98\x8E", "\xE3\x83\x90\xE3\x83\x8B\xE3\x83\xA9" " (%s)", "MOD: %s",
    "\xE6\x9C\xAA\xE3\x82\xB0\xE3\x83\xAB\xE3\x83\xBC\xE3\x83\x97" ":" "\xE3\x83\x90\xE3\x83\x8B\xE3\x83\xA9", "\xE6\x9C\xAA\xE3\x82\xB0\xE3\x83\xAB\xE3\x83\xBC\xE3\x83\x97" ":%s", "\xE3\x82\xB0\xE3\x83\xAB\xE3\x83\xBC\xE3\x83\x97\xE3\x81\xAE\xE3\x81\xAA\xE3\x81\x84\xE3\x83\x90\xE3\x83\x8B\xE3\x83\xA9\xE7\xA8\xAE\xE6\x97\x8F", "MOD\"%s\"" "\xE3\x81\xAE\xE3\x82\xB0\xE3\x83\xAB\xE3\x83\xBC\xE3\x83\x97\xE3\x81\xAE\xE3\x81\xAA\xE3\x81\x84\xE7\xA8\xAE\xE6\x97\x8F",
    "PlayableConfigurable  (%d/%d " "\xE6\x9C\x89\xE5\x8A\xB9" ")", "   (%d/%d " "\xE6\x9C\x89\xE5\x8A\xB9" ")", "[ " "\xE6\x9C\x89\xE5\x8A\xB9" " ]  ", "[ " "\xE7\x84\xA1\xE5\x8A\xB9" " ]  ", "  (" "\xE5\x8B\x95\xE7\x89\xA9" ")",
    "\xE7\xA8\xAE\xE6\x97\x8F", "\xE3\x81\x99\xE3\x81\xB9\xE3\x81\xA6\xE6\x9C\x89\xE5\x8A\xB9\xE5\x8C\x96", "\xE3\x81\x99\xE3\x81\xB9\xE3\x81\xA6\xE7\x84\xA1\xE5\x8A\xB9\xE5\x8C\x96", "\xE3\x83\x87\xE3\x83\x95\xE3\x82\xA9\xE3\x83\xAB\xE3\x83\x88", "\xE3\x83\x90\xE3\x83\x8B\xE3\x83\xA9\xE3\x81\xAE\xE3\x81\xBF",
    "\xE5\x8B\x95\xE7\x89\xA9\xE3\x82\x92\xE8\xA8\xB1\xE5\x8F\xAF" ": " "\xE6\x9C\x89\xE5\x8A\xB9", "\xE5\x8B\x95\xE7\x89\xA9\xE3\x82\x92\xE8\xA8\xB1\xE5\x8F\xAF" ": " "\xE7\x84\xA1\xE5\x8A\xB9",
    "\xE9\x96\x8B\xE5\xA7\x8B\xE5\x88\xB6\xE9\x99\x90\xE3\x82\x92\xE8\xA7\xA3\xE9\x99\xA4" ": " "\xE6\x9C\x89\xE5\x8A\xB9" "(" "\xE5\x85\x83\xE3\x81\xAB\xE6\x88\xBB\xE3\x81\x99\xE3\x81\xAB\xE3\x81\xAF\xE5\x86\x8D\xE8\xB5\xB7\xE5\x8B\x95\xE3\x81\x8C\xE5\xBF\x85\xE8\xA6\x81" ")",
    "\xE9\x96\x8B\xE5\xA7\x8B\xE5\x88\xB6\xE9\x99\x90\xE3\x82\x92\xE8\xA7\xA3\xE9\x99\xA4" ": " "\xE7\x84\xA1\xE5\x8A\xB9",
    "\xE8\xA8\x80\xE8\xAA\x9E",
    "Kenshi" "\xE8\x87\xAA\xE4\xBD\x93\xE3\x81\xAE\xE8\xA8\x80\xE8\xAA\x9E\xE3\x81\xA8\xE8\x8B\xB1\xE8\xAA\x9E\xE3\x81\xA0\xE3\x81\x91\xE3\x81\x8C\xE7\xA2\xBA\xE5\xAE\x9F\xE3\x81\xAB\xE8\xA1\xA8\xE7\xA4\xBA\xE3\x81\x95\xE3\x82\x8C\xE3\x81\xBE\xE3\x81\x99\xE3\x80\x82\xE3\x81\x9D\xE3\x82\x8C\xE4\xBB\xA5\xE5\xA4\x96\xE3\x81\xAF\xE6\x96\x87\xE5\xAD\x97\xE3\x81\x8C\xE7\xA9\xBA\xE7\x99\xBD\xE3\x81\xAB\xE3\x81\xAA\xE3\x82\x8B\xE3\x81\x93\xE3\x81\xA8\xE3\x81\x8C\xE3\x81\x82\xE3\x82\x8A\xE3\x81\xBE\xE3\x81\x99"
},
/* ko_KR */ {
    "\xEC\xA2\x85\xEC\xA1\xB1" " " "\xEA\xB7\xB8\xEB\xA3\xB9", "\xEB\xAA\xA8\xEB\x93\x9C" " " "\xEA\xB7\xB8\xEB\xA3\xB9", "\xEB\xAF\xB8\xEB\xB6\x84\xEB\xA5\x98",
    "\xEC\xB6\x9C\xEC\xB2\x98" " " "\xEB\xB6\x88\xEB\xAA\x85", "\xEB\xB0\x94\xEB\x8B\x90\xEB\x9D\xBC" " (%s)", "\xEB\xAA\xA8\xEB\x93\x9C" ": %s",
    "\xEB\xAF\xB8\xEA\xB7\xB8\xEB\xA3\xB9" ": " "\xEB\xB0\x94\xEB\x8B\x90\xEB\x9D\xBC", "\xEB\xAF\xB8\xEA\xB7\xB8\xEB\xA3\xB9" ": %s", "\xEA\xB7\xB8\xEB\xA3\xB9\xEC\x9D\xB4" " " "\xEC\x97\x86\xEB\x8A\x94" " " "\xEB\xB0\x94\xEB\x8B\x90\xEB\x9D\xBC" " " "\xEC\xA2\x85\xEC\xA1\xB1", "\xEB\xAA\xA8\xEB\x93\x9C" " \"%s\"" "\xEC\x9D\x98" " " "\xEA\xB7\xB8\xEB\xA3\xB9\xEC\x9D\xB4" " " "\xEC\x97\x86\xEB\x8A\x94" " " "\xEC\xA2\x85\xEC\xA1\xB1",
    "PlayableConfigurable  (%d/%d " "\xEC\xBC\x9C\xEC\xA7\x90" ")", "   (%d/%d " "\xEC\xBC\x9C\xEC\xA7\x90" ")", "[ " "\xEC\xBC\x9C\xEC\xA7\x90" " ]  ", "[ " "\xEA\xBA\xBC\xEC\xA7\x90" " ]  ", "  (" "\xEB\x8F\x99\xEB\xAC\xBC" ")",
    "\xEC\xA2\x85\xEC\xA1\xB1", "\xEC\xA0\x84\xEC\xB2\xB4" " " "\xED\x99\x9C\xEC\x84\xB1\xED\x99\x94", "\xEC\xA0\x84\xEC\xB2\xB4" " " "\xEB\xB9\x84\xED\x99\x9C\xEC\x84\xB1\xED\x99\x94", "\xEA\xB8\xB0\xEB\xB3\xB8\xEA\xB0\x92", "\xEB\xB0\x94\xEB\x8B\x90\xEB\x9D\xBC\xEB\xA7\x8C",
    "\xEB\x8F\x99\xEB\xAC\xBC" " " "\xED\x97\x88\xEC\x9A\xA9" ": " "\xEC\xBC\x9C\xEC\xA7\x90", "\xEB\x8F\x99\xEB\xAC\xBC" " " "\xED\x97\x88\xEC\x9A\xA9" ": " "\xEA\xBA\xBC\xEC\xA7\x90",
    "\xEC\x8B\x9C\xEC\x9E\x91" " " "\xEC\xA2\x85\xEC\xA1\xB1" " " "\xEC\xA0\x9C\xED\x95\x9C" " " "\xED\x95\xB4\xEC\xA0\x9C" ": " "\xEC\xBC\x9C\xEC\xA7\x90" " (" "\xEB\x90\x98\xEB\x8F\x8C\xEB\xA6\xAC\xEB\xA0\xA4\xEB\xA9\xB4" " " "\xEC\x9E\xAC\xEC\x8B\x9C\xEC\x9E\x91" " " "\xED\x95\x84\xEC\x9A\x94" ")",
    "\xEC\x8B\x9C\xEC\x9E\x91" " " "\xEC\xA2\x85\xEC\xA1\xB1" " " "\xEC\xA0\x9C\xED\x95\x9C" " " "\xED\x95\xB4\xEC\xA0\x9C" ": " "\xEA\xBA\xBC\xEC\xA7\x90",
    "\xEC\x96\xB8\xEC\x96\xB4",
    "Kenshi " "\xEC\x9E\x90\xEC\xB2\xB4" " " "\xEC\x96\xB8\xEC\x96\xB4\xEC\x99\x80" " " "\xEC\x98\x81\xEC\x96\xB4\xEB\xA7\x8C" " " "\xEC\x95\x88\xEC\xA0\x95\xEC\xA0\x81\xEC\x9C\xBC\xEB\xA1\x9C" " " "\xED\x91\x9C\xEC\x8B\x9C\xEB\x90\xA9\xEB\x8B\x88\xEB\x8B\xA4" ". " "\xEB\x8B\xA4\xEB\xA5\xB8" " " "\xEC\x96\xB8\xEC\x96\xB4\xEB\x8A\x94" " " "\xEB\xB9\x88" " " "\xEB\xAC\xB8\xEC\x9E\x90\xEB\xA1\x9C" " " "\xEB\xB3\xB4\xEC\x9D\xBC" " " "\xEC\x88\x98" " " "\xEC\x9E\x88\xEC\x8A\xB5\xEB\x8B\x88\xEB\x8B\xA4"
},
/* pt_BR */ {
    "Grupos de Ra" "\xC3\xA7" "a", "Grupos de Mods", "Sem categoria",
    "origem desconhecida", "Vanilla (%s)", "Mod: %s",
    "Sem grupo: Vanilla", "Sem grupo: %s", "ra" "\xC3\xA7" "as vanilla sem grupo de ra" "\xC3\xA7" "a", "ra" "\xC3\xA7" "as do mod \"%s\" sem grupo de ra" "\xC3\xA7" "a",
    "PlayableConfigurable  (%d/%d ativas)", "   (%d/%d ativas)", "[ ON ]  ", "[ off ]  ", "  (animal)",
    "RA" "\xC3\x87" "AS", "ATIVAR TUDO", "DESATIVAR TUDO", "PADR" "\xC3\x83" "O", "SOMENTE VANILLA",
    "Animais permitidos: ON", "Animais permitidos: OFF",
    "Desbloquear restri" "\xC3\xA7\xC3\xB5" "es de in" "\xC3\xAD" "cio: ON (reinicie para desfazer)",
    "Desbloquear restri" "\xC3\xA7\xC3\xB5" "es de in" "\xC3\xAD" "cio: OFF",
    "Idioma", "Somente o idioma do seu Kenshi e o ingl" "\xC3\xAA" "s s" "\xC3\xA3" "o exibidos com confian" "\xC3\xA7" "a; outros podem mostrar caracteres em branco"
},
/* uk_UA */ {
    "\xD0\x93\xD1\x80\xD1\x83\xD0\xBF\xD0\xB8" " " "\xD1\x80\xD0\xB0\xD1\x81", "\xD0\x93\xD1\x80\xD1\x83\xD0\xBF\xD0\xB8" " " "\xD0\xBC\xD0\xBE\xD0\xB4\xD1\x96\xD0\xB2", "\xD0\x91\xD0\xB5\xD0\xB7" " " "\xD0\xBA\xD0\xB0\xD1\x82\xD0\xB5\xD0\xB3\xD0\xBE\xD1\x80\xD1\x96\xD1\x97",
    "\xD0\xB4\xD0\xB6\xD0\xB5\xD1\x80\xD0\xB5\xD0\xBB\xD0\xBE" " " "\xD0\xBD\xD0\xB5\xD0\xB2\xD1\x96\xD0\xB4\xD0\xBE\xD0\xBC\xD0\xB5", "\xD0\x92\xD0\xB0\xD0\xBD\xD1\x96\xD0\xBB\xD1\x8C\xD0\xBD\xD0\xB0" " (%s)", "\xD0\x9C\xD0\xBE\xD0\xB4" ": %s",
    "\xD0\x91\xD0\xB5\xD0\xB7" " " "\xD0\xB3\xD1\x80\xD1\x83\xD0\xBF\xD0\xB8" ": " "\xD0\xB2\xD0\xB0\xD0\xBD\xD1\x96\xD0\xBB\xD1\x8C", "\xD0\x91\xD0\xB5\xD0\xB7" " " "\xD0\xB3\xD1\x80\xD1\x83\xD0\xBF\xD0\xB8" ": %s", "\xD0\xB2\xD0\xB0\xD0\xBD\xD1\x96\xD0\xBB\xD1\x8C\xD0\xBD\xD1\x96" " " "\xD1\x80\xD0\xB0\xD1\x81\xD0\xB8" " " "\xD0\xB1\xD0\xB5\xD0\xB7" " " "\xD0\xB3\xD1\x80\xD1\x83\xD0\xBF\xD0\xB8" " " "\xD1\x80\xD0\xB0\xD1\x81", "\xD1\x80\xD0\xB0\xD1\x81\xD0\xB8" " " "\xD0\xBC\xD0\xBE\xD0\xB4\xD0\xB0" " \"%s\" " "\xD0\xB1\xD0\xB5\xD0\xB7" " " "\xD0\xB3\xD1\x80\xD1\x83\xD0\xBF\xD0\xB8",
    "PlayableConfigurable  (%d/%d " "\xD1\x83\xD0\xB2\xD1\x96\xD0\xBC\xD0\xBA" ")", "   (%d/%d " "\xD1\x83\xD0\xB2\xD1\x96\xD0\xBC\xD0\xBA" ")", "[ " "\xD0\xA3\xD0\x92\xD0\x86\xD0\x9C\xD0\x9A" " ]  ", "[ " "\xD0\xB2\xD0\xB8\xD0\xBC\xD0\xBA" " ]  ", "  (" "\xD1\x82\xD0\xB2\xD0\xB0\xD1\x80\xD0\xB8\xD0\xBD\xD0\xB0" ")",
    "\xD0\xA0\xD0\x90\xD0\xA1\xD0\x98", "\xD0\xA3\xD0\x92\xD0\x86\xD0\x9C\xD0\x9A\xD0\x9D\xD0\xA3\xD0\xA2\xD0\x98" " " "\xD0\x92\xD0\xA1\xD0\x95", "\xD0\x92\xD0\x98\xD0\x9C\xD0\x9A\xD0\x9D\xD0\xA3\xD0\xA2\xD0\x98" " " "\xD0\x92\xD0\xA1\xD0\x95", "\xD0\x97\xD0\x90" " " "\xD0\x97\xD0\x90\xD0\x9C\xD0\x9E\xD0\x92\xD0\xA7\xD0\xA3\xD0\x92\xD0\x90\xD0\x9D\xD0\x9D\xD0\xAF\xD0\x9C", "\xD0\xA2\xD0\x86\xD0\x9B\xD0\xAC\xD0\x9A\xD0\x98" " " "\xD0\x92\xD0\x90\xD0\x9D\xD0\x86\xD0\x9B\xD0\xAC",
    "\xD0\xA2\xD0\xB2\xD0\xB0\xD1\x80\xD0\xB8\xD0\xBD\xD0\xB8" " " "\xD0\xB4\xD0\xBE\xD0\xB7\xD0\xB2\xD0\xBE\xD0\xBB\xD0\xB5\xD0\xBD\xD1\x96" ": " "\xD0\xA3\xD0\x92\xD0\x86\xD0\x9C\xD0\x9A", "\xD0\xA2\xD0\xB2\xD0\xB0\xD1\x80\xD0\xB8\xD0\xBD\xD0\xB8" " " "\xD0\xB4\xD0\xBE\xD0\xB7\xD0\xB2\xD0\xBE\xD0\xBB\xD0\xB5\xD0\xBD\xD1\x96" ": " "\xD0\x92\xD0\x98\xD0\x9C\xD0\x9A",
    "\xD0\x97\xD0\xBD\xD1\x8F\xD1\x82\xD0\xB8" " " "\xD0\xBE\xD0\xB1\xD0\xBC\xD0\xB5\xD0\xB6\xD0\xB5\xD0\xBD\xD0\xBD\xD1\x8F" " " "\xD1\x81\xD1\x82\xD0\xB0\xD1\x80\xD1\x82\xD1\x96\xD0\xB2" ": " "\xD0\xA3\xD0\x92\xD0\x86\xD0\x9C\xD0\x9A" " (" "\xD0\xB4\xD0\xBB\xD1\x8F" " " "\xD1\x81\xD0\xBA\xD0\xB0\xD1\x81\xD1\x83\xD0\xB2\xD0\xB0\xD0\xBD\xD0\xBD\xD1\x8F" " " "\xD0\xBF\xD0\xBE\xD1\x82\xD1\x80\xD1\x96\xD0\xB1\xD0\xB5\xD0\xBD" " " "\xD0\xBF\xD0\xB5\xD1\x80\xD0\xB5\xD0\xB7\xD0\xB0\xD0\xBF\xD1\x83\xD1\x81\xD0\xBA" ")",
    "\xD0\x97\xD0\xBD\xD1\x8F\xD1\x82\xD0\xB8" " " "\xD0\xBE\xD0\xB1\xD0\xBC\xD0\xB5\xD0\xB6\xD0\xB5\xD0\xBD\xD0\xBD\xD1\x8F" " " "\xD1\x81\xD1\x82\xD0\xB0\xD1\x80\xD1\x82\xD1\x96\xD0\xB2" ": " "\xD0\x92\xD0\x98\xD0\x9C\xD0\x9A",
    "\xD0\x9C\xD0\xBE\xD0\xB2\xD0\xB0",
    "\xD0\x9D\xD0\xB0\xD0\xB4\xD1\x96\xD0\xB9\xD0\xBD\xD0\xBE" " " "\xD0\xB2\xD1\x96\xD0\xB4\xD0\xBE\xD0\xB1\xD1\x80\xD0\xB0\xD0\xB6\xD0\xB0\xD1\x8E\xD1\x82\xD1\x8C\xD1\x81\xD1\x8F" " " "\xD0\xBB\xD0\xB8\xD1\x88\xD0\xB5" " " "\xD0\xBC\xD0\xBE\xD0\xB2\xD0\xB0" " " "\xD0\xB2\xD0\xB0\xD1\x88\xD0\xBE\xD0\xB3\xD0\xBE" " Kenshi " "\xD1\x82\xD0\xB0" " " "\xD0\xB0\xD0\xBD\xD0\xB3\xD0\xBB\xD1\x96\xD0\xB9\xD1\x81\xD1\x8C\xD0\xBA\xD0\xB0" "; " "\xD1\x96\xD0\xBD\xD1\x88\xD1\x96" " " "\xD0\xBC\xD0\xBE\xD0\xB6\xD1\x83\xD1\x82\xD1\x8C" " " "\xD0\xBF\xD0\xBE\xD0\xBA\xD0\xB0\xD0\xB7\xD1\x83\xD0\xB2\xD0\xB0\xD1\x82\xD0\xB8" " " "\xD0\xBF\xD1\x80\xD0\xBE\xD0\xBF\xD1\x83\xD1\x81\xD0\xBA\xD0\xB8"
},
/* pl_PL */ {
    "Grupy ras", "Grupy mod" "\xC3\xB3" "w", "Bez kategorii",
    "nieznane pochodzenie", "Vanilla (%s)", "Mod: %s",
    "Bez grupy: Vanilla", "Bez grupy: %s", "rasy vanilla bez grupy ras", "rasy moda \"%s\" bez grupy",
    "PlayableConfigurable  (%d/%d w" "\xC5\x82" ".)", "   (%d/%d w" "\xC5\x82" ".)", "[ W" "\xC5\x81" " ]  ", "[ wy" "\xC5\x82" " ]  ", "  (zwierz" "\xC4\x99" ")",
    "RASY", "W" "\xC5\x81\xC4\x84" "CZ WSZYSTKO", "WY" "\xC5\x81\xC4\x84" "CZ WSZYSTKO", "DOMY" "\xC5\x9A" "LNIE", "TYLKO VANILLA",
    "Zwierz" "\xC4\x99" "ta dozwolone: W" "\xC5\x81", "Zwierz" "\xC4\x99" "ta dozwolone: WY" "\xC5\x81",
    "Zniesienie ogranicze" "\xC5\x84" " startu: W" "\xC5\x81" " (restart cofa)",
    "Zniesienie ogranicze" "\xC5\x84" " startu: WY" "\xC5\x81",
    "J" "\xC4\x99" "zyk", "Tylko j" "\xC4\x99" "zyk Twojego Kenshi i angielski wy" "\xC5\x9B" "wietlaj" "\xC4\x85" " si" "\xC4\x99" " poprawnie; inne mog" "\xC4\x85" " pokazywa" "\xC4\x87" " puste znaki"
},
/* zh_TW */ {
    "\xE7\xA8\xAE\xE6\x97\x8F\xE5\x88\x86\xE7\xB5\x84", "\xE6\xA8\xA1\xE7\xB5\x84\xE5\x88\x86\xE7\xB5\x84", "\xE6\x9C\xAA\xE5\x88\x86\xE9\xA1\x9E",
    "\xE4\xBE\x86\xE6\xBA\x90\xE6\x9C\xAA\xE7\x9F\xA5", "\xE5\x8E\x9F\xE7\x89\x88" " (%s)", "\xE6\xA8\xA1\xE7\xB5\x84\xEF\xBC\x9A" "%s",
    "\xE6\x9C\xAA\xE5\x88\x86\xE7\xB5\x84\xEF\xBC\x9A\xE5\x8E\x9F\xE7\x89\x88", "\xE6\x9C\xAA\xE5\x88\x86\xE7\xB5\x84\xEF\xBC\x9A" "%s", "\xE7\x84\xA1\xE5\x88\x86\xE7\xB5\x84\xE7\x9A\x84\xE5\x8E\x9F\xE7\x89\x88\xE7\xA8\xAE\xE6\x97\x8F", "\xE4\xBE\x86\xE8\x87\xAA\xE6\xA8\xA1\xE7\xB5\x84" "\"%s\"" "\xE7\x9A\x84\xE7\x84\xA1\xE5\x88\x86\xE7\xB5\x84\xE7\xA8\xAE\xE6\x97\x8F",
    "PlayableConfigurable  (%d/%d " "\xE5\xB7\xB2\xE5\x95\x9F\xE7\x94\xA8" ")", "   (%d/%d " "\xE5\xB7\xB2\xE5\x95\x9F\xE7\x94\xA8" ")", "[ " "\xE9\x96\x8B" " ]  ", "[ " "\xE9\x97\x9C" " ]  ", "  (" "\xE5\x8B\x95\xE7\x89\xA9" ")",
    "\xE7\xA8\xAE\xE6\x97\x8F", "\xE5\x85\xA8\xE9\x83\xA8\xE5\x95\x9F\xE7\x94\xA8", "\xE5\x85\xA8\xE9\x83\xA8\xE7\xA6\x81\xE7\x94\xA8", "\xE6\x81\xA2\xE5\xBE\xA9\xE9\xBB\x98\xE8\xAA\x8D", "\xE5\x83\x85\xE5\x8E\x9F\xE7\x89\x88",
    "\xE5\x85\x81\xE8\xA8\xB1\xE5\x8B\x95\xE7\x89\xA9\xEF\xBC\x9A\xE9\x96\x8B", "\xE5\x85\x81\xE8\xA8\xB1\xE5\x8B\x95\xE7\x89\xA9\xEF\xBC\x9A\xE9\x97\x9C",
    "\xE8\xA7\xA3\xE9\x99\xA4\xE5\x87\xBA\xE7\x94\x9F\xE9\x99\x90\xE5\x88\xB6\xEF\xBC\x9A\xE9\x96\x8B\xEF\xBC\x88\xE6\x92\xA4\xE9\x8A\xB7\xE9\x9C\x80\xE8\xA6\x81\xE9\x87\x8D\xE5\x95\x9F\xEF\xBC\x89",
    "\xE8\xA7\xA3\xE9\x99\xA4\xE5\x87\xBA\xE7\x94\x9F\xE9\x99\x90\xE5\x88\xB6\xEF\xBC\x9A\xE9\x97\x9C",
    "\xE8\xAA\x9E\xE8\xA8\x80",
    "\xE5\x8F\xAA\xE6\x9C\x89\xE4\xBD\xA0" " Kenshi " "\xE7\x9A\x84\xE8\xAA\x9E\xE8\xA8\x80\xE5\x92\x8C\xE8\x8B\xB1\xE8\xAA\x9E\xE8\x83\xBD\xE5\x8F\xAF\xE9\x9D\xA0\xE9\xA1\xAF\xE7\xA4\xBA\xEF\xBC\x9B\xE5\x85\xB6\xE4\xBB\x96\xE8\xAA\x9E\xE8\xA8\x80\xE5\x8F\xAF\xE8\x83\xBD\xE5\x87\xBA\xE7\x8F\xBE\xE7\xA9\xBA\xE7\x99\xBD\xE5\xAD\x97\xE7\xAC\xA6"
}
};
struct Lang { const char* code; const char* full; unsigned testcp; };
const Lang LANGS[L_COUNT] = {
    { "en_GB", "English | English", 'E' },
    { "ru_RU", "Russian | " "\xD0\xA0\xD1\x83\xD1\x81\xD1\x81\xD0\xBA\xD0\xB8\xD0\xB9", 0x0420 },
    { "es_ES", "Spanish | Espa" "\xC3\xB1" "ol", 0x00F1 },
    { "zh_CN", "Chinese (Simplified) | " "\xE4\xB8\xAD\xE6\x96\x87", 0x4E2D },
    { "de_DE", "German | Deutsch", 0x00E4 },
    { "fr_FR", "French | Fran" "\xC3\xA7" "ais", 0x00E7 },
    { "ja_JP", "Japanese | " "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E", 0x65E5 },
    { "ko_KR", "Korean | " "\xED\x95\x9C\xEA\xB5\xAD\xEC\x96\xB4", 0xD55C },
    { "pt_BR", "Portuguese | Portugu" "\xC3\xAA" "s", 0x00EA },
    { "uk_UA", "Ukrainian | " "\xD0\xA3\xD0\xBA\xD1\x80\xD0\xB0\xD1\x97\xD0\xBD\xD1\x81\xD1\x8C\xD0\xBA\xD0\xB0", 0x0456 },
    { "pl_PL", "Polish | Polski", 0x0142 },
    { "zh_TW", "Chinese (Traditional) | " "\xE7\xB9\x81\xE9\xAB\x94\xE4\xB8\xAD\xE6\x96\x87", 0x8A9E }
};
int g_lang = L_EN;
const char* T(int id) { return STR[g_lang][id]; }
S Fmt(int id, const S& a) { char b[2048]; sprintf_s(b, T(id), a.c_str()); return S(b); }
int LangByCode(const S& code)
{
    for (int i = 0; i < L_COUNT; i++) if (code == LANGS[i].code) return i;
    for (int i = 0; i < L_COUNT; i++) if (code.compare(0, 3, LANGS[i].code, 3) == 0) return i;
    return -1;
}

bool FontHas(unsigned cp)
{
    MyGUI::IFont* f = MyGUI::FontManager::getInstance().getByName("Kenshi_StandardFont_Medium");
    MyGUI::ResourceTrueTypeFont* tt = f ? f->castType<MyGUI::ResourceTrueTypeFont>(false) : NULL;
    if (!tt) return true;
    std::vector<std::pair<MyGUI::Char, MyGUI::Char> > rg = tt->getCodePointRanges();
    for (size_t i = 0; i < rg.size(); i++) if (cp >= rg[i].first && cp <= rg[i].second) return true;
    return false;
}

struct Cfg
{
    bool animals, force;
    S limitsFile;
    S lang;
    SBM states;
    Cfg() : animals(false), force(false), limitsFile(".\\data\\editor\\editor_data_human.xml") {}
};

struct Rows
{
    lektor<GD*> l;
    Rows(itemType t) { ou->gamedata.getDataOfType(l, t); }
    ~Rows() { if (l.stuff) free(l.stuff); }
    int n() const { return (int)l.size(); }
    GD* operator[](int i) { return l[i]; }
};

struct Race { S sid, name, origin; bool want, lim; };
struct Cat  { S sid, name, origin; std::vector<int> mem; bool open; int tier; };
struct Row  { int kind; int cat, race; };

Cfg g;
bool g_loaded = false;
volatile bool g_busy = false;
std::map<S, S> Names;
SBM Orig;
std::vector<Race> R;
std::vector<Cat> K;
std::vector<Row> D;
SBM OpenMemo;
WN* wnd = NULL;
B* launch = NULL;
SV* sv = NULL;
B* tip = NULL;
B* optA = NULL;
B* optF = NULL;
B* optL = NULL;
TXB* langLabel = NULL;
std::vector<B*> poolE, poolR;
std::vector<TXB*> poolD;

void Draw();
void Commit();
void Ensure(size_t needed);
void Rebuild();
void SyncLangBtn();

void Emit(void (*fn)(const std::string&), const char* fmt, va_list a)
{
    char b[512];
    strcpy_s(b, TAG);
    vsprintf_s(b + strlen(TAG), sizeof(b) - strlen(TAG), fmt, a);
    fn(b);
}
void Log(const char* fmt, ...) { va_list a; va_start(a, fmt); Emit(DebugLog, fmt, a); va_end(a); }
void Err(const char* fmt, ...) { va_list a; va_start(a, fmt); Emit(ErrorLog, fmt, a); va_end(a); }

S Trim(const S& s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    return a == S::npos ? S() : s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}

int Glyphs(const S& s)
{
    int n = 0;
    for (size_t i = 0; i < s.size(); i++) if (((unsigned char)s[i] & 0xC0) != 0x80) n++;
    return n;
}

bool Bool(const S& v, bool d)
{
    S t = Trim(v);
    if (t == "true" || t == "1" || t == "yes" || t == "on") return true;
    if (t == "false" || t == "0" || t == "no" || t == "off") return false;
    return d;
}

template<class M> typename M::mapped_type Get(M& m, const char* k, typename M::mapped_type d)
{
    typename M::iterator i = m.find(k);
    return i == m.end() ? d : i->second;
}

bool GetB(GD* d, const char* k, bool v) { return Get(d->bdata, k, v); }
S GetF(GD* d, const char* k) { return Get(d->filesdata, k, S()); }

void SetB(GD* d, const char* k, bool v)
{
    auto i = d->bdata.find(k);
    if (i != d->bdata.end()) i->second = v; else d->add(S(k), v, S(""), true);
}

void SetF(GD* d, const char* k, const S& v)
{
    auto i = d->filesdata.find(k);
    if (i != d->filesdata.end()) i->second = v;
    else d->addFileName(S(k), v, S("XML|*.xml"), S(""), true);
}

bool Tail(const S& s, const char* t)
{
    size_t n = strlen(t);
    return s.size() >= n && s.compare(s.size() - n, n, t) == 0;
}

S OrgFile(const S& s)
{
    size_t d = s.find('-');
    return (d == S::npos || d + 1 >= s.size()) ? S() : s.substr(d + 1);
}

bool InList(const S& s, const char* const* list, int n)
{
    for (int i = 0; i < n; i++) if (s == list[i]) return true;
    return false;
}

bool Van(const S& s)
{
    S f = OrgFile(s);
    if (f.empty()) return false;
    std::transform(f.begin(), f.end(), f.begin(), ::tolower);
    return InList(f, VANILLA, COUNTOF(VANILLA));
}

S Org(const S& s)
{
    S f = OrgFile(s);
    if (f.empty()) return S(T(T_UNKNOWN_ORIGIN));
    return Van(s) ? Fmt(T_VANILLA_PAREN, f) : Fmt(T_MOD_COLON, f);
}

bool VanillaPlayable(const S& sid) { return InList(sid, VANILLA_PLAYABLE, COUNTOF(VANILLA_PLAYABLE)); }

bool ModEnabled()
{
    FILE* f = NULL;
    if (fopen_s(&f, "data\\mods.cfg", "r") != 0 || !f) return true;
    char buf[512];
    bool on = false;
    while (fgets(buf, sizeof(buf), f))
    {
        if (_stricmp(Trim(buf).c_str(), "PlayableConfigurable.mod") == 0) { on = true; break; }
    }
    fclose(f);
    return on;
}

int DetectLang()
{
    FILE* f = NULL;
    if (fopen_s(&f, "settings.cfg", "r") != 0 || !f) return L_EN;
    char buf[256];
    int lang = L_EN;
    while (fgets(buf, sizeof(buf), f))
    {
        S line = Trim(buf);
        if (line.compare(0, 9, "language=") == 0)
        {
            int l = LangByCode(line.substr(9));
            if (l >= 0) lang = l;
            break;
        }
    }
    fclose(f);
    return lang;
}

S CfgPath()
{
    const char* rel = "mods\\PlayableConfigurable\\PlayableConfigurable.config.txt";
    if (GetFileAttributesA(rel) != INVALID_FILE_ATTRIBUTES) return S(rel);
    char b[MAX_PATH];
    DWORD n = GetEnvironmentVariableA("LOCALAPPDATA", b, MAX_PATH);
    if (n > 0 && n < MAX_PATH)
    {
        S dir = S(b) + "\\kenshi";
        CreateDirectoryA(dir.c_str(), NULL);
        return dir + "\\PlayableConfigurable.config.txt";
    }
    return S(rel);
}

void LoadCfg()
{
    g = Cfg();
    g_loaded = true;
    S path = CfgPath();
    FILE* f = NULL;
    if (fopen_s(&f, path.c_str(), "r") != 0 || !f)
    {
        Log("no config found - defaults to the game's own selection");
        return;
    }
    char buf[1024];
    int n = 0, na = 0;
    while (fgets(buf, sizeof(buf), f))
    {
        S t = Trim(buf);
        if (t.empty()) continue;
        bool arch = t.size() > 1 && t[0] == '#' && t[1] == '~';
        if (arch) t = Trim(t.substr(2));
        else if (t[0] == '#') continue;
        if (t.empty()) continue;
        if (t[0] == '@')
        {
            size_t e = t.find('=');
            if (e == S::npos) continue;
            S k = Trim(t.substr(1, e - 1)), v = Trim(t.substr(e + 1));
            if (k == "allowAnimals") g.animals = Bool(v, false);
            else if (k == "clearForceRace") g.force = Bool(v, false);
            else if (k == "animalLimitsFile" && !v.empty()) g.limitsFile = v;
            else if (k == "lang") g.lang = v;
            continue;
        }
        size_t p1 = t.find('|');
        if (p1 == S::npos) continue;
        size_t p2 = t.find('|', p1 + 1);
        S st = Trim(t.substr(0, p1));
        S sid = p2 == S::npos ? Trim(t.substr(p1 + 1)) : Trim(t.substr(p1 + 1, p2 - p1 - 1));
        if (sid.empty()) continue;
        if (p2 != S::npos)
        {
            size_t p3 = t.find('|', p2 + 1);
            S nm = p3 == S::npos ? Trim(t.substr(p2 + 1)) : Trim(t.substr(p2 + 1, p3 - p2 - 1));
            if (!nm.empty()) Names[sid] = nm;
        }
        if (st == "on") { g.states[sid] = true; if (arch) na++; else n++; }
        else if (st == "off") { g.states[sid] = false; if (arch) na++; else n++; }
    }
    fclose(f);
    Log("config loaded (%d entries, %d archived) from %s", n, na, path.c_str());
}

void SaveCfg()
{
    S path = CfgPath();
    FILE* f = NULL;
    if (fopen_s(&f, path.c_str(), "w") != 0 || !f)
    { Err("cannot write config: %s", path.c_str()); return; }
    fprintf(f, "# PlayableConfigurable config - written by the in-game window; safe to edit by hand\n");
    fprintf(f, "# state | stringId | name | note      ('#~' = remembered, its mod is not loaded now)\n");
    fprintf(f, "@allowAnimals = %s\n", g.animals ? "true" : "false");
    fprintf(f, "@animalLimitsFile = %s\n", g.limitsFile.c_str());
    fprintf(f, "@clearForceRace = %s\n", g.force ? "true" : "false");
    fprintf(f, "@lang = %s\n", g.lang.c_str());
    SS live;
    for (size_t i = 0; i < R.size(); i++)
    {
        fprintf(f, "%s|%s|%s|\n", R[i].want ? "on " : "off", R[i].sid.c_str(), R[i].name.c_str());
        live.insert(R[i].sid);
        Names[R[i].sid] = R[i].name;
    }
    for (SBM::const_iterator i = g.states.begin(); i != g.states.end(); ++i)
    {
        if (live.count(i->first)) continue;
        std::map<S, S>::const_iterator nm = Names.find(i->first);
        fprintf(f, "#~ %s|%s|%s|\n", i->second ? "on " : "off", i->first.c_str(),
                nm == Names.end() ? "" : nm->second.c_str());
    }
    fclose(f);
}

void Apply()
{
    if (g_busy) return;
    g_busy = true;
    if (!g_loaded) LoadCfg();

    SS grouped;
    {
        Rows q(RACE_GROUP);
        for (int i = 0; i < q.n(); i++)
            if (RV* r = q[i]->getReferenceListIfExists("races"))
                for (size_t j = 0; j < r->size(); j++) grouped.insert((*r)[j].sid);
    }

    int en = 0, di = 0, sk = 0, mk = 0, st = 0;
    {
        Rows q(RACE);
        for (int i = 0; i < q.n(); i++)
        {
            GD* d = q[i];
            const S& sid = d->stringID;
            bool lim = !GetF(d, "editor limits").empty();
            bool now = GetB(d, "playable", true);
            bool in = grouped.count(sid) != 0;
            bool vis = now && lim && in;
            if (!Orig.count(sid)) Orig[sid] = vis;
            SBM::const_iterator c = g.states.find(sid);
            bool want = c != g.states.end() ? c->second : Orig[sid];
            if (want == vis) continue;
            if (want)
            {
                if (!lim && !g.animals) { sk++; continue; }
                bool hit = false;
                if (!now) { SetB(d, "playable", true); hit = true; }
                if (!lim) { SetF(d, "editor limits", g.limitsFile); hit = true; }
                if (!in)
                {
                    S id = "PCG::" + sid;
                    GD* p = ou->gamedata.getData(id);
                    if (!p) { p = ou->gamedata.createNewData(RACE_GROUP, id, d->name); mk++; }
                    if (p && !p->findInList("races", sid)) p->addToList("races", sid, 0, 0, 0);
                    hit = true;
                }
                if (hit) en++;
            }
            else
            {
                SetB(d, "playable", false);
                di++;
            }
        }
    }

    if (g.force)
    {
        Rows q(NEW_GAME_STARTOFF);
        for (int i = 0; i < q.n(); i++)
        {
            RV* r = q[i]->getReferenceListIfExists("force race");
            if (r && !r->empty()) { q[i]->clearList("force race"); st++; }
        }
    }

    Log("applied: %d enabled/fixed, %d disabled, %d skipped (no limits), "
        "%d runtime groups, %d starts unlocked", en, di, sk, mk, st);
    g_busy = false;
}

void (*PostProcess_orig)(GDM*) = NULL;
void PostProcess_hook(GDM* self) { Apply(); PostProcess_orig(self); }

void Rebuild()
{
    if (PostProcess_orig && ou) PostProcess_orig(&ou->gamedata);
}

bool ByRace(const Race& a, const Race& b) { return a.name < b.name; }
bool ByCat(const Cat& a, const Cat& b) { return a.tier != b.tier ? a.tier < b.tier : a.name < b.name; }
bool ByMem(int a, int b) { return R[a].name < R[b].name; }

void Scan()
{
    for (size_t i = 0; i < K.size(); i++) OpenMemo[K[i].sid] = K[i].open;
    R.clear();
    K.clear();

    {
        Rows q(RACE);
        for (int i = 0; i < q.n(); i++)
        {
            GD* d = q[i];
            Race e;
            e.sid = d->stringID;
            e.name = d->name;
            e.origin = Org(e.sid);
            e.lim = !GetF(d, "editor limits").empty();
            SBM::const_iterator c = g.states.find(e.sid);
            SBM::const_iterator o = Orig.find(e.sid);
            e.want = c != g.states.end() ? c->second : (o != Orig.end() ? o->second : e.lim);
            R.push_back(e);
        }
    }
    std::sort(R.begin(), R.end(), ByRace);

    SIM idx;
    for (int i = 0; i < (int)R.size(); i++) idx[R[i].sid] = i;

    std::set<int> grouped;
    {
        Rows q(RACE_GROUP);
        for (int i = 0; i < q.n(); i++)
        {
            GD* d = q[i];
            const S& sid = d->stringID;
            if (sid.compare(0, 5, "PCG::") == 0) continue;
            if (Tail(sid, "-PlayableConfigurable.mod")) continue;
            Cat c;
            c.sid = sid;
            c.name = d->name;
            c.origin = Org(sid);
            c.tier = Van(sid) ? 0 : 1;
            if (RV* r = d->getReferenceListIfExists("races"))
                for (size_t j = 0; j < r->size(); j++)
                {
                    SIM::iterator f = idx.find((*r)[j].sid);
                    if (f != idx.end()) c.mem.push_back(f->second);
                }
            if (c.mem.empty()) continue;
            std::sort(c.mem.begin(), c.mem.end(), ByMem);
            c.mem.erase(std::unique(c.mem.begin(), c.mem.end()), c.mem.end());
            c.open = Get(OpenMemo, c.sid.c_str(), false);
            for (size_t j = 0; j < c.mem.size(); j++) grouped.insert(c.mem[j]);
            K.push_back(c);
        }
    }

    std::map<S, Cat> ungrouped;
    for (int i = 0; i < (int)R.size(); i++)
    {
        if (grouped.count(i)) continue;
        const S& sid = R[i].sid;
        bool van = Van(sid);
        S key = van ? S("::vanilla") : ("::mod:" + OrgFile(sid));
        Cat& c = ungrouped[key];
        if (c.sid.empty())
        {
            c.sid = key;
            c.name = van ? S(T(T_UNGROUPED_VANILLA)) : Fmt(T_UNGROUPED_MOD, OrgFile(sid));
            c.origin = van ? S(T(T_UNGROUPED_VANILLA_DESC)) : Fmt(T_UNGROUPED_MOD_DESC, OrgFile(sid));
            c.open = Get(OpenMemo, c.sid.c_str(), false);
            c.tier = 2;
        }
        c.mem.push_back(i);
    }
    for (std::map<S, Cat>::iterator it = ungrouped.begin(); it != ungrouped.end(); ++it)
    {
        std::sort(it->second.mem.begin(), it->second.mem.end(), ByMem);
        K.push_back(it->second);
    }
    std::sort(K.begin(), K.end(), ByCat);
}

void Flat()
{
    D.clear();
    int lastTier = -1;
    for (int c = 0; c < (int)K.size(); c++)
    {
        if (K[c].tier != lastTier)
        {
            lastTier = K[c].tier;
            Row d = { 0, lastTier, -1 };
            D.push_back(d);
        }
        Row h = { 1, c, -1 };
        D.push_back(h);
        if (!K[c].open) continue;
        for (size_t m = 0; m < K[c].mem.size(); m++)
        {
            Row r = { 2, c, K[c].mem[m] };
            D.push_back(r);
        }
    }
}

int OnCount(const Cat& c)
{
    int n = 0;
    for (size_t m = 0; m < c.mem.size(); m++) if (R[c.mem[m]].want) n++;
    return n;
}

void SetOpt(B* b, bool on, const char* yes, const char* no)
{
    if (!b) return;
    b->setCaption(on ? yes : no);
    b->setTextColour(on ? C_ON : C_OFF);
}

template<class W2> void RowSet(W2* w, int x, int y, int wid, int h, const S& cap, const CL& col)
{
    w->setCoord(x, y, wid, h);
    w->setCaption(cap);
    w->setTextColour(col);
}

void HideRow(size_t i) { poolE[i]->setVisible(false); poolR[i]->setVisible(false); poolD[i]->setVisible(false); }

void Draw()
{
    if (!wnd || !sv) return;
    Ensure(D.size());

    int on = 0;
    for (size_t i = 0; i < R.size(); i++) if (R[i].want) on++;
    char buf[192];
    sprintf_s(buf, T(T_WIN_TITLE), on, (int)R.size());
    wnd->setCaption(buf);

    int wide = sv->getViewCoord().width;
    if (wide < 80) wide = sv->getWidth() - 28;
    int y = 4;
    for (size_t i = 0; i < poolR.size(); i++)
    {
        if (i >= D.size()) { HideRow(i); continue; }
        const Row& d = D[i];
        if (d.kind == 0)
        {
            poolE[i]->setVisible(false);
            poolR[i]->setVisible(false);
            poolD[i]->setVisible(true);
            sprintf_s(buf, "-- %s --", T(T_RACE_GROUPS + d.cat));
            RowSet(poolD[i], 2, y, wide - 4, ROW_H - 4, buf, C_ON);
            y += ROW_H;
            continue;
        }
        poolD[i]->setVisible(false);
        poolE[i]->setVisible(d.kind == 1);
        poolR[i]->setVisible(true);
        if (d.kind == 1)
        {
            const Cat& c = K[d.cat];
            int hits = OnCount(c);
            RowSet(poolE[i], 2, y, EXP_W, ROW_H - 4, c.open ? "-" : "+", C_ON);
            sprintf_s(buf, T(T_CAT_COUNT), hits, (int)c.mem.size());
            RowSet(poolR[i], EXP_W + 6, y, wide - EXP_W - 12, ROW_H - 4, c.name + buf,
                   hits == (int)c.mem.size() ? C_ON : hits == 0 ? C_OFF : C_MIX);
        }
        else
        {
            const Race& e = R[d.race];
            RowSet(poolR[i], INDENT, y, wide - INDENT - 10, ROW_H - 4,
                   S(e.want ? T(T_ON_TAG) : T(T_OFF_TAG)) + e.name + (e.lim ? S("") : S(T(T_ANIMAL_SUFFIX))), e.want ? C_ON : C_OFF);
        }
        y += ROW_H;
    }
    sv->setCanvasSize(wide, y + 4);
    SetOpt(optA, g.animals, T(T_ANIMALS_ON), T(T_ANIMALS_OFF));
    SetOpt(optF, g.force, T(T_FORCE_ON), T(T_FORCE_OFF));
    if (langLabel)
    {
        bool warn = !FontHas(LANGS[g_lang].testcp);
        langLabel->setCaption(warn ? "Font not loaded - may show blanks" : T(T_LANGUAGE));
        langLabel->setTextColour(warn ? C_WARN : C_ON);
    }
    SyncLangBtn();
}

void Commit()
{
    for (size_t i = 0; i < R.size(); i++) g.states[R[i].sid] = R[i].want;
    SaveCfg();
    Apply();
    Rebuild();
    Draw();
}

int Idx(WP s)
{
    int i = atoi(s->getName().c_str() + 5);
    return (i < 0 || i >= (int)D.size()) ? -1 : i;
}

void OnRow(WP s)
{
    int i = Idx(s);
    if (i < 0 || D[i].kind == 0) return;
    if (D[i].kind == 1)
    {
        Cat& c = K[D[i].cat];
        bool any = OnCount(c) < (int)c.mem.size();
        for (size_t m = 0; m < c.mem.size(); m++) R[c.mem[m]].want = any;
    }
    else R[D[i].race].want = !R[D[i].race].want;
    Commit();
}

void OnExpand(WP s)
{
    int i = Idx(s);
    if (i < 0 || D[i].kind != 1) return;
    Cat& c = K[D[i].cat];
    c.open = !c.open;
    OpenMemo[c.sid] = c.open;
    Flat();
    Draw();
}

void OnPreset(WP s)
{
    int k = s->getName()[3] - '0';
    for (size_t i = 0; i < R.size(); i++)
    {
        Race& e = R[i];
        SBM::const_iterator o = Orig.find(e.sid);
        bool def = o != Orig.end() ? o->second : e.lim;
        e.want = k == 0 ? true : k == 1 ? false : k == 2 ? def : VanillaPlayable(e.sid);
    }
    Commit();
}

void OnOpt(WP s)
{
    bool& b = s->getName()[3] == '0' ? g.animals : g.force;
    b = !b;
    Commit();
}

void OnLang(WP s)
{
    g_lang = (g_lang + 1) % L_COUNT;
    g.lang = LANGS[g_lang].code;
    if (launch) launch->setCaption(T(T_RACES_BTN));
    SaveCfg();
    Scan();
    Flat();
    Draw();
}

void SyncLangBtn()
{
    if (optL) optL->setCaption(LANGS[g_lang].full);
}

void ShowTip(const S& text, const TT& info)
{
    tip->setCaption(text);
    tip->setSize(Glyphs(text) * 9 + 26, 34);
    tip->setPosition(info.point.left + 18, info.point.top + 10);
    tip->setVisible(true);
}

void OnLangTip(WP s, const TT& info)
{
    TIP_GUARD(info)
    ShowTip(T(T_LANG_HINT), info);
}

void OnTip(WP s, const TT& info)
{
    TIP_GUARD(info)
    int i = Idx(s);
    if (i < 0 || D[i].kind == 0) return;
    if (D[i].kind == 1)
    {
        const Cat& c = K[D[i].cat];
        ShowTip(c.sid[0] == ':' ? c.origin : (S(T(T_RACE_GROUPS)) + ": " + c.origin), info);
    }
    else ShowTip(R[D[i].race].origin, info);
}

void OnLaunch(WP s)
{
    if (!wnd) return;
    bool show = !wnd->getVisible();
    if (show) { Scan(); Flat(); Draw(); }
    wnd->setVisible(show);
}

void OnClose(WN* w, const S& b)
{
    w->setVisible(false);
    if (tip) tip->setVisible(false);
}

B* Mk(W* p, float x, float y, float w, float h, const char* name, const char* cap, void (*cb)(WP))
{
    B* b = p->createWidgetReal<B>(SK, x, y, w, h, AL::Default, name);
    if (cap) b->setCaption(cap);
    b->setTextColour(C_ON);
    if (cb) b->eventMouseButtonClick += DG(cb);
    return b;
}

void Ensure(size_t needed)
{
    if (!sv) return;
    while (poolD.size() < needed)
    {
        char name[32];
        sprintf_s(name, "PCDiv%d", (int)poolD.size());
        TXB* t = sv->createWidget<TXB>("Kenshi_TextboxStandardText", IC(2, 0, 200, ROW_H - 4), AL::Default, name);
        t->setTextAlign(AL::Center);
        t->setVisible(false);
        poolD.push_back(t);
    }
    while (poolR.size() < needed)
    {
        int i = (int)poolR.size();
        char name[32];
        sprintf_s(name, "PCExp%d", i);
        B* e = sv->createWidget<B>(SK, IC(2, 0, EXP_W, ROW_H - 4), AL::Default, name);
        e->eventMouseButtonClick += DG(OnExpand);
        e->setNeedToolTip(true);
        e->eventToolTip += DG(OnTip);
        e->setVisible(false);
        poolE.push_back(e);

        sprintf_s(name, "PCRow%d", i);
        B* r = sv->createWidget<B>(SK, IC(EXP_W + 6, 0, 200, ROW_H - 4), AL::Default, name);
        r->eventMouseButtonClick += DG(OnRow);
        r->setNeedToolTip(true);
        r->eventToolTip += DG(OnTip);
        r->setVisible(false);
        poolR.push_back(r);
    }
}

void BuildUi()
{
    MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
    if (!gui) return;
    if (WN* had = gui->findWidget<WN>("PlayableConfigurableWindow", false)) { wnd = had; return; }

    launch = gui->createWidgetReal<B>(SK, 0.895f, 0.015f, 0.095f, 0.045f,
                                      AL::Default, "Window", "PlayableConfigurableLauncher");
    launch->setCaption(T(T_RACES_BTN));
    launch->eventMouseButtonClick += DG(OnLaunch);

    wnd = gui->createWidgetReal<WN>("Kenshi_WindowCX", 0.70f, 0.10f, 0.28f, 0.80f,
                                    AL::Default, "Window", "PlayableConfigurableWindow");
    wnd->setVisible(false);
    wnd->eventWindowButtonPressed += DG(OnClose);

    W* c = wnd->getClientWidget();
    Mk(c, 0.02f, 0.012f, 0.47f, 0.05f, "PCP0", T(T_ENABLE_ALL), OnPreset);
    Mk(c, 0.51f, 0.012f, 0.47f, 0.05f, "PCP1", T(T_DISABLE_ALL), OnPreset);
    Mk(c, 0.02f, 0.068f, 0.47f, 0.05f, "PCP2", T(T_DEFAULT_ALL), OnPreset);
    Mk(c, 0.51f, 0.068f, 0.47f, 0.05f, "PCP3", T(T_VANILLA_ONLY), OnPreset);

    sv = c->createWidgetReal<SV>("Kenshi_ScrollView", 0.02f, 0.128f, 0.96f, 0.670f,
                                 AL::Stretch, "PCScroll");
    sv->setCanvasAlign(AL::Left | AL::Top);
    sv->setVisibleVScroll(true);
    sv->setVisibleHScroll(false);

    optA = Mk(c, 0.02f, 0.808f, 0.96f, 0.055f, "PCO0", NULL, OnOpt);
    optF = Mk(c, 0.02f, 0.868f, 0.96f, 0.055f, "PCO1", NULL, OnOpt);

    langLabel = c->createWidgetReal<TXB>("Kenshi_TextboxStandardText", 0.02f, 0.928f, 0.30f, 0.055f,
                                          AL::Default, "PCLangLabel");
    langLabel->setTextAlign(AL::Left | AL::VCenter);
    langLabel->setTextColour(C_ON);
    langLabel->setNeedToolTip(true);
    langLabel->eventToolTip += DG(OnLangTip);

    optL = Mk(c, 0.34f, 0.928f, 0.64f, 0.055f, "PCO2", NULL, OnLang);
    SyncLangBtn();

    tip = gui->createWidget<B>(SK, IC(0, 0, 300, 34), AL::Default, "ToolTip", "PCToolTip");
    tip->setTextColour(C_ON);
    tip->setVisible(false);

    Log("config window ready (RACES button, top-right)");
}

TS* (*Title_orig)(TS*) = NULL;
TS* Title_hook(TS* self) { TS* t = Title_orig(self); BuildUi(); return t; }

void (*TitleShow_orig)(TS*, bool) = NULL;
void TitleShow_hook(TS* self, bool on)
{
    TitleShow_orig(self, on);
    if (launch) launch->setVisible(on);
    if (!on)
    {
        if (wnd) wnd->setVisible(false);
        if (tip) tip->setVisible(false);
    }
}

} // namespace

__declspec(dllexport) void startPlugin()
{
    Log("plugin starting");
    g_lang = DetectLang();
    Log("language: %s", LANGS[g_lang].code);
    if (!ModEnabled())
    {
        Log("mod is disabled in the mod list - staying inactive");
        return;
    }
    LoadCfg();
    if (!g.lang.empty())
    {
        int l = LangByCode(g.lang);
        if (l >= 0) g_lang = l;
    }

    HOOK(&GDM::postProcessingTheDatas, PostProcess_hook, PostProcess_orig,
         "could not hook GameDataManager::postProcessingTheDatas - will still apply once at startup");

    if (KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&TS::_CONSTRUCTOR),
                                                 Title_hook, &Title_orig))
        Err("could not hook TitleScreen - no in-game config window (config file still works)");
    else if (ou && TS::getSingleton())
        BuildUi();

    HOOK(&TS::_NV_show, TitleShow_hook, TitleShow_orig,
         "could not hook TitleScreen::show - RACES button will stay visible in-game");

    Apply();
}
