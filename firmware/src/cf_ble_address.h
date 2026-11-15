/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
/* Same flash -> same static-random address across boots and firmware updates.
 * Do not advertise a shared fallback identity if this chip has no readable UID. */
static int cf_ble_address_from_uid(const uint8_t uid[16], uint8_t addr[6])
{
    uint32_t h = 2166136261u, g = 0x9E3779B9u, i, different = 0;
    for (i = 0; i < 16u; i++) {
        different |= uid[i] ^ uid[0];
        h = (h ^ uid[i]) * 16777619u;
        g = (g ^ uid[i]) * 2246822519u;
    }
    if (!different) return 0;
    h ^= h >> 16;
    addr[0] = (uint8_t)((h & 0xFEu) | 2u);
    addr[1] = (uint8_t)(h >> 8);
    addr[2] = (uint8_t)g;
    addr[3] = (uint8_t)(g >> 8);
    addr[4] = (uint8_t)(g >> 16);
    addr[5] = (uint8_t)((g >> 24) | 0xC0u);
    return 1;
}
