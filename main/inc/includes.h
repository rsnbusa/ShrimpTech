#ifndef INCLUDES_H
#define INCLUDES_H

// ============================================================================
// STANDARD C/C++ LIBRARIES
// ============================================================================
#include <inttypes.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/lock.h>
#include <unistd.h>

// ============================================================================
// PROJECT DEFINES
// ============================================================================
#include "defines.h"

// ============================================================================
// C++ COMPATIBLE ESP HEADERS (Must be outside extern "C")
// ============================================================================
#include "AutoPID-for-ESP-IDF.h"  // C++ compatible, must be outside extern "C"

// ============================================================================
// C++ COMPATIBLE ESP HEADERS (Must be outside extern "C")
// ============================================================================
#include "esp_lcd_panel_io.h"  // C++ compatible, must be outside extern "C"

// ============================================================================
// BLE/NIMBLE HEADERS (C++ compatible)
// ============================================================================
#include "esp_nimble_hci.h"
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

// ============================================================================
// MODBUS
// ============================================================================
#include "mbcontroller.h"
#include "misparams.h"  // Modbus parameter structures


// valves and pumps

#include "Valve.hpp"
#include "Pump.hpp"

extern "C" {
#include "nmea_parser.h"
// ============================================================================
// FREERTOS (ALWAYS include FreeRTOS.h first)
// ============================================================================
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/timers.h"

// ============================================================================
// ESP-IDF CORE
// ============================================================================
#include "esp_app_desc.h"
#include "esp_chip_info.h"
#include "esp_console.h"
#include "esp_crc.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_log_buffer.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_random.h"
#include "esp_sntp.h"
#include "esp_system.h"
#include "esp_timer.h"

// ============================================================================
// ESP-IDF NETWORKING
// ============================================================================
#include "esp_http_client.h"
#include "esp_http_server.h"
#include "esp_https_ota.h"
#include "esp_https_server.h"
#include "esp_mesh.h"
#include "esp_ota_ops.h"
#include "esp_wifi.h"
#include "esp_wifi_netif.h"
#include "mesh_netif.h"
#include "mqtt_client.h"

// ============================================================================
// ESP-IDF DRIVERS
// ============================================================================
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/uart.h"
#include "onewire_bus.h"
#include "ds18b20.h"
#include "hx711.h"
// ============================================================================
// ESP-IDF LCD
// ============================================================================

#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"

// ============================================================================
// ESP-IDF STORAGE
// ============================================================================
#include "esp_spiffs.h"
#include "nvs.h"
#include "nvs_flash.h"

// ============================================================================
// SECURITY/CRYPTO
// ============================================================================
// #include "aes_alt.h"  // HW acceleration

// ============================================================================
// THIRD-PARTY LIBRARIES
// ============================================================================
#include "argtable3/argtable3.h"
#include "cJSON.h"
#include "lvgl.h"

// ============================================================================
// PROJECT COMPONENTS
// ============================================================================
#include "framI2C.h"
#include "private_include/console_private.h"  // For prompt change
#include "time.h"


// ============================================================================
// C++ HEADERS (Must be INSIDE extern "C") -> FramI2C.h forces it
// ============================================================================
#include "BlowerClass.h"
// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================
void app_main();

}

#endif // INCLUDES_H
