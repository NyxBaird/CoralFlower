/* SPDX-License-Identifier: GPL-3.0-only */
/* Native BLE peripheral. Radio IRQs only touch their own rings; TIMER5 is
 * still the sole producer of the shared MIDI input ring. No OTA over BLE. */
#define BLE_DEVICE_NAME "CoralFlower"
#define BLE_DIAG 0
#define BLE_CENTRAL 3
#define BLE_SCAN_N 8
#define BLE_SCAN_RING 4
#define BLE_SMP_LEGACY 0
#include "ble_stack.c"

#define CBQ 64u
static struct { uint32_t pkt, ms; } cb_tx[CBQ] __attribute__((section(".pool")));
static uint32_t cb_rx[CBQ] __attribute__((section(".pool")));
static volatile uint32_t cb_tw, cb_tr, cb_rw, cb_rr;
static volatile uint8_t cb_ready, cb_lost, cb_panic;
static uint8_t cb_panic_pos;

static void cf_ble_send(uint32_t pkt) /* audio ISR: never waits for the radio */
{
    if (!cb_ready || cb_panic) return;
    if (cb_tw - cb_tr >= CBQ) {
        cb_panic = 1; /* lost note-off: drain and send pedal-off + panic */
        return;
    }
    cb_tx[cb_tw % CBQ].pkt = pkt;
    cb_tx[cb_tw % CBQ].ms = fm1_ms;
    RING_PUBLISH();
    cb_tw++;
}
BLE_API int ble_app_midi_peek(uint32_t *pkt, uint32_t *ms)
{
    if (cb_panic) {
        cb_tr = cb_tw;
        *pkt = 0x0Bu | (0xB0u | cb_panic_pos / 3u) << 8 |
            (cb_panic_pos % 3u == 0 ? 64u : cb_panic_pos % 3u == 1 ? 120u : 123u) << 16;
        *ms = fm1_ms;
        return 1;
    }
    if (cb_tr == cb_tw) return 0;
    *pkt = cb_tx[cb_tr % CBQ].pkt;
    *ms = cb_tx[cb_tr % CBQ].ms;
    return 1;
}
BLE_API void ble_app_midi_pop(void)
{
    if (cb_panic) {
        if (++cb_panic_pos == 48u) { cb_panic_pos = 0; cb_panic = 0; }
    } else if (cb_tr != cb_tw) cb_tr++;
}
BLE_API void ble_app_midi_in(uint32_t pkt, uint16_t ts, uint16_t last_ts)
{
    uint32_t status = (pkt >> 8) & 255u;
    (void)ts; (void)last_ts;
    /* Musical channel messages and clock/transport only: never SysEx commands. */
    if (!(status >= 0x80u && status < 0xF0u) && status != 0xF8u &&
        status != 0xFAu && status != 0xFBu && status != 0xFCu) return;
    if (cb_rw - cb_rr >= CBQ) { cb_lost = 1; return; }
    cb_rx[cb_rw % CBQ] = pkt;
    RING_PUBLISH();
    cb_rw++;
}
BLE_API void ble_app_state(void)
{
    uint8_t ready = (uint8_t)ble_midi_ready();
    if (!ready) {
        if (cb_ready) cb_lost = 1;
        cb_ready = 0;
        cb_tr = cb_tw;
        cb_panic = cb_panic_pos = 0;
    } else cb_ready = 1;
}
static void cf_ble_poll_input(void) /* TIMER5, alongside USB and TRS */
{
    if (cb_lost) {
        cb_rr = cb_rw;
        midi_in_overflow = 1; /* existing audio-side panic releases held notes */
        cb_lost = 0;
    }
    while (cb_rr != cb_rw) {
        midi_enqueue(cb_rx[cb_rr % CBQ], 3u);
        RING_PUBLISH();
        cb_rr++;
    }
}
#if BLE_HW_WL82
static int cf_ble_address(uint8_t addr[6]);
static uint8_t cb_started;
static int cf_ble_start(void)
{
    static const struct ble_rf_trims trims = {{0x0B, 0x0B}, {1, 7, 4, 7, 11, 1, 7}, {0}, {0}};
    uint8_t addr[6];
    if (!cf_ble_address(addr)) return 0;
    ble_init(addr, 1);
    fm1_guard_unlock_top();
    fm1_ble_irq_attach(BLE_HW_IRQ_PRIO);
    fm1_ble_irqs_hold(1);
    fm1_guard_lock_top();
    fm1_rf_cal_on = 1;
    ble_hw_wl82_start(&trims);
    fm1_ble_irqs_hold(1);
    ble_enable(1);
    fm1_ble_irqs_hold(0);
    cb_started = 1;
    return 1;
}
static void cf_ble_stop(void) /* before staging/jumping to the USB updater */
{
    if (!cb_started) return;
    fm1_ble_irqs_hold(1);
    ble_enable(0);
    ble_hw_conn_stop(); /* stop the DMA even when disconnect was not acknowledged */
    cb_ready = 0;
    cb_lost = 1;
    cb_started = 0;
}
#include "cf_ble_target.c"
#endif
