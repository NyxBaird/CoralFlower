/* SPDX-License-Identifier: GPL-3.0-only */
#define BLE_API static
#define BLE_HW_WL82 0
static int test_connected;
static int ble_midi_ready(void) { return test_connected; }
