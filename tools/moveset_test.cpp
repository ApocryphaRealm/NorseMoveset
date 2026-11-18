// moveset_test.cpp - offline test of NorseMoveset.hpp: the shipped default file and broken samples.
#include "NorseMoveset.hpp"

#include <cstdio>

static int g_pass = 0, g_fail = 0;
static void Check(bool ok, const char* what)
{
    if (ok) ++g_pass;
    else
    {
        ++g_fail;
        std::printf("FAIL: %s\n", what);
    }
}

int main(int argc, char** argv)
{
    namespace fs = std::filesystem;
    fs::path def = argc > 1 ? fs::path(argv[1]) : fs::path("package/movesets/_default.ini");
    fs::path tmp = fs::temp_directory_path() / "norse-moveset-test";
    fs::create_directories(tmp);
    std::ofstream(tmp / "z-broken.ini") << "[Broken]\nClass = X\nStart.light = Nope\nA = /Game/a.a\n\n"
                                           "[NoPath]\nClass = Y\nStart.light = A\nA = notapath\n\n"
                                           "[BadNext]\nClass = Z\nStart.light = A\nA = /Game/a.a | light Missing\n\n"
                                           "[Drengr]\nClass = Drengr\nRate = 2\nStart.light = Only\nOnly = /Game/only.only | impact 0.3\n";
    nmv::Library lib;
    std::vector<std::string> log;
    lib.Load({def, tmp / "z-broken.ini"}, [&](const std::string& s) {
        log.push_back(s);
        std::printf("%s\n", s.c_str());
    });
    auto logged = [&](const char* t) { return std::any_of(log.begin(), log.end(), [&](const std::string& s) { return s.find(t) != std::string::npos; }); };
    Check(lib.ForClass(L"Rekkr") && lib.ForClass(L"Berserkr"), "the default movesets load");
    Check(!lib.ForClass(L"X") && logged("names 'Nope', which is not a move"), "a start naming no move leaves the moveset out");
    Check(!lib.ForClass(L"Y") && logged("has no animation path"), "a move without a path leaves the moveset out");
    Check(!lib.ForClass(L"Z") && logged("names 'Missing'"), "a follow-up naming no move leaves the moveset out");
    const nmv::Moveset* d = lib.ForClass(L"Drengr");
    Check(d && d->moves.size() == 1 && d->rate == 2 && logged("replaces the earlier one"), "a later file's moveset of the same name replaces it");
    Check(d && d->Find(L"Only")->impact == 0.3, "impact is read");

    // the default Rekkr chain: light x3 cycles, power is the step
    const nmv::Moveset* r = lib.ForClass(L"Rekkr");
    nmv::ComboState st;
    double t = 0;
    auto press = [&](const wchar_t* in, double at) {
        const nmv::Move* m = nmv::Library::Choose(*r, st, in, at);
        if (m) st.lastEnd = at + 1.0; // each move "lasts" 1 s here
        return m ? m->name : std::wstring(L"(none)");
    };
    Check(press(L"light", t) == L"Slash1", "first light press starts the chain");
    Check(press(L"light", t += 1.3) == L"Slash2", "inside the window: the next move");
    Check(press(L"light", t += 1.3) == L"Slash3", "third move");
    Check(press(L"light", t += 1.3) == L"Slash1", "the chain loops");
    Check(press(L"light", t += 5.0) == L"Slash1", "after the window: back to the start");
    Check(press(L"power", t += 1.2) == L"Step", "power inside the window from Slash1 follows its 'power' link");
    Check(press(L"LIGHT", t += 1.2) == L"Slash1", "a move with no follow-up for the input: the start move (inputs case-insensitive)");
    Check(r->Find(L"Slash1")->motion == "GsSlash" && r->RateOf(*r->Find(L"Slash1")) > 1.1f, "motion table and rate");
    std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
    fs::remove_all(tmp);
    return g_fail ? 1 : 0;
}
