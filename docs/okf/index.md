---
okf_version: "0.2"
---

# Bustin’ Out project knowledge

Start with project architecture, then follow the area relevant to the change. Concept IDs are file paths without `.md`. Source links leave this bundle and resolve in the repository; initialize the SDK submodule to follow them.

## Architecture and implementation

- [Project architecture](architecture.md) - Static recompilation boundaries, ownership, and runtime execution.
- [Recompilation and guest ABI](recompilation.md) - Analysis, generated code, dispatch, and override conventions.
- [Host lifecycle and storage](host.md) - Platform startup, discovery, boot, settings, and relaunch.
- [Kernel services](kernel.md) - Memory, threads, objects, files, and failure handling.
- [Graphics pipeline](graphics.md) - State translation, asynchronous rendering, shaders, and presentation.
- [Audio and peripherals](services.md) - DirectSound mixing, controller reports, and SDK networking.
- [Title integration](title.md) - Timing, callbacks, cheats, keyboard input, and overlay.

## Development and evidence

- [Build and distribution](development.md) - Build routes, dependencies, and packaging.
- [Validation and maintenance](validation.md) - Evidence levels, diagnostic tools, checks, and unresolved limits.

This bundle follows the [official OKF v0.2 specification](https://github.com/GoogleCloudPlatform/open-knowledge-format/blob/main/SPEC.md). It describes source inspected on 2026-11-01. Historical gameplay claims remain attributed to their reports. No `verified` events are asserted: source inspection and structural checks do not establish complete runtime correctness.
