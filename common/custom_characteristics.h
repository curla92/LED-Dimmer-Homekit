#ifndef __HOMEKIT_CUSTOM_CHARACTERISTICS__
#define __HOMEKIT_CUSTOM_CHARACTERISTICS__

#define HOMEKIT_CUSTOM_UUID(value) (value "-03a1-4971-92bf-af2b7d833922")

#define HOMEKIT_SERVICE_CUSTOM_SETUP HOMEKIT_CUSTOM_UUID("F00000FF")

#define HOMEKIT_CHARACTERISTIC_CUSTOM_SHOW_SETUP HOMEKIT_CUSTOM_UUID("F0000106")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_SHOW_SETUP(_value, ...) \
.type = HOMEKIT_CHARACTERISTIC_CUSTOM_SHOW_SETUP, \
.description = "Show Setup", \
.format = homekit_format_bool, \
.permissions = homekit_permissions_paired_read \
| homekit_permissions_paired_write \
| homekit_permissions_notify, \
.value = HOMEKIT_BOOL_(_value), \
##__VA_ARGS__

#define HOMEKIT_CHARACTERISTIC_CUSTOM_DEVICE_TYPE_NAME HOMEKIT_CUSTOM_UUID("F0000107")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_DEVICE_TYPE_NAME(_value, ...) \
.type = HOMEKIT_CHARACTERISTIC_CUSTOM_DEVICE_TYPE_NAME, \
.description = "1) Dev Type", \
.format = homekit_format_string, \
.permissions = homekit_permissions_paired_read, \
.value = HOMEKIT_STRING_(_value), \
##__VA_ARGS__

#define HOMEKIT_CHARACTERISTIC_CUSTOM_LED_GPIO HOMEKIT_CUSTOM_UUID("F0000108")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_LED_GPIO(_value, ...) \
.type = HOMEKIT_CHARACTERISTIC_CUSTOM_LED_GPIO, \
.description = "2) Led GPIO", \
.format = homekit_format_uint8, \
.permissions = homekit_permissions_paired_read \
| homekit_permissions_paired_write \
| homekit_permissions_notify, \
.min_value = (float[]) {0}, \
.max_value = (float[]) {16}, \
.min_step = (float[]) {1}, \
.value = HOMEKIT_UINT8_(_value), \
##__VA_ARGS__

#define HOMEKIT_CHARACTERISTIC_CUSTOM_LED_INVERTED HOMEKIT_CUSTOM_UUID("F0000109")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_LED_INVERTED(_value, ...) \
.type = HOMEKIT_CHARACTERISTIC_CUSTOM_LED_INVERTED, \
.description = "3) Led Inverted", \
.format = homekit_format_uint8, \
.permissions = homekit_permissions_paired_read \
| homekit_permissions_paired_write \
| homekit_permissions_notify, \
.min_value = (float[]) {0}, \
.max_value = (float[]) {1}, \
.min_step = (float[]) {1}, \
.valid_values = { \
.count = 2, \
.values = (uint8_t[]) { 0, 1 }, \
}, \
.value = HOMEKIT_UINT8_(_value), \
##__VA_ARGS__

#define HOMEKIT_CHARACTERISTIC_CUSTOM_INIT_STATE_LED HOMEKIT_CUSTOM_UUID("F0000110")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_INIT_STATE_LED(_value, ...) \
.type = HOMEKIT_CHARACTERISTIC_CUSTOM_INIT_STATE_LED, \
.description = "4) Led Init State", \
.format = homekit_format_uint8, \
.permissions = homekit_permissions_paired_read \
| homekit_permissions_paired_write \
| homekit_permissions_notify, \
.min_value = (float[]) {0}, \
.max_value = (float[]) {2}, \
.min_step = (float[]) {1}, \
.valid_values = { \
.count = 3, \
.values = (uint8_t[]) {0, 1, 2}, \
}, \
.value = HOMEKIT_UINT8_(_value), \
##__VA_ARGS__

#define HOMEKIT_CHARACTERISTIC_CUSTOM_LED_DELAY HOMEKIT_CUSTOM_UUID("F0000111")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_LED_DELAY(_value, ...) \
.type = HOMEKIT_CHARACTERISTIC_CUSTOM_LED_DELAY, \
.description = "5) Fading Effect Time", \
.format = homekit_format_float, \
.unit = homekit_unit_seconds, \
.permissions = homekit_permissions_paired_read \
| homekit_permissions_paired_write \
| homekit_permissions_notify, \
.min_value = (float[]) {0}, \
.max_value = (float[]) {5}, \
.min_step = (float[]) {1}, \
.value = HOMEKIT_FLOAT_(_value), \
##__VA_ARGS__

#define HOMEKIT_CHARACTERISTIC_CUSTOM_TOGGLE_GPIO HOMEKIT_CUSTOM_UUID("F0000112")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_TOGGLE_GPIO(_value, ...) \
.type = HOMEKIT_CHARACTERISTIC_CUSTOM_TOGGLE_GPIO, \
.description = "6) Toggle GPIO", \
.format = homekit_format_uint8, \
.permissions = homekit_permissions_paired_read \
| homekit_permissions_paired_write \
| homekit_permissions_notify, \
.min_value = (float[]) {0}, \
.max_value = (float[]) {16}, \
.min_step = (float[]) {1}, \
.value = HOMEKIT_UINT8_(_value), \
##__VA_ARGS__

#define HOMEKIT_CHARACTERISTIC_CUSTOM_EXTERNAL_TOGGLE HOMEKIT_CUSTOM_UUID("F0000113")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_EXTERNAL_TOGGLE(_value, ...) \
.type = HOMEKIT_CHARACTERISTIC_CUSTOM_EXTERNAL_TOGGLE, \
.description = "7) External Toggle Type", \
.format = homekit_format_uint8, \
.permissions = homekit_permissions_paired_read \
| homekit_permissions_paired_write \
| homekit_permissions_notify, \
.min_value = (float[]) {0}, \
.max_value = (float[]) {2}, \
.min_step = (float[]) {1}, \
.valid_values = { \
.count = 3, \
.values = (uint8_t[]) {0, 1, 2}, \
}, \
.value = HOMEKIT_UINT8_(_value), \
##__VA_ARGS__

#define HOMEKIT_CHARACTERISTIC_CUSTOM_IP_ADDR HOMEKIT_CUSTOM_UUID("F0000114")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_IP_ADDR(_value, ...) \
.type = HOMEKIT_CHARACTERISTIC_CUSTOM_IP_ADDR, \
.description = "8) Wifi IP Addr", \
.format = homekit_format_string, \
.permissions = homekit_permissions_paired_read, \
.value = HOMEKIT_STRING_(_value), \
##__VA_ARGS__

#define HOMEKIT_CHARACTERISTIC_CUSTOM_WIFI_RESET HOMEKIT_CUSTOM_UUID("F0000115")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_WIFI_RESET(_value, ...) \
.type = HOMEKIT_CHARACTERISTIC_CUSTOM_WIFI_RESET, \
.description = "9) Wifi Reset", \
.format = homekit_format_bool, \
.permissions = homekit_permissions_paired_read \
| homekit_permissions_paired_write \
| homekit_permissions_notify, \
.value = HOMEKIT_BOOL_(_value), \
##__VA_ARGS__

#define HOMEKIT_CHARACTERISTIC_CUSTOM_REBOOT_DEVICE HOMEKIT_CUSTOM_UUID("F0000116")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_REBOOT_DEVICE(_value, ...) \
.type = HOMEKIT_CHARACTERISTIC_CUSTOM_REBOOT_DEVICE, \
.description = "10) Reboot", \
.format = homekit_format_bool, \
.permissions = homekit_permissions_paired_read \
| homekit_permissions_paired_write \
| homekit_permissions_notify, \
.value = HOMEKIT_BOOL_(_value), \
##__VA_ARGS__

#define HOMEKIT_CHARACTERISTIC_CUSTOM_OTA_UPDATE HOMEKIT_CUSTOM_UUID("F0000117")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_OTA_UPDATE(_value, ...) \
.type = HOMEKIT_CHARACTERISTIC_CUSTOM_OTA_UPDATE, \
.description = "11) Firmware Update", \
.format = homekit_format_bool, \
.permissions = homekit_permissions_paired_read \
| homekit_permissions_paired_write \
| homekit_permissions_notify, \
.value = HOMEKIT_BOOL_(_value), \
##__VA_ARGS__

#endif
