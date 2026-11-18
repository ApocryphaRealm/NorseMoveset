// NorseMoveset.hpp - combos and attack chains as data (2026-11-17). Header-only and game-free: the parser, the checks
// and the "which move comes next" chooser. The player's attacks (NorseRealtimeNative) and, later, the per-actor AI read
// the same files - building both on one table from the start is the lesson of Skyrim's MCO (player only) and SCAR
// (added NPC combos afterwards).
//
// Files: ue4ss\Mods\NorseMoveset\movesets\*.ini, in NorseManifest's pack order (a later file's moveset of the same name
// replaces an earlier one). One [section] per moveset:
//   [Drengr]
//   Class = Drengr                 ; the unit class it is for (NorseRealtimeNative: the hero's class folder)
//   Rate = 1.15                    ; play rate of its moves (a move may override)
//   ComboWindow = 0.6              ; seconds after a move ends in which the next press continues the chain
//   Start.light = Slash1           ; the first move for each input
//   Start.power = Lunge
//   Slash1 = /Game/.../A_sword_and_shield_slash.A_sword_and_shield_slash | motion SnsSlash | light Slash2 | power Lunge
//   Slash2 = /Game/... | motion SnsSlash3 | light Slash3
//   Lunge  = /Game/... | motion SnsLunge
// A move: Name = animation path | then any of: motion <table> (the clip's own Root path and impact frame),
// light <move> / power <move> (what that press does next, inside the window), rate <x>, window <s>, impact <s> (the hit
// time when there is no motion table; default 40% of the clip). Modelled on the ADXP / MCO attack framework and SCAR.
#pragma once

#include <Windows.h>

#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace nmv
{
namespace fs = std::filesystem;

inline std::wstring Trim(const std::wstring& s)
{
    size_t a = 0, b = s.size();
    while (a < b && iswspace(s[a])) ++a;
    while (b > a && iswspace(s[b - 1])) --b;
    return s.substr(a, b - a);
}

inline std::wstring Lower(std::wstring s)
{
    for (auto& c : s) c = static_cast<wchar_t>(towlower(c));
    return s;
}

inline std::string N(const std::wstring& w)
{
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    std::string s(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), static_cast<int>(w.size()), s.data(), n, nullptr, nullptr);
    return s;
}

struct Move
{
    std::wstring name, anim;
    std::string motion;                      // motion table name, or empty
    float rate = 0;                          // 0 = the moveset's
    double window = -1;                      // < 0 = the moveset's
    double impact = -1;                      // seconds into the clip; < 0 = from the motion table, or 40%
    std::map<std::wstring, std::wstring> next; // input -> move
    std::string where;
};

struct Moveset
{
    std::wstring name, klass;
    float rate = 1.0f;
    double window = 0.6;
    std::map<std::wstring, std::wstring> start; // input -> move
    std::map<std::wstring, Move> moves;         // lower-case name -> move
    std::string where;

    const Move* Find(const std::wstring& n) const
    {
        auto it = moves.find(Lower(n));
        return it == moves.end() ? nullptr : &it->second;
    }
    float RateOf(const Move& m) const { return m.rate > 0 ? m.rate : rate; }
    double WindowOf(const Move& m) const { return m.window >= 0 ? m.window : window; }
};

// One unit's place in its chain.
struct ComboState
{
    const Moveset* set = nullptr;
    std::wstring last;    // the move played last
    double lastEnd = -1e9; // when it ended (the caller sets it once the move's length is known)
};

class Library
{
  public:
    using LogFn = std::function<void(const std::string&)>;

    // Reads the files in the order given (NorseManifest's), checks every moveset, keeps the sound ones.
    void Load(const std::vector<fs::path>& files, LogFn log = nullptr)
    {
        m_sets.clear();
        for (const auto& f : files) Read(f, log);
        for (auto it = m_sets.begin(); it != m_sets.end();)
        {
            std::string why = Check(it->second);
            if (!why.empty())
            {
                if (log) log("moveset [" + N(it->second.name) + "] (" + it->second.where + ") left out: " + why);
                it = m_sets.erase(it);
            }
            else ++it;
        }
        if (log)
            for (auto& [k, s] : m_sets)
                log("moveset [" + N(s.name) + "] for class " + N(s.klass) + ": " + std::to_string(s.moves.size()) + " move(s) (" + s.where + ")");
    }

    const Moveset* ForClass(const std::wstring& klass) const
    {
        for (auto& [k, s] : m_sets)
            if (_wcsicmp(s.klass.c_str(), klass.c_str()) == 0) return &s;
        return nullptr;
    }

    size_t Size() const { return m_sets.size(); }

    // The move an input plays now: the last move's follow-up while its window is open, else the moveset's start move.
    // Updates the state's last move (the caller sets lastEnd). Null when the moveset has nothing for that input.
    static const Move* Choose(const Moveset& set, ComboState& st, const std::wstring& input, double now)
    {
        std::wstring in = Lower(input);
        const Move* chosen = nullptr;
        if (st.set == &set && !st.last.empty())
            if (const Move* last = set.Find(st.last); last && now - st.lastEnd <= set.WindowOf(*last))
                if (auto it = last->next.find(in); it != last->next.end()) chosen = set.Find(it->second);
        if (!chosen)
            if (auto it = set.start.find(in); it != set.start.end()) chosen = set.Find(it->second);
        st.set = &set;
        if (chosen) st.last = chosen->name;
        return chosen;
    }

  private:
    std::map<std::wstring, Moveset> m_sets; // lower-case name -> moveset

    static std::vector<std::wstring> Split(const std::wstring& s, wchar_t sep)
    {
        std::vector<std::wstring> out;
        std::wstringstream ss(s);
        std::wstring part;
        while (std::getline(ss, part, sep)) out.push_back(Trim(part));
        return out;
    }

    void Read(const fs::path& f, const LogFn& log)
    {
        std::ifstream in(f, std::ios::binary);
        std::string all((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        if (all.size() >= 3 && static_cast<unsigned char>(all[0]) == 0xEF) all.erase(0, 3);
        int wn = MultiByteToWideChar(CP_UTF8, 0, all.c_str(), static_cast<int>(all.size()), nullptr, 0);
        std::wstring w(static_cast<size_t>(wn), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, all.c_str(), static_cast<int>(all.size()), w.data(), wn);
        std::wstringstream ss(w);
        std::wstring line;
        Moveset* cur = nullptr;
        int n = 0;
        std::string file = N(f.filename().wstring());
        while (std::getline(ss, line))
        {
            ++n;
            if (!line.empty() && line.back() == L'\r') line.pop_back();
            for (size_t i = 0; i < line.size(); ++i)
                if ((line[i] == L';' || line[i] == L'#') && (i == 0 || iswspace(line[i - 1])))
                {
                    line.resize(i);
                    break;
                }
            line = Trim(line);
            if (line.empty()) continue;
            std::string where = file + ":" + std::to_string(n);
            if (line.front() == L'[' && line.back() == L']')
            {
                std::wstring name = Trim(line.substr(1, line.size() - 2));
                if (m_sets.count(Lower(name)) && log) log(where + ": moveset [" + N(name) + "] replaces the earlier one");
                Moveset s;
                s.name = name;
                s.klass = name;
                s.where = where;
                m_sets[Lower(name)] = s;
                cur = &m_sets[Lower(name)];
                continue;
            }
            size_t eq = line.find(L'=');
            if (!cur || eq == std::wstring::npos)
            {
                if (log) log(where + ": not a [moveset] or key = value line - skipped");
                continue;
            }
            std::wstring k = Trim(line.substr(0, eq)), v = Trim(line.substr(eq + 1)), lk = Lower(k);
            if (lk == L"class") cur->klass = v;
            else if (lk == L"rate") cur->rate = static_cast<float>(_wtof(v.c_str()));
            else if (lk == L"combowindow") cur->window = _wtof(v.c_str());
            else if (lk.rfind(L"start.", 0) == 0) cur->start[lk.substr(6)] = v;
            else
            {
                auto parts = Split(v, L'|');
                Move m;
                m.name = k;
                m.anim = parts[0];
                m.where = where;
                for (size_t i = 1; i < parts.size(); ++i)
                {
                    size_t sp = parts[i].find(L' ');
                    std::wstring key = Lower(Trim(parts[i].substr(0, sp))), val = sp == std::wstring::npos ? L"" : Trim(parts[i].substr(sp + 1));
                    if (key == L"motion") m.motion = N(val);
                    else if (key == L"rate") m.rate = static_cast<float>(_wtof(val.c_str()));
                    else if (key == L"window") m.window = _wtof(val.c_str());
                    else if (key == L"impact") m.impact = _wtof(val.c_str());
                    else if (!val.empty()) m.next[key] = val; // light <move>, power <move>, any other input name
                    else if (log) log(where + ": '" + N(parts[i]) + "' has no value - ignored");
                }
                cur->moves[Lower(k)] = m;
            }
        }
    }

    static std::string Check(const Moveset& s)
    {
        if (s.moves.empty()) return "no moves";
        if (s.start.empty()) return "no Start.<input> line";
        for (auto& [in, mv] : s.start)
            if (!s.Find(mv)) return "Start." + N(in) + " names '" + N(mv) + "', which is not a move here";
        for (auto& [k, m] : s.moves)
        {
            if (m.anim.empty() || m.anim[0] != L'/') return "move " + N(m.name) + " (" + m.where + ") has no animation path";
            for (auto& [in, nx] : m.next)
                if (!s.Find(nx)) return "move " + N(m.name) + " (" + m.where + "): " + N(in) + " names '" + N(nx) + "', which is not a move here";
        }
        return {};
    }
};
} // namespace nmv
