#ifndef TYPESprod_H_
#define TYPESprod_H_
#define GLOBAL
#include "globals.h"
#include "forwards.h"
extern void writeLog(char *que);
extern void save_ota_version(char *version);

void send_start_production(uint8_t active_profile, uint8_t curr_day, uint8_t mux_time,char* theorder)
{
    cJSON *root=cJSON_CreateObject();
    if(root)
    {
        cJSON_AddStringToObject(root,"cmd",internal_cmds[PRODUCTION]);
        cJSON_AddNumberToObject(root,"prof",active_profile);
        cJSON_AddNumberToObject(root,"day",curr_day);
        cJSON_AddNumberToObject(root,"mux",mux_time);
        cJSON_AddStringToObject(root,"order",theorder);
        char *mensaje=cJSON_PrintUnformatted(root);
        if(!mensaje)
        {
            ESP_LOGE(MESH_TAG,"Start Production failed create unformatted msg");
            cJSON_Delete(root);
        }
        if(root_mesh_broadcast_msg(mensaje)!=ESP_OK)
            ESP_LOGE(MESH_TAG,"Start Production Msg not sent");
        free(mensaje);
        cJSON_Delete(root);
    }
    else
    {
        ESP_LOGE(MESH_TAG,"Could not create root Start Prodcution");
    }
}

int cmdProd(void *argument)
{
    cJSON               *root,*cualm,*lstate,*elcmd;
    mesh_data_t         data;
    int                 err,cualorden;
    mqttSender_t        meshMsg;
    char                laorden[10],*log_msg;
    /* Msg json format
    {"cmd":"prod","prof":1,"day":1,"mux":30,"start":0}
    */
    if((theConf.debug_flags >> dXCMDS) & 1U)  
        ESP_LOGI(MESH_TAG, "%sCMd Prod",DBG_XCMDS);
    elcmd=(cJSON *)argument;
    if(elcmd)
    {
        cJSON *prof= 		    cJSON_GetObjectItem(elcmd,"prof");       
        cJSON *pday= 		    cJSON_GetObjectItem(elcmd,"day");       
        cJSON *pmux= 		    cJSON_GetObjectItem(elcmd,"mux");       
        cJSON *order= 		    cJSON_GetObjectItem(elcmd,"order");     
        if(prof && pday && pmux && order)
        {
            // sanity checks
            uint8_t iprof,iday,imux; 
            iprof=prof->valueint-1;
            iday=pday->valueint-1;
            imux=pmux->valueint;
            if(iprof<MAXPROFILES && iday<MAXDAYCYCLE)
            {
                theConf.dayCycle=iday;
                theConf.activeProfile=iprof;
                theConf.test_timer_div=imux;
                write_to_flash();
                // should send mnsg to all stations broadcast to start production with these parameters
                // and start your self
                // which orer Start, Stop, Pause-Resume
                strcpy(laorden,order->valuestring);
                if (strcmp(laorden,"start")==0)
                    cualorden=0;
                if (strcmp(laorden,"stop")==0)
                    cualorden=1;
                if (strcmp(laorden,"pause")==0)
                    cualorden=2;
                if (strcmp(laorden,"resume")==0)
                    cualorden=3;
                if (cualorden<0)
                    return ESP_OK;
                char * log_msg=(char*)calloc(200,1);
                if(!log_msg)
                {
                    ESP_LOGE(TAG,"No Ram CmdProd log msg");
                    return ESP_FAIL;
                }
                if(cualorden==0)
                {   
                    if(schedulef)
                    {
                        if((theConf.debug_flags >> dXCMDS) & 1U)  
                            ESP_LOGI(MESH_TAG,"%sCMd Prod again Start %s",DBG_XCMDS,laorden);
                            return ESP_OK;
                    }
                    sprintf(log_msg,"CMd Prod Start %s",laorden);
                    if((theConf.debug_flags >> dXCMDS) & 1U)  
                        ESP_LOGI(MESH_TAG,"%sCMd Prod Start %s",DBG_XCMDS,laorden);
                    writeLog(log_msg);
                    free(log_msg);
                    theConf.blower_mode=1;
                    write_to_flash(); 
                    xSemaphoreGive(workTaskSem);
                // send start production to children
                    send_start_production(iprof,iday,theConf.test_timer_div,laorden);
                }
                if(cualorden==1)
                {
                    // stop 
                    // reset values
                    if(!schedulef)
                    {
                        if((theConf.debug_flags >> dXCMDS) & 1U)  
                            ESP_LOGI(MESH_TAG,"%sCMd Prod Stop and not scheduling %s",DBG_XCMDS,laorden);
                            return ESP_OK;
                    }
                    theConf.dayCycle=0;
                    theConf.blower_mode=0;
                    write_to_flash();
                    sprintf(log_msg,"CMd Prod Stop %s",laorden);
                    writeLog(log_msg);
                    free(log_msg);
                    if((theConf.debug_flags >> dXCMDS) & 1U)  
                        ESP_LOGI(MESH_TAG, "%sCMd Prod Stop %s",DBG_XCMDS,laorden);
                    vTaskDelete(scheduleHandle);    //task killed
                    xTaskCreate(&start_schedule_timers,"sched",1024*10,NULL, 5, &scheduleHandle); 	       
                    // send broadcast to all to stop
                    send_start_production(iprof,iday,theConf.test_timer_div,laorden);
                    schedulef=false;
                    return ESP_OK;
                }
                if(cualorden==2)
                {
                    // puase 
                    // reset values
                    if (!schedulef) 
                    {
                        if((theConf.debug_flags >> dXCMDS) & 1U)  
                            ESP_LOGI(MESH_TAG,"%sCMd Prod Pause and not scheduling %s",DBG_XCMDS,laorden);
                        return ESP_OK;
                    }
                    if((theConf.debug_flags >> dXCMDS) & 1U)  
                        ESP_LOGI(MESH_TAG, "%sCMd Prod Paused %s",DBG_XCMDS,laorden);
                    sprintf(log_msg,"CMd Prod Pause %s",laorden);
                    writeLog(log_msg);
                    send_start_production(iprof,iday,theConf.test_timer_div,laorden);
                    pausef=true;
                    // send broadcast to all to stop
                    free(log_msg);
                    return ESP_OK;
                }
                if(cualorden==3)
                {
                    // resume 
                    // reset values
                    if (!schedulef) 
                    {
                        if((theConf.debug_flags >> dXCMDS) & 1U)  
                            ESP_LOGI(MESH_TAG,"%sCMd Prod Resume and not scheduling %s",DBG_XCMDS,laorden);
                        return ESP_OK;
                    }
                    if((theConf.debug_flags >> dXCMDS) & 1U)  
                        ESP_LOGI(MESH_TAG, "%sCMd Prod Resumed %s",DBG_XCMDS,laorden);
                    // send broadcast to all to stop
                    sprintf(log_msg,"CMd Prod Resume %s",laorden);
                    pausef=false;
                    send_start_production(iprof,iday,theConf.test_timer_div,laorden);
                    writeLog(log_msg);
                    free(log_msg);
                    return ESP_OK;
                }
            }
        }
        else
        {
            ESP_LOGE(MESH_TAG,"Start Production Cmd parameter missing or Task already started");
            return ESP_FAIL;
        }
        return ESP_OK;
    }
    else 
    {
       ESP_LOGE(MESH_TAG,"NO CMD argument passed. Internal error");
    }
    return ESP_FAIL;
}
#endif