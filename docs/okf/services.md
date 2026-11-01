---
type: Architecture
title: Audio and peripherals
description: DirectSound mixing, controller reports, and SDK networking.
sources:
  - resource: ../../sdk/runtime/xdk/dsound.cpp
  - resource: ../../sdk/runtime/audio/audio.h
  - resource: ../../sdk/runtime/audio/sdl_audio.cpp
  - resource: ../../sdk/runtime/audio/xaudio2.cpp
  - resource: ../../sdk/runtime/core/input.cpp
  - resource: ../../sdk/runtime/xdk/xapi.cpp
  - resource: ../../sdk/runtime/xdk/xnet.cpp
  - resource: ../../config/game.toml
---

# Audio flow

[`dsound.cpp`](../../sdk/runtime/xdk/dsound.cpp) owns Voice records and stream Packet queues behind DirectSound replacements. Voices track playback, format/rate, position, loops, mix parameters and packets. The shared Render mixer clears stereo float output, iterates active voices under a recursive mutex, decodes samples including Xbox ADPCM, interpolates at the voice-to-output rate, applies channel gains and clamps the sum.

[`audio::StartOutput`](../../sdk/runtime/audio/audio.h) accepts a pull callback for fixed 48 kHz output. [SDL audio](../../sdk/runtime/audio/sdl_audio.cpp) and [XAudio2](../../sdk/runtime/audio/xaudio2.cpp) implement device output. Completed stream work is queued as Done records. DirectSoundDoWork and a fallback host thread drain completions and enter guest callbacks through CallGuest after releasing the mixer lock. Preserve status/event/callback ordering; do not call arbitrary guest code directly from the audio device callback.

Several HLE operations return success without full DSP behaviour, including effect/deferred-setting functions visible in dsound.cpp. Historical audible intro playback does not prove DirectSound fidelity. [bink.cpp](../../sdk/runtime/xdk/bink.cpp) is SDK capability, but this title’s Bink symbols are zero; not every SDK media replacement is bound for this game.

# Controller reports

[`core/input.cpp`](../../sdk/runtime/core/input.cpp) manages SDL controllers and returns Xbox-shaped Pad reports. Physical state is collected under its lock, then GameInfo::pad runs outside that lock. [`xapi.cpp`](../../sdk/runtime/xdk/xapi.cpp) exposes input services to guest code. Root [PadHook](../../src/common/game_info.cpp) layers scripts and keyboard/mouse on these reports; [title integration](title.md) explains focus and overlay ownership.

Device lifetimes, rumble and physical polling belong to the SDK. Scripts and keyboard state need independent synchronization because multiple guest threads can poll. Historical playtests did not use a physical controller; the implemented path is not evidence of tested device behaviour.

# Networking scope

[`xnet.cpp`](../../sdk/runtime/xdk/xnet.cpp) supplies host sockets/system-link facilities, loopback instance addressing and join-IP routing. Windows uses Winsock; Linux uses [winsock_compat.h](../../sdk/runtime/xdk/winsock_compat.h). Comments describe intentionally limited initialization, not a full Xbox Live service implementation.

The title [config](../../config/game.toml) leaves socket/WSA/XNet/XOnline function symbols at zero. SDK support and the join_ip setting must not be presented as proven Bustin’ Out multiplayer. Check generated bindings and actual calls before extending it. [gap-analysis.md](../gap-analysis.md) records historical absent-symbol classification. Peripheral services share [kernel memory/callback contracts](kernel.md).
