---
type: Architecture
title: Title integration
description: Timing, callbacks, cheats, keyboard input, and overlay.
sources:
  - resource: ../../src/game/hooks.cpp
  - resource: ../../src/game/cheats.cpp
  - resource: ../../src/common/cheats.h
  - resource: ../../src/common/input_kbm.cpp
  - resource: ../../src/common/overlay.cpp
  - resource: ../../src/common/game_info.cpp
---

# Timing overrides

[`hooks.cpp`](../../src/game/hooks.cpp) replaces quantizer `sub_001232F0` and wraps caller `sub_00123430`. The quantizer reads elapsed seconds from the guest stack, caps positive time at 0.25 seconds, substitutes 1 ms for nonpositive/NaN input, and computes 1..250 ticks rounded from 1 ms units. It writes tick count/per-tick duration whose product equals the chosen elapsed time, updates the running total, returns ticks in EAX and performs guest retn-4 stack cleanup.

The caller wrapper saves `this+5`, forces the original’s fixed-step product path for the call and restores the byte. Both delegate to original bodies when SIMSBO_NO_STEP is nonempty and not `"0"`. Comments explain why replacing only the quantizer leaves the old smoothed/clamped path in charge. [frame-rate.md](../frame-rate.md) records call-site analysis and consumers still needing play validation; the overrides do not establish independence for every simulation subsystem.

# Callback and aspect overrides

D3DDevice_InsertCallback takes callback/context from arguments 1/2 and immediately calls the guest callback if nonzero. Source comments explain that the original push-buffer token otherwise has no delivery path in the SDK draw walker, leaving an engine waiter stalled. Immediate delivery is the chosen approximation, not proof of native GPU completion.

Linux `sub_001287C0` runs the original aspect getter then replaces its x87 result only for listed projection-path return addresses and active HostAspect. Windows `sub_001316B0` temporarily scales the first row of the culling inverse-projection matrix and restores exact bits. Both respect SIMSBO_NO_WIDE. [Graphics](graphics.md) covers their differing mechanisms and the experimental gate.

# Cheats and thread ownership

The overlay uses [cheats.h](../../src/common/cheats.h), implemented by [cheats.cpp](../../src/game/cheats.cpp). Requests are atomic toggle bits; two pending clicks on one row cancel. The `sub_00012280` override consumes requests on the title thread before original input processing. It checks guest pointer ranges, overflow/committed pages, latch `0x3DCF70`, pad manager `0x35D3F0` and recognizer at `app+0x1E4`.

Accepted requests enter guest setter `sub_00072010` with options object `0x310B98`, using ECX plus one stack argument via mapped ABI. The setter toggles its own mask and sound; the host does not directly manufacture effects. Requests are consumed/logged as rejected when the game is not ready, not indefinitely retried. Flags are preserved around injected calls. [cheats.md](../cheats.md) retains Give Money play evidence and limits for other rows.

# Input and UI

[`input_kbm.cpp`](../../src/common/input_kbm.cpp) merges into port 0, reports connected, ORs buttons and increases axis deflection only where synthetic input is stronger. WASD drives left stick; arrows D-pad; Space A; Escape/Backspace B; Enter Start. Camera uses right-stick Q/E, R/F, wheel pulses and right-button mouse displacement. Full mappings remain in [README](../../README.md).

Windows collects held input through messages and releases on focus loss. Linux polls keyboard state and reads mouse data cached on the window thread. Focus gates input; F1 captures keyboard/mouse. [`overlay.cpp`](../../src/common/overlay.cpp) draws settings/cheats (F1) and performance (F2) through ImGui. Display changes use [app event handling](host.md), not native-window calls from render callbacks.

[`game_info.cpp`](../../src/common/game_info.cpp) layers opt-in scripts before keyboard merging. SIMSBO_STEP names a token file, takes precedence over autostart and samples every 50 ms under a mutex. Changed NAMExN tokens generate up to 40 pulses: 130 ms on/off for buttons, 160 ms for sticks. Leaving the same token does not restart the burst. SIMSBO_AUTOSTART otherwise cycles Start/Up/A after an initial delay; it is not a full unattended household test.
