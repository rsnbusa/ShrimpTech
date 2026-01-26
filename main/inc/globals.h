#ifndef MAIN_GLOBALS_H
#define MAIN_GLOBALS_H

/**
 * @file globals.h
 * @brief Global variable declarations for the ShrimpIoT project
 * 
 * This file uses the EXTERN macro pattern to declare global variables.
 * When included in the main file with GLOBAL defined, variables are defined.
 * In all other files, they are declared as extern.
 * 
 * Usage:
 *   main.c: #define GLOBAL before including this file
 *   other files: Just include this file normally
 */

#ifdef GLOBAL
    #define EXTERN extern
#else
    #define EXTERN
#endif

#include "typedef.h"
#include "defines.h"
#include "mongoose_glue.h"  // For website definitions

// ============================================================================
// EVENT BIT DEFINITIONS
// ============================================================================

// WiFi/MQTT Event Group Bits
const static int MQTT_BIT       = BIT0;
const static int WIFI_BIT       = BIT1;
const static int PUB_BIT        = BIT2;
const static int DONE_BIT       = BIT3;
const static int SNTP_BIT       = BIT4;
const static int SENDMQTT_BIT   = BIT5;
const static int SENDH_BIT      = BIT6;
const static int DISCO_BIT      = BIT7;
const static int ERROR_BIT      = BIT8;
const static int ERRDATE_BIT    = BIT9;

// Other Event Group Bits
const static int REPEAT_BIT     = BIT0;
const static int MESHRX_BIT     = BIT1;
const static int TIMER2_BIT     = BIT2;

// ============================================================================
// TASK HANDLES
// ============================================================================

EXTERN TaskHandle_t scheduleHandle;
EXTERN TaskHandle_t recoTaskHandle;
EXTERN TaskHandle_t mqttSendHandle;
EXTERN TaskHandle_t mqttMgrHandle;
EXTERN TaskHandle_t showHandle;
EXTERN TaskHandle_t configureHandle;
EXTERN TaskHandle_t dispHandle;
EXTERN TaskHandle_t oledDisp;
EXTERN TaskHandle_t adc_task_handle;
EXTERN TaskHandle_t blinkHandle;
EXTERN TaskHandle_t ssidHandle;

// ============================================================================
// SYNCHRONIZATION PRIMITIVES
// ============================================================================

// Semaphores
EXTERN SemaphoreHandle_t recoSem;
EXTERN SemaphoreHandle_t flashSem;
EXTERN SemaphoreHandle_t tableSem;
EXTERN SemaphoreHandle_t scheduleSem;
EXTERN SemaphoreHandle_t workTaskSem;

// Event Groups
EXTERN EventGroupHandle_t wifi_event_group;
EXTERN EventGroupHandle_t s_wifi_event_group;
EXTERN EventGroupHandle_t otherGroup;

// Queues
EXTERN QueueHandle_t rs485Q;
EXTERN QueueHandle_t mqttQ;
EXTERN QueueHandle_t mqttR;
EXTERN QueueHandle_t mqttSender;
EXTERN QueueHandle_t mqtt911;
EXTERN QueueHandle_t meshQueue;

// ============================================================================
// TIMER HANDLES
// ============================================================================

EXTERN TimerHandle_t dispTimer;
EXTERN TimerHandle_t beatTimer;
EXTERN TimerHandle_t firstTimer;
EXTERN TimerHandle_t collectTimer;
EXTERN TimerHandle_t webTimer;
EXTERN TimerHandle_t sendMeterTimer;
EXTERN TimerHandle_t reconfTimer;
EXTERN TimerHandle_t recconectTimer;
EXTERN TimerHandle_t confirmTimer;
EXTERN TimerHandle_t recoverTimer;
EXTERN TimerHandle_t loginTimer;
EXTERN TimerHandle_t start_timers[MAXHORARIOS];
EXTERN TimerHandle_t end_timers[MAXHORARIOS];
EXTERN TimerHandle_t scheduleTimer;

// ============================================================================
// CONFIGURATION & STORAGE
// ============================================================================

EXTERN config_flash theConf;
EXTERN nvs_handle nvshandle;
EXTERN nvs_handle nvshandlep;
EXTERN FILE *myFile;

// ============================================================================
// SYSTEM STATE FLAGS
// ============================================================================

EXTERN bool schedulef;
EXTERN bool pausef;
EXTERN bool donef;
EXTERN bool mqttf;
EXTERN bool meshf;
EXTERN bool webLogin;
EXTERN bool mesh_started;
EXTERN bool nakf;
EXTERN bool logof;
EXTERN bool okf;
EXTERN bool favf;
EXTERN bool framFlag;
EXTERN bool sendMeterf;
EXTERN bool medidorlock;
EXTERN bool hostflag;
EXTERN bool loadedf;
EXTERN bool firstheap;
EXTERN bool loginf;
EXTERN bool gdispf;
EXTERN bool wifiready;
EXTERN bool mesh_init_done;
EXTERN bool mesh_on;

// ============================================================================
// COUNTERS & STATISTICS
// ============================================================================

EXTERN uint8_t vanTimersStart;
EXTERN uint8_t vanTimersEnd;
EXTERN uint8_t mqttErrors;
EXTERN uint8_t ssignal;
EXTERN uint8_t dia;
EXTERN uint8_t hora;
EXTERN uint8_t mes;
EXTERN uint8_t MESH_ID[6];
EXTERN uint8_t lastline;
EXTERN uint8_t logCount;

EXTERN int16_t theGuard;
EXTERN int16_t timeSlotStart;
EXTERN int16_t timeSlotEnd;
EXTERN int16_t sentMqtt;
EXTERN int16_t meterCount;

EXTERN int lastheap;
EXTERN int acumheap;
EXTERN int s_retry_num;
EXTERN int mesh_layer;
EXTERN int BASETIMER;
EXTERN int ck;
EXTERN int ck_d;
EXTERN int ck_h;
EXTERN int s_route_table_size;
EXTERN int counting_nodes;
EXTERN int msgout;
EXTERN int lastKnowCount;
EXTERN int theAddress;
EXTERN int wmeter;
EXTERN int binary_file_length;
EXTERN int meshDelay;
EXTERN int meshMissed;
EXTERN int resetroutet;
EXTERN int routet;

EXTERN uint32_t gCRC;
EXTERN uint32_t TEST;
EXTERN time_t   elapsed[MAXHORARIOS];

EXTERN uint16_t sys_limits[18][2];  // [0]=min, [1]=max

// ============================================================================
// NETWORK & MESH
// ============================================================================

EXTERN esp_netif_t *esp_sta;
EXTERN mesh_addr_t mesh_parent_addr;
EXTERN mesh_addr_t s_route_table[CONFIG_MESH_ROUTE_TABLE_SIZE];
EXTERN mesh_addr_t GroupID;
EXTERN esp_ip4_addr_t s_current_ip;
EXTERN master_node_t masterNode;
EXTERN meshp_t param;
EXTERN mesh_addr_t poolNodes[MAXNODES];
EXTERN int countPoolNodes;
EXTERN poolNodes_t poolNodesTable;
// ============================================================================
// MQTT
// ============================================================================

EXTERN esp_mqtt_client_config_t mqtt_cfg;
EXTERN esp_mqtt_client_handle_t clientCloud;

// ============================================================================
// WEB SERVER
// ============================================================================

EXTERN httpd_handle_t wserver;
EXTERN wstate_t webState;

// ============================================================================
// SECURITY & ENCRYPTION
// ============================================================================

EXTERN esp_aes_context actx;
EXTERN char iv[16];
EXTERN char key[32];

// ============================================================================
// STRING BUFFERS & QUEUES
// ============================================================================

EXTERN char alarmQueue[60];
EXTERN char gwStr[20];
EXTERN char *tempb;
EXTERN char cmdQueue[60];
EXTERN char infoQueue[60];
EXTERN char limitsQueue[60];
EXTERN char internal_cmds[MAXINTCMDS][20];
EXTERN char emergencyQueue[60];
EXTERN char cmdBroadcast[60];
EXTERN char discoQueue[60];
EXTERN char installQueue[60];
EXTERN char *globalDupStr;
EXTERN char apssid[32];
EXTERN char appsw[10];

// ============================================================================
// COMMAND STRUCTURES
// ============================================================================

EXTERN cmdRecord cmds[MAXCMDS];

// ============================================================================
// CONSOLE COMMAND ARGUMENTS
// ============================================================================

EXTERN framArgs_t framArg;
EXTERN dbg_t dbgArg;
EXTERN loglevel_t loglevel;
EXTERN timerl_t basetimer;
EXTERN prepaid_t prepaidcmd;
EXTERN resetconf_t resetlevel;
EXTERN securit_t kbdsedcurity;
EXTERN securit_t appSSID;
EXTERN securit_t appNode;
EXTERN securit_t appSkip;
EXTERN logargs_t logArgs;
EXTERN blow_t blowArgs;
EXTERN cfg_t  configArgs;
EXTERN aes_en_dec_t endec;

// ============================================================================
// CONSOLE COMMAND DEFINITIONS
// ============================================================================

EXTERN esp_console_cmd_t blow_cmd;
EXTERN esp_console_cmd_t fram_cmd;
EXTERN esp_console_cmd_t meter_cmd;
EXTERN esp_console_cmd_t mid_cmd;
EXTERN esp_console_cmd_t config_cmd;
EXTERN esp_console_cmd_t erase_cmd;
EXTERN esp_console_cmd_t loglevel_cmd;
EXTERN esp_console_cmd_t basetimer_cmd;
EXTERN esp_console_cmd_t prepaid_cmd;
EXTERN esp_console_cmd_t resetconf_cmd;
EXTERN esp_console_cmd_t aes_cmd;
EXTERN esp_console_cmd_t security_cmd;
EXTERN esp_console_cmd_t app_cmd;
EXTERN esp_console_cmd_t node_cmd;
EXTERN esp_console_cmd_t skip_cmd;
EXTERN esp_console_cmd_t log_cmd;
EXTERN esp_console_cmd_t adc_cmd;
EXTERN esp_console_cmd_t findunit_cmd;
EXTERN esp_console_cmd_t meshreset_cmd;
EXTERN esp_console_cmd_t debug_cmd;

// ============================================================================
// CONSOLE REPL
// ============================================================================

EXTERN esp_console_repl_t *repl;
EXTERN esp_console_repl_config_t repl_config;

// ============================================================================
// GPIO & HARDWARE
// ============================================================================

EXTERN gpio_config_t io_conf;

// ============================================================================
// DISPLAY & UI
// ============================================================================

EXTERN BlowerClass theBlower;
EXTERN lv_disp_t *disp;
EXTERN esp_lcd_panel_handle_t panel_handle;

// ============================================================================
// MODBUS
// ============================================================================

EXTERN modbus_general_t generalParams;
EXTERN modbus_generic_t genericParams;
EXTERN modbus_rec_t modbusRecord;
EXTERN answer_t reply;
EXTERN modbus_array_t modbusArray[MAXMODBUS];
EXTERN uint16_t globalErrors;           // 8 bits, upper 4 are limits and lower 4 are errors; 16 bits variable for alignment
// ============================================================================
// SOLAR SYSTEM DATA
// ============================================================================

EXTERN struct limits milimits;  // Limits as a structure
EXTERN solarSystem_t solarSystemData;
EXTERN energy_t energyData;
EXTERN battery_t batteryData;
EXTERN pvPanel_t pvPanelData;
EXTERN sensor_t sensorData;
// EXTERN wschedule_t scheduleData;

#endif // MAIN_GLOBALS_H
