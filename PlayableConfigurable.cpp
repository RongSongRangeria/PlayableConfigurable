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

enum { ROW_H = 34, EXP_W = 38, INDENT = 30 };
const CL C_ON(0.95f, 0.93f, 0.86f), C_OFF(0.45f, 0.44f, 0.40f), C_MIX(0.72f, 0.70f, 0.64f);
const char* const VANILLA[] = {
    "gamedata.quack", "gamedata.base", "dialogue.mod", "newwworld.mod", "rebirth.mod",
    "stick_people.mod", "chareditor.mod", "small_changes_otto.mod", "newwworld plus.mod"
};
const char* const TIER_NAMES[] = { "Race Groups", "Modded Groups", "Uncategorized" };
const char* const VANILLA_PLAYABLE[] = {
    "17-gamedata.quack", "18019-gamedata.base", "5276-chareditor.mod", "17946-stick_people.mod",
    "55396-gamedata.base", "5346-stick_people.mod", "18961-gamedata.base", "18960-gamedata.base"
};

struct Cfg
{
    bool animals, force;
    S limitsFile;
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
std::vector<B*> poolE, poolR;
std::vector<TXB*> poolD;

void Draw();
void Commit();
void Ensure(size_t needed);
void Rebuild();

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
    if (f.empty()) return S("unknown origin");
    return Van(s) ? ("Vanilla (" + f + ")") : ("Mod: " + f);
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
            c.name = van ? "Ungrouped: Vanilla" : ("Ungrouped: " + OrgFile(sid));
            c.origin = van ? "vanilla races with no race group" : ("mod \"" + OrgFile(sid) + "\" races with no race group");
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
    sprintf_s(buf, "PlayableConfigurable  (%d/%d on)", on, (int)R.size());
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
            sprintf_s(buf, "-- %s --", TIER_NAMES[d.cat]);
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
            sprintf_s(buf, "%s   (%d/%d on)", c.name.c_str(), hits, (int)c.mem.size());
            RowSet(poolR[i], EXP_W + 6, y, wide - EXP_W - 12, ROW_H - 4, buf,
                   hits == (int)c.mem.size() ? C_ON : hits == 0 ? C_OFF : C_MIX);
        }
        else
        {
            const Race& e = R[d.race];
            RowSet(poolR[i], INDENT, y, wide - INDENT - 10, ROW_H - 4,
                   (e.want ? "[ ON ]  " : "[ off ]  ") + e.name + (e.lim ? "" : "  (animal)"), e.want ? C_ON : C_OFF);
        }
        y += ROW_H;
    }
    sv->setCanvasSize(wide, y + 4);
    SetOpt(optA, g.animals, "Animals allowed: ON", "Animals allowed: OFF");
    SetOpt(optF, g.force, "Unlock forced starts: ON (restart to undo)", "Unlock forced starts: OFF");
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

void OnTip(WP s, const TT& info)
{
    if (!tip) return;
    if (info.type == TT::Hide) { tip->setVisible(false); return; }
    if (info.type != TT::Show) return;
    int i = Idx(s);
    if (i < 0 || D[i].kind == 0) return;
    S text;
    if (D[i].kind == 1)
    {
        const Cat& c = K[D[i].cat];
        char b[64];
        sprintf_s(b, "  |  %d subrace(s)", (int)c.mem.size());
        text = c.sid[0] == ':' ? (c.origin + b) : ("Race group - " + c.origin + b);
    }
    else text = R[D[i].race].origin;
    tip->setCaption(text);
    tip->setSize((int)text.size() * 9 + 26, 34);
    tip->setPosition(info.point.left + 18, info.point.top + 10);
    tip->setVisible(true);
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
    launch->setCaption("RACES");
    launch->eventMouseButtonClick += DG(OnLaunch);

    wnd = gui->createWidgetReal<WN>("Kenshi_WindowCX", 0.70f, 0.10f, 0.28f, 0.80f,
                                    AL::Default, "Window", "PlayableConfigurableWindow");
    wnd->setVisible(false);
    wnd->eventWindowButtonPressed += DG(OnClose);

    W* c = wnd->getClientWidget();
    Mk(c, 0.02f, 0.012f, 0.47f, 0.05f, "PCP0", "ENABLE ALL", OnPreset);
    Mk(c, 0.51f, 0.012f, 0.47f, 0.05f, "PCP1", "DISABLE ALL", OnPreset);
    Mk(c, 0.02f, 0.068f, 0.47f, 0.05f, "PCP2", "DEFAULT ALL", OnPreset);
    Mk(c, 0.51f, 0.068f, 0.47f, 0.05f, "PCP3", "VANILLA ONLY", OnPreset);

    sv = c->createWidgetReal<SV>("Kenshi_ScrollView", 0.02f, 0.128f, 0.96f, 0.735f,
                                 AL::Stretch, "PCScroll");
    sv->setCanvasAlign(AL::Left | AL::Top);
    sv->setVisibleVScroll(true);
    sv->setVisibleHScroll(false);

    optA = Mk(c, 0.02f, 0.872f, 0.96f, 0.055f, "PCO0", NULL, OnOpt);
    optF = Mk(c, 0.02f, 0.932f, 0.96f, 0.055f, "PCO1", NULL, OnOpt);

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
    if (!ModEnabled())
    {
        Log("mod is disabled in the mod list - staying inactive");
        return;
    }
    LoadCfg();

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
