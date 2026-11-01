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
    const uint32_t button = QuitButton(controller);
    const uint32_t label = QuitLabel();
    if (!button || !label) return;
    const X86Context saved = ctx;
    hle::Call(ctx, 0xDFAA0u, hle::Make({hle::ecx, hle::stack}), {button, label});
    ctx = saved;
    LOGR("simsbo: native main-menu button 6 labelled Quit Game");
}

// Original visibility/layout refresh. Vanilla counts only Credits as a small button and
// hides Author Mode every frame. Let it maintain all save-dependent buttons, then expose
// button 6 and give the authored Flash layout two small buttons via its normal resetMenu.
X86_FUNC(__imp__sub_000DB850);
X86_FUNC(sub_000DB850) {
    const uint32_t controller = ctx.ecx;
    const uint32_t old_big = kernel::R32(controller + 0x70u);
    const uint32_t old_small = kernel::R32(controller + 0x74u);
    if (old_small == 2u) kernel::W32(controller + 0x74u, 1u);
    __imp__sub_000DB850(ctx, base);

    const uint32_t button = QuitButton(controller);
    if (!button) return;
    kernel::W32(button + 8u, kernel::R32(button + 8u) | kVisible);
    kernel::W32(controller + 0x74u, 2u);
    if (old_small == 2u && old_big == kernel::R32(controller + 0x70u)) return;

    const X86Context saved = ctx;
    // FD140 is cdecl; resetMenu queries DB540's number_small_items and emits the
    // button5_x/y callbacks which DB310 maps to native button ID 6.
    kernel::CallGuest(ctx, 0xFD140u, {kernel::R32(0x310C28u), 0x2E55B8u, 0u, 0u, 0u});
    ctx = saved;
    LOGR("simsbo: native main-menu layout includes Credits and Quit Game");
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
