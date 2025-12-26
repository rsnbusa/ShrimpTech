 📝  🚀🔥✨❤️💼🏠📣🧑‍💻

# Program Flow

General description of the Flow of Routines in the application in words vs a Flowchart

## app_main

As is normal, entry point of app. THis is a cpp (C++) so in the includes.h forlder there is an entry for extern "C" {void app_main()}

- It will determine partially the program flow, speciality at the begining

### Initial booting phase 🚀

    - flashsem & i2cSem created here since it controls access to Flash for all tasks and will immediately be used
    - NVS as per usual
    - read flash to get configurations and centinel guard. If no CENTINEL Erase/configure the Flash to defaults
    - Sanity checks for hardware
    - Init a bunch of variables, queues, etc. No tasks launched. See init vars routine

##### Main Task launched. main_app()

    - This is the meat and bones of this Application.
    - Will get pulses from up to 8 meters and begin managing them, save to Fram etc
    - very simple rotuine, works on thersholds, for each Meter 1/10 of their BPK as upper limit
    - connect HW like FRAM and I2... Fram Failure is FATAL🔥🔥🔥
    - infinite loop

#### Kbd task launched if debug mode set in defines.h,

    - extremely helpful to debug, work. Not necessary on final production but.....
    - It can launch tasks, but is not part of the main logic concern. Just a tool to test features and configurations

### Configuration checks 📝

    -- Check Provison state(skippable) and Meter Config (compulsorty)
    -- If clean code/MCU will try to PROV so it will be a Main Root
    ---🔥 Its skippable for a non main Root node (first MESH configuration)
    -- ✨If OLED attached it can be reached via Menu Option (long flash button)

### OLED Manager

    - Task is launched. Very independent does not get involved in functionality
    - Outer "Loop" check for short click (Flash Btn)
    -- short will show next Meter data
    -- long will take to Config Menu📝

### MESH 🚀

    - Mesh is now started
    -- It will start a very intricate set of  ewvent driven conditions
    -- WiFi is managed by mesh routines
        • MAIN ROOT node (first config of MESH) will send STA/PSW
            too child nodes in case of Roots death
        • WHen  event ✨got_IP✨ if node is ROOT🔥 it will start
            • MQTT app start in charge fo MQTT
                • once done, SNTP to get time

### RUNNING

    -- THe Main Task Pulse counter keeps count of all Meters and
        saves datra to FRAM
    -- A repeated timer every CYCLE ms is in charge of sending Readinmg to HQ

#### Data Management

    -- When timer awakes, ROOT Node will send a Broadcast Msg to all nodes
            to send their meters data
        -- will wait for all connected Nodes or Timeout and send what it has
        -- MQTT manager is on a Need Basis
         ✨Connect -> Send _> Disconnect✨  no permanete connection
        CHECK DOCUMENTATION OF THIS ROUTIME
