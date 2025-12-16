#ifndef TYPESlogs_H_
#define TYPESlogs_H_
#define GLOBAL
#include "includes.h" 
#include "globals.h"
extern void writeLog(char* que);

int cmdLogs(void *argument)
{
    char *logs,*copystart;
    int vamos=0,totalsize=0,cuanto;
    mqttSender_t        mqttMsg;
    char    linea[200];

    if(!esp_mesh_is_root())
        return ESP_OK;

    cJSON *cmd=(cJSON *)argument;
    ESP_LOGI(MESH_TAG,"Log CMD");
/*
{"cmd":"log","f":"xxxxxx","mid";"thismeter","lines":nnn,"erase":"y/n"}   //thismeter for the timje is irrelevant only root node will answer this
*/
    if(cmd)
    {
        cJSON *lines= 		cJSON_GetObjectItem(cmd,"lines");
        cJSON *erase= 		cJSON_GetObjectItem(cmd,"erase");
        if(lines)
        {
            // printf("Start reading %d lines\n",lines->valueint);
            fclose(myFile);
            myFile= fopen("/spiffs/log.txt", "r");
            cuanto=lines->valueint;
            totalsize=lines->valueint*100;
            logs=(char*)calloc(1,totalsize);      //hopefully we have that ram
            if(logs)
            {   //we do
                // printf("Ram allocated logs %d\n",totalsize);
                copystart=logs;         //origen of ram block
                while(cuanto>0)
                {
                    if( fgets (linea, sizeof(linea), myFile)!=NULL ) 
                    {
                        // printf("Line %s\n",linea);
                        if((vamos+strlen(linea))<totalsize)
                        {
                            memcpy(logs,linea,strlen(linea));
                            logs+=strlen(linea);
                            cuanto--;
                            vamos+=strlen(linea);
                        }
                    }
                    else
                        break;
                }
                if(vamos>0)
                {
                    // send it
                    // printf("Log size to send %d lines %d\n",vamos,lines->valueint);

                    mqttMsg.queue=                      discoQueue;
                    mqttMsg.msg=                        (char*)copystart;
                    mqttMsg.lenMsg=                     vamos;
                    mqttMsg.code=                       NULL;
                    mqttMsg.param=                      NULL;
                    if(xQueueSend(mqttSender,&mqttMsg,0)!=pdTRUE)       
                    {
                        ESP_LOGE(MESH_TAG,"Error queueing msg logs");
                        if(mqttMsg.msg)
                            free(mqttMsg.msg);                           //due to failure
                        return ESP_FAIL;
                    }
                    fclose(myFile);
                    myFile= fopen("/spiffs/log.txt", "a");
                    if (myFile == NULL) 
                        ESP_LOGE(MESH_TAG,"Failed to open file for append");
                    return 0;
                }
                    
            }
        }


        if (erase) 
        {
            int borrar=strcmp(erase->valuestring,"y");
            if(borrar==0)
            {
                ESP_LOGI(MESH_TAG,"Erase Log file cmd");
                fclose(myFile);
                remove("/spiffs/log.txt");
                myFile= fopen("/spiffs/log.txt", "a");
                if (myFile == NULL) 
                {
                    ESP_LOGE(MESH_TAG,"Failed to open file for append erase");
                }
            }
        }

        return ESP_OK;
    }
    else
        return ESP_FAIL;
}
#endif