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
    src/cmd/cmdDisplay.cpp
    src/cmd/cmdEraseMetrics.cpp
    src/cmd/cmdFormat.cpp
    src/cmd/cmdLock.cpp
    src/cmd/cmdLogs.cpp
    src/cmd/cmdMQTT.cpp
    src/cmd/cmdNetw.cpp
    src/cmd/cmdProd.cpp
    src/cmd/cmdReset.cpp
    src/cmd/cmdSendMetrics.cpp
    src/cmd/cmdUpdate.cpp
)

# Keyboard/Console interface commands
set(KBD_SOURCES
    kbd/cmdApp.cpp
    kbd/cmdBlow.cpp
    kbd/cmdConfig.cpp
    kbd/cmdDebug.cpp
    kbd/cmdEnDecrypt.cpp
    kbd/cmdErase.cpp
    kbd/cmdFindUnit.cpp
    kbd/cmdFram.cpp
    kbd/cmdLog.cpp
    kbd/cmdLogLevel.cpp
    kbd/cmdMeter.cpp
    kbd/cmdMetersreset.cpp
    kbd/cmdNode.cpp
    kbd/cmdResetConf.cpp
    kbd/cmdSecurity.cpp
)

# Core application sources
set(CORE_SOURCES
    src/BlowerClass.cpp
    src/displayManager.cpp
    src/FramI2C.cpp
    src/kbd.cpp
    src/lcd.cpp
    src/logmanager.cpp
    src/mesh_netif.c
    src/meter.cpp
    src/nimble.cpp
    src/rs485.cpp
    src/sendData.cpp
    src/webserver.cpp
)

# Combine all source files
set(src
    ${CMD_SOURCES}
    ${KBD_SOURCES}
    ${CORE_SOURCES}
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
        json
        spiffs
        
        # Application components
        app_update
        bt
        console
    
    # EMBED_TXTFILES 
    #     "${html}"
    
    # EMBED_FILES 
    #     "${bins}"
)

# Note: Unused component include directory variable
set(COMPONENT_ADD_INCLUDEDIRS "")