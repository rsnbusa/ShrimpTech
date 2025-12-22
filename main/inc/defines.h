
#ifdef  CONFIG_IDF_TARGET_ESP32
#define UTXD                            (25)
#define URXD                            (27)
// RTS for RS485 Half-Duplex Mode manages DE/~RE
#define URTS                            (13)
#endif 
#ifdef  CONFIG_IDF_TARGET_ESP32S3
#define UTXD                            (4)
#define URXD                            (5)
// RTS for RS485 Half-Duplex Mode manages DE/~RE
#define URTS                            (6)
#endif 
#define UCTS                            (-1)
#define ECHO_UART_PORT                  (1)

#define MBUF_SIZE                       (200)
#define BAUD_RATE                       (9600)
#define REPLYWAIT                       (800)
#define ECHO_READ_TOUT                  (3) // 3.5T * 8 = 28 ticks, TOUT=3 -> ~24..33 ticks
#define PACKET_READ_TICS                (100 / portTICK_PERIOD_MS)
#define MAXMODBUS                       (10) // sensors
// #define FRAMSPI
//FRAM pins SPI
#ifdef  CONFIG_IDF_TARGET_ESP32
#define FMOSI							(23)
#define FMISO							(19)
#define FCLK							(18)
// #define FCS								(5)
#define OLED_SDA                        (22)
#define OLED_SCL                        (21)
#define RELAY                           (14)
#define WIFILED                         (7)     // is really 4 burt need test modbus
#define BEATPIN                         (26)
// I2C Fram
#define FSDA                            FCLK        
#define FSCL                            FMOSI

//lcd 
#define PIN_NUM_SDA                     FMOSI 
#define PIN_NUM_SCL                     FCLK 

#endif
//s3 pcb design EasyEDA meterIoTPSRAMS3 pins Dic/15/2025
#ifdef  CONFIG_IDF_TARGET_ESP32S3

#define FMOSI							(01)
#define FMISO							(40)
#define FCLK							(39)
#define FCS								(38)

#define OLED_SDA                        (2)
#define OLED_SCL                        (42)
#define RELAY                           (8)
#define WIFILED                         (41)
#define BEATPIN                         (19)
// I2C Fram
#define FSDA                            FMOSI      // latest reviewed Diuc 15-2025 happy birthdat Robert!!!
#define FSCL                            FCLK

#define DISPLAY
//lcd 
#define PIN_NUM_SDA                     FMOSI 
#define PIN_NUM_SCL                     FCLK 
#endif

#define I2C_BUS_PORT                    (1)
#define LCD_PIXEL_CLOCK_HZ              (400 * 1000)

#define PIN_NUM_RST                     (-1)
#define I2C_HW_ADDR                     (0x3C)
#define LCD_H_RES                       (128)
#define LCD_V_RES                       (64)
// Bit number used to represent command and parameter
#define LCD_CMD_BITS                    8
#define LCD_PARAM_BITS                  8

#define RELAYON                         (0)
#define RELAYOFF                        (1)
#define DEBB
#define LOCALTIME                       "GMT+5"
#define LOGOPT
#define SAVEDATE                        (5)
#define FIXEDMESH                       (1)
#define STATSM
#define STATSDELAY                      (60000)
#define MAXCMDS                         (12) 

#define CENTINEL                        (0x12345678)

// #define BIASHOUR                        (14)

#define MQTTBIG                         (3000)  //big cause certiticate is at least 1700 bytes
#define MESH_TAG                        "Shrimp"
#define WIFI_CONNECTED_BIT              BIT0
#define WIFI_FAIL_BIT                   BIT1

#define MQTTRECONNECTTIME               (2000)
#define SNTPTRY                         (10)
#define FREEANDNULL(x)		            if(x) {free(x);x=NULL;}
#define MAXINTCMDS                      (18)
#define SUPERSECRET                     "mi mama me mima mucho y MepsiCia"
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
#define EMERGENCY                       (0)
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

#define DEFAULT_MESH_SSID              "NETLIFE-RSNCasa"
#define DEFAULT_MESH_PASSW             "csttpstt"
#define NORETAIN                        (0)
#define RETAIN                          (1)
#define QOS1                            (1)
#define ALL

#define SSIDBLINKTIME                   (80)

// mqtt topic names
#define MQTTCMD                         "cmd"
#define MQTTINFO                        "info"
#define MQTTALARM                       "alarm"
#define MQTTEMER                        "911"
#define MQTTBROADCAST                   "broadcast"
#define MQTTDISCO                       "answer"
#define MQTTINSTALL                     "install"

#define MESH_INT_DATA_BIN               (0x88995566)
#define MESH_INT_DATA_CJSON             (0x33221144)

#define DEFAULT_SSID                    "NETLIFE-RSNCasa"
#define DEFAULT_PSWD                    "csttpstt"

#define MAXHORARIOS                     (10)
#define MAXCICLOS                       (10)
#define MAXPROFILES                     (2)
#define MAXDAYCYCLE                     (100)

#define PROFILE_FILE                    "/spiffs/profilejson.txt"
#define LOG_FILE                        "/spiffs/log.txt"

#define BLE_MODE                        (0)
#define MESH_MODE                       (1)

#define SENDMUX                         (2)
#define LOGINTIME                       (60000)         //1 minute

#define DBG_SCH						    "\e[36m[SCH]\e[0m"               //blue
#define DBG_MESH					    "\e[35m[MESH]\e[0m"              // Magenta
#define DBG_BLE						    "\e[32m[BLE]\e[0m]"              //Green
#define DBG_MQTT					    "\e[33m[MQTT]\e[0m"              //Yellow
#define DBG_XCMDS					    "\e[37m[XCMDS]\e[0m"                //White
// #define UNKNOWSTATE						"\e[31m[UNK]\e[0m"                  //Red
// #define OFFLINESTATE    				"\e[37m[OFFLINE]\e[0m"              //White
// #define SYSTEM          				"\e[90;3;4m\t[SYSTEM]\e[0m"         //Gray
// #define KBD          				    "\e[31m[KBD]\e[0m"                  //Red
// #define WEB          				    "\e[33;1m[APP]\e[0m"                  //Yellow


// #define BLACKC							"\e[30m"
// #define BLUE							"\e[34m"
// #define CYAN							"\e[36m"
// #define GRAY							"\e[90m"
// #define GREEN							"\e[32m"
// #define LGREEN							"\e[92m"
// #define LRED							"\e[91m"
// #define LYELLOW							"\e[93m"
// #define MAGENTA							"\e[35m"
// #define RED								"\e[31m"
// #define RESETC							"\e[0m"
// #define WHITEC							"\e[37m"
// #define YELLOW							"\e[33m"
