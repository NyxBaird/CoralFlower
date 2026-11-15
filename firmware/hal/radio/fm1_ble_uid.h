/* SPDX-License-Identifier: GPL-3.0-only */
/* Read-only flash UID command, adapted from felucca-bsp fl_uid_ram.
 * Requires fm1_flash.h, caller's IRQs off, destination in RAM. No erase/write. */
#pragma once
static RAMFN int cf_flash_uid_ram(uint8_t *dst)
{
    fl_saved_t s;
    uint32_t i;
    int rc = fl_enter(&s);
    if (rc) return rc;
    fl_cs(0);
    for (i = 0; i < 5u; i++) fl_tx(i ? 0 : 0x4B);
    for (i = 0; i < 16u; i++) dst[i] = fl_rx();
    fl_cs(1);
    fl_exit(&s);
    return 0;
}
