#ifndef TYPESmqtt1_H_
#define TYPESmqtt1_H_
#define GLOBAL
#include "includes.h" 
#include "globals.h"
extern void write_to_flash();
extern void writeLog(char* que);
extern void delay(uint32_t cuanto);

int cmdMQTT(void *argument)
{

    ESP_LOGW(MESH_TAG,"Cmd MQTT in force!!!");
    cJSON *elcmd=(cJSON *)argument;

/*
{"cmd":"mqtt","f":"000000","passw":"xxxx","server":"xxxxxxx","userm":"cccccc","passm":"xxxxxx","cert":"at leat 1800 bytes"}

*/
    if(elcmd)
    {
        cJSON *server= 		cJSON_GetObjectItem(elcmd,"server");
        cJSON *users= 	    cJSON_GetObjectItem(elcmd,"userm");
        cJSON *passw= 		cJSON_GetObjectItem(elcmd,"passm");
        cJSON *cert= 		cJSON_GetObjectItem(elcmd,"cert");
        cJSON *ver= 		cJSON_GetObjectItem(elcmd,"ver");       //version to set possible OTA
            
        if(server && users && passw && cert)
        {
            if(strlen(server->valuestring)>20)
            {
                if(true)
                // if(strcmp(theConf.mqttServer,server->valuestring)!=0)
                {
                    if(strlen(users->valuestring)>0)
                    {
                        if(strlen(passw->valuestring)>0)
                        {
                            bzero(theConf.mqttcert,sizeof(theConf.mqttcert));
                            bzero(theConf.mqttServer,sizeof(theConf.mqttServer));
                            bzero(theConf.mqttUser,sizeof(theConf.mqttUser));
                            bzero(theConf.mqttPass,sizeof(theConf.mqttPass));
                            strcpy(theConf.mqttServer,server->valuestring);
                            strcpy(theConf.mqttUser,users->valuestring);
                            strcpy(theConf.mqttPass,passw->valuestring);
                            if(strlen(cert->valuestring)>0)
                            {
                                strcpy(theConf.mqttcert,cert->valuestring);
                                theConf.mqttcert[strlen(theConf.mqttcert)]=0xa;
                                theConf.mqttcertlen=strlen(theConf.mqttcert)+1;
                            }
                            write_to_flash();
                            ESP_LOGW(MESH_TAG,"Mqtt Server parameters changed");
                            char *tmp=(char*)calloc(100,1);
                            if(tmp)
                            {
                                sprintf(tmp,"MQTT Command Server %s",theConf.mqttServer);
                                writeLog(tmp);
                                free(tmp);
                            }
                            char *aca=(char*)malloc(100);
                            sprintf(aca,"MQTT installed %s",server->valuestring);
                            writeLog(aca);
                            free(aca);
                            delay(1000);
                            esp_restart();
                        }
                        else 
                            ESP_LOGW(MESH_TAG,"Mqtt bad password length");
                    }
                    else 
                        ESP_LOGW(MESH_TAG,"Mqtt bad User parameter");
                }
                else 
                    ESP_LOGW(MESH_TAG,"Mqtt server same. No update");
            }
            else 
                ESP_LOGW(MESH_TAG,"Mqtt Server parameter");
        }
        else 
            ESP_LOGW(MESH_TAG,"Mqtt Server invalid parameters");
    }
    else
        return ESP_FAIL;
    return ESP_OK;
}
#endif
