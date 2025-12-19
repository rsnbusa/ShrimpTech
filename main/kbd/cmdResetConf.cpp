#ifndef TYPESrconf_H_
#define TYPESrconf_H_
#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

extern void erase_config();
extern void root_collect_meter_data(TimerHandle_t  algo);

char    resetf[10][10]={"ALL","METER","MESH","COLLECT","SEND","MQTT","CONF","STATUS","SCH","BLE"};

int findFlag(char * cual)
{
  for (int a=0;a<10;a++)
    if(strcmp(cual,resetf[a])==0)
      return a;
  
  return -1;
}


int cmdResetConf(int argc, char **argv)
{
  uint32_t pop=0;
  mqttSender_t mensaje;
  char *mqttmsg;
  mesh_data_t data;
  wifi_config_t       configsta;
  int err,lev;
  char  aca[10];

  int nerrors = arg_parse(argc, argv, (void **)&resetlevel);
  if (nerrors != 0) {
      arg_print_errors(stderr, resetlevel.end, argv[0]);
      return 0;
  }
  if (resetlevel.cflags->count) 
  {
    strcpy(aca,resetlevel.cflags->sval[0]);
    for (int x=0; x<strlen(aca); x++)
        aca[x]=toupper(aca[x]);
    lev=findFlag(aca);
    if(lev>=0)
    {
      // printf("Flags set to %s/%d\n",aca,lev);
      switch(lev) {
        case 0:
          printf("All flags resetted\n");
          theConf.meterconf=0;
          // theConf.meshconf=0;
          erase_config();
          break;
        case 1:
          printf("Blower resetted\n");
          theConf.meterconf=2;
          break;
        case 2:
          printf("Mesh resetted\n");
          // theConf.meshconf=0;
          break;
        case 3:
          printf("Collect Data\n");
          if(esp_mesh_is_root()) //only non ROOT 
              root_collect_meter_data(NULL);
            break;
        case 4:
          printf("Sendmeterf resetted\n");
          sendMeterf=0;
          break;
        case 5:
          printf("Mqttf released\n");
          mqttf=true;
          break;
        case 6:
          printf("Reconfigure mode\n");
          theConf.meterconf=3;
          write_to_flash();
          break;
        case 7:
          printf("Divisor\n");
          if(TEST==1)
            TEST=10;
          else
            TEST=1;
          return 0;
          break;
        case 8:
          printf("Start Schedule\n");
          xSemaphoreGive(workTaskSem);
          return 0;
          break;
        case 9:
          printf("Boot Ble\n");
          theConf.bleboot=0;
          write_to_flash();
          esp_restart();
          return 0;
          break;
        default:
          printf("Wrong choice of reset\n");
      }
      write_to_flash();
      esp_restart();
    }
    return 0;
  }
  else 
    printf("No such Flag\n");
  return 0;
}

#endif