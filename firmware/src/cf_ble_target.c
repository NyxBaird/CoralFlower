/* SPDX-License-Identifier: GPL-3.0-only */
/* Main-loop target selection; radio mutations hold only the radio IRQs. */
static struct ble_scan_tab cb_scan __attribute__((section(".pool")));
static uint8_t cb_target = 255, cb_switching;
static uint32_t cb_switch_ms, cb_attempt_ms, cb_age_ms;
static int cb_op1_name(const char *s)
{
    return s[0] == 'O' && s[1] == 'P' && s[2] == '-' && s[3] == '1' &&
           (s[4] == 0 || s[4] == ' ');
}
static const char *cf_ble_status(void)
{
    if (!cb_started) return "Unavailable";
    if (cb_switching) return "Switching";
    if (cb_target == 2u) return "Off";
    if (cb_ready) return "Connected";
    if (cb_target == 0u) return "Waiting";
    if (ble_central_fail() == BLE_CF_NEED_PAIR) return "Needs pairing";
    if (ble_central_state() != BLE_CS_IDLE) return "Connecting";
    return "Searching";
}
static void cf_ble_target_poll(uint8_t target)
{
    uint32_t now = fm1_ms, i;
    uint8_t pdu[39], n;
    uint16_t rssi;
    if (!cb_started) return;
    if (target > 2u) target = 0;
    if (target == cb_target && cb_switching) cb_switching = 0;
    if (target != cb_target) {
        if (!cb_switching) {
            cb_switching = 1; cb_switch_ms = now;
            /* Send pedal-off and all-notes-off before leaving the old instrument. */
            fm1_ble_irqs_hold(1);
            if (cb_ready) { cb_panic_pos = 0; cb_panic = 1; }
            fm1_ble_irqs_hold(0);
        }
        if (cb_ready && now - cb_switch_ms < 250u) return;
        fm1_ble_irqs_hold(1);
        ble_enable(0);
        if (ble_connected()) ll_close(BLE_ERR_LOCAL_HOST);
        ble_ll_scan(target == 1u);
        cb_ready = 0; cb_lost = 1; cb_tr = cb_tw;
        cb_panic = cb_panic_pos = 0;
        ble_scan_clear(&cb_scan);
        cb_target = target; cb_switching = 0;
        cb_attempt_ms = cb_age_ms = now;
        if (target != 2u) ble_enable(1);
        fm1_ble_irqs_hold(0);
    }
    if (target != 1u) return;
    if (now - cb_age_ms >= 1000u) { ble_scan_age(&cb_scan, now); cb_age_ms = now; }
    /* A bounded batch: unrelated nearby advertisers cannot starve the UI. */
    for (i = 0; i < BLE_SCAN_RING && (n = ble_ll_scan_take(pdu, &rssi)) != 0; i++)
        ble_scan_add(&cb_scan, pdu, n, rssi, now);
    if (cb_ready) { cb_attempt_ms = now; return; }
    if (ble_central_state() != BLE_CS_IDLE) {
        if (now - cb_attempt_ms > 12000u) {
            fm1_ble_irqs_hold(1); ble_central_cancel(); fm1_ble_irqs_hold(0);
            cb_attempt_ms = now;
        }
        return;
    }
    if (now - cb_attempt_ms < 1500u) return;
    for (i = 0; i < BLE_SCAN_N; i++) {
        const struct ble_found *f = &cb_scan.e[i];
        if (f->used && f->midi && cb_op1_name(f->name) && now - f->seen_ms < 3000u) {
            struct ble_peer peer;
            ble_zero((uint8_t *)&peer, sizeof peer);
            ble_cpy(peer.addr, f->addr, 6); peer.addr_rand = f->addr_rand;
            fm1_ble_irqs_hold(1);
            ble_central_connect(&peer);
            fm1_ble_irqs_hold(0);
            cb_attempt_ms = now;
            break;
        }
    }
}
