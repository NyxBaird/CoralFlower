/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/src/cf_ble_address.h"
int main(void) {
    uint8_t uid[16]={0}, a[6], b[6], c[6];
    assert(!cf_ble_address_from_uid(uid,a));
    memset(uid,255,sizeof uid); assert(!cf_ble_address_from_uid(uid,a));
    for(unsigned i=0;i<16;i++) uid[i]=(uint8_t)i;
    assert(cf_ble_address_from_uid(uid,a));
    assert(cf_ble_address_from_uid(uid,b)); assert(!memcmp(a,b,6));
    uid[8]^=0x80; assert(cf_ble_address_from_uid(uid,c)); assert(memcmp(a,c,6));
    assert((a[5]&0xC0)==0xC0 && (a[0]&3)==2);
    puts("BLE identity: stable, device-specific, static-random, invalid UID refused");
}
