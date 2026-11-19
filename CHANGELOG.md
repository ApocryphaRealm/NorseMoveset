# Changelog

## 0.1.0 - 2026-11-17 (tested offline, 14 checks, and in game: TEST-RESULT-0.1.0.txt)
### Added
- Movesets in movesets/*.ini: Class, Rate, ComboWindow, Start.<input>, moves with light/power links, motion, rate, window, impact.
- include/NorseMoveset.hpp: parser, checks, Library::Choose; _default.ini with the Drengr, Rekkr and Berserkr chains of NorseRealtimeNative.

### Changed
- The included movesets use the game's own attack animations (Drengr, Rekkr, Berserkr, Bogmathr, Hersir), so nothing else needs installing; the earlier defaults named animations from a separate pack.