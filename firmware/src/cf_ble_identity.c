/* SPDX-License-Identifier: GPL-3.0-only */
#include "fm1_ble_uid.h"
#include "cf_ble_address.h"
static int cf_ble_address(uint8_t addr[6])
{
    uint8_t uid[16];
    uint32_t f = irq_save();
    int rc = FL_FAR(cf_flash_uid_ram)(uid);
    irq_restore(f);
    return !rc && cf_ble_address_from_uid(uid, addr);
}
