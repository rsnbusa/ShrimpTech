#ifndef TYPESrest_H_
#define TYPESrestH_
#define GLOBAL
#include "includes.h" 
#include "globals.h"
extern void writeLog(char* que);
extern void write_to_flash();
extern int findFlag(char * cual);
extern void erase_config();


int cmdReset(void *argument)
{
    char aca[10];
    int lev=-1;
     cJSON *cmd=(cJSON *)argument;
     ESP_LOGI(MESH_TAG,"Reset CMD");
/*
{"cmd":"reset","f":"xxxxxx","option":'ALL","METER","MESH","COLLECT","SEND","MQTT","CONF","STATUS"',"cha":"xxxx"}
*/
    if(cmd)
    {
        cJSON *menu= 		cJSON_GetObjectItem(cmd,"option");
        if(menu)
        {
            if(strlen(menu->valuestring)>0)
            {
                strcpy(aca,menu->valuestring);
                for (int x=0; x<strlen(aca); x++)
                    aca[x]=toupper(aca[x]);
                lev=findFlag(aca);
                if(lev>=0)
                {
                    switch(lev) 
                    {
                        case 0:
                            writeLog("All flags resetted\n");
                            erase_config();
                            esp_restart();
                            break;
                        case 1:
                            writeLog("Meter resetted\n");
                            theConf.meterconf=0;
                            write_to_flash();
                            esp_restart();
                            break;
                        case 2:
                            writeLog("Mesh resetted\n");
                            // theConf.meshconf=0;
                            write_to_flash();
                            esp_restart();
                            break;
                        case 3:
                            break;
                        case 4:
                            break;
                        case 5:
                            break;
                        case 6:
                            writeLog("Reconfigure mode set\n");
                            theConf.meterconf=3;
                            write_to_flash();
                            esp_restart();
                            break;
                        case 7:
                            writeLog("Configuration mode set\n");
                            theConf.meterconf=4;
                            write_to_flash();
                            esp_restart();
                            break;
                        default:
                        writeLog("Reset Cmd Wrong choice of reset\n");
                    }

                }
        }
        return ESP_OK;
    }
    else
        return ESP_FAIL;
    }
}
#endif