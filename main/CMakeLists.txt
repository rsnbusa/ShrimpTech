# ==============================================================================
# ShrimpTech ESP32 Main Component CMakeLists
# ==============================================================================
# This file defines the main component build configuration for the ShrimpTech
# IoT project including source files, dependencies, and embedded resources.
# ==============================================================================

# C++ Standard Configuration
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



# ==============================================================================
# Source Files Organization
# ==============================================================================

# Command handlers (MQTT/Mesh commands)
set(CMD_SOURCES
    src/cmdMqtt/cmdDisplay.cpp
    src/cmdMqtt/cmdEraseMetrics.cpp
    src/cmdMqtt/cmdFormat.cpp
    src/cmdMqtt/cmdLock.cpp
    src/cmdMqtt/cmdLogs.cpp
    src/cmdMqtt/cmdMQTT.cpp
    src/cmdMqtt/cmdNetw.cpp
    src/cmdMqtt/cmdProd.cpp
    src/cmdMqtt/cmdReset.cpp
    src/cmdMqtt/cmdSendMetrics.cpp
    src/cmdMqtt/cmdUpdate.cpp
    src/cmdMqtt/cmdPanels.cpp
    src/cmdMqtt/cmdBattery.cpp
    src/cmdMqtt/cmdSensors.cpp
    src/cmdMqtt/cmdInverter.cpp
    src/cmdMqtt/cmdVFDCmd.cpp
    src/cmdMqtt/cmdVFDMon.cpp
    src/cmdMqtt/cmdFeedVFDCmd.cpp
    src/cmdMqtt/cmdFeedVFDMon.cpp
    src/cmdMqtt/cmdInvertStatus.cpp
    src/cmdMqtt/cmdProfile.cpp
    src/cmdMqtt/cmdFeedProfile.cpp
    src/cmdMqtt/cmdPID.cpp
    src/cmdMqtt/cmdDOEX.cpp
)

# Keyboard/Console interface commands
set(KBD_SOURCES
    src/kbd/cmdApp.cpp
    src/kbd/cmdBlow.cpp
    src/kbd/cmdConfig.cpp
    src/kbd/cmdDebug.cpp
    # src/kbd/cmdEnDecrypt.cpp
    src/kbd/cmdErase.cpp
    src/kbd/cmdFindUnit.cpp
    src/kbd/cmdFram.cpp
    src/kbd/cmdLog.cpp
    src/kbd/cmdLogLevel.cpp
    src/kbd/cmdMeter.cpp
    src/kbd/cmdMetersreset.cpp
    src/kbd/cmdNode.cpp
    src/kbd/cmdResetConf.cpp
    src/kbd/cmdSecurity.cpp
)

# Core application sources
set(CORE_SOURCES
    src/shrimp/BlowerClass.cpp
    src/shrimp/displayManager.cpp
    src/shrimp/FramI2C.cpp
    src/shrimp/Pump.cpp
    src/shrimp/Valve.cpp
    src/shrimp/feederScheduler.cpp
    src/shrimp/kbd.cpp
    # src/shrimp/lcd.cpp
    src/shrimp/logmanager.cpp
    src/shrimp/mesh_netif.c
    src/shrimp/shrimpmgr.cpp
    src/shrimp/sendData.cpp
    src/shrimp/webserver.cpp
    src/shrimp/nmea_parser.cpp
    src/shrimp/getGpsTime.cpp
)
# Modbus and Sensors
set(MODBUS_SOURCES
    src/modbusDevs/printSensors.cpp
    src/modbusDevs/rs485.cpp
    src/modbusDevs/desc4build.cpp
    src/modbusDevs/genericSensor.cpp
)   

#utilities functions
set(UTILS_SOURCES
 #   src/utilities/crypto_utils.cpp
    src/utilities/time_utils.cpp
    src/utilities/led_utils.cpp
    )
#BLE Sources
set(BLE_SOURCES)

# Combine all source files
set(src
    ${CMD_SOURCES}
    ${KBD_SOURCES}
    ${CORE_SOURCES}
    ${MODBUS_SOURCES}
    ${UTILS_SOURCES}
    ${BLE_SOURCES}
)

# ==============================================================================
# Embedded Text Resources (HTML, certificates)
# ==============================================================================

# set(html 
#     web/index-min.html
#     cert/cloudamp.pem
# )

# ==============================================================================
# Embedded Binary Resources (images, icons)
# ==============================================================================

# set(bins
#     web/cancel.png
#     web/cher.png
#     web/favicon.ico
#     web/nak.png
#     web/ok.png
# )

# ==============================================================================
# Third-Party Sources
# ==============================================================================

# Mongoose HTTP/networking library
FILE(GLOB mongoose_files src/mongoose/*.c)

# ==============================================================================
# Component Registration
# ==============================================================================

idf_component_register(
    SRCS 
        "${src}" 
        ${mongoose_files}
    
    INCLUDE_DIRS 
        inc 
        src/mongoose
    
    REQUIRES 
        # Core ESP-IDF components
        driver
        esp_driver_i2c
        esp_event
        esp_netif
        esp_rom
        esp_timer
        esp_wifi
        nvs_flash
        
        # Networking & protocols
        esp_http_client
        esp_https_ota
        esp_https_server
        mbedtls
        mqtt
        
        # Storage & data
        spiffs
        
        # Application components
        app_update
        bt
        console
        
        # Managed components
        espressif__onewire_bus
        espressif__ds18b20
    
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE MG_ENABLE_LOG=0)


# Note: Unused component include directory variable
set(COMPONENT_ADD_INCLUDEDIRS "")
