#ifndef includes_h
#define includes_h
#include "defines.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "time.h"
#include "math.h"
#include <unistd.h>
#include <sys/lock.h>

#include "esp_lcd_panel_io.h"

#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "mbcontroller.h"

extern "C"{

#include "lvgl.h"
#include "freertos/FreeRTOS.h"      // ALWAYS first before all other freertos
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "cJSON.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "framI2C.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_console.h"
#include "private_include/console_private.h"            //for prompt change
#include "argtable3/argtable3.h"
#include "esp_timer.h"
#include "esp_sntp.h"
#include "mqtt_client.h"
#include "esp_random.h"
#include "esp_mac.h"
#include "esp_chip_info.h"
#include <esp_log.h>
#include "esp_http_client.h"
#include "esp_mesh.h"
#include "esp_wifi_netif.h"
#include "mesh_netif.h"
#include "esp_http_server.h"
#include "esp_https_server.h"
#include "aes_alt.h"			//hw acceleration

#include "lvgl.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

#include "driver/uart.h"


#include "misparams.h"  // for modbus parameters structures
    // ble


// #ifdef  CONFIG_IDF_TARGET_ESP32
// #include "esp32/rom/crc.h"
// #endif
// #ifdef  CONFIG_IDF_TARGET_ESP32s3
// #include "esp32s3/rom/crc.h"
// #endif
#include <esp_spiffs.h>
#include "BlowerClass.h"
#include "esp_crc.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_app_desc.h"
    void app_main();
}



#endif
