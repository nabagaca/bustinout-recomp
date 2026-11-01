# Opening sequence bypass

Implemented 2026-11-01 in `src/game/hooks.cpp`; controlled Windows Release launch checked on the same date. Generated guest instructions were inspected read-only. No generated code or assets were changed.

## Video open

`sub_00175A60` is movie vtable `0x2EF974` slot 0. ECX is the movie, the fourth stack argument (`esp+0x10`) is the movie ID, AL is success, and both returns clean 16 argument bytes (`0x175F32`, `0x175F5A`). The resolver at `0x175A93..0x175C52` selects EA variants, Sims intro variants, reward videos, and the attract video. Its default branch selects `ea_games.xmv`.

The override retains the resolver's reward/attract IDs and delegates those to the original. For opening IDs it performs the existing reset from `0x175C52..0x175CB1` under the movie mutex: destroy prior decoder, close prior file, free prior read buffer, clear counters, set state 2. It returns success before file open at `0x175CB4` or decoder creation at `0x175D6A`. No new video texture or audio setup runs.

The constructor `sub_00175680` initializes state 2 and null decoder/resources. The completion method `sub_001758F0` returns true for states other than 0 and 1. `sub_00175930` returns false without calling the decoder when its pointer is null. Thus a bypassed movie is already finished and cannot submit a frame.

## Maxis image state

Maxis uses a separate image state, vtable `0x2EAD98`. Its enter method `sub_000F73D0` resolves `maxis_logo_brand` at `0xF73EA`, then configures its image and fade timers. Its update `sub_000F8660` normally finishes by requesting state 5 with zero transition time at `0xF86F5..0xF86FB`, through `sub_000F5480`.

With skipping enabled, enter requests that same transition before brand loading. Update requests it directly as well; draw `sub_000F74A0` returns without rendering during the queued transition. Enter has a plain return, while update and draw clean one stack argument. Original construction and exit retain ownership of the generic UI objects. With skipping disabled all three methods delegate to their original bodies.

## Controlled launch evidence

Windows Release rebuilt successfully. The default-on launch logged EA ID `D5AEC993` bypassed at 0.399 seconds, Maxis before brand load at 0.413 seconds, and Sims intro ID `B5968ED8` at 0.447 seconds. The live game window subsequently showed the title screen and “Please press START to begin”. No menu input was sent; F2 was used only to focus and inspect the running window. The test process was then closed.

This launch establishes the enabled startup path on Windows. Disabled playback and reward/attract playback were checked by source routing, not a new gameplay run. The earlier synthetic Start implementation was removed; it did not skip the unskippable logo sequence and is not evidence for this implementation.
