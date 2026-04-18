#ifndef DEFINES_H
#define DEFINES_H
// #include "hal/hal-Sch-Shrimpo-Feeder-TVL.h"
#include "hal/hal-SCH-ShrimpPcb.h"
extern void my_log(const char * color,const char* tag, const char* format, ...);
#define MESP_LOGI(tag, format, ...) my_log(GRAY,tag, format, ##__VA_ARGS__)
#define MESP_LOGW(tag, format, ...) my_log(BK_YELLOW,tag, format, ##__VA_ARGS__)
#define MESP_LOGE(tag, format, ...) my_log(BK_RED,tag, format, ##__VA_ARGS__)
#define MESP_LOGD(tag, format, ...) my_log(BLUE,tag, format, ##__VA_ARGS__)

// GPS Sensor
#define YEAR_BASE                       (2000)  //date in GPS starts from 2000
#define TIME_ZONE                       (-5)   
#define GPS_RATE                        (9600)

#define SIMULATE 
#define TIMERUNITS                     (1000000) // 1 second in microseconds for esp_timer, adjust if using a different timer system
#define ONEWIRE_MAX_DS18B20             (1)

//GPIOS assignment accordin to PCB Shrimp-Feeder-TVL April 16/2026
// RS232C standard ports ... in theory rx-43 and tx-44 
#define UART_PORT                       (1)

#define MBUF_SIZE                       (200)
#define BAUD_RATE                       (9600)
#define REPLYWAIT                       (800)
#define ECHO_READ_TOUT                  (3) // 3.5T * 8 = 28 ticks, TOUT=3 -> ~24..33 ticks
#define PACKET_READ_TICS                (100 / portTICK_PERIOD_MS)
#define MAXMODBUS                       (10) // sensors

#define HX711_SCALE_FACTOR              (903.0f)    // Raw ADC counts per gram
#define HX711_WEIGHT_CORRECTION         (1.0062f)      // If 100g reads 90g use (100.0f/99.4.0f); if 110g use (100.0f/110.0f)
#define HX711_TARE_SAMPLES              (10)
#define HX711_READ_SAMPLES              (5)
#define HX711_READ_PERIOD_MS            (1000)

#define RELAYON                         (0)
#define RELAYOFF                        (1)
#define DEBB
#define LOCALTIME                       "GMT+5"
#define LOGOPT
#define SAVEDATE                        (5)
#define FIXEDMESH                       (1)
#define STATSM
#define STATSDELAY                      (60000)
#define MAXCMDS                         (30)            // use a good number not sharing ram with anybody else

#define SENTINEL                        (0x12345678)
#define MQTTBIG                         (3500)  // big enough for 3500-byte MQTT payloads
#define MESH_TAG                        "Shrimp"
#define WIFI_CONNECTED_BIT              BIT0
#define WIFI_FAIL_BIT                   BIT1

#define MQTTRECONNECTTIME               (2000)
#define SNTPTRY                         (10)
#define FREEANDNULL(x)		            if(x) {free(x);x=NULL;}
#define MAXINTCMDS                      (18)
#define APPNAME                         "shrimp"
#define MQTTSECURE       
#define MAXNODES                        (20)                
#define MAXDEVSS                        (1)

#define MQTTSENDER                      (2000)
#define MQTTSENDERWAIT                  (10)
#define SLOTSIZE                        (2)        // slot time in seconds
#define HOUR                            (60)      // in seconds
#define QUEUE                           APPNAME
#define MAXMQTTERRS                     (10)
#define NUMDUPS                         (500)
#define TAG                             APPNAME
#define EXPECTED_NODES                  (2500000)
#define EXPECTED_CONNS                  (10000)
#define EXPECTED_DELIVERY_TIME          (2)

#define TURNOFF                         (1)
#define TURNON                          (0)
#define MINPREPAID                      (1)       //as of today aprox $1
#define MESHTIMEOUT                     (1000)     // mesh timeout ms for all messages in mesh... a lot for 10... use 1000


#define MAXMQTTERR                      (2)
#define SCHEDULE                        (0)
#define BOOTRESP                        (1)
#define SENDMETRICS                     (2)
#define METERSDATA                      (3)
#define PRODUCTION                      (4)
#define CONFIRMLOCK                     (5)
#define CONFIRMLOCKERR                  (6)
#define INSTALLATION                    (7)
#define REINSTALL                       (8)
#define CONFIRMINST                     (9)
#define FORMAT                          (10)
#define UPDATEMETER                     (11)
#define ERASEMETRICS                    (12)
#define MQTTMETRICS                     (13)
#define METRICRESP                      (14)
#define SHOWDISPLAY                     (15)

#define CONFIRMTIMER                    (1000)
#define METER_NOT_FOUND                 (0x1234)

#define DEFAULT_SSID                    "NETLIFE-RSNCasa"
#define DEFAULT_PSWD                    "csttpstt"
#define DEFAULT_MESH_SSID              "NETLIFE-RSNCasa"
#define DEFAULT_MESH_PASSW             "csttpstt"
#define NORETAIN                        (0)
#define RETAIN                          (1)
#define QOS1                            (1)
#define ALL

#define DEFAULTMQTT                     "mqtt://64.23.180.233:1883"
#define DEFAULTMQTTUSER                 "robert"
#define DEFAULTMQTTPASS                 "csttpstt"

#define SSIDBLINKTIME                   (80)

// mqtt topic names
#define MQTTCMD                         "cmd"
#define MQTTINFO                        "metrics"
#define MQTTALARM                       "alarm"
#define MQTTCONTROL                     "control"
#define MQTTEMER                        "911"
#define MQTTBROADCAST                   "broadcast"
#define MQTTDISCO                       "answer"
#define MQTTINSTALL                     "install"

#define MESH_INT_DATA_BIN               (0x88995566)
#define MESH_INT_DATA_CJSON             (0x33221144)
#define MESH_INT_DATA_MODBUS            (0x33221155)

#define MAXHORARIOS                     (10)
#define MAXCICLOS                       (10)
#define MAXPROFILES                     (2)
#define MAXDAYCYCLE                     (100)

#define PROFILE_FILE                    "/spiffs/profilejson.txt"
#define FEEDPROFILE_FILE                "/spiffs/feedprofilejson.txt"
#define LOG_FILE                        "/spiffs/log.txt"

#define BLE_MODE                        (0)
#define MESH_MODE                       (1)

#define SENDMUX                         (2)
#define LOGINTIME                       (60000)         //1 minute
#define FCHOLDING                       (0x03)
#define FCINPUT                         (0x01)

#define POOLREADY                       (0)   // stopped but active pool
#define POOLBLOWERON                    (1)   // actually blowing
#define POOLBLOWERNEXT                  (2)    // in cycle waitng next hour scheule
#define POOLBLOWEROFF                   (3)    // currently beign harvbested
#define POOLNEXT                        (4)    // currently beign harvbested
#define POOLCROP                        (5)    // currently beign harvbested
#define POOLPARK                        (6)    // not being used

#define MODBUSMINUTES                   (60000)   
// Modbus descriptor fields offsets
#define DADDR                           (4)
#define DOFFSET                         (3)
#define DSTART                          (2)
#define DPOINTS                         (1)
#define DTYPE                           (0)
#define SENSOR_ERROR_BIT                (0) // Bit 0 for sensor errors
#define PANELS_ERROR_BIT                (1) // Bit 1 for panel errors
#define BATTERY_ERROR_BIT               (2) // Bit 2 for battery errors
#define ENERGY_ERROR_BIT                (3) // Bit 3 for energy errors
#define VFD_ERROR_BIT                   (8) // Bit 8 for VFD errors

#define SENSOR_LIMIT_ERROR_BIT          (4) // Bit 4 for limit errors
#define PANELS_LIMIT_ERROR_BIT          (5) // Bit 5 for limit errors
#define BATTERY_LIMIT_ERROR_BIT         (6) // Bit 6 for limit errors
#define ENERGY_LIMIT_ERROR_BIT          (7) // Bit 7 for limit errors
#define VFD_LIMIT_ERROR_BIT             (9) // Bit 7 for limit errors

// for a VFC min should be 30Hz and max 60Hz nomally.  Could be adjusted specifically MAX to 90Hz if needed
#define OUTPUT_MIN                      (30)
#define OUTPUT_MAX                      (60)
#define SETPOINT_DO                     (7.0)    

// Motor specs 
#define MOTORKW                         (2200)  // 2200 W motor
#define MOTORVOLTS                      (220)  // 220 V motor -> 10amps

#define WIFI_MESH                       (0)


// Color definitons for debugging 

#define DBG_SCH						    "\e[36m[SCH]\e[0m"               
#define DBG_BLOW						"\e[34m[SCH]\e[0m"               
#define DBG_MESH					    "\e[35m[MESH]\e[0m"              
#define DBG_BLE						    "\e[32m[BLE]\e[0m]"              
#define DBG_MQTT					    "\e[33m[MQTT]\e[0m"              
#define DBG_XCMDS					    "\e[37m[XCMDS]\e[0m"              
#define DBG_MODB					    "\e[91m[MODB]\e[0m"                
#define DBG_TEMP					    "\e[34m[TEMP]\e[0m"            
#define DBG_BATTERY					    "\e[90m[BATT]"                
#define DBG_PANELS					    "\e[33m[PANELS]"                
#define DBG_ENERGY					    "\e[35m[ENERGY]"                
#define DBG_SENSORS					    "\e[34m[SENSORS]"                
#define DBG_VFD					        "\e[33m[VFD]"                
#define DBG_INVSTATUS   		        "\e[93m[INVSTATUS]"                

#define BLACKC							"\e[30m"
#define BLUE							"\e[34m"
#define CYAN							"\e[36m"
#define GRAY							"\e[90m"
#define GREEN							"\e[32m"
#define LGREEN							"\e[92m"
#define LRED							"\e[91m"
#define LYELLOW							"\e[93m"
#define MAGENTA							"\e[35m"
#define RED								"\e[31m"
#define RESETC							"\e[0m"
#define WHITEC							"\e[37m"
#define YELLOW							"\e[33m"

#define BK_BLACKC						"\e[40m"
#define BK_BLUE							"\e[44m"
#define BK_CYAN							"\e[46m"
#define BK_GREEN						"\e[42m"
#define BK_YELLOW						"\e[43m"
#define BK_MAGENTA						"\e[45m"
#define BK_RED							"\e[41m"
#define BK_WHITEC						"\e[47m"
#define BK_GRAY 						"\e[100m"

#endif