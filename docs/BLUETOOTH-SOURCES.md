# Bluetooth source provenance

`firmware/ble/`: charlesvestal/fm1-base at c8e05997ce970875038156d2bbd69e712eac1543, bt/ble/.
`firmware/hal/radio/`: hugelton/felucca-bsp at 18eadadb8c364abd026accd2b1ea1c14ca347f06, chip/.
ble_rf_tables_v15.h is renamed ble_rf_tables.h. Original notices are retained.
The GPL-3.0-only OR MPL-2.0 sources are used under GPL-3.0 here.
The stack originates in Henri David's Optimist. No JieLi Bluetooth library is linked.
Radio assumptions require tests on the actual FM-1. Compilation does not prove radio operation.

`tests/ble_midi_test.c` is copied from the same fm1-base commit (tests/bt/),
with its include path adjusted. `fm1_ble_uid.h` adapts the BSP read-only UID routine.
