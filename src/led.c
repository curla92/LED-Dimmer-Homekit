#include <stdio.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <espressif/esp_common.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>

#include <esplibs/libmain.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include "../common/custom_characteristics.h"
#include <wifi_config.h>

#include <adv_button.h>

#include <sysparam.h>
#include <rboot-api.h>

#include <pwm.h>

// GPIO
#define INITIAL_LED_GPIO 4
#define INITIAL_TOGGLE_GPIO 5

#define SHOW_SETUP_SYSPARAM "A"
#define LED_GPIO_SYSPARAM "B"
#define INVERTED_LED_SYSPARAM "C"
#define INIT_STATE_LED_SYSPARAM "D"
#define LAST_STATE_LED_SYSPARAM "E"
#define LAST_STATE_BRIGHTNESS_SYSPARAM "F"
#define LED_DELAY_SYSPARAM "G"
#define TOGGLE_GPIO_SYSPARAM "H"
#define EXTERNAL_TOGGLE_SYSPARAM "I"

#define ALLOWED_FACTORY_RESET_TIME 60000

// Global variables
uint8_t led_gpio;
uint8_t toggle_gpio;

uint8_t reset_toggle_counter = 0;

bool led_on = false;

int current_brightness = 0;
int target_brightness = 0;

ETSTimer device_restart_timer, change_settings_timer, save_states_timer, reset_toggle_timer;

uint8_t pins[1];

volatile bool paired = false;
volatile bool Wifi_Connected = false;

homekit_value_t read_ip_addr();

void switch_on_callback(homekit_characteristic_t *_ch, homekit_value_t on, void *context);
void update_brightness(homekit_characteristic_t *ch, homekit_value_t value, void *context);

void led_notify(int blinking_params);
void change_led_delay();

void show_setup_callback();
void save_states_callback();
void save_settings();
void change_settings_callback();
void reboot_callback();
void ota_firmware_callback();

// GENERAL AND CUSTOM
homekit_characteristic_t show_setup = HOMEKIT_CHARACTERISTIC_(CUSTOM_SHOW_SETUP, true, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(show_setup_callback));
homekit_characteristic_t device_type_name = HOMEKIT_CHARACTERISTIC_(CUSTOM_DEVICE_TYPE_NAME, "ESP8266", .id=107);
homekit_characteristic_t custom_led_gpio = HOMEKIT_CHARACTERISTIC_(CUSTOM_LED_GPIO, INITIAL_LED_GPIO, .id=108, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(change_settings_callback));
homekit_characteristic_t custom_led_inverted = HOMEKIT_CHARACTERISTIC_(CUSTOM_LED_INVERTED, 1, .id=109, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(change_settings_callback));
homekit_characteristic_t custom_init_state_led = HOMEKIT_CHARACTERISTIC_(CUSTOM_INIT_STATE_LED, 0, .id=110, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(change_settings_callback));
homekit_characteristic_t custom_led_delay = HOMEKIT_CHARACTERISTIC_(CUSTOM_LED_DELAY, 1, .id=111, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(change_led_delay));
homekit_characteristic_t custom_toggle_gpio = HOMEKIT_CHARACTERISTIC_(CUSTOM_TOGGLE_GPIO, INITIAL_TOGGLE_GPIO, .id=112, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(change_settings_callback));
homekit_characteristic_t external_toggle = HOMEKIT_CHARACTERISTIC_(CUSTOM_EXTERNAL_TOGGLE, 0, .id=113, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(change_settings_callback));
homekit_characteristic_t ip_addr = HOMEKIT_CHARACTERISTIC_(CUSTOM_IP_ADDR, "", .id = 114, .getter = read_ip_addr);
homekit_characteristic_t wifi_reset = HOMEKIT_CHARACTERISTIC_(CUSTOM_WIFI_RESET, false, .id=115, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(change_settings_callback));
homekit_characteristic_t reboot_device = HOMEKIT_CHARACTERISTIC_(CUSTOM_REBOOT_DEVICE, false, .id = 116, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(reboot_callback));
homekit_characteristic_t ota_firmware = HOMEKIT_CHARACTERISTIC_(CUSTOM_OTA_UPDATE, false, .id = 117, .callback = HOMEKIT_CHARACTERISTIC_CALLBACK(ota_firmware_callback));

// LED CHARACTERISTICS
homekit_characteristic_t switch_on = HOMEKIT_CHARACTERISTIC_(ON, false, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(switch_on_callback));
homekit_characteristic_t brightness_set  = HOMEKIT_CHARACTERISTIC_(BRIGHTNESS, 100, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(update_brightness));

void on_led(bool on) {
    if(on){
        led_on = true;
    }else{
        led_on = false;
    }
}

TaskHandle_t xHandle;

// LED SET
void pwm_led_task(void *pvParameters) {
    
    int pwm;
    
    while(1) {
        if (led_on) {
            target_brightness = brightness_set.value.int_value;
        } else {
            target_brightness = 0;
        }
        
        if (target_brightness > current_brightness) {
            current_brightness++;
            pwm = (UINT16_MAX - UINT16_MAX * current_brightness / 100);
            pwm_set_duty(pwm);
            //printf("Up pwm %d Target %d Current %d\r\n", pwm, target_brightness, current_brightness);
            
            if (target_brightness <= current_brightness) {
                current_brightness = target_brightness;
                vTaskSuspend(xHandle);
            }
            
        }else if (target_brightness < current_brightness) {
            current_brightness--;
            pwm = (UINT16_MAX - UINT16_MAX * current_brightness / 100);
            pwm_set_duty(pwm);
            //printf("Down pwm %d Target %d Current %d\r\n", pwm, target_brightness, current_brightness);
            
            if (target_brightness >= current_brightness) {
                current_brightness = target_brightness;
                vTaskSuspend(xHandle);
            }
        }
        vTaskDelay(custom_led_delay.value.float_value * 10 / portTICK_PERIOD_MS);
    }
}

// LED SET
void pwm_led() {
    xTaskCreate(pwm_led_task, "Led Set", 512, NULL, 2, &xHandle);
}

void change_led_delay() {
    vTaskSuspend(xHandle);
    
    save_settings();
}

// CHANGE SETTINGS
void change_settings_callback() {
    sdk_os_timer_arm(&change_settings_timer, 3500, 0);
}

// SAVE LAST STATE
void save_states_callback() {
    sdk_os_timer_arm(&save_states_timer, 5000, 0);
}

// SAVE SETTINGS
void save_settings() {
    sysparam_status_t status, flash_error = SYSPARAM_OK;
    
    status = sysparam_set_bool(SHOW_SETUP_SYSPARAM, show_setup.value.bool_value);
    if (status != SYSPARAM_OK) {
        flash_error = status;
    }
    
    status = sysparam_set_int8(LED_GPIO_SYSPARAM, custom_led_gpio.value.int_value);
    if (status != SYSPARAM_OK) {
        flash_error = status;
    }
    
    status = sysparam_set_int8(INVERTED_LED_SYSPARAM, custom_led_inverted.value.int_value);
    if (status != SYSPARAM_OK) {
        flash_error = status;
    }
    
    status = sysparam_set_int8(INIT_STATE_LED_SYSPARAM, custom_init_state_led.value.int_value);
    if (status != SYSPARAM_OK) {
        flash_error = status;
    }
    
    
    status = sysparam_set_int32(LED_DELAY_SYSPARAM, custom_led_delay.value.float_value * 100);
    if (status != SYSPARAM_OK) {
        flash_error = status;
    }
    
    status = sysparam_set_int8(TOGGLE_GPIO_SYSPARAM, custom_toggle_gpio.value.int_value);
    if (status != SYSPARAM_OK) {
        flash_error = status;
    }
    
    status = sysparam_set_int8(EXTERNAL_TOGGLE_SYSPARAM, external_toggle.value.int_value);
    if (status != SYSPARAM_OK) {
        flash_error = status;
    }
    
    if (flash_error != SYSPARAM_OK) {
        printf("Saving settings error -> %i\n", flash_error);
    }
}

// SETTINGS INIT
void settings_init() {
    sysparam_status_t status, flash_error = SYSPARAM_OK;
    
    bool bool_value;
    
    int8_t int8_value;
    int32_t int32_value;
    
    status = sysparam_get_bool(SHOW_SETUP_SYSPARAM, &bool_value);
    if (status == SYSPARAM_OK) {
        show_setup.value.bool_value = bool_value;
    } else {
        status = sysparam_set_bool(SHOW_SETUP_SYSPARAM, true);
        if (status != SYSPARAM_OK) {
            flash_error = status;
        }
    }
    
    status = sysparam_get_int8(LED_GPIO_SYSPARAM, &int8_value);
    if (status == SYSPARAM_OK) {
        custom_led_gpio.value.int_value = int8_value;
    } else {
        status = sysparam_set_int8(LED_GPIO_SYSPARAM, INITIAL_LED_GPIO);
        if (status != SYSPARAM_OK) {
            flash_error = status;
        }
    }
    
    status = sysparam_get_int8(INVERTED_LED_SYSPARAM, &int8_value);
    if (status == SYSPARAM_OK) {
        custom_led_inverted.value.int_value = int8_value;
    } else {
        status = sysparam_set_int8(INVERTED_LED_SYSPARAM, 1);
        if (status != SYSPARAM_OK) {
            flash_error = status;
        }
    }
    
    status = sysparam_get_int8(INIT_STATE_LED_SYSPARAM, &int8_value);
    if (status == SYSPARAM_OK) {
        custom_init_state_led.value.int_value = int8_value;
    } else {
        status = sysparam_set_int8(INIT_STATE_LED_SYSPARAM, 0);
        if (status != SYSPARAM_OK) {
            flash_error = status;
        }
    }
    
    status = sysparam_get_int32(LED_DELAY_SYSPARAM, &int32_value);
    if (status == SYSPARAM_OK) {
        custom_led_delay.value.float_value = int32_value / 100.00f;
    } else {
        status = sysparam_set_int32(LED_DELAY_SYSPARAM, 1 * 100);
        if (status != SYSPARAM_OK) {
            flash_error = status;
        }
    }
    
    status = sysparam_get_int8(TOGGLE_GPIO_SYSPARAM, &int8_value);
    if (status == SYSPARAM_OK) {
        custom_toggle_gpio.value.int_value = int8_value;
    } else {
        status = sysparam_set_int8(TOGGLE_GPIO_SYSPARAM, INITIAL_TOGGLE_GPIO);
        if (status != SYSPARAM_OK) {
            flash_error = status;
        }
    }
    
    status = sysparam_get_int8(EXTERNAL_TOGGLE_SYSPARAM, &int8_value);
    if (status == SYSPARAM_OK) {
        external_toggle.value.int_value = int8_value;
    } else {
        status = sysparam_set_int8(EXTERNAL_TOGGLE_SYSPARAM, 0);
        if (status != SYSPARAM_OK) {
            flash_error = status;
        }
    }
    
    // LOAD SAVED STATES
    if (custom_init_state_led.value.int_value > 1) {
        status = sysparam_get_bool(LAST_STATE_LED_SYSPARAM, &bool_value);
        if (status == SYSPARAM_OK) {
            if (custom_init_state_led.value.int_value == 2) {
                switch_on.value.bool_value = bool_value;
                on_led(switch_on.value.bool_value);
            }
        } else {
            status = sysparam_set_bool(LAST_STATE_LED_SYSPARAM, false);
            if (status != SYSPARAM_OK) {
                flash_error = status;
            }
        }
    } else if (custom_init_state_led.value.int_value == 1) {
        switch_on.value.bool_value = true;
        on_led(switch_on.value.bool_value);
    }
    
    status = sysparam_get_int8(LAST_STATE_BRIGHTNESS_SYSPARAM, &int8_value);
    if (status == SYSPARAM_OK) {
        brightness_set.value.int_value = int8_value;
    } else {
        status = sysparam_set_int8(LAST_STATE_BRIGHTNESS_SYSPARAM, 100);
        if (status != SYSPARAM_OK) {
            flash_error = status;
        }
    }
    
    if (flash_error == SYSPARAM_OK) {
    } else {
        printf("ERROR settings INIT -> %i\n", flash_error);
    }
}

// IP Address
homekit_value_t read_ip_addr() {
    struct ip_info info;
    
    if (sdk_wifi_get_ip_info(STATION_IF, &info)) {
        char *buffer = malloc(16);
        snprintf(buffer, 16, IPSTR, IP2STR(&info.ip));
        return HOMEKIT_STRING(buffer);
    }
    return HOMEKIT_STRING("");
}

// SAVE STATES
void save_states() {
    sysparam_status_t status, flash_error = SYSPARAM_OK;
    
    if (custom_init_state_led.value.int_value > 1) {
        status = sysparam_set_bool(LAST_STATE_LED_SYSPARAM, switch_on.value.bool_value);
        if (status != SYSPARAM_OK) {
            flash_error = status;
        }
    }
    
    status = sysparam_set_int8(LAST_STATE_BRIGHTNESS_SYSPARAM, brightness_set.value.int_value);
    if (status != SYSPARAM_OK) {
        flash_error = status;
    }
    
    if (flash_error != SYSPARAM_OK) {
        printf("Saving last states error -> %i\n", flash_error);
    }
}

//RESTART
void device_restart_task() {
    vTaskDelay(5500 / portTICK_PERIOD_MS);
    
    if (wifi_reset.value.bool_value) {
        wifi_config_reset();
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
    
    if (ota_firmware.value.bool_value) {
        rboot_set_temp_rom(1);
        vTaskDelay(150 / portTICK_PERIOD_MS);
    }
    
    sdk_system_restart();
    vTaskDelete(NULL);
}

void device_restart() {
    printf("Restarting device\n");
    xTaskCreate(device_restart_task, "device_restart_task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
}

// RESET
void reset_configuration_task() {
    
    led_notify(2);
    printf("Resetting Wifi Config\n");
    
    wifi_config_reset();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    printf("Resetting HomeKit Config\n");
    homekit_server_reset();
    
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    printf("Restarting\n");
    
    sdk_system_restart();
    vTaskDelete(NULL);
    
}

void reset_mode_call(const uint8_t gpio, void *args) {
    if (xTaskGetTickCountFromISR() < ALLOWED_FACTORY_RESET_TIME / portTICK_PERIOD_MS) {
        xTaskCreate(reset_configuration_task, "Reset configuration", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    } else {
        printf("Factory reset not allowed after %ims since boot. Repower device and try again\n", ALLOWED_FACTORY_RESET_TIME);
    }
}

// RESET COUNT CALLBACK
void reset_toggle_upcount() {
    reset_toggle_counter++;
    sdk_os_timer_arm(&reset_toggle_timer, 3100, 0);
}

void reset_toggle() {
    if (reset_toggle_counter > 10) {
        reset_mode_call(0, NULL);
    }
    reset_toggle_counter = 0;
}

// REBOOT
void reboot_callback() {
    if (reboot_device.value.bool_value) {
        sdk_os_timer_setfn(&device_restart_timer, device_restart, NULL);
        sdk_os_timer_arm(&device_restart_timer, 5000, 0);
    } else {
        sdk_os_timer_disarm(&device_restart_timer);
    }
}

// OTA
void ota_firmware_callback() {
    if (ota_firmware.value.bool_value) {
        sdk_os_timer_setfn(&device_restart_timer, device_restart, NULL);
        sdk_os_timer_arm(&device_restart_timer, 5000, 0);
    } else {
        sdk_os_timer_disarm(&device_restart_timer);
    }
}

// SHOW SETUP CALLBACK
void show_setup_callback() {
    if (show_setup.value.bool_value) {
        sdk_os_timer_setfn(&device_restart_timer, device_restart, NULL);
        sdk_os_timer_arm(&device_restart_timer, 5000, 0);
    } else {
        sdk_os_timer_disarm(&device_restart_timer);
    }
    
    save_settings();
}

// LED IDENTIFY TASK
void led_identify_task(void *_args) {
    // Identify dispositive by Pulsing LED.
    led_notify(3);
    vTaskDelete(NULL);
}

// LED IDENTIFY
void led_identify(homekit_value_t _value) {
    printf("Led Identify\n");
    xTaskCreate(led_identify_task, "Led identify", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
}

// SWITCH ON/OFF LED
void switch_on_callback(homekit_characteristic_t *_ch, homekit_value_t on, void *context) {
    on_led(switch_on.value.bool_value);
    vTaskResume(xHandle);
    
    save_states_callback();
}

// BRIGHTNESS LED UPDATE
void update_brightness(homekit_characteristic_t *ch, homekit_value_t value, void *context) {
    printf("Brightness update: %d\n", brightness_set.value.int_value);
    
    vTaskResume(xHandle);
}

// BUTTON SINGLE PRESS CALLBACK
void singlepress_callback(const uint8_t gpio, void *args, const uint8_t param) {
    
    switch_on.value.bool_value = !switch_on.value.bool_value;
    on_led(switch_on.value.bool_value);
    // IF BRI IS 0 SET NEW VALUE OF 100
    if (brightness_set.value.int_value == 0) {
        brightness_set.value.int_value = 100;
    }
    printf("Pressing button \n Target brightness: %d\n", brightness_set.value.int_value);
    // NOTIFY TO HOMEKIT FROM TOGGLE
    homekit_characteristic_notify(&brightness_set, brightness_set.value);
    homekit_characteristic_notify(&switch_on, switch_on.value);
    
    reset_toggle_upcount();
}

// TOGGLE CALLBACK
void toggle_callback(const uint8_t gpio, void *args, const uint8_t param) {
    
    switch_on.value.bool_value = !switch_on.value.bool_value;
    on_led(switch_on.value.bool_value);
    // IF BRI IS 0 SET NEW VALUE OF 100
    if (brightness_set.value.int_value == 0) {
        brightness_set.value.int_value = 100;
    }
    printf("Toggling switch \n Target brightness %d\n", brightness_set.value.int_value);
    // NOTIFY TO HOMEKIT FROM TOGGLE
    homekit_characteristic_notify(&brightness_set, brightness_set.value);
    homekit_characteristic_notify(&switch_on, switch_on.value);
    
    reset_toggle_upcount();
}

// HOMEKIT
void on_event(homekit_event_t event) {
    
    switch (event) {
        case HOMEKIT_EVENT_SERVER_INITIALIZED:
            printf("on_homekit_event: Server initialised\n");
            break;
        case HOMEKIT_EVENT_CLIENT_CONNECTED:
            printf("on_homekit_event: Client connected\n");
            break;
        case HOMEKIT_EVENT_CLIENT_VERIFIED:
            printf("on_homekit_event: Client verified\n");
            if (!paired ){
                paired = true;
            }
            break;
        case HOMEKIT_EVENT_CLIENT_DISCONNECTED:
            printf("on_homekit_event: Client disconnected\n");
            break;
        case HOMEKIT_EVENT_PAIRING_ADDED:
            printf("on_homekit_event: Pairing added\n");
            break;
        case HOMEKIT_EVENT_PAIRING_REMOVED:
            printf("on_homekit_event: Pairing removed\n");
            if (!homekit_is_paired())
            /* if we have no more pairtings then restart */
                printf("on_homekit_event: no more pairings so restart\n");
            sdk_system_restart();
            break;
        default:
            printf("on_homekit_event: Default event %d ", event);
    }
    
}

// GLOBAL CHARACTERISTICS
homekit_characteristic_t manufacturer =
HOMEKIT_CHARACTERISTIC_(MANUFACTURER, "Curla92");
homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, "Dimmable Led");
homekit_characteristic_t serial = HOMEKIT_CHARACTERISTIC_(SERIAL_NUMBER, NULL);
homekit_characteristic_t model = HOMEKIT_CHARACTERISTIC_(MODEL, "ESP8266");
homekit_characteristic_t revision =
HOMEKIT_CHARACTERISTIC_(FIRMWARE_REVISION, "2.5.0");
homekit_characteristic_t identify_function =
HOMEKIT_CHARACTERISTIC_(IDENTIFY, led_identify);

homekit_characteristic_t service_name =
HOMEKIT_CHARACTERISTIC_(NAME, "Dimmable Led");
homekit_characteristic_t setup_name = HOMEKIT_CHARACTERISTIC_(NAME, "Setup");

// ACCESSORY NAME
void create_accessory_name() {
    // Accessory Name
    uint8_t macaddr[6];
    sdk_wifi_get_macaddr(STATION_IF, macaddr);
    
    int name_len = snprintf(NULL, 0, "Dimmable Led-%02X%02X%02X", macaddr[3],
                            macaddr[4], macaddr[5]);
    
    char *name_value = malloc(name_len + 1);
    snprintf(name_value, name_len + 1, "Dimmable Led-%02X%02X%02X", macaddr[3],
             macaddr[4], macaddr[5]);
    name.value = HOMEKIT_STRING(name_value);
    
    // Accessory Serial
    char *serial_value = malloc(13);
    snprintf(serial_value, 13, "%02X%02X%02X%02X%02X%02X", macaddr[0], macaddr[1],
             macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
    serial.value = HOMEKIT_STRING(serial_value);
}

homekit_server_config_t config;

// CREATE ACCESSORY
void create_accessory() {
    
    uint8_t service_count = 3;
    
    if (show_setup.value.bool_value) {
        service_count++;
    }
    
    homekit_accessory_t **accessories = calloc(2, sizeof(homekit_accessory_t *));
    
    homekit_accessory_t *accessory = accessories[0] =
    calloc(1, sizeof(homekit_accessory_t));
    accessory->id = 1;
    accessory->category = homekit_accessory_category_lightbulb;
    accessory->services = calloc(service_count, sizeof(homekit_service_t *));
    
    homekit_service_t *accessory_info = accessory->services[0] =
    calloc(1, sizeof(homekit_service_t));
    accessory_info->type = HOMEKIT_SERVICE_ACCESSORY_INFORMATION;
    accessory_info->characteristics =
    calloc(7, sizeof(homekit_characteristic_t *));
    
    accessory_info->characteristics[0] = &name;
    accessory_info->characteristics[1] = &manufacturer;
    accessory_info->characteristics[2] = &serial;
    accessory_info->characteristics[3] = &model;
    accessory_info->characteristics[4] = &revision;
    accessory_info->characteristics[5] = &identify_function;
    accessory_info->characteristics[6] = NULL;
    
    homekit_service_t *led_function = accessory->services[1] =
    calloc(1, sizeof(homekit_service_t));
    led_function->type = HOMEKIT_SERVICE_LIGHTBULB;
    led_function->primary = true;
    led_function->characteristics = calloc(5, sizeof(homekit_characteristic_t *));
    
    led_function->characteristics[0] = &service_name;
    led_function->characteristics[1] = &switch_on;
    led_function->characteristics[2] = &brightness_set;
    led_function->characteristics[3] = &show_setup;
    led_function->characteristics[4] = NULL;
    
    if (show_setup.value.bool_value) {
        homekit_service_t *led_setup = accessory->services[2] =
        calloc(1, sizeof(homekit_service_t));
        led_setup->type = HOMEKIT_SERVICE_CUSTOM_SETUP;
        led_setup->primary = false;
        led_setup->characteristics = calloc(13, sizeof(homekit_characteristic_t *));
        
        led_setup->characteristics[0] = &setup_name;
        led_setup->characteristics[1] = &device_type_name;
        led_setup->characteristics[2] = &custom_led_gpio;
        led_setup->characteristics[3] = &custom_led_inverted;
        led_setup->characteristics[4] = &custom_init_state_led;
        led_setup->characteristics[5] = &custom_led_delay;
        led_setup->characteristics[6] = &custom_toggle_gpio;
        led_setup->characteristics[7] = &external_toggle;
        led_setup->characteristics[8] = &ip_addr;
        led_setup->characteristics[9] = &wifi_reset;
        led_setup->characteristics[10] = &reboot_device;
        led_setup->characteristics[11] = &ota_firmware;
        led_setup->characteristics[12] = NULL;
    } else {
        accessory->services[2] = NULL;
    }
    
    config.accessories = accessories;
    config.password = "277-66-227";
    config.setupId="1QJ8",
    config.on_event = on_event;
    
    homekit_server_init(&config);
}

// WIFI
void on_wifi_event(wifi_config_event_t event) {
    if (event == WIFI_CONFIG_CONNECTED) {
        printf("CONNECTED TO >>> WIFI <<<\n");
        Wifi_Connected = true;
        
        create_accessory_name();
        create_accessory();
        
        led_notify(2);
    } else if (event == WIFI_CONFIG_DISCONNECTED) {
        Wifi_Connected = false;
        printf("DISCONNECTED FROM >>> WIFI <<<\n");
    }
}

// TOGGLE GPIO INIT
void toggle_gpio_init() {
    switch (external_toggle.value.int_value) {
        case 1:
            toggle_gpio = custom_toggle_gpio.value.int_value;
            printf("External toggle type 1 at gpio -> %d\n", toggle_gpio);
            adv_button_create(toggle_gpio, true, false);
            adv_button_register_callback_fn(toggle_gpio, singlepress_callback, SINGLEPRESS_TYPE, NULL, 0);
            break;
        case 2:
            toggle_gpio = custom_toggle_gpio.value.int_value;
            printf("External toggle type 2 at gpio -> %d\n", toggle_gpio);
            adv_button_create(toggle_gpio, true, false);
            adv_button_register_callback_fn(toggle_gpio, toggle_callback, INVSINGLEPRESS_TYPE, NULL, 0);  // Low
            adv_button_register_callback_fn(toggle_gpio, toggle_callback, SINGLEPRESS_TYPE, NULL, 0);   // High
            break;
        default:
            break;
    }
}

// LED PULSING NOTIFY
void led_notify_task(void * parameter) {
    vTaskSuspend(xHandle);
    for (int j=0; j<(int)parameter; j++) {
        for (int i=0; i<=50; i++) {
            int x = brightness_set.value.int_value;
            int w;
            w = (UINT16_MAX - UINT16_MAX*i/40);
            if(i>25) {
                w = (UINT16_MAX - UINT16_MAX*abs(i-50)/40);
            }
            pwm_set_duty(w);
            if (i == 50 && j == (int)parameter -1 && switch_on.value.bool_value == true) {
                for (int y=0; y<=x; y++) {
                    w = (UINT16_MAX - UINT16_MAX*y/100);
                    pwm_set_duty(w);
                    if (y==x) {
                        target_brightness = x;
                        current_brightness = target_brightness;
                        break;
                    }
                    vTaskDelay(20 / portTICK_PERIOD_MS);
                }
            }
            vTaskDelay(20 / portTICK_PERIOD_MS);
        }
    }
    vTaskDelete(NULL);
}

void led_notify(int blinking_params) {
    xTaskCreate(led_notify_task, "led_notify_task", configMINIMAL_STACK_SIZE, (void*)(int)blinking_params, 2, NULL);
}

// STATUS LED INIT
void led_init() {
    
    // LED GPIO INIT
    led_gpio = custom_led_gpio.value.int_value;
    printf("Led gpio init -> %d\n", led_gpio);
    printf("Led delay init -> %.1fs\n", custom_led_delay.value.float_value);
    
    pins[0] = led_gpio;
    
    // PWM INIT INVERTED TRUE/FALSE
    switch (custom_led_inverted.value.int_value) {
        case 0:
            pwm_init(1, pins, false);
            pwm_set_freq(1000);
            printf("pwm_set_freq(1000)     # 1 kHz\n");
            pwm_set_duty(0);
            break;
        case 1:
            pwm_init(1, pins, true);
            pwm_set_freq(1000);
            printf("pwm_set_freq(1000)     # 1 kHz\n");
            pwm_set_duty(UINT16_MAX);
            break;
        default:
            printf("No action \n");
    }
    
    pwm_start();
    pwm_led();
    
    led_notify(1);
    
    toggle_gpio_init();
}

// INIT
void user_init(void) {
    uart_set_baud(0, 115200);
    
    settings_init();
    
    led_init();
    
    sdk_os_timer_setfn(&reset_toggle_timer, reset_toggle, NULL);
    sdk_os_timer_setfn(&change_settings_timer, save_settings, NULL);
    sdk_os_timer_setfn(&save_states_timer, save_states, NULL);
    
    wifi_config_init2("Dimmable Led", NULL, on_wifi_event);
}

