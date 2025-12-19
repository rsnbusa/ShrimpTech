#ifndef MAIN_GLOBALS_H
#define MAIN_GLOBALS_H

#ifdef GLOBAL
    #define EXTERN extern
#else
    #define EXTERN
#endif

#include "typedef.h"    
#include "defines.h"
const static int MQTT_BIT 				= BIT0;
const static int WIFI_BIT 				= BIT1;
const static int PUB_BIT 				= BIT2;
const static int DONE_BIT 				= BIT3;
const static int SNTP_BIT 				= BIT4;
const static int SENDMQTT_BIT			= BIT5;
const static int SENDH_BIT 				= BIT6;
const static int DISCO_BIT 				= BIT7;
const static int ERROR_BIT 				= BIT8;
const static int ERRDATE_BIT 			= BIT9;
// for othergroup
const static int REPEAT_BIT 			= BIT0;
const static int MESHRX_BIT 			= BIT1;
const static int TIMER2_BIT 			= BIT2;


EXTERN TaskHandle_t                 scheduleHandle,recoTaskHandle,mqttSendHandle,mqttMgrHandle,showHandle,configureHandle,dispHandle,oledDisp;
EXTERN esp_aes_context		        actx ;
EXTERN uint8_t                      vanTimersStart,vanTimersEnd,mqttErrors,ssignal,dia,hora,mes,MESH_ID[6],lastline,logCount;
EXTERN int16_t                      theGuard,timeSlotStart,timeSlotEnd,sentMqtt,meterCount;
EXTERN int                          lastheap,acumheap,s_retry_num,mesh_layer,BASETIMER;
EXTERN bool                         schedulef,pausef,donef,mqttf,meshf,webLogin,mesh_started,nakf,logof,okf,favf,framFlag,sendMeterf,
                                    medidorlock,hostflag,loadedf,firstheap,loginf,gdispf,wifiready,mesh_init_done,mesh_on;
EXTERN SemaphoreHandle_t 		    recoSem,flashSem,tableSem,scheduleSem,workTaskSem;
EXTERN framArgs_t                   framArg;
EXTERN dbg_t                        dbgArg;
EXTERN esp_mqtt_client_config_t 	mqtt_cfg;
EXTERN esp_mqtt_client_handle_t     clientCloud;
EXTERN QueueHandle_t 				mqttQ,mqttR,mqttSender,mqtt911,meshQueue; 
EXTERN cmdRecord 					cmds[MAXCMDS];
EXTERN config_flash                 theConf;
EXTERN nvs_handle 					nvshandle,nvshandlep;
EXTERN EventGroupHandle_t 			wifi_event_group,s_wifi_event_group,otherGroup;
EXTERN TimerHandle_t                dispTimer,beatTimer,firstTimer,collectTimer,webTimer,sendMeterTimer,reconfTimer,recconectTimer,confirmTimer,
                                    recoverTimer,start_timers[MAXHORARIOS],end_timers[MAXHORARIOS],loginTimer;
EXTERN mesh_addr_t                  s_route_table[CONFIG_MESH_ROUTE_TABLE_SIZE];
EXTERN int                          ck,ck_d,ck_h,s_route_table_size,counting_nodes,msgout;
//kbd stuff
EXTERN loglevel_t                   loglevel;
EXTERN timerl_t                     basetimer;
EXTERN prepaid_t                    prepaidcmd;
EXTERN resetconf_t                  resetlevel;
EXTERN securit_t                    kbdsedcurity,appSSID,appNode,appSkip; 
EXTERN logargs_t                    logArgs;
EXTERN blow_t                       blowArgs;
// end kbd 
EXTERN aes_en_dec_t                 endec;
EXTERN esp_netif_t*                 esp_sta; 
EXTERN mesh_addr_t                  mesh_parent_addr;    
EXTERN esp_ip4_addr_t               s_current_ip;       
EXTERN httpd_handle_t 				wserver;
EXTERN wstate_t						webState;
EXTERN char                         alarmQueue[60],gwStr[20],*tempb,iv[16],key[32],cmdQueue[60],infoQueue[60],internal_cmds[MAXINTCMDS][20],
                                    emergencyQueue[60],cmdBroadcast[60],discoQueue[60],installQueue[60],*globalDupStr;
EXTERN mesh_addr_t                  GroupID; 
EXTERN master_node_t                masterNode;
EXTERN esp_console_cmd_t            blow_cmd,fram_cmd,meter_cmd,mid_cmd,config_cmd,erase_cmd,loglevel_cmd,basetimer_cmd,prepaid_cmd,resetconf_cmd,
                                    aes_cmd,security_cmd,app_cmd,node_cmd,skip_cmd,log_cmd,adc_cmd,findunit_cmd,meshreset_cmd,debug_cmd;
EXTERN gpio_config_t 	            io_conf;
EXTERN FILE*                        myFile;
EXTERN TaskHandle_t                 adc_task_handle,blinkHandle,ssidHandle;
EXTERN int                          lastKnowCount,theAddress,wmeter;
EXTERN esp_console_repl_t           *repl;
EXTERN esp_console_repl_config_t    repl_config ;
EXTERN char                         apssid[32],appsw[10];
EXTERN uint32_t                     gCRC,TEST;
EXTERN meshp_t                      param;
EXTERN BlowerClass                  theBlower;
EXTERN lv_disp_t                   *disp;
EXTERN esp_lcd_panel_handle_t       panel_handle;
EXTERN int                          binary_file_length,meshDelay,meshMissed,resetroutet,routet;
EXTERN uint32_t                     elapsed[MAXHORARIOS];
EXTERN uint16_t                     sys_limits[18][2];              // 0 min 1 max
#endif
