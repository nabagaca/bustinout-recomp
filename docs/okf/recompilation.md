---
type: Architecture
title: Recompilation and guest ABI
description: Analysis, generated code, dispatch, and override conventions.
sources:
  - resource: ../../sdk/tools/recomp/main.cpp
  - resource: ../../sdk/tools/recomp/analysis.h
  - resource: ../../sdk/tools/recomp/analysis.cpp
  - resource: ../../sdk/tools/recomp/codegen_emit.cpp
  - resource: ../../sdk/runtime/cpu/x86_context.h
  - resource: ../../sdk/runtime/cpu/dispatch.cpp
  - resource: ../../sdk/runtime/xdk/hle.h
  - resource: ../../config/game.toml
---

# Analysis to generated code

[`xbe_recomp` main](../../sdk/tools/recomp/main.cpp) loads configuration and XDK data, parses the XBE, checks configured title/entry when nonzero, runs Analysis, checks overrides and invokes CodeGen. Configuration-relative input/output paths resolve from the parent of `config/`.

[`Analysis::Run`](../../sdk/tools/recomp/analysis.cpp) seeds the image entry and explicit extra functions, walks reachable control flow and adopts direct call targets. Optional pointer seeding and gap filling expand discovery; immediate code addresses in PUSH/MOV are considered callbacks. Exception continuations are discovered separately. Coverage distinguishes unknown bytes, instruction starts/bodies and data. These heuristics explain why embedded data or missed indirect targets require configuration corrections.

[`Config`](../../sdk/tools/recomp/analysis.h) carries function ends, switch targets, data ranges, hooks, symbols, layouts and argument maps. [game.toml](../../config/game.toml) is the effective title input; `found-*.toml` files retain analysis material. CMake watches all config/XDK TOMLs, so a supporting file can trigger regeneration without being the effective symbol input.

# ABI and overrides

[`codegen_emit.cpp`](../../sdk/tools/recomp/codegen_emit.cpp) emits an original `__imp__sub_XXXXXXXX` and weak `sub_XXXXXXXX` alias. A strong `X86_FUNC(sub_XXXXXXXX)` replaces the public function; calling the `__imp__` symbol enters the original. Generated `image_symbols.h` binds HLE names to addresses and ABI maps. Never edit generated output.

The execution signature is `void(X86Context& ctx, uint8_t* base)`. [`X86Context`](../../sdk/runtime/cpu/x86_context.h) holds general registers, flags, FS base, x87/MMX/SSE state and runtime bookkeeping. Guest addresses remain 32-bit offsets in a 64-bit host. Native call frames coexist with the guest stack: a C++ return alone does not implement guest RET. Preserve register results, floating return representation and guest stack cleanup when overriding code.

[`hle.h`](../../sdk/runtime/xdk/hle.h) provides `SYM`, `GUEST`, `GUEST_ORIG`, argument maps and HLE macros. A map can place arguments in registers rather than nominal stack slots. `hle::Call` sets mapped registers, calls through `kernel::CallGuest` and restores saved registers; argument-count mismatch traps. Verify `[abi]` and guest code instead of inferring an ABI from an API name.

# Dispatch and callbacks

[`dispatch::Init`](../../sdk/runtime/cpu/dispatch.cpp) builds a function slot per byte of code range and a continuation bitmap. `x86_icall` handles guest functions, synthetic kernel thunks at `0xFE000000 + ordinal*4`, and registered host thunks from `0xFD000000`. Continuations raise `X86Resume` carrying the guest stack position. Unknown targets log target, return address and ESP and trap.

`kernel::CallGuest` saves general registers, pushes arguments right-to-left plus a sentinel return address, invokes dispatch, captures EAX and restores registers/ESP. This bridges [services](services.md) and [title callbacks](title.md) to guest functions; it does not supply an unknown function’s calling convention.

For a missing target, determine whether the cause is missing code discovery, an incorrect switch, embedded data or corrupted state. Correct the smallest justified config entry, regenerate and inspect diagnostics. Behaviour changes belong in title overrides, with original-body fallback where appropriate. [Validation](validation.md) distinguishes discovery coverage from execution correctness.
