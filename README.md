# NorseMoveset

Combos and attack chains as data for the Norse overhaul (NORSE: Oath of Blood). Which move a press plays, and which
move the next press plays while the combo window is open - per unit class, in plain files an animation pack can ship
without code. The player's attacks read them (NorseRealtimeNative 0.5.27); the per-actor AI is meant to read the same
files, the lesson of Skyrim's MCO (player only) and SCAR (NPC combos added afterwards). Survey:
`RESEARCH-more-frameworks.md` (#6).

```ini
[Drengr]
Class = Drengr
Rate = 1.15
ComboWindow = 0.6
Start.light = Slash1
Start.power = Lunge
Slash1 = /Game/.../A_sword_and_shield_slash.A_sword_and_shield_slash | motion SnsSlash | light Slash2
Slash2 = /Game/.../A_sword_and_shield_slash_3.A_sword_and_shield_slash_3 | motion SnsSlash3 | light Slash3
Slash3 = /Game/.../A_sword_and_shield_slash_5.A_sword_and_shield_slash_5 | motion SnsSlash5 | light Slash1
Lunge  = /Game/.../A_sword_and_shield_attack_2.A_sword_and_shield_attack_2 | motion SnsLunge
```

Author guide: `package\movesets\README.txt`. `package\movesets\_default.ini` is the hero's chains of
NorseRealtimeNative 0.5.23-0.5.27, moved out of its code unchanged.

## Parts

- `include/NorseMoveset.hpp` - header-only, game-free: parser, checks (a moveset naming a move that does not exist, or
  a move without an animation path, is left out with the reason), and `Library::Choose(moveset, state, input, now)`.
- No DLL: the data folder `ue4ss\Mods\NorseMoveset\movesets\` and its NorseManifest manifest. NorseRealtimeNative reads
  the files in pack order at start (`nrn.movesets` re-reads them) and keeps its built-in chains only when none are
  installed.
- `tools/moveset_test.cpp` - the offline test: `xmake build moveset_test && xmake run moveset_test <path to _default.ini>`.

## Status

0.1.0 tested offline (14 checks) and in game (TEST-RESULT-0.1.0.txt, the Bogmathr dagger chain). Next: AnimSwap
(conditional moves), NorseInput (held / double-press inputs), NorseConditions on a move, NorsePayload windows for
cancels, and the AI choosing from the same movesets.
