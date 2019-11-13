PROGRAM = main

EXTRA_COMPONENTS = \
    extras/onewire \
	extras/http-parser \
	extras/dhcpserver \
	extras/pwm \
	extras/rboot-ota \
	$(abspath ../lib/esp-adv-button) \
	$(abspath ../components/esp-8266/esp-wifi-config) \
	$(abspath ../components/esp-8266/esp-cjson) \
	$(abspath ../components/common/esp-wolfssl) \
	$(abspath ../components/common/esp-homekit) \

FLASH_SIZE = 8
FLASH_MODE = dout
FLASH_SPEED = 40

HOMEKIT_SPI_FLASH_BASE_ADDR = 0x8c000
HOMEKIT_MAX_CLIENTS = 16
HOMEKIT_SMALL = 0

EXTRA_CFLAGS += -I../.. -DHOMEKIT_SHORT_APPLE_UUIDS
EXTRA_CFLAGS += -DHOMEKIT_OVERCLOCK_PAIR_VERIFY
EXTRA_CFLAGS += -DHOMEKIT_OVERCLOCK_PAIR_SETUP

## DEBUG
#EXTRA_CFLAGS += -DHOMEKIT_DEBUG=1

include $(SDK_PATH)/common.mk

LIBS += m

monitor:
	$(FILTEROUTPUT) --port $(ESPPORT) --baud 115200 --elf $(PROGRAM_OUT)
