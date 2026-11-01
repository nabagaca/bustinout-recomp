// Extend the title's own main menu using its hidden Author Mode button (ID 6).
// This is the same native model, font, Flash layout and controller focus path as Credits.
#include <cstdint>
#include <cstring>

#include "app/app.h"
#include "core/log.h"
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "xdk/hle.h"

namespace {

constexpr uint32_t kQuitId = 6;
constexpr uint32_t kVisible = 1;
constexpr uint32_t kNativeHiddenIds[] = {3, 4};
// F79C0 sets this only after the save scan has published the Continue/Free Play flag.
constexpr uint32_t kSaveScanReady = 0x321CD5u;
constexpr uint32_t kHasSavedGame = 0x321CD6u;

// The controller owns a native menu at +0x60. Its child vector includes type-1 buttons;
// DB780 assigns their action IDs at +0x0C, including the dormant Author Mode entry.
uint32_t QuitButton(uint32_t controller) {
    const uint32_t menu = kernel::R32(controller + 0x60u);
    if (!menu) return 0;
    const uint32_t begin = kernel::R32(menu + 0x0Cu);
    const uint32_t end = kernel::R32(menu + 0x10u);
    if (end < begin || end - begin > 32u * 4u) return 0;
    for (uint32_t at = begin; at < end; at += 4u) {
        const uint32_t child = kernel::R32(at);
        if (child && kernel::R32(child + 4u) == 1u && kernel::R32(child + 0x0Cu) == kQuitId)
            return child;
    }
    return 0;
}

void HideNativeButtonsUntilLayoutReady(uint32_t controller) {
    const uint32_t menu = kernel::R32(controller + 0x60u);
    if (!menu) return;
    const uint32_t begin = kernel::R32(menu + 0x0Cu);
    const uint32_t end = kernel::R32(menu + 0x10u);
    if (end < begin || end - begin > 32u * 4u) return;
    for (uint32_t at = begin; at < end; at += 4u) {
        const uint32_t child = kernel::R32(at);
        if (!child || kernel::R32(child + 4u) != 1u) continue;
        const uint32_t id = kernel::R32(child + 0x0Cu);
        for (uint32_t hidden_id : kNativeHiddenIds) {
            if (id == hidden_id) {
                kernel::W32(child + 8u, kernel::R32(child + 8u) & ~kVisible);
                break;
            }
        }
    }
}

uint32_t QuitLabel() {
    // Native text rendering (DC270) reads 16-bit characters. Keep the guest string alive
    // across menu unload/reload, just like the title's localized strings.
    static const uint32_t label = [] {
        constexpr char16_t text[] = u"Quit Game";
        const uint32_t address = mem::AllocSystem(sizeof(text));
        if (address) std::memcpy(kernel::HostPtr(address), text, sizeof(text));
        return address;
    }();
    return label;
}

}  // namespace

// Original main-menu label/style setup. DFAA0 is the native text setter: ECX button,
// one stack argument containing a UTF-16 guest string, retn 4.
X86_FUNC(__imp__sub_000DB780);
X86_FUNC(sub_000DB780) {
    const uint32_t controller = ctx.ecx;
    __imp__sub_000DB780(ctx, base);
    // Fresh native buttons start visible. Apply the menu's normal hidden state for
    // Options and Online Weekend before the first draw can expose them. ID 6 is already
    // relabelled Quit Game below, so leave it visible with its button from the first frame.
    if (!X86_MEM8(kSaveScanReady)) HideNativeButtonsUntilLayoutReady(controller);
    const uint32_t button = QuitButton(controller);
    const uint32_t label = QuitLabel();
    if (!button || !label) return;
    const X86Context saved = ctx;
    hle::Call(ctx, 0xDFAA0u, hle::Make({hle::ecx, hle::stack}), {button, label});
    ctx = saved;
    LOGR("simsbo: native main-menu button 6 labelled Quit Game");
}

// Original visibility/layout refresh. DBA10 calls this before the asynchronous save scan
// finishes; defer that initial layout so the menu first appears with its final big buttons.
// The ordinary refresh/reset path remains responsible for layout, selection and save changes.
X86_FUNC(__imp__sub_000DB850);
X86_FUNC(sub_000DB850) {
    if (!X86_MEM8(kSaveScanReady)) {
        LOG_FIRST(1, "simsbo: deferred native menu layout until save discovery completes");
        ctx.esp += 4u;  // plain ret
        return;
    }
    const uint32_t controller = ctx.ecx;
    const uint32_t old_big = kernel::R32(controller + 0x70u);
    const uint32_t old_small = kernel::R32(controller + 0x74u);
    __imp__sub_000DB850(ctx, base);

    const uint32_t button = QuitButton(controller);
    if (!button) return;
    kernel::W32(button + 8u, kernel::R32(button + 8u) | kVisible);
    if (old_big != kernel::R32(controller + 0x70u) || old_small != kernel::R32(controller + 0x74u))
        LOGR("simsbo: native menu layout ready: %u big buttons, 2 small buttons",
             kernel::R32(controller + 0x70u));
}

// The authored layout queries its button counts through this native string getter.
// Report the additional small button during the original resetMenu, avoiding a second reset.
// Before save discovery finishes, report zero buttons to loadMainMenu's initial queries.
X86_FUNC(__imp__sub_000DB540);
X86_FUNC(sub_000DB540) {
    const uint32_t controller = ctx.ecx;
    const char* name = static_cast<const char*>(kernel::HostPtr(kernel::R32(ctx.esp + 4u)));
    const bool small = std::strcmp(name, "number_small_items") == 0;
    const bool big = std::strcmp(name, "number_big_items") == 0;
    if (!small && !big) {
        __imp__sub_000DB540(ctx, base);
        return;
    }
    const uint32_t field = controller + (small ? 0x74u : 0x70u);
    const uint32_t count = kernel::R32(field);
    const bool ready = X86_MEM8(kSaveScanReady) != 0;
    // Use the published scan result even if Flash queries before the first DB850 refresh.
    // The original visibility switch exposes one big button, or three with a saved game.
    const uint32_t final_count = small ? 2u : (X86_MEM8(kHasSavedGame) ? 3u : 1u);
    kernel::W32(field, ready ? final_count : 0u);
    __imp__sub_000DB540(ctx, base);
    kernel::W32(field, count);
}

// The native A/Select path calls this handler with (button ID, state machine), retn 8.
// AL=1 keeps the caller's ordinary successful-selection sound and menu cleanup path.
X86_FUNC(__imp__sub_000DB1D0);
X86_FUNC(sub_000DB1D0) {
    if (kernel::R32(ctx.esp + 4u) != kQuitId) {
        __imp__sub_000DB1D0(ctx, base);
        return;
    }
    LOGR("simsbo: native Quit Game selected");
    ctx.eax = (ctx.eax & 0xFFFFFF00u) | 1u;
    ctx.esp += 12u;
    app::RequestQuit();
}
