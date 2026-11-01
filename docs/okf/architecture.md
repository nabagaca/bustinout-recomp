---
type: Architecture
title: Project architecture
description: Static recompilation boundaries, ownership, and runtime execution.
sources:
  - resource: ../../CMakeLists.txt
  - resource: ../../.gitmodules
  - resource: ../../config/game.toml
  - resource: ../../sdk/runtime/kernel/boot.cpp
  - resource: ../../src/common/game_info.cpp
---

# Scope and boundaries

The project builds a native host for the USA Xbox release of The Sims: Bustin’ Out. It translates an externally supplied XBE to C++ at build time and replaces selected Xbox library and kernel services. The game simulation remains in generated guest functions; host code supplies services and selected fixes. This is not a general CPU interpreter or a reconstruction of original game source.

[game.toml](../../config/game.toml) selects title `0x45410043`, entry `0x100EA2`, XDK 5558, six code sections and 64 output partitions. The historical expected SHA-1 is `6feea04a12fb134253abe2b5662da634d0048232` in the [README](../../README.md); CMake embeds the hash of the actual build input rather than hardcoding that digest.

# Component map

| Owner | Responsibility | Knowledge |
| --- | --- | --- |
| Root CMake and `config/` | Title identity, symbols, ABI maps, discovery corrections | [Build](development.md), [recompilation](recompilation.md) |
| `src/game/` | Strong guest replacements and title-specific HLE | [Title integration](title.md) |
| `src/common/` | GameInfo, overlay, keyboard/mouse bridge | [Host](host.md), [title](title.md) |
| `sdk/tools/` | Signature scanning, analysis, C++ generation | [Recompilation](recompilation.md) |
| `sdk/runtime/` | CPU context, kernel, XDK services, graphics/audio, platform host | [Kernel](kernel.md), [graphics](graphics.md), [services](services.md) |
| `sdk/harness/`, `tools/`, `docs/` | Headless execution, artifact analysis, historical evidence | [Validation](validation.md) |

The SDK is a separate Git submodule. At inspection it was at `1752be33779c7ea584858bad67de272ae51d227b`; [.gitmodules](../../.gitmodules) names `nabagaca/XboxXBERecomp` and branch `codex/display-and-quit-fixes`. README SDK revision references describe older history. Check `git submodule status` and source before relying on them. Root CMake also has sibling-checkout and network-fetch fallbacks, which can select another implementation.

# Execution path

1. `xbe_symbols` can bootstrap symbol TOML from the XBE and XDK signatures. The committed configuration includes manual corrections and is normally reused.
2. CMake runs `xbe_recomp`, emitting function bodies, mappings and symbol/ABI headers under ignored `generated/game/`.
3. Generated code, title overrides, common integration, runtime and platform implementations link into one executable.
4. The platform host loads settings, finds data, creates its window and calls `kernel::Boot`.
5. Boot maps the image, resolves imports, mounts devices and starts the guest entry thread. Native functions operate on an explicit guest context and memory base.
6. Replaced XDK calls enter host services. Graphics commands go through a render-thread wrapper to Vulkan or D3D12 while the host event loop continues independently.

# Constraints and supported rationale

Keep generated output disposable: override weak symbols or correct analysis inputs. This mechanism is explicit in [code generation](../../sdk/tools/recomp/codegen_emit.cpp) and root CMake’s separation of common sources from title overrides. Platform variants implement shared SDK interfaces and are selected at link time: SDL3/Vulkan/SDL audio on Linux, Win32/D3D12/XAudio2 on Windows. A build for one does not validate the other.

Do not distribute XBEs, ISOs, extracted assets or generated game code. [Ignore rules](../../.gitignore) and [release scripts](development.md) separate user data from distribution. Original simulation details beyond available analysis evidence remain outside this bundle’s confirmed scope.
