---
type: Architecture
title: Graphics pipeline
description: State translation, asynchronous rendering, shaders, and presentation.
sources:
  - resource: ../../sdk/runtime/xdk/d3d8.cpp
  - resource: ../../sdk/runtime/gpu/state.h
  - resource: ../../sdk/runtime/gpu/backend.h
  - resource: ../../sdk/runtime/gpu/render_thread.cpp
  - resource: ../../sdk/runtime/gpu/vulkan.cpp
  - resource: ../../sdk/runtime/gpu/d3d12.cpp
  - resource: ../../sdk/runtime/app/settings.cpp
---

# Translation and state

[`xdk/d3d8.cpp`](../../sdk/runtime/xdk/d3d8.cpp) replaces XDK D3D8 entry points through generated symbol/layout bindings. It decodes guest resources/state and drives [`gpu::Backend`](../../sdk/runtime/gpu/backend.h). [`State`](../../sdk/runtime/gpu/state.h) holds translated render state, textures/streams, shaders, constants, targets and transforms. HLE changes must update state and dirty tracking together.

[vertex.cpp](../../sdk/runtime/gpu/vertex.cpp), [texture.cpp](../../sdk/runtime/gpu/texture.cpp) and [shaders.cpp](../../sdk/runtime/gpu/shaders.cpp) handle vertex declarations/conversion, texture conversion and shader translation. Native resources, pipelines, uploads/readbacks and presentation belong to [D3D12](../../sdk/runtime/gpu/d3d12.cpp) or [Vulkan](../../sdk/runtime/gpu/vulkan.cpp). Linux uses [spirv.cpp](../../sdk/runtime/gpu/spirv.cpp) for HLSL-to-SPIR-V compilation; configure-time detection and runtime compiler availability are separate [requirements](development.md).

# Asynchronous lifetime

[`CreateThreadedBackend`](../../sdk/runtime/gpu/render_thread.cpp) wraps a concrete backend with a worker and command batches. `Cmd` identifies operations/copied data; `Batch` owns bytes and shared shader references. API state, resource headers, indices and rectangles are captured at record time. Vertex/texture/surface payloads are read when draws execute, rather than fully snapshotted per call.

State is sent in deltas. Explicit `StateRanges` skip pointers/separately managed headers and select relevant transforms. Pieces align with dirty blocks. `State::dirty` must remain the last member. Adding state requires reviewing ranges, dirty marks and reconstruction; `XBE_STATE_VERIFY` detects changes missed by dirty marks and `XBE_NO_STATE_DIRTY` enables full comparisons.

The wrapper starts a detached worker. Its destructor signals stop and waits up to 500 ms; if the worker has not exited it releases ownership of the inner backend instead of freeing it under active work. This is an explicit shutdown lifetime tradeoff in the implementation, not proof of leak-free teardown.

`Sync` supplies GPU results before CPU access; `SyncLate` permits older completed copies for nonwaiting locks. `DeliverLate` and `RecordLate` separate delivery from ordered copy recording. Fences and resource waits coordinate guest access with queued work. Keep resources alive until consumption finishes: recording is not native GPU completion. The title’s immediate [InsertCallback override](title.md) is a narrower approximation.

# Presentation versus projection

GameInfo declares picture row 16 and height 448 within the authored 640x480 buffer. Cropping this region, rendering at another scale, changing output dimensions and changing camera projection solve different problems. [picture-rect.md](../picture-rect.md), [guest-projection.md](../guest-projection.md) and [widescreen-flag.md](../widescreen-flag.md) retain title analysis.

[`HostAspect`](../../sdk/runtime/app/settings.cpp) on Linux is gated by `SIMSBO_HORPLUS`; enabling the ultrawide setting alone does not activate experimental projection widening. Vulkan `WidensFrame` follows active HostAspect. `PresentSrcAspect` handles finished-picture aspect, while the Linux getter override changes projection only at identified return addresses. The Windows override instead temporarily changes CPU culling; see [hooks.cpp](../../src/game/hooks.cpp).

Ordinary Linux auto-aspect follows the authored widescreen decision and pillarboxes a 16:9 picture on wider output. Older README passages saying Vulkan widening is logging-only predate code now present. Experimental Hor+ remains gated and [ultrawide.md](../ultrawide.md) records unresolved visual issues; code presence does not make it proven normal functionality.

# Debugging

Separate missing draws/state from native pipeline errors, upload lifetime, shader results and presentation. [Diagnostics](validation.md) provide perf logs and captures. [black-scene.md](../black-scene.md) records a historical allocation-ordering investigation; its successful run is not a guarantee across current drivers/backends. Existing reports associate Vulkan validation plus periodic capture with machine freezes on their test environment; use the recorded logging route first there.
