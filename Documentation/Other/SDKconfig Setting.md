📝  🚀🔥✨❤️💼🏠📣🧑‍💻

# SDKconfig Settings
Since we have esp32 and esp32s3 we need to set target to each mcu. When we select the target and it changes, it will erase the current sdkconfig to create a new sdkconfig BLANK or GENERIC, loosing any specific changes made like PSRAM, lwip nat, iram wifi settings etc.

# What to do
Make an initial valid settings for the target in sdkconfig via menuconfig and then copy the resulting sdkconfig with a name like sdkconfigesp32 and skdconfigs3 which states the target

# Then what
When you change again to another target mcu, it will erase the current sdkconfig and create a new one. Once its done, erase the sdkconfig, make a copy the target sdkconfig as stated above and rename it sdkconfig. Now we have the valid sdkconfig to compile.

# Using current PBC board for both ESP32 and S3
THe pbc board was desinged for esp32 and not the s3 but settings in the defines.h allow for its use with ONE caveat, no PSRAM allowed in the S3 version. Until we have a S3 pcb board that's it.