/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#define RING_PUBLISH() do {} while (0)
static uint32_t fm1_ms, received[128], received_n;
static uint8_t midi_in_overflow;
static int midi_enqueue(uint32_t pkt, uint32_t source)
{ assert(source == 3); received[received_n++] = pkt; return 1; }
#include "../firmware/src/cf_ble.c"
int main(void)
{
    uint32_t pkt, ms, i;
    const uint32_t note = 0x643C9009u;
    cf_ble_send(note); assert(!ble_app_midi_peek(&pkt,&ms));
    test_connected = 1; ble_app_state(); fm1_ms = 8193;
    cf_ble_send(note); assert(ble_app_midi_peek(&pkt,&ms));
    assert(pkt == note && ms == 8193); ble_app_midi_pop();
    assert(!ble_app_midi_peek(&pkt,&ms));
    for (i=0; i<CBQ+1; i++) cf_ble_send(note);
    assert(cb_panic);
    for (i=0; i<48; i++) {
        assert(ble_app_midi_peek(&pkt,&ms));
        assert(((pkt>>8)&255) == 0xB0+i/3);
        assert(((pkt>>16)&255) == (i%3==0 ? 64 : i%3==1 ? 120 : 123));
        ble_app_midi_pop();
    }
    assert(!cb_panic && !ble_app_midi_peek(&pkt,&ms));
    cf_ble_send(note); test_connected=0; ble_app_state();
    assert(!ble_app_midi_peek(&pkt,&ms));
    cf_ble_poll_input(); assert(midi_in_overflow); midi_in_overflow=0;
    ble_app_midi_in(0x0022F004u,0,0); /* OTA SysEx must not reach USB parser */
    ble_app_midi_in(0x0000F10Fu,0,0); /* unsupported system common */
    cf_ble_poll_input(); assert(received_n==0);
    ble_app_midi_in(note,0,0); ble_app_midi_in(0xF80F,0,0);
    cf_ble_poll_input(); assert(received_n==2 && received[0]==note && received[1]==0xF80F);
    for(i=0;i<CBQ+1;i++) ble_app_midi_in(note,0,0);
    cf_ble_poll_input(); assert(midi_in_overflow && received_n==2);
    puts("BLE routing: queues, clock, disconnect, overflow panic, SysEx isolation passed");
}
