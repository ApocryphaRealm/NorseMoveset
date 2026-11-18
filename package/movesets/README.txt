NorseMoveset movesets
=====================

Every *.ini file here describes attack chains: which move a press plays, and which move the next press plays while
the combo window is open. The player's attacks read these files (and the AI will). A later file's moveset of the
same [name] replaces an earlier one, so an animation pack can ship a new moveset for a class without code.

    [Drengr]
    Class = Drengr                 ; the unit class (the hero's class: Drengr, Rekkr, Berserkr, Bogmathr, ...)
    Rate = 1.15                    ; play rate of its moves
    ComboWindow = 0.6              ; seconds after a move ends in which the next press continues the chain
    Start.light = Slash1           ; the first move for each input (light, power)
    Start.power = Lunge
    Slash1 = /Game/.../A_sword_and_shield_slash.A_sword_and_shield_slash | motion SnsSlash | light Slash2 | power Lunge
    Slash2 = /Game/... | light Slash3
    Lunge  = /Game/... | motion SnsLunge

A move: Name = animation path | then any of: light <move> / power <move> (what that press does next), motion <table>
(the clip's own movement and hit frame, from NorseRealtimeNative), rate <x>, window <seconds>, impact <seconds> (the
hit time when there is no motion table; default 40% of the clip). A moveset with a mistake (a move it names that
does not exist, a move without an animation path) is left out, and NorseRealtimeNative.log says why.
Console: nrn.movesets re-reads these files while the game runs.
