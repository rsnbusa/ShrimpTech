
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set (src
    src/cmd/cmdFormat.cpp
    src/cmd/cmdEraseMetrics.cpp
    src/cmd/cmdSendMetrics.cpp
    src/cmd/cmdLock.cpp
    src/cmd/cmdProd.cpp
    src/cmd/cmdMQTT.cpp
    src/cmd/cmdNetw.cpp
    src/cmd/cmdUpdate.cpp
    src/cmd/cmdDisplay.cpp
    src/cmd/cmdLogs.cpp
    src/FramI2C.cpp
    src/kbd.cpp
    src/logmanager.cpp
    src/mesh_netif.c
    src/meter.cpp
    src/BlowerClass.cpp
    src/sendData.cpp
    src/webserver.cpp
    src/lcd.cpp
    src/displayManager.cpp
    src/nimble.cpp
    src/cmd/cmdReset.cpp
    src/cmd/cmdReset.cpp
    src/rs485.cpp
    kbd/cmdApp.cpp
    kbd/cmdConfig.cpp
    kbd/cmdEnDecrypt.cpp
    kbd/cmdErase.cpp
    kbd/cmdFindUnit.cpp
    kbd/cmdFram.cpp
    kbd/cmdLog.cpp
    kbd/cmdLogLevel.cpp
    kbd/cmdMeter.cpp
    kbd/cmdMetersreset.cpp
    kbd/cmdResetConf.cpp
    kbd/cmdSecurity.cpp
    kbd/cmdNode.cpp
    kbd/cmdDebug.cpp
    kbd/cmdBlow.cpp
    )

    set (html 
    web/index-min.html
    cert/cloudamp.pem
    # cert/cert.pem
    )

    set(bins
    web/nak.png
    web/ok.png
    web/favicon.ico
    web/cher.png
    web/cancel.png
    )

    FILE(GLOB mongoose_files src/mongoose/*.c)

    
idf_component_register(SRCS "${src}" ${mongoose_files}
                       INCLUDE_DIRS inc
REQUIRES driver bt esp_timer nvs_flash esp_wifi  esp_event esp_netif spiffs json console app_update mqtt esp_http_client esp_https_server esp_driver_i2c esp_https_ota
esp_http_client app_update esp_https_ota esp_netif mbedtls nvs_flash esp_rom
# REQUIRES driver esp_timer nvs_flash esp_https_ota esp_timer esp_wifi esp_adc esp_event esp_netif spiffs json console u8g2 app_update wifi_provisioning mqtt esp_http_client esp_https_server u8g2-hal-esp-idf

                       EMBED_TXTFILES "${html}" 
                       EMBED_FILES "${bins}"
)

# Add permissive only for C++ files
# THIS DOES WORK for cpp fpermissive etc.
# target_compile_options(${COMPONENT_LIB} PRIVATE
#     $<$<COMPILE_LANGUAGE:CXX>:-fpermissive>
#     $<$<COMPILE_LANGUAGE:CXX>:-Wno-error=address>
# )

set(COMPONENT_ADD_INCLUDEDIRS "")