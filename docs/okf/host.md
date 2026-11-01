---
type: Architecture
title: Host lifecycle and storage
description: Platform startup, discovery, boot, settings, and relaunch.
sources:
  - resource: ../../sdk/runtime/app/main.cpp
  - resource: ../../sdk/runtime/app/main_sdl.cpp
  - resource: ../../sdk/runtime/app/game_files.cpp
  - resource: ../../sdk/runtime/app/settings.cpp
  - resource: ../../sdk/runtime/kernel/boot.cpp
  - resource: ../../src/common/game_info.cpp
---

# Host entry and ownership

Windows enters [`wWinMain`](../../sdk/runtime/app/main.cpp); Linux enters [`main`](../../sdk/runtime/app/main_sdl.cpp). Both implement the interface in [app.h](../../sdk/runtime/app/app.h). [`app::kGame`](../../src/common/game_info.cpp) supplies names, settings filename, image mapping, overlay and pad callbacks. This title leaves init, loaded and window callbacks null, although the SDK supports them.

The host initializes logging/settings, invokes optional init, locates files, sets up window/UI/input and boots the title. It continues processing events while guest threads execute. Linux caches drawable size, focus and mouse state for other threads; custom SDL events marshal display changes and window opening onto the window thread. `UiLock` coordinates ImGui event processing and rendering. Keep main-thread-only SDL calls out of guest polling threads.

# Discovery versus build input

[`FindGame`](../../sdk/runtime/app/game_files.cpp) locates assets, considers the extracted XBE and sorted ISO candidates, and compares SHA-1 to the CMake-embedded `kImageSha1`. A matching extracted folder with other game-shaped files wins first. “Complete” means `CountGameFiles > 0`, a heuristic rather than validation of all disc contents; host artifacts and ISOs do not count.

An XBE-only folder is deferred so a matching ISO can supply data. With no matching ISO it can still be selected, logging the predicted disc error. If only another release exists, the host asks whether to proceed. Hash matching therefore is not an unconditional rejection of every differing release. Boot separately checks entry identity. The build still requires `assets/default.xbe` even if runtime will use an ISO.

# Boot sequence

[`kernel::Boot`](../../sdk/runtime/kernel/boot.cpp) initializes memory, parses the XBE and checks entry against `X86EntryPoint`. It reserves image space, copies headers/preload sections, sets their reference counts and invokes optional loaded. It then initializes kernel data/dispatch, rewrites imports to data addresses or synthetic function thunks, logs missing exports and mounts devices.

Launch data can be restored from `hdd/launch.bin`; the consumed file is removed. Persisted display state from `display.bin` is bounds-checked and reconstructed in contiguous guest memory, then the file is removed. Boot allocates TLS/stack state and starts the entry thread. A successful return establishes thread startup, not later gameplay success.

# Settings and paths

[`settings.cpp`](../../sdk/runtime/app/settings.cpp) reads executable-relative `settings.toml`, named by GameInfo. Missing/unparseable input leaves defaults from [app.h](../../sdk/runtime/app/app.h). Settings, output/render-scale/aspect helpers and ApplyDisplay connect the [overlay](title.md) to [presentation](graphics.md).

Logs and HDD data are host-resolved relative to the executable; multiple instances can choose a distinct HDD suffix. Guest device mappings belong to the [kernel filesystem](kernel.md), and `MountDisc` owns an optional ISO. Developer repository assets and selected runtime assets are distinct concepts.

# Quit, crashes and relaunch

`app::RequestQuit` posts a close message on Windows or quit event on SDL. Title launch/persisted-display handling lives in [boot.cpp](../../sdk/runtime/kernel/boot.cpp); [handover.cpp](../../sdk/runtime/app/handover.cpp) coordinates visible transitions. GameInfo maps this title’s `default.xbe` to `SimsBustinOut`, not arbitrary executables.

Windows supports minidumps. Linux signal handling logs available guest registers/stack, flushes, restores default signal handling and terminates. [Guest traps](kernel.md) are fatal. Startup source inspection does not establish correct shutdown, relaunch or platform failure handling under every execution path.
