---
type: Architecture
title: Kernel services
description: Memory, threads, objects, files, and failure handling.
sources:
  - resource: ../../sdk/runtime/kernel/memory.h
  - resource: ../../sdk/runtime/kernel/memory.cpp
  - resource: ../../sdk/runtime/kernel/threads.cpp
  - resource: ../../sdk/runtime/kernel/kernel.h
  - resource: ../../sdk/runtime/kernel/objects.cpp
  - resource: ../../sdk/runtime/kernel/files.cpp
  - resource: ../../sdk/runtime/kernel/iocp.cpp
  - resource: ../../sdk/runtime/cpu/dispatch.cpp
---

# Address space

[`memory.h`](../../sdk/runtime/kernel/memory.h) defines 4 KiB pages, user VA `0x00010000..0x7FFE0000`, 64 MiB physical memory exposed at contiguous VA base `0x80000000`, write-combine alias base `0xF0000000`, system allocations at `0xB0000000` and stacks at `0xD0000000`. The first physical MiB holds kernel data. [`memory.cpp`](../../sdk/runtime/kernel/memory.cpp) supplies platform mappings and tracks virtual, contiguous, system and stack allocations.

`kernel::g_base` supplies guest-address translation; [kernel.h](../../sdk/runtime/kernel/kernel.h) provides guest-sized reads/writes. Reserved does not mean committed/readable. Inspecting uncertain pointers requires range, overflow and page checks; [cheats.cpp](../../src/game/cheats.cpp) demonstrates this. Do not store native pointers in guest structures. Match allocation/free families and preserve physical/virtual alias semantics used by graphics.

# Threads and objects

[`threads.cpp`](../../sdk/runtime/kernel/threads.cpp) implements guest execution through host threads, thread-local current ThreadRef and per-thread X86Context. Setup allocates guest stack/TLS, KPCR and ETHREAD blocks and fills guest-visible fields. FS accesses use `ctx.fs_base`; context initialization sets x87/SSE defaults. Native stack capacity matters because recompiled calls nest host frames as well as maintaining guest stack state.

`g_threads_lock` protects the registry. Exiting threads signal their guest object under `DispatchLock`. [kernel.h](../../sdk/runtime/kernel/kernel.h) and [objects.cpp](../../sdk/runtime/kernel/objects.cpp) define waitable objects and handle management. Shared references make object lifetime distinct from guest handle values. Follow the dispatcher locking/notification protocol when modifying waits, events or semaphores.

# Files and completion

[`files.cpp`](../../sdk/runtime/kernel/files.cpp) maps normalized Xbox links/devices to host directories and optional [Disc](../../sdk/runtime/kernel/disc.h). Lookup performs case-insensitive host matching. A File owns a host FILE or disc-entry reference, position, enumeration state and completion-port association. Its I/O mutex keeps seek-plus-transfer together for shared handles; destruction closes streams and applies delete-on-close.

Completion ports and I/O notification structures span files.cpp and [iocp.cpp](../../sdk/runtime/kernel/iocp.cpp). Preserve guest NTSTATUS and notification contracts: successful host reads alone do not establish asynchronous completion correctness. [disc.cpp](../../sdk/runtime/kernel/disc.cpp) parses/accesses the disc filesystem separately from [app discovery](host.md).

# Imports and failures

Registration macros in [exports.h](../../sdk/runtime/kernel/exports.h) populate dispatch exports. Boot patches imports to data or synthetic function thunks. Missing exports warn during boot and trap when called. [`x86_trap`](../../sdk/runtime/cpu/dispatch.cpp) logs guest context, flushes and aborts, with a debugger break first on Windows. Registered services can still return errors or approximations.

The [coverage check](validation.md) finds all 160 imports registered among 275 exports. This is registration coverage, not proof of API fidelity. [Graphics](graphics.md) and [audio](services.md) share guest memory and callbacks with kernel services; lifetime, synchronization and callback thread selection must be considered together.
