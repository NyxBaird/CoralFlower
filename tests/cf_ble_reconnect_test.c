/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#define FELUCCA_BLE_STUB 1
#define BLE_DIAG 0
#define BLE_DEVICE_NAME "CoralFlower"
#include "../firmware/ble/ble_stack.c"
BLE_API int ble_app_midi_peek(uint32_t *pkt, uint32_t *t) { (void)pkt;(void)t;return 0; }
BLE_API void ble_app_midi_pop(void) {}
BLE_API void ble_app_midi_in(uint32_t p,uint16_t t,uint16_t l) { (void)p;(void)t;(void)l; }
BLE_API void ble_app_state(void) {}
int main(void) {
    const uint8_t addr[6]={2,3,4,5,6,0xC7};
    ble_init(addr,1); ble_enable(1); assert(bll.state==LL_ADV);
    /* Drive the real link-layer's close path with a simulated established link. */
    bll.state=LL_CONN; ble_host_connected();
    ll_close_by(BLE_ERR_CONN_TIMEOUT, BDC_SUPERVISION);
    assert(bll.state==LL_ADV && bll.enabled);
    bll.state=LL_CONN; ble_host_connected();
    ll_close_by(BLE_ERR_REMOTE_USER, BDC_PEER);
    assert(bll.state==LL_ADV && bll.enabled);
    ble_enable(0); assert(bll.state==LL_OFF && !bll.enabled);
    puts("BLE reconnect: boot advertising, timeout/peer-loss advertising, explicit stop passed");
}
