---
type: Reference
title: Validation and maintenance
description: Evidence levels, diagnostic tools, checks, and unresolved limits.
sources:
  - resource: ../../tools/kernel_coverage.py
  - resource: ../../sdk/harness/README.md
  - resource: ../../sdk/harness/main_test.cpp
  - resource: ../../docs/status.md
  - resource: ../../docs/playtest.md
  - resource: ../../docs/ultrawide.md
  - resource: ../../sdk/runtime/core/diag.h
---

# Evidence policy

This bundle describes checked-out source inspected on 2026-11-01. Source confirms control flow, not successful gameplay. Existing [status](../status.md), [playtest](../playtest.md), [black-scene](../black-scene.md) and [frame-rate](../frame-rate.md) reports retain historical run evidence. Ignored out/ logs/screenshots may be absent in fresh clones; revision-specific observations are not current guarantees.

No build, GUI playtest, physical controller, save/reload or multiplayer test was performed for this documentation change. No verified metadata is asserted. Sources identify inspected artifacts; source links and symbols locate implementation details. Git records change history without fabricated timestamps, confidence scores or freshness deadlines.

# Available checks

| Question | Tool | Limit |
| --- | --- | --- |
| XBE/header/import identity? | [xbe_info.py](../../tools/xbe_info.py) | Structural analysis, not guest correctness |
| Imported ordinals registered? | `python tools/kernel_coverage.py assets/default.xbe` | Registration coverage, not API fidelity |
| Disc readable? | [xiso_extract.py](../../tools/xiso_extract.py) info/extract | Not proof of runtime data access |
| Analysis/code generation? | Recompiler diagnostics/stats | Counts do not prove indirect paths |
| Headless guest progress? | [Harness](../../sdk/harness/README.md) | Stand-ins differ from native devices |
| Correct visible play? | Platform build, screenshots/logs, deliberate input | Must record revision/settings/path |

The harness combines generated code with app_headless, scripted pad, null audio/backend and optional CPU reference rendering. [`main_test.cpp`](../../sdk/harness/main_test.cpp) boots then loops flushing logs; it is not a pass/fail unit suite. run.sh supplies duration, scripts and multiple instances. Snapshots/draw lists help localize state, but do not validate native GPU output or audible playback. Root CMake defines no CTest suite.

# Diagnostic routes

- Use logs and XBE_PERF=1 to separate dispatch stalls, draw failures and pacing.
- F3 requests guest stack/frame capture; XBE_CAPTURE_DIR selects output. Vulkan supports XBE_VK_CAPTURE_EVERY and XBE_VK_DEBUG_DRAW. Check [diag.h](../../sdk/runtime/core/diag.h) and backend for precise variable semantics.
- Use XBE_STATE_VERIFY for missing dirty marks, or XBE_NO_STATE_DIRTY for full comparisons. Respect [graphics lifetimes](graphics.md).
- Compare timing with SIMSBO_NO_STEP=1 at controlled caps; inspect both [timing overrides](title.md).
- Existing reports warn of machine freezes with Vulkan validation plus periodic capture on their Linux environment. This task did not reproduce or diagnose that failure.

# Checks performed

The local kernel coverage check returned **160 imports, 160 registered, 0 missing; 275 SDK exports**. xbe_info also confirmed the local input's 3,211,264-byte size, SHA-1 `6feea04a12fb134253abe2b5662da634d0048232`, title `0x45410043`, entry `0x100EA2` and XDK 5558. The documentation [validator](validate.py) checks YAML frontmatter, version/type fields, provenance targets, relative Markdown file links, root index coverage and concept reachability. Repository source links are dependencies outside the knowledge bundle, not OKF concepts. Run `python docs/okf/validate.py` with PyYAML available as a documentation-tool dependency. No application dependency is added.

All ten Markdown documents and 200 local references passed the structural check at creation, including the nine indexed concepts and their provenance/file links. The check used an already cached PyYAML package because the default Python environment lacked it. This does not certify semantic correctness or remote URL availability. No application tests were needed for documentation-only changes.

# Limits and maintenance

Historical reports leave physical controller use, visible save/reload, most cheats and remaining frame-rate consumers unproven. Experimental Linux Hor+ has unresolved visual issues and remains gated. Older Windows reports mix never-compiled language with later cross-build/Wine evidence; native Windows play is not established by those statements. SDK networking is not title multiplayer validation.

Update affected concepts and sources when changing implementation, then review neighbours through the [index](index.md). GPU State changes require checking ranges, dirty marks and both backends; hook/config changes require ABI/stack/binding review; discovery/packaging changes require extracted and ISO cases. Record new execution results with revision, platform, settings and reproducible evidence. Keep uncertainty explicit until evidence resolves it.
