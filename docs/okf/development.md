---
type: Playbook
title: Build and distribution
description: Build routes, dependencies, and packaging.
sources:
  - resource: ../../CMakeLists.txt
  - resource: ../../CMakePresets.json
  - resource: ../../sdk/cmake/XbeRecomp.cmake
  - resource: ../../build.sh
  - resource: ../../build.bat
  - resource: ../../release.sh
  - resource: ../../release.bat
  - resource: ../../cmake/windows-cross.cmake
---

# Inputs and dependencies

From repository root, initialize the SDK with `git submodule update --init --recursive`. Supply full game data in assets; the build requires `assets/default.xbe` even when runtime mounts an ISO. Inspect input using `python tools/xbe_info.py assets/default.xbe`. Extraction is documented in [README](../../README.md) and [iso-extract.md](../iso-extract.md). Never commit game inputs.

Root [CMake](../../CMakeLists.txt) requires 3.25 and C++20. Presets use Ninja, clang/clang++ and x86-64-v2. Prefer the initialized SDK: fallback tries `../XboxXBERecomp`, then fetches `TGP482/XboxXBERecomp` at main, which is not a reproducible replacement for the recorded submodule revision.

[`XbeRecomp.cmake`](../../sdk/cmake/XbeRecomp.cmake) fetches Zydis v4.1.1, toml++ v3.4.0, Dear ImGui v1.91.9b and SDL3 release-3.4.16 (static). Linux requires Vulkan, zlib and an HLSL-to-SPIR-V path: DXC headers plus runtime libdxcompiler.so, or detected glslang/SPIRV headers/libraries. XBE_SHADER_BACKEND=dxc|glslang can require one at configure time. CMake comments explicitly justify the ImGui pin through SDL3 API compatibility.

# Build routes

| Target | Commands | Output |
| --- | --- | --- |
| Windows native | `build.bat` | out/build/release/SimsBustinOut.exe |
| Linux native | `./build.sh` or `./build.sh linux-debug` | out/build/preset/SimsBustinOut |
| Linux-hosted Windows | Configure/build host-tools, then windows-cross | out/build/windows-cross/SimsBustinOut.exe |

Windows uses Visual Studio’s C++ environment and may offer missing tool installation; [env.bat](../../tools/env.bat) is an additional local helper. Linux checks clang, clang++, cmake and ninja, configures, then reuses an existing game.toml; only an absent config triggers xbe_symbols. Windows checks for a symbols section. Do not delete manually corrected config as a routine rebuild step.

Explicit Linux commands are `cmake --preset linux-release` then `cmake --build --preset linux-release`. Cross compilation needs native host xbe_recomp because it executes during the build. The cross preset points XBE_HOST_TOOLS_DIR at host-tools output. Inspect [windows-cross.cmake](../../cmake/windows-cross.cmake) and [toolchain.md](../toolchain.md) for clang-cl/lld-link and local xwin paths.

# Build graph

add_xbe_image reads 64 partitions here, generates bodies/mappings/headers through xbe_recomp and links generated sources, SDK, common code and title overrides. The codegen stamp depends on the XBE, recompiler and config/XDK TOMLs. CMake embeds input SHA-1 for [discovery](host.md). Generated code uses optimization/disabled strict aliasing; linker selection supports weak overrides. Editing an override does not require manually editing a generated body.

# Distribution

[`release.sh`](../../release.sh) produces SimsBustinOut-linux-x64.tar.zst, superseding older README tar.gz text. It reconfigures/relinks linux-release with $ORIGIN/lib and old DT_RPATH so copied glslang/SPIRV dependencies resolve transitively. Staging contains binary, selected libraries, README, notices and an empty assets folder with instructions. Vulkan loader/driver remain system dependencies.

The optional build-directory argument changes the copied binary, while configure/build still targets linux-release; review this when packaging custom output. The script also packages an existing cross-built Windows exe into SimsBustinOut-win64.zip with redistributable instructions. [release.bat](../../release.bat) is a separate Windows packaging route. Review archives and exclude all game/generated data. No packaging script was run for this task.

Settings/logs/saves use host-resolved executable-relative paths. Confirm [assets discovery](host.md) instead of assuming working directory alone determines runtime layout. Historical platform claims are [evidence reports](validation.md), not new execution tests.
