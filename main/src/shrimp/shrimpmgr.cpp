#include "includes.h"
#include "defines.h"
#include "globals.h"
#include "forwards.h"
#include "crypto_utils.h"
#include "time_utils.h"
#include "led_utils.h"
#include "display_utils.h"
/**
 * ? que sera
 * ! warning
 * * Important
 * TODO: algo
 * @param xTimer este parametro
 */

extern const uint8_t cert_start[]           asm("_binary_cloudamp_pem_start");
extern const uint8_t cert_end[]             asm("_binary_cloudamp_pem_end");

// extern const uint8_t cert_start[]           asm("_binary_cert_pem_start");
// extern const uint8_t cert_end[]             asm("_binary_cert_pem_end");

modbus_sensor_type_t * setModbusSensor(char * sensor_name,int numberDescriptors, int numColumns
            ,void *descriptors,char * colores,void * theData,int dataSize,printcb printer)
{
            modbus_sensor_type_t *theSensor=(modbus_sensor_type_t *)calloc(1,sizeof(modbus_sensor_type_t));
            if (theSensor)
            {
                theSensor->modbus_sensor_name=sensor_name;
                theSensor->modbus_sensor_spec_count=numberDescriptors;
                theSensor->modbus_sensor_spec_columns=numColumns;
                theSensor->modbus_sensor_specs=descriptors;
                theSensor->modbus_sensor_data=theData;
                theSensor->color=(char*)colores;
                theSensor->modbus_print_function=printer;
                theSensor->modbus_sensor_data_size=dataSize;
            }
            return theSensor;       //null or filled
}

void launch_sensors()
{

            // Battery Modbus Device
            modbus_sensor_type_t *battery=setModbusSensor((char*)"Battery",4,4,
                (void*)&theConf.modbus_battery,GRAY,(void*)&batteryData,sizeof(batteryData),&print_battery_data);

            // Panels Modbus Device
            modbus_sensor_type_t *panels=setModbusSensor((char*)"Panels",5,4,
                (void*)&theConf.modbus_panels,LYELLOW,(void*)&pvPanelData,sizeof(pvPanelData),&print_panel_data);

            // Energy Modbus Device
            modbus_sensor_type_t *energy=setModbusSensor((char*)"Energy",10,4,
                (void*)&theConf.modbus_inverter,MAGENTA,(void*)&energyData,sizeof(energyData),&print_energy_data);

            // Sensors Modbus Device
            modbus_sensor_type_t *sensorDev=setModbusSensor((char*)"Sensors",5,5,
                (void*)&theConf.modbus_sensors,CYAN,(void*)&sensorData,sizeof(sensorData),&print_sensor_data);

            if(battery)
                xTaskCreate(&generic_modbus_task,battery->modbus_sensor_name,1024*4,(void*)battery, 5, NULL); 
            else
                ESP_LOGE(TAG, "Failed to create Battery modbus sensor task due to memory allocation failure"); 

            if(panels)  
                xTaskCreate(&generic_modbus_task,panels->modbus_sensor_name,1024*4,(void*)panels, 5, NULL); 
            else
                ESP_LOGE(TAG, "Failed to create Panels modbus sensor task due to memory allocation failure"); 

            if(energy)
                xTaskCreate(&generic_modbus_task,energy->modbus_sensor_name,1024*4,(void*)energy, 5, NULL); 
            else
                ESP_LOGE(TAG, "Failed to create Energy modbus sensor task due to memory allocation failure"); 

            if(sensorDev)
                xTaskCreate(&generic_modbus_task,sensorDev->modbus_sensor_name,1024*4,(void*)sensorDev, 5, NULL); 
            else
                ESP_LOGE(TAG, "Failed to create Sensors modbus sensor task due to memory allocation failure"); 
}

void print_blower(char * title,solarSystem_t *msolar,bool dumphex)
{
    if((theConf.debug_flags >> dBLOW) & 1U)  
    {
        printf("From %s\n",title);
        if(dumphex)
            esp_log_buffer_hex("SOL",msolar,sizeof(solarSystem_t));
        printf("Charge Curr %d pv1V %.02f pv2V %.02f pv1Amp %.02f pv2Amp %.02f\n",msolar->pvPanel.chargeCurr,msolar->pvPanel.pv1Volts,msolar->pvPanel.pv2Volts,msolar->pvPanel.pv1Amp,msolar->pvPanel.pv2Amp); 
        printf("Bat SOC %d SOH %d Cycles %d Temp %.02f \n",msolar->battery.batSOC,msolar->battery.batSOH,msolar->battery.batteryCycleCount,msolar->battery.batBmsTemp); 
        printf("Enery ChgAHToday %d DischAHToday %d ChgAHTotal %d DiscAHTotal %d GenToday %.02f UsedToday %.02f \
        LoadConsdumeTotal %.02f \
        chgKWHToday %.02f dischKWHToday %.02f \
        LoadConsumeToday %.02f\n",msolar->energy.batChgAHToday,msolar->energy.batDischgAHToday,
        msolar->energy.batChgAHTotal,msolar->energy.batDischgAHTotal,
        msolar->energy.generateEnergyToday,msolar->energy.usedEnergyToday,
        msolar->energy.gLoadConsumLineTotal,
        msolar->energy.batChgkWhToday,msolar->energy.batDischgkWhToday,msolar->energy.genLoadConsumToday);
        printf("Sensors DO %.02f PH %.02f WTemp %.02f ATemp %.02f AHum %.02f\n",msolar->sensors.DO,msolar->sensors.PH,msolar->sensors.WTemp,msolar->sensors.ATemp,msolar->sensors.AHum);    
    }
}


void show_lvgl(void *showLVGL)
{
    show_lvgl_t                  *msg=(show_lvgl_t *)showLVGL;
	static _lock_t               lvgl_api_lock;
    if(msg)
    {
        // printf("lvgl clean\n");
        lv_obj_clean(lv_scr_act());     //must be first call...not documented unreliable api
        // printf("lvgl show\n");
        lv_obj_t *scr = lv_display_get_screen_active(disp);
        lv_obj_t *label = lv_label_create(scr);
        lv_style_t style;
        esp_lcd_panel_disp_on_off(panel_handle, true);     //show display
        if(scr && label)
        {
            _lock_acquire(&lvgl_api_lock);
            delay(10);
            lv_style_init(&style);
            switch(msg->bigf)
            {
                case 0:
                    lv_style_set_text_font(&style, &lv_font_montserrat_14);
                    break;
                case 1:
                    lv_style_set_text_font(&style, &lv_font_montserrat_18); 
                    break;
                case 2:
                    // lv_style_set_text_font(&style, &lv_font_montserrat_22); 
                    break;
                case 3:
                    lv_style_set_text_font(&style, &lv_font_montserrat_30); 
                    break;
                default:
                    lv_style_set_text_font(&style, &lv_font_montserrat_14);
            }

            lv_label_cut_text(label,0,20);
            lv_obj_add_style(label, &style, 0); 
            char *tmp=(char*)calloc(1,100);
            if(tmp)
            {
                sprintf(tmp,"%s",msg->msg);
                lv_label_set_text(label,tmp);
                free(tmp);
            }
            else
            {
                ESP_LOGE("LVGL","Failed to allocate memory for label text");
            }
            lv_obj_align(label,LV_ALIGN_CENTER,0,0);
            _lock_release(&lvgl_api_lock);
            delay(msg->wait);
            if(msg->msg)
                free(msg->msg);
            free(msg);
            esp_lcd_panel_disp_on_off(panel_handle, false);

        }
        else
            printf("Scr or Label failed\n");
    }

    oledDisp=NULL;
    vTaskDelete(NULL);
}

void showLVGL(char *que, uint32_t cuanto,uint8_t bigf)
{
    show_lvgl_t                  *themsg;
    if(gdispf)
    {
        if(oledDisp)
        {
            vTaskDelete(oledDisp);
            oledDisp=NULL;
        }

        if( que)
        {
            char * text=(char*)calloc(1,strlen(que)+10);
            if(text)
            {
                themsg=(show_lvgl_t*)calloc(1,sizeof(show_lvgl_t));
                if(themsg)
                {
                    strcpy(text,que);
                    themsg->msg=text;
                    themsg->wait=cuanto;
                    themsg->bigf=bigf;
                    xTaskCreate(&show_lvgl,"sdata",1024*20,(void*)themsg, 10, &oledDisp); 
                }
                else
                {
                    ESP_LOGE("LVGL","Failed to allocate show_lvgl_t structure");
                    free(text);  // Free text if themsg allocation fails
                }
            }
            else
            {
                ESP_LOGE("LVGL","Failed to allocate memory for text");
            }
        } 
    }
    // else
    //     ESP_LOGI(MESH_TAG,"Showlvgl no disp\n");
}

// AES encryption/decryption functions moved to crypto_utils.c
// delay() and xmillis() functions moved to time_utils.c
// blinkRoot() and blinkConf() functions moved to led_utils.cpp

// uint32_t xmillisFromISR()
// {
// 	return pdTICKS_TO_MS(xTaskGetTickCountFromISR());
// }

/**
 * @brief Find the index of an internal command by name
 * 
 * @param cual Command name to search for
 * @return int Index of the command in internal_cmds array, ESP_FAIL if not found
 */
static int findInternalCmds(const char * cual)
{
	for (int a=0;a<MAXINTCMDS;a++)
	{
		if(strcmp(internal_cmds[a],cual)==0)
			return a;
	}
	return ESP_FAIL;
}

// LED blink functions moved to led_utils.cpp

/**
 * @brief Get mesh routing table and store in masterNode
 * 
 * Retrieves the current mesh network routing table from ESP-MESH,
 * updates node count statistics, and prepares for metric collection.
 * Thread-safe using tableSem semaphore.
 * 
 * @return int ESP_OK on success, ESP_FAIL on error
 * @note Clears previous routing data before fetching new table
 */
int get_routing_table()
{
    portMUX_TYPE    xTimerLock = portMUX_INITIALIZER_UNLOCKED;
    int             err;

    //some checks
    //new round of metrics, in case not already done, erase ram... shouldnt happend
    if(xSemaphoreTake(tableSem, portMAX_DELAY))		
    {  
        for (int a=0;a<masterNode.existing_nodes;a++)
        {
            if(masterNode.theTable.thedata[a])
            {
                ESP_LOGW(MESH_TAG,"Free RAM in get route %d\n",a);
                free(masterNode.theTable.thedata[a]);
                masterNode.theTable.thedata[a]=NULL;
            }
        }

        // if skip is implemented can not zero this
        bzero(&masterNode,sizeof(masterNode));  //always zero this stuff
        // taskENTER_CRITICAL( &xTimerLock );
        err=esp_mesh_get_routing_table((mesh_addr_t *) &masterNode.theTable.big_table,MAXNODES * 6, &masterNode.existing_nodes);
        // taskEXIT_CRITICAL( &xTimerLock );

        counting_nodes=masterNode.existing_nodes;  //copy for counting purposes
        // for (int a=0;a<counting_nodes;a++) 
        // {
        //     masterNode.theTable.thedata[a]=NULL;       //no data yet
        //     masterNode.theTable.sendit[a]=true;        //default send it
        //     masterNode.theTable.skipcounter[a]=0;      
        //     masterNode.theTable.lastkwh[a]=0;
        // }
        time_t  now;
        theBlower.setStatsLastNodeCount(masterNode.existing_nodes);
        time(&now);
        theBlower.setStatsLastCountTS(now);
        xSemaphoreGive(tableSem);
        if(err)
            return ESP_FAIL;
        else
            return ESP_OK;
    }  
    return ESP_FAIL;
}

/**
 * @brief Delete and free mesh routing table data
 * 
 * Frees all allocated memory for routing table node data and resets
 * the node counter. Thread-safe using tableSem semaphore.
 * 
 * @return int ESP_OK on success, ESP_FAIL on error
 */
int root_delete_routing_table()
{
    int err;
    if(xSemaphoreTake(tableSem, portMAX_DELAY))		
    {  
        for (int a=0;a<masterNode.existing_nodes;a++)
        {
            if(masterNode.theTable.thedata[a])
            {
                free(masterNode.theTable.thedata[a]);       //free ram
                masterNode.theTable.thedata[a]=NULL;
            }
        }
        bzero(&masterNode,sizeof(masterNode));          // was commented out now active but might cause probloems but in theory it should be done every sendmetrics    
        counting_nodes=0;                                   
        xSemaphoreGive(tableSem);
        return ESP_OK;
    }
    return ESP_FAIL;
}

//ladata is  meshunion_t * myNode
int root_load_routing_table_mac(mesh_addr_t *who,void *ladata)
{
    int err;
    meshunion_t* aNode=(meshunion_t*)ladata;
    print_blower("load routing", &aNode->nodedata.solarData.solarSystem,true);
    if(xSemaphoreTake(tableSem, portMAX_DELAY))		
    {  
        for (int a=0;a<masterNode.existing_nodes;a++)
        {
            if (MAC_ADDR_EQUAL(masterNode.theTable.big_table[a].addr, who->addr)) 
            {
                if(masterNode.theTable.thedata[a])
                    free(masterNode.theTable.thedata[a]);           //erase old data
                masterNode.theTable.thedata[a]=ladata;              //new pointer
                masterNode.theTable.sendit[a]=true;     //default send it
                xSemaphoreGive(tableSem);
                return ESP_OK;
            }
            // else
            //     ESP_LOGE(TAG,"Check mac %d " MACSTR " vs " MACSTR ,a,MAC2STR(masterNode.theTable.big_table[a].addr),MAC2STR(who->addr));
        }
        xSemaphoreGive(tableSem);
        ESP_LOGE(MESH_TAG,"Did not find mac addr " MACSTR ,who->addr);
        return ESP_FAIL;
    }   //else is not really necessary I guess
}

/**
 * @brief Send lock confirmation message via mesh network
 * 
 * Creates and queues a JSON confirmation message for a lock/unlock command.
 * The message includes the meter ID and new lock state.
 * 
 * @param mid Meter ID string
 * @param lstate Lock state (1=locked, 0=unlocked)
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 * @note Message memory is managed by the mesh queue
 */
esp_err_t send_confirmation_message(char *mid, int lstate)
{
    int                 err;
    mesh_data_t         datas;
    cJSON               *root;
    mqttSender_t        meshMsg;


        root=cJSON_CreateObject();
        if(root)
        {
            cJSON_AddStringToObject(root,"cmd",internal_cmds[CONFIRMLOCK]);
            cJSON_AddStringToObject(root,"mid",mid);
            cJSON_AddNumberToObject(root,"state",lstate);

            char *intmsg=cJSON_PrintUnformatted(root);
            if(intmsg)
            {  
                meshMsg.msg=intmsg;
                meshMsg.lenMsg=strlen(intmsg);
                meshMsg.queue=NULL;
                meshMsg.addr=NULL;
                if(xQueueSend(meshQueue,&meshMsg,0)!=pdTRUE)      //will free todo 
                {
                    ESP_LOGE(MESH_TAG,"Error queueing confirm msg");
                    if(meshMsg.msg)         //already checked but ....
                        free(meshMsg.msg);  //due to failure
                    cJSON_Delete(root); 
                    return ESP_FAIL;
                }
                ESP_LOGI(MESH_TAG,"Confirmation sent for Meter [%s]",mid);
            }
            else
            {
                ESP_LOGE(MESH_TAG,"No RAM Confirm");
                cJSON_Delete(root); 
                return ESP_FAIL;
            }
            cJSON_Delete(root); 
            return ESP_OK;
        }
        else
            ESP_LOGE(MESH_TAG,"No Root Confirm");

        return ESP_FAIL;
}

/**
 * @brief Send device metrics response message
 * 
 * Creates and queues a JSON message containing device metrics including
 * firmware version and last update timestamp.
 * 
 * @param mid Meter ID string
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 * @note Message is sent to discoQueue for discovery responses
 */
esp_err_t send_metrics_message(char *mid)
{
    int                 err;
    mesh_data_t         datas;
    cJSON               *root;
    mqttSender_t        meshMsg;
    char                *buff;

        root=cJSON_CreateObject();
        if(root)
        {
            const esp_app_desc_t *mip=esp_app_get_description();
            if(mip)
                cJSON_AddStringToObject(root,"version",mip->version);
            cJSON_AddStringToObject(root,"cmd",internal_cmds[METRICRESP]);
            cJSON_AddStringToObject(root,"mid",mid);
            // cJSON_AddNumberToObject(root,"kwh",theBlower.getLkwh());
            // cJSON_AddNumberToObject(root,"beat",theBlower.getBeats());
            // cJSON_AddNumberToObject(root,"maxamp",theBlower.getMaxamp());
            // cJSON_AddNumberToObject(root,"minamp",theBlower.getMinamp());

            buff=(char*)calloc(30,1);
            if(buff)
            {
                time_t lastup=theBlower.getLastUpdate();
                strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&lastup));
                cJSON_AddStringToObject(root,"update",buff);
                free(buff);
            }

            char *intmsg=cJSON_PrintUnformatted(root);
            if(intmsg)
            {  
                meshMsg.msg=intmsg;
                meshMsg.lenMsg=strlen(intmsg);
                meshMsg.queue=discoQueue;
                meshMsg.addr=NULL;
                if(xQueueSend(meshQueue,&meshMsg,0)!=pdTRUE)      //will free todo 
                {
                    ESP_LOGE(MESH_TAG,"Error queueing confirm msg");
                    if(meshMsg.msg)                 // already checked but ...
                        free(meshMsg.msg);          //due to failure
                    cJSON_Delete(root); 
                    return ESP_FAIL;
                }
                ESP_LOGI(MESH_TAG,"Confirmation sent for Meter [%s]",mid);
            }
            else
            {
                ESP_LOGE(MESH_TAG,"No RAM Send Metrics");
                cJSON_Delete(root); 
                return ESP_FAIL;
            }
            cJSON_Delete(root); 
            return ESP_OK;
        }
        else
            ESP_LOGE(MESH_TAG,"No Root Metrics");

        return ESP_FAIL;
}

int  turn_display(char *cmd)
{
    cJSON               *cualm,*ltime,*elcmd,*ver;

    if(!gdispf)
        return ESP_FAIL;
    //text to json
    elcmd=cJSON_Parse(cmd);
    if(elcmd)
    {
        cualm= 		cJSON_GetObjectItem(elcmd,"mid");



        if(cualm)
        {
           if(true)
        //    if(strcmp(cualm->valuestring,theBlower.getMID())==0)
           {
                ltime= 		cJSON_GetObjectItem(elcmd,"time");
                if(!ltime)
                {
                    ESP_LOGE(MESH_TAG,"Display No state time");
                    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, false));
                    cJSON_free(elcmd);
                    return ESP_FAIL;
                }

                int duration=ltime->valueint*1000;

                ESP_LOGW(MESH_TAG,"Display MId [%s me] Time [%d]",cualm->valuestring,ltime->valueint);
                if(!showHandle && duration>0)     //is it not active
                {
                    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
                    xTaskCreate(&showData,"sdata",1024*3,NULL, 5, &showHandle); 
                    dispTimer=xTimerCreate("DispT",pdMS_TO_TICKS(duration),pdFALSE,NULL, []( TimerHandle_t xTimer)
                    { 				
                        vTaskDelete(showHandle);
                        showHandle=NULL;
                        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, false));

                    });   
                    xTimerStart(dispTimer,0);
                    //cjson will be deleted at end of routine
                }
                else
                {
                    if(duration==0)
                    {
                        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, false));
                        if(showHandle)
                            vTaskDelete(showHandle);
                        showHandle=NULL;
                        if(dispTimer)
                            xTimerStop(dispTimer,0);
                    }
                }
           }
        }
        else 
        {
            ESP_LOGE(MESH_TAG,"Display Cmd no MID\n");
            cJSON_free(elcmd);
            return ESP_FAIL;
        }
        cJSON_free(elcmd);
    }
    else 
        ESP_LOGE(MESH_TAG,"NO CMD argument passed. Turn Display error\n");
    return ESP_OK;
}

int  check_my_metrics(char *cmd,char *mid)
{
    cJSON               *cualm,*lstate,*elcmd,*ver;

    elcmd=cJSON_Parse(cmd);
    if(elcmd)
    {
        cualm= 		cJSON_GetObjectItem(elcmd,"mid");

        if(cualm)
        {
            strcpy(mid,cualm->valuestring);
            if(true)
            // if(strcmp(cualm->valuestring,theBlower.getMID())==0)
            {
                ESP_LOGW(MESH_TAG,"MId [%s me] Metrics",cualm->valuestring);
                send_metrics_message(cualm->valuestring);
            }
        }
        else 
            ESP_LOGE(MESH_TAG,"Metrics Cmd no MID\n");
        cJSON_free(elcmd);
    }
    else 
        ESP_LOGE(MESH_TAG,"Metrics Internal error not parsable\n");

    return ESP_OK;
}

int  update_my_meter(char *cmd)     
{
    cJSON               *cualm,*bpk,*ks,*k,*elcmd,*sk,*skn,*ver,*baset,*repeatt;

    elcmd=cJSON_Parse(cmd);
    if(elcmd)
    {
        cualm= 		cJSON_GetObjectItem(elcmd,"mid");       // the meter

        if(cualm)
        {
            if(true)
            // if(strcmp(theBlower.getMID(),cualm->valuestring)==0)
            {   //its for me

                bpk= 		cJSON_GetObjectItem(elcmd,"bpk");       //bpk value
                ks= 		cJSON_GetObjectItem(elcmd,"ks");        //kwh start
                k= 		    cJSON_GetObjectItem(elcmd,"k");         //kwh life
                sk= 		cJSON_GetObjectItem(elcmd,"sk");        // skip acitve y/n
                skn= 		cJSON_GetObjectItem(elcmd,"skn");       // skip count
                ver= 		cJSON_GetObjectItem(elcmd,"ver");       //version to set possible OTA
                baset= 		cJSON_GetObjectItem(elcmd,"baset");       //base time for development
                repeatt= 	cJSON_GetObjectItem(elcmd,"repeat");       //repeat time for developmentA

                if(bpk && k && ks)
                {
                    if(k->valueint<ks->valueint)
                    {
                        ESP_LOGW(MESH_TAG,"Update kwh [%d} < Kwstart [%d}",k->valueint,ks->valueint);
                        cJSON_free(elcmd);
                        return  ESP_FAIL;
                    }
                    char *tmp=(char*)calloc(200,1);
                    if(tmp)
                    {
                        // sprintf(tmp,"MId [%s me] Update Bpk:[%d->%d} kWh:[%d->%d] kWhStart:[%d->%d] Skip:%s SkipCount %d",cualm->valuestring,theBlower.getBPK(),bpk->valueint,
                        //             theBlower.getLkwh(),k->valueint,theBlower.getKstart(),ks->valueint,sk->valuestring,skn->valueint);
                        // writeLog(tmp);
                        free(tmp);
                    }
                    theBlower.eraseBlower();           //erase all metrics given new ks and k
                    // theBlower.setBPK(bpk->valueint);
                    // theBlower.setLkwh(k->valueint);
                    // theBlower.setKstart(ks->valueint);
                    theBlower.saveBlower();
                }
                // if(sk)
                // {
                //     if(strcmp("y",sk->valuestring)==0)
                //         theConf.allowSkips=true;
                //     else
                //         theConf.allowSkips=false;
                // }
                // if(skn)
                //     theConf.maxSkips=skn->valueint;
                if(baset)
                    theConf.baset=baset->valueint;
                if(repeatt)
                    theConf.repeat=repeatt->valueint;

                if (sk || skn || baset || repeatt)
                    write_to_flash();

                if( baset || repeatt)
                    esp_restart();      //for timers need to reboot

            }
            else
                ESP_LOGI(MESH_TAG,"Update It wasnt for me!");
        }
        else 
            ESP_LOGE(MESH_TAG,"Update Cmd missing required param\n");
        cJSON_free(elcmd);
    }
    else 
        ESP_LOGE(MESH_TAG,"Update Internal error not parsable\n");

    return ESP_OK;
}

// internal Mesh network message, we use cjson since no big limit
// used to give to all connecting nodes certain "global" system parameter
// Date, last known ssid and passw, time slot, mqtt params and Application location params like prov, canton,etc
// Only Root sends this message to connecting Child

esp_err_t root_send_data_to_node(mesh_addr_t thismac)
{
    int                 err;
    wifi_config_t       configsta;
    mesh_data_t         data;
    char                *topic;
    time_t              now;
    cJSON               *root;
    // meshunion_t         *intMessage;
    mqttSender_t        meshMsg;

    time(&now);
    //first send
        root=cJSON_CreateObject();
        if(root)
        {
            cJSON_AddStringToObject(root,"cmd",internal_cmds[BOOTRESP]);
            cJSON_AddNumberToObject(root,"time",(uint32_t)now);
            cJSON_AddNumberToObject(root,"profile",theConf.activeProfile);
            cJSON_AddNumberToObject(root,"day",theConf.dayCycle);
            cJSON_AddNumberToObject(root,"timermux",theConf.test_timer_div);
            cJSON_AddNumberToObject(root,"start",theConf.blower_mode);      // this will determine the node to start or not

            char *intmsg=cJSON_PrintUnformatted(root);
            if(intmsg)
            {  
                meshMsg.msg=intmsg;
                meshMsg.lenMsg=strlen(intmsg);
                meshMsg.addr=&thismac;
                if(xQueueSend(meshQueue,&meshMsg,0)!=pdPASS)
                    ESP_LOGE(MESH_TAG,"Cannot queue fram");   //queue it he will free the message
            }
            else
                ESP_LOGE(MESH_TAG,"send data node no RAM");

            cJSON_Delete(root); 
            return ESP_OK;
        }
        else
            return ESP_FAIL;
            delay(500);   // let time to node to process first message

        // now send the theConf modbus configurations binary data
        int big=sizeof(theConf.limits)+sizeof(theConf.modbus_battery)+
                        sizeof(theConf.modbus_panels)+
                        sizeof(theConf.modbus_inverter)+
                        sizeof(theConf.modbus_sensors );
        printf("Big Modbus data to send %d\n",big);
        
        meshunion_t *intMessage=(meshunion_t*)calloc(1,big+4);     // the reserving of space for the union is not the sizeof union just arbitrary 
        void *p=(void*)intMessage+4;
        memcpy(p,&theConf.milim,big);        //copy the message itself, offset 4 for cmd uint32 start offset is milim
        intMessage->cmd=MESH_INT_DATA_MODBUS;
        data.proto = MESH_PROTO_BIN;
        data.tos = MESH_TOS_P2P;
        data.data=(uint8_t*)intMessage;
        data.size=big+4;
        esp_mesh_send(meshMsg.addr, &data, meshMsg.addr==NULL? MESH_DATA_FROMDS:MESH_DATA_P2P, NULL, 0);
        free(intMessage);

    return ESP_OK;
}   

int root_sendNodeACK(void *parg)
{
    mqttSender_t        meshMsg;

    mesh_data_t data;
    mesh_addr_t *from=(mesh_addr_t*)parg;
    // printf("Send Node Install confirmation sent to "MACSTR"\n",MAC2STR(from->addr));
    char *mensaje=(char*)calloc(1,100);
    if(mensaje)
    {
        strcpy(mensaje,"{\"cmd\":\"installconf\"}");      //cJSON is long /elaborate for this simple message

            meshMsg.msg=mensaje;
            meshMsg.lenMsg=strlen(mensaje);
            meshMsg.queue=NULL;
            meshMsg.addr=from;      //to specific node
            if(xQueueSend(meshQueue,&meshMsg,0)!=pdTRUE)      //will free mensaje INTERNAL Message 
            {
                ESP_LOGE(MESH_TAG,"Error queueing sendNodeACK msg");
                if(meshMsg.msg)        
                    free(meshMsg.msg);  //due to failure
                return ESP_FAIL;
            }
    }
    return ESP_OK;
}

esp_err_t root_send_confirmation_central(char *msg,uint16_t size,char *cualQ)
{
    mqttSender_t        mqttMsg;
    char *confmsg=(char*)calloc(1,size);
    // printf("SentoCentral [%s]/%d to Queue [%s]\n",msg,size,cualQ);
    memcpy(confmsg,msg,size);
    mqttMsg.queue=cualQ;
    mqttMsg.msg=confmsg;                                    // freed by mqtt sender
    mqttMsg.lenMsg=size;
    mqttMsg.code=NULL;
    mqttMsg.param=0;
    if(xQueueSend(mqttSender,&mqttMsg,0)!=pdTRUE)               // NETWORK message
    {
        ESP_LOGE(MESH_TAG,"Error queueing confirm msg %s",cualQ);
        if(mqttMsg.msg)                     // already done but ....
            free(mqttMsg.msg);              //due to failure
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t root_send_confirmation_central_cb(char *msg,char *cualQ,mesh_addr_t *from)
{
    mqttSender_t        mqttMsg;
    mesh_addr_t         *localcopy;

        localcopy=(mesh_addr_t*)calloc(1,sizeof(mesh_addr_t));
        memcpy(localcopy,from,sizeof(mesh_addr_t));
        // printf("SentoCentral [%s]/%d to Queue [%s]\n",msg,strlen(msg),cualQ);
        mqttMsg.queue=cualQ;
        mqttMsg.msg=msg;                                    // freed by mqtt sender
        mqttMsg.lenMsg=strlen(msg);
        mqttMsg.code=root_sendNodeACK;
        mqttMsg.param=(uint32_t*)localcopy;                   //the beaty of C++ do whatever u want it will leak...now freed also when sent
        if(xQueueSend(mqttSender,&mqttMsg,0)!=pdTRUE)       
        {
            ESP_LOGE(MESH_TAG,"Error queueing confirm msg %s",cualQ);
            if(mqttMsg.msg)
                free(mqttMsg.msg);  //due to failure
            return ESP_FAIL;
        }
        return ESP_OK;
}

esp_err_t root_average_Solar(solarSystem_t *accum, solarSystem_t *src,uint8_t count)
{
    accum->pvPanel.chargeCurr+=src->pvPanel.chargeCurr/count;
    accum->pvPanel.pv1Volts+=src->pvPanel.pv1Volts/count;
    accum->pvPanel.pv2Volts+=src->pvPanel.pv2Volts/count;
    accum->pvPanel.pv1Amp+=src->pvPanel.pv1Amp/count;
    accum->pvPanel.pv2Amp+=src->pvPanel.pv2Amp/count;

    accum->battery.batSOC+=src->battery.batSOC/count;
    accum->battery.batSOH+=src->battery.batSOH/count;
    accum->battery.batteryCycleCount+=src->battery.batteryCycleCount/count;
    accum->battery.batBmsTemp+=src->battery.batBmsTemp/count;

    accum->energy.batChgAHToday+=src->energy.batChgAHToday/count;
    accum->energy.batDischgAHToday+=src->energy.batDischgAHToday/count;
    accum->energy.batChgAHTotal+=src->energy.batChgAHTotal/count;
    accum->energy.batDischgAHTotal+=src->energy.batDischgAHTotal/count;  
    accum->energy.generateEnergyToday+=src->energy.generateEnergyToday/count;
    accum->energy.usedEnergyToday+=src->energy.usedEnergyToday/count; 
    accum->energy.gLoadConsumLineTotal+=src->energy.gLoadConsumLineTotal/count; 
    accum->energy.batChgkWhToday+=src->energy.batChgkWhToday/count; 
    accum->energy.batDischgkWhToday+=src->energy.batDischgkWhToday/count; 
    accum->energy.genLoadConsumToday+=src->energy.genLoadConsumToday/count; 

    accum->sensors.DO+=src->sensors.DO/count;
    accum->sensors.PH+=src->sensors.PH/count;
    accum->sensors.WTemp+=src->sensors.WTemp/count;  
    accum->sensors.ATemp+=src->sensors.ATemp/count;
    accum->sensors.AHum+=src->sensors.AHum/count;   

    return ESP_OK;
}

esp_err_t root_send_collected_nodes(uint32_t cuantos)        //root only
{
    mqttSender_t        mqttMsg;
    solarSystem_t       *solar=NULL,*solarPad=NULL; 
    meshunion_t*        meshMsg;

    theBlower.setStatsLastBlowerCount(cuantos);
// TickType_t xRemainingTime =xTimerGetExpiryTime( sendMeterTimer ) - xTaskGetTickCount();
// ESP_LOGI(MESH_TAG,"Collect Data time %dms",10000-pdTICKS_TO_MS(xRemainingTime));

//build mqtt message
//since its binary and can change the number of nodes, first uint32 of message is the number of Nodes in this message
           
    uint32_t hp=esp_get_free_heap_size();
    uint32_t tosend=1;    // //sk   // if we use S
    //if we are skipping then logic will change thisip any node that has the do not send AND is powered on. If powered off skip this skip jaja
    // if(theConf.allowSkips)
    // {

    // }

    if((theConf.debug_flags >> dMESH) & 1U)  
        ESP_LOGW(MESH_TAG,"%sCollected NODES heap %d nodes %d send %d",DBG_MESH,hp,cuantos,tosend);

    solarPad=(solarSystem_t*)calloc(1,sizeof(solarSystem_t));
    if(!solarPad)
    {
        ESP_LOGE(MESH_TAG,"No RAM solarPad");
        sendMeterf=false;
        if( xTimerStart(collectTimer, 0 ) != pdPASS )
            ESP_LOGE(MESH_TAG,"Repeat Timer collected  failed");   
        return ESP_FAIL;
    }
    for (int a=0;a<cuantos;a++)                                             //data is stored in the MasterNode Structure
    {
        // if(theConf.allowSkips)
        // {
        //     if(masterNode.theTable.sendit[a])
        //     {
        //         if(masterNode.theTable.thedata[a])                                  //if data present
        //         {
        //             meshMsg=(meshunion_t*)masterNode.theTable.thedata[a];
        //             solar=(solarSystem_t*)&meshMsg->nodedata.solarData;
        //             root_average_Solar(solarPad,solar,cuantos);
        //         }
        //         else 
        //             ESP_LOGW(MESH_TAG,"Error null data on node skip %d\n",a);
        //     }
        // }
        // else
        // {
            if(masterNode.theTable.thedata[a])                                  //if data present
            {
                meshMsg=(meshunion_t*)masterNode.theTable.thedata[a];
                solar=&meshMsg->nodedata.solarData.solarSystem;
                root_average_Solar(solarPad,solar,cuantos);
            }
            else 
                ESP_LOGE(MESH_TAG,"Error null data on node %d " MACSTR "\n",a,MAC2STR(masterNode.theTable.big_table[a].addr));
        // }
    }
    // average done, set the shrimp mesasge structure
    shrimpMsg_t *shmsg=(shrimpMsg_t*)calloc(1,sizeof(shrimpMsg_t));
    if(!shmsg)
    {
        //fuck
        ESP_LOGE(MESH_TAG,"No ram shmsg");
        sendMeterf=false;
        if( xTimerStart(collectTimer, 0 ) != pdPASS )
            ESP_LOGE(MESH_TAG,"Repeat Timer collected  failed");
        free(solarPad);
        return ESP_FAIL;
    }

    // set data and copy solarPad into shrimp message to send to MAIN HOST via MQTT HQ
    memcpy(&shmsg->poolAvgMetrics,solarPad,sizeof(solarSystem_t));
    print_blower("Root Average Solar Data", &shmsg->poolAvgMetrics,false);
    shmsg->msgTime=time(NULL);
    shmsg->poolid=theConf.poolid;
    shmsg->countnodes=cuantos;
    shmsg->centinel=0x12345678;

    // if((theConf.debug_flags >> dSCH) & 1U)  
    //     printf("Schedule Cycle %d Day %d Horario %d Start %d End %d  Status %d PWM %d\n",scheduleData.currentCycle,scheduleData.currentDay,
    //         scheduleData.currentHorario,scheduleData.currentStartHour,scheduleData.currentEndHour,scheduleData.status,scheduleData.currentPwmDuty);

    memcpy(&shmsg->schedule,&scheduleData,sizeof(wschedule_t));

    free(solarPad); // not neeed anymore
    solarPad=NULL;

    if(root_delete_routing_table()!=ESP_OK)
    {
        ESP_LOGE(MESH_TAG,"Error deleting Routing table");
        return ESP_FAIL;
    }

    // if(tosend==0)
    // {
    //     sendMeterf=false;
    //     if( xTimerStart(collectTimer, 0 ) != pdPASS )
    //         ESP_LOGE(MESH_TAG,"Repeat Timer collected  failed");  
    //     return ESP_OK;          //nothing to send or to do
    // }

    // printf("Sending Average Solar Data to MQTT HQ size %d\n", sizeof(shrimpMsg_t)); 
    // esp_log_buffer_hex(MESH_TAG,shmsg,sizeof(shrimpMsg_t)); 
    mqttMsg.queue=                      infoQueue;
    mqttMsg.msg=                        (char*)shmsg;                                // freed by mqtt sender
    mqttMsg.lenMsg=                     sizeof(shrimpMsg_t);
    mqttMsg.code=                       NULL;
    mqttMsg.param=                      NULL;
    if(xQueueSend(mqttSender,&mqttMsg,0)!=pdTRUE)      //will free todo 
    {
        ESP_LOGE(MESH_TAG,"Error queueing msg");
        if(mqttMsg.msg)
            free(mqttMsg.msg);  //due to failure
            // need to reset timer due to "exceptional" error and not consiedred generic failure
        sendMeterf=false;
        if( xTimerStart(collectTimer, 0 ) != pdPASS )
            ESP_LOGE(MESH_TAG,"Repeat Timer collected mqttsender failed");  
        return ESP_FAIL;
    }
    else
            //must set the wifi_event_bit SEND_MQTT_BIT, else it will just collect the message in the queue
        xEventGroupSetBits(wifi_event_group, SENDMQTT_BIT);	// Send everything now !!!!!

    return ESP_OK;
}

// works with a timer in case an expected node never sends the meter data
// Gets messages sent from other Child Nodes with their meter reading. When last one "counting_nodes" is received send the Data to MQTT HQ
// using Mqtts segmenting capabilities 
// ladata is  meshunion_t * myNode
esp_err_t root_check_incoming_meters(mesh_addr_t *who, void* ladata)
{
    mqttSender_t        mqttMsg;
    esp_err_t           err;

    if(root_load_routing_table_mac(who,ladata)==ESP_OK)      //save messages in our masternode table
    {
        counting_nodes--;

        if(counting_nodes==0)                           //when we reach existing node count, start sending process
        {
            //stop watchdog timer
            // if(xTimerIsTimerActive(sendMeterTimer))
            // {
                if( xTimerStop(sendMeterTimer, 0 ) != pdPASS )
                    ESP_LOGE(MESH_TAG,"Failed to stop SendMeter Timer");
            // }
            // printf("Sending data \n");
            err= root_send_collected_nodes(masterNode.existing_nodes);        //all existing nodes
            if(err)
            {
                    ESP_LOGE(MESH_TAG,"Error sending mqtt msg");
                    return ESP_FAIL;
            }
            else
                return ESP_OK;   
        }
        else
            return ESP_OK;
    }
    else
        return ESP_FAIL;
}

void root_timer(void *parg)
{
    mqttSender_t        mqttMsg;
    esp_err_t           err;
    
    while(true)
    {
        xEventGroupWaitBits(otherGroup,REPEAT_BIT,pdTRUE,pdFALSE,portMAX_DELAY);    //wait forever, this is the starting gun, flag will be cleared

        ESP_LOGW(MESH_TAG,"Mesh Timeout. Will send only %d nodes",masterNode.existing_nodes-counting_nodes);
for (int a=0;a<masterNode.existing_nodes;a++)
{
    if(!masterNode.theTable.thedata[a])
        esp_rom_printf("Node not sending %d of %d" MACSTR " \n",a,masterNode.existing_nodes,MAC2STR(masterNode.theTable.big_table[a].addr));
    else
        esp_rom_printf("Valid answer %d of %d" MACSTR " \n",a,masterNode.existing_nodes,MAC2STR(masterNode.theTable.big_table[a].addr));
}

        if(sendMeterf)      //recheck we are sending mode
        {
            if((masterNode.existing_nodes-counting_nodes)>0)
            {
                err= root_send_collected_nodes(masterNode.existing_nodes-counting_nodes);        //existing nodes minus pending
                if(err)
                    ESP_LOGE(MESH_TAG,"Error sending meter timeout msg");
            }
            else
                sendMeterf=false;       //reset flag on error
        }
        // if(!xTimerIsTimerActive(collectTimer))
            if( xTimerStart(collectTimer, 0 ) != pdPASS )
                ESP_LOGE(MESH_TAG,"Repeat Timer mqtt saender failed");
    }
}


void root_collect_meter_data(TimerHandle_t algo)
{
    int                 err;    
    mesh_data_t         data;
    char                *buf;
    struct tm           timeinfo;
    time_t              now;
    char*               broadcast;
    meshunion_t         *intMessage;

// get routing table if root
    if(esp_mesh_is_root())
    {
        if(sendMeterf )
        {
            ESP_LOGW(MESH_TAG,"Collect called without previous called finished"); //very import check, else HEAP collapse eventually
            //maybe just reset the flag and return
            // analyze this carefully its vital
            // the flag is reset by the MQtt manager meaning a message was sent or error in root_timer
            sendMeterf=false;
            return;
        }

        err=get_routing_table();        // Get our routing table, This is how many nodes we should esxpect messages from
        if(err)
        {
            ESP_LOGE(MESH_TAG,"Could not get routing table FATAL");
            sendMeterf=false;
            return;
        }
        // send Broadcast  to start receiving meters from nodes 
        intMessage=(meshunion_t*)calloc(1,sizeof(meshunion_t));     // the reserving of space for the union is not the sizeof union, an 
                                                    // arbitrary number of bytes
        if(!intMessage)
        {
            ESP_LOGE(MESH_TAG,"Could not calloc collect meter FATAL");      //very unlikely unless free unbalance 
            return;
        }
        sendMeterf=true;            //now in sendmeter mode so start timer on this message to nodes
   // changed timer here IMPORTANT
        // if(!xTimerIsTimerActive(sendMeterTimer))
        // {
            // esp_rom_printf("First sendtimer\n");
            if( xTimerStart(sendMeterTimer, 0 ) != pdPASS )         // mesh rx timer
                ESP_LOGE(MESH_TAG,"SendMeter Timer failed");
        // }
        intMessage->cmd=MESH_INT_DATA_CJSON;
        strcpy(intMessage->parajson,"{\"cmd\":\"sendmetrics\"}");      //cJSON is long /elaborate for this simple message
        data.data   =(uint8_t*)intMessage;
        data.size   = strlen(intMessage->parajson)+4;
        data.proto  = MESH_PROTO_BIN;
        data.tos    = MESH_TOS_P2P;
        //send a Broadcast Message to all nodes to send their data
        err= esp_mesh_send( &GroupID, &data, MESH_DATA_P2P, NULL, MESH_OPT_SEND_GROUP);         //broadcast msg to mesh, must be freed by us
        if(err)
        {
            ESP_LOGE(MESH_TAG,"Broadcast failed. Now message in this slots %x",err);
            root_delete_routing_table();    
            free(intMessage); 
            sendMeterf=false;
            // return; 
        }
        free(intMessage);    
    }
}

void set_sta_cmd(char *cjText)      //message from Root giving stations ids and passwords and other stuff
{
    int err;
    wifi_config_t       configsta;
    struct timeval      now;
    time_t              nowt;

    cJSON *elcmd=cJSON_Parse(cjText);
    if(!elcmd)
    {
        ESP_LOGE(MESH_TAG,"Invalid Station cjson [%s]",cjText);
        return;
    }
    err=esp_wifi_get_config( WIFI_IF_STA,&configsta);      // get station ssid and password and others
    if(!err)
    {
        // cJSON *ssid= 		cJSON_GetObjectItem(elcmd,"ssid");
        // cJSON *pswd= 		cJSON_GetObjectItem(elcmd,"psw");
        cJSON *cjtime= 		    cJSON_GetObjectItem(elcmd,"time");
        cJSON *prof= 		    cJSON_GetObjectItem(elcmd,"profile");
        cJSON *dayp= 		    cJSON_GetObjectItem(elcmd,"day");
        cJSON *mux= 	        cJSON_GetObjectItem(elcmd,"timermux");
        cJSON *schedule= 	    cJSON_GetObjectItem(elcmd,"start");
        // cJSON *mqttserver=	cJSON_GetObjectItem(elcmd,"mqtts");
        // cJSON *mqttuser= 	cJSON_GetObjectItem(elcmd,"mqttu");
        // cJSON *mqttpass=	cJSON_GetObjectItem(elcmd,"mqttp");
        // cJSON *skip=	    cJSON_GetObjectItem(elcmd,"sk");
        // cJSON *skipcount=	cJSON_GetObjectItem(elcmd,"skn");
        // if(ssid && pswd)
        // {
        //     memcpy(&configsta.sta.ssid,ssid->valuestring,strlen(ssid->valuestring));
        //     memcpy(&configsta.sta.password,pswd->valuestring,strlen(pswd->valuestring));                        //set ssid and password of internal NVS configuration
        //     err=esp_wifi_set_config( WIFI_IF_STA,&configsta);                                                   // save new ssid and password
        //     if(err)
        //         ESP_LOGE(MESH_TAG,"Failed to save new ssid %x",err);
        //     else
        //     {
        //         strcpy(theConf.thepass,pswd->valuestring);
        //         strcpy(theConf.thessid,ssid->valuestring);
        //     }
        // }
        // else
        //     ESP_LOGE(MESH_TAG,"Update SSID without ssid or pswd");

        //set time
        setenv("TZ", LOCALTIME, 1);
        tzset();
        now.tv_sec=cjtime->valueint;
        now.tv_usec=0;
        settimeofday(&now, NULL);
        time(&nowt);
        
        if((theConf.debug_flags >> dMESH) & 1U)  
            ESP_LOGI(MESH_TAG,"%sGot new Time %s",DBG_MESH,ctime(&nowt));

            if(!(prof && dayp && schedule))
                {
                    printf("Error in set sta parameters \n");
                    return;
                }
        //set location parameters
        if(prof)
            theConf.activeProfile=prof->valueint;
        if(dayp)
            theConf.dayCycle=dayp->valueint;
        if(mux)
            theConf.test_timer_div=mux->valueint;

        if((theConf.debug_flags >> dMESH) & 1U)  
            ESP_LOGI(MESH_TAG,"%sStation got Profile %d Day %d Mux %d Restart %s",DBG_MESH,theConf.activeProfile,theConf.dayCycle,
                theConf.test_timer_div,schedule->valueint?"Yes":"No");
        if(schedule->valueint>0)
        {
            // xTaskCreate(&start_schedule_timers,"sched",1024*10,NULL, 5, &scheduleHandle);
            xSemaphoreGive(workTaskSem);    //activate task
            theConf.blower_mode=1;
        }	       

        // if(skip)
        //     theConf.allowSkips=skip->valueint;
        // if(skipcount)
        //     theConf.maxSkips=skipcount->valueint;
        // if(mqttserver)
        //     strcpy(theConf.mqttServer,mqttserver->valuestring);
        // if(mqttuser)
        //     strcpy(theConf.mqttUser,mqttuser->valuestring);
        // if(mqttpass)
        //     strcpy(theConf.mqttPass,mqttpass->valuestring);

        write_to_flash();      
        //cmd and info queue names derived from the Config so do it now
        // sprintf(cmdQueue,"%s/%d/%s",QUEUE,theConf.controllerid,MQTTCMD);
        // sprintf(infoQueue,"%s/%d/%s",QUEUE,theConf.controllerid,MQTTINFO);
        // sprintf(emergencyQueue,"%s/%s",QUEUE,MQTTEMER);
        // sprintf(cmdBroadcast,"%s/%s",QUEUE,MQTTBROADCAST);
        // sprintf(discoQueue,"%s/%s",QUEUE,MQTTDISCO);
        // sprintf(installQueue,"%s/%s",QUEUE,MQTTINSTALL);
        // if(!theConf.masternode)
        // {
        //     printf("Start Schedule Non root\n");
        //     xTaskCreate(&start_schedule_timers,"sched",1024*10,NULL, 5, &scheduleHandle); 	       
        //     // if(theConf.blower_mode==1)
        //         xSemaphoreGive(workTaskSem);
        // }
    }
    else
        ESP_LOGE(MESH_TAG,"Could not get STA config update ssid %x", err);    
    cJSON_Delete(elcmd);
}


esp_err_t send_datos_to_root()
{
    int                 err;
    mesh_data_t         datas;

    // this is binary data and comes with the required header
    meshunion_t * myNode=sendData(true);
    if(!myNode)
    {
        ESP_LOGE(MESH_TAG,"Error send datos root msg\n");
        return -2;      //if NULL myNode it wasnt allocated so just return error
    }

        print_blower("send datos root",&myNode->nodedata.solarData.solarSystem,false);
    if(theConf.delay_mesh>0)
                delay(theConf.delay_mesh*theConf.unitid*SENDMUX);       //trying to avoid congestion UNIT id *ms
    datas.data      =(uint8_t*)myNode;
    datas.size      = sizeof(meshunion_t);
    datas.proto     = MESH_PROTO_BIN;
    datas.tos       = MESH_TOS_P2P;
    err= esp_mesh_send( NULL, &datas, MESH_DATA_P2P, NULL, 1); 
    if(err)
        ESP_LOGE(MESH_TAG,"Send Meters failed %x",err);

    free(myNode);          
    return ESP_OK;
}

// void format_my_meter(cJSON *cmd)
// {
//     time_t  now;
//     cJSON *mid= 		cJSON_GetObjectItem(cmd,"mid");
//     cJSON *reconf= 		cJSON_GetObjectItem(cmd,"erase");

//     if(mid)
//     {
//         if (strcmp(mid->valuestring,theBlower.getMID())==0)
//         {
//             theBlower.deinit();
//             theBlower.format();
//             time(&now);
//             theBlower.writeCreationDate(now);

//             if(reconf)
//             {
//                 ESP_LOGW(MESH_TAG,"Format Meter Erase %s\n",reconf->valuestring);
//                 if(strcmp(reconf->valuestring,"Y")==0)
//                     erase_config(); 
//             }

//             char *tmp=(char*)calloc(1,200);
//             if(tmp)
//             {
//                 sprintf(tmp,"Format meter [%s] FRAM Command executed",theBlower.getMID());
//                 writeLog(tmp);
//                 fclose(myFile);
//                 free(tmp);
//             }                     
//             ESP_LOGW(MESH_TAG,"Format MID [%s] sent to [%s] Fram and Erase configuration. Virgin chip",theBlower.getMID(),mid->valuestring);
//             delay(1000);
//             esp_restart();                
//         }
//     }

// }
err_t root_mesh_broadcast_msg(char * msg)        // csjon format only    
    {
    meshunion_t * intMessage=(meshunion_t*)calloc(1,strlen(msg)+10);     // the reserving of space for the union is not the sizeof union, an 
    if(!intMessage) 
    {
        ESP_LOGE(MESH_TAG,"Root broadcast no RAM");
        return ESP_FAIL;
    }

    intMessage->cmd=MESH_INT_DATA_CJSON;
    mesh_data_t data;
    strcpy(intMessage->parajson,msg);      //cJSON is long /elaborate for this simple message

    data.data   =(uint8_t*)intMessage;
    data.size   = strlen(intMessage->parajson)+4;
    data.proto  = MESH_PROTO_BIN;
    data.tos    = MESH_TOS_P2P;
    //send a Broadcast Message to all nodes to send their data
    int err= esp_mesh_send( &GroupID, &data, MESH_DATA_P2P, NULL, MESH_OPT_SEND_GROUP);         //broadcast msg to mesh, must be freed by us
    if(err)
    {
        ESP_LOGE(MESH_TAG,"Broadcast failed. Now message in this slots %x",err);
        return err;
    }
    return ESP_OK;  
}

// void erase_my_meter(cJSON *cmd)
// {
//     time_t  now;
//     cJSON *mid= 		cJSON_GetObjectItem(cmd,"mid");

//     if(mid)
//     {
//         if (strcmp(mid->valuestring,theBlower.getMID())==0)
//         {
//             theBlower.eraseBlower();//erase kws/kwh/and other fields and save the same MID

//             char *tmp=(char*)calloc(100,1);
//             if(tmp)
//             {
//                 sprintf(tmp,"Erase meter [%s] Command executed",theBlower.getMID());
//                 writeLog(tmp);
//                 fclose(myFile);
//                 free(tmp);
//             }                               
//         }
//     }

// }

void process_binary_mesh_msg(mesh_addr_t *from, mesh_data_t *data,uint32_t reserved)
{
    meshunion_t *       aNode;
    esp_err_t           err;

    if(esp_mesh_is_root() && reserved==MESH_INT_DATA_BIN)         // check out for meter data                             //only ROOT is central manager. ONE connection to MQTT
    {
        aNode=(meshunion_t *)calloc(data->size,1);                 // allocate buffer, will be erased after processing
        if(!aNode)
        {
            ESP_LOGE(MESH_TAG,"mesh binary RECV Failed calloc");
            return;
        }
        memcpy(aNode,(char*)data->data,data->size);             //now copy since we knwo sizew is not greater than meshunion_t
        if(root_check_incoming_meters(from,aNode)!=ESP_OK)      //saves data into a working table unless error
        {
            ESP_LOGE(MESH_TAG,"Check Incoming Error");
            free(aNode);
        }
    // DO NOT Free aNode its used to get the sent data by Load_table
        return;
    }
    // else check other binary commands here
    if(!esp_mesh_is_root() && reserved==MESH_INT_DATA_MODBUS)         // check out for meter data                             //only ROOT is central manager. ONE connection to MQTT
    {
        // update modbus data all nodes
        printf("Child Node got Modbus data from Root\n");
        aNode=(meshunion_t *)calloc(data->size,1);                 // allocate buffer
        if(!aNode)
        {
            ESP_LOGE(MESH_TAG,"mesh binary RECV Modbus Failed calloc");
            return; 
        }
        // do something with modbus data
        free(aNode);
        return;
    }

}

void child_production(cJSON *elcmd) 
{
    char laorden[10],*log_msg;

        cJSON *prof=        cJSON_GetObjectItem(elcmd,"prof");
        cJSON *pday=        cJSON_GetObjectItem(elcmd,"day");
        cJSON *pmux=        cJSON_GetObjectItem(elcmd,"mux");
        cJSON *order=       cJSON_GetObjectItem(elcmd,"order");
        if(prof && pday && pmux &&order)
        {
            strcpy(laorden,order->valuestring);
            if((theConf.debug_flags >> dMESH) & 1U)  
                ESP_LOGI(MESH_TAG, "%Mesh Production Cmd Profile %d Day %d Mux %d Order %s",
                    prof->valueint,pday->valueint,pmux->valueint,laorden);

            theConf.activeProfile=prof->valueint;
            theConf.dayCycle=pday->valueint;
            theConf.test_timer_div=pmux->valueint;
            write_to_flash();
            if((theConf.debug_flags >> dMESH) & 1U) 
            ESP_LOGI(TAG,"%sMesh Production Cmd Profile %d Day %d Mux %d Order %s",DBG_MESH,
                    prof->valueint,pday->valueint,pmux->valueint,laorden);
            log_msg=(char*)calloc(200,1);
            if(!log_msg)    //just warning continue processing 
                ESP_LOGE(TAG,"No Ram CmdProd log msg");
            if(log_msg ) 
                writeLog(log_msg); 

            if (strcmp(laorden,"start")==0)
            {
                if((theConf.debug_flags >> dMESH) & 1U)  
                    ESP_LOGI(MESH_TAG, "%sMesh Production Cmd order to Start %s",DBG_MESH,laorden);
                xSemaphoreGive(workTaskSem);        //start it
                if(log_msg) 
                {
                    sprintf(log_msg,"Mesh Production Cmd order to Start %s",DBG_MESH,laorden);
                    writeLog(log_msg);
                }
            }
            if (strcmp(laorden,"stop")==0)
            {
                if((theConf.debug_flags >> dMESH) & 1U)  
                    ESP_LOGI(MESH_TAG, "%sMesh Production Cmd order to Stop %s",DBG_MESH,laorden);
                    if(scheduleHandle) // set sta should have started it or it finished
                        vTaskDelete(scheduleHandle);
                    xTaskCreate(&start_schedule_timers,"sched",1024*10,NULL, 5, &scheduleHandle); 
                if(log_msg) 
                {
                    sprintf(log_msg,"Mesh Production Cmd order to Stop %s",DBG_MESH,laorden);
                    writeLog(log_msg);
                }	       
                schedulef=false;
                // xSemaphoreGive(workTaskSem);
            }
            if (strcmp(laorden,"pause")==0)
            {
                if((theConf.debug_flags >> dMESH) & 1U)  
                    ESP_LOGI(MESH_TAG, "%sMesh Production Cmd order to Pause %s",DBG_MESH,laorden);
                if(log_msg) 
                {
                    sprintf(log_msg,"Mesh Production Cmd order to Pause %s",DBG_MESH,laorden);
                    writeLog(log_msg);
                }
                pausef=true;

            }
            if (strcmp(laorden,"resume")==0)
            {
                if((theConf.debug_flags >> dMESH) & 1U)  
                    ESP_LOGI(MESH_TAG, "%sMesh Production Cmd order to Resume %s",DBG_MESH,laorden);
                if(log_msg) 
                {
                    sprintf(log_msg,"Mesh Production Cmd order to Resume %s",DBG_MESH,laorden);
                    writeLog(log_msg);
                }
                pausef=false;
            }
        }
        else
            if((theConf.debug_flags >> dMESH) & 1U)  
                ESP_LOGI(MESH_TAG, "%sProduction missing parameter ",DBG_MESH);
}
//here we received all mesh traffic data
void static mesh_manager(mesh_addr_t *from, mesh_data_t *data)
{
    cJSON               *elcmd;
    static char         cualMID[20];
    esp_err_t           err;
    locker_t            lock;
    char                *msg;
    uint32_t            reserved;
    char                laorden[10];

    // msg from the Mesh has to have a defined structure with the first 2 bytes identifying
    // the msg typpe: 
    // BINARY
        // MESH_INT_DATA_BIN -> binary meter msg from nodes
        // MESH_MOTA_START -> Mesh OTA start
        // MESH_MOTA_DATA -> Mesh OTA data in
        // MESH_MOTA_DATA_END -> Mesh OTA end without data
        // MESH_MOTA_DATA_REC -> Mesh OTA rcovery data in
        // MESH_MOTA_DATA _REC_END-> Mesh OTA data in and end of recovery

    // MESH_INT_DATA_CJSON -> Mesh internal cmds text (cJSON)

    if(data->size==0)
    {
        ESP_LOGE(MESH_TAG,"Meshmgr got 0 len");
        // free(aNode);
        return;
    }
    // identify the msg type
    memcpy(&reserved,data->data,4);     

    if(reserved!=MESH_INT_DATA_CJSON)            //its binary meter data or mesh ota
    {
        process_binary_mesh_msg(from, data,reserved);
        return;
    }

    // internal cmd
    data->data+=4;      //skip command
    //size is with cmd so take 4 out of it
    data->size-=4;
    data->data[data->size]=0;
    elcmd=cJSON_Parse((char*)data->data); 
    if(!elcmd)
    {
        ESP_LOGE(MESH_TAG,"CSJON cannot be parsed mesh manager %s",data);
        return;
    }

    // all msgs MUST have a cmd 
    cJSON *cualm=       cJSON_GetObjectItem(elcmd,"cmd");
    if(cualm)
    {
        if((theConf.debug_flags >> dMESH) & 1U)  
          ESP_LOGI(MESH_TAG,"%sfind internal cmd %s",DBG_MESH,cualm->valuestring);
        int cualf=findInternalCmds(cualm->valuestring);
        if(cualf>=0)
        {
            switch (cualf)
            {
                // ESP_LOGI(MESH_TAG,"Found %d",cualf);
                // TODO redo this. Make direct call not a fall thru
                case EMERGENCY: //911 call
                        ESP_LOGI(MESH_TAG,"Mesh 911 call");
                        // mqttMsg.queue=emergencyQueue;   // fall thru  --. LAST PART OF ROUTINE, another alternative????`
                        // message=(char*)calloc(data->size,1);   //will be freed by mqttsender at delivery confirmation
                        // memcpy(message,data->data,data->size);
                        // mqttMsg.msg=message;
                        // mqttMsg.lenMsg=data->size;
                        // mqttMsg.code=NULL;
                        // mqttMsg.param=NULL; 
                        break;                             
                case BOOTRESP: // a msg from Root giving the current ssid/pswd bootresp
                        // ESP_LOGI(MESH_TAG,"Mesh Boot response");
                        if(!esp_mesh_is_root()) //only non ROOT 
                            set_sta_cmd((char*)data->data);
                        cJSON_Delete(elcmd);
                        return;
                case SENDMETRICS:// host requires Node sends its meter data 
                        // ESP_LOGI(MESH_TAG,"Send Meters cmd from Host");
                        err=send_datos_to_root();
                        if(err)
                            ESP_LOGE(MESH_TAG,"Error send Meters %d",err);
                        cJSON_Delete(elcmd);
                        return;
                case PRODUCTION:// start Production
                        if((theConf.debug_flags >> dMESH) & 1U)  
                            ESP_LOGI(MESH_TAG, "%sMesh Production CMd ",DBG_MESH);
                        if(!esp_mesh_is_root()) //only non ROOT 
                        {
                          child_production(elcmd);
                        }
                        cJSON_Delete(elcmd);
                        return;
                case UPDATEMETER:// update important settings meter
                        if((theConf.debug_flags >> dMESH) & 1U)  
                            ESP_LOGI(MESH_TAG,"Mesh Update Mesh Cmd");
                        update_my_meter((char*)data->data);
                        cJSON_Delete(elcmd);
                        return;
                case FORMAT:    // format the meter
                        if((theConf.debug_flags >> dMESH) & 1U)  
                            ESP_LOGI(MESH_TAG,"Mesh Format cmd");
                        // format_my_meter(elcmd);
                        cJSON_Delete(elcmd);
                        return;
                case ERASEMETRICS:    // erase metrics
                        if((theConf.debug_flags >> dMESH) & 1U)  
                            ESP_LOGI(MESH_TAG,"Mesh Erase Metrics cmd");
                        // erase_my_meter(elcmd);
                        cJSON_Delete(elcmd);
                        return;
                case MQTTMETRICS:
                        if((theConf.debug_flags >> dMESH) & 1U)  
                            ESP_LOGI(MESH_TAG,"Mesh Metrics requested cmd");
                        check_my_metrics((char*)data->data,cualMID);
                        cJSON_Delete(elcmd);
                        return;     
                case METRICRESP:
                        if((theConf.debug_flags >> dMESH) & 1U)  
                            ESP_LOGI(MESH_TAG,"Mesh Metrics respond cmd");
                        msg=(char*)calloc(1,data->size+1);
                        if(msg)
                        {
                            memcpy(msg,data->data,data->size);
                            if(root_send_confirmation_central_cb(msg,discoQueue,from)==ESP_FAIL)
                                ESP_LOGE(MESH_TAG,"Error sending meterics response to Mqtt Mgr");
                        }
                        else
                            ESP_LOGE(MESH_TAG,"Error RAM Response metrics mesh mgr");
                        cJSON_Delete(elcmd);
                        return;     
                case SHOWDISPLAY:
                        if((theConf.debug_flags >> dMESH) & 1U)  
                            ESP_LOGI(MESH_TAG,"Mesh Display cmd");
                        turn_display((char*)data->data);
                        cJSON_Delete(elcmd);
                        return;     
                default:    // shouldnt happen... maybe during development
                        if((theConf.debug_flags >> dMESH) & 1U)  
                            ESP_LOGE(MESH_TAG,"%sMesh Internal not found %s",DBG_MESH,cualm->valuestring);
                        cJSON_Delete(elcmd);
                        return;
            }
        }
        else
        {
            ESP_LOGW(MESH_TAG,"Mesh cmd not found %s",cualm->valuestring);
            cJSON_Delete(elcmd);
            return;
        }
    }
    else
    {
        ESP_LOGI(MESH_TAG,"MeshMgr Internal cJSON but no CMD found [%s]",data->data);
        esp_log_buffer_hex(MESH_TAG,data->data,60);
        cJSON_Delete(elcmd);
        return;
    }
}


err_t write_log(char *LTAG, char *que) {
    time_t now = time(NULL);

    // Format the current time without a newline
    char *time_str = ctime(&now);
    if (time_str) {
        time_str[strcspn(time_str, "\n")] = 0; // Remove newline
    }

    FILE *f = fopen(LOG_FILE, "a");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open Log for writing");
        return ESP_FAIL;
    }

    char *log_entry = (char *)calloc(1000, 1);
    if (!log_entry) {
        ESP_LOGE(TAG, "No RAM for writing log entry");
        fclose(f);
        return ESP_FAIL;
    }

    snprintf(log_entry, 1000, "%s:%s:%s\n", LTAG, time_str, que);
    fprintf(f, "%s", log_entry);

    fclose(f);
    free(log_entry);

    return ESP_OK;
}

err_t read_log(int nlines)
{
    char *buf=(char*)calloc(1000,1);
    if (!buf)
    {
        ESP_LOGE(TAG, "Failed RAM log fro reading");
        return ESP_FAIL;
    }

    FILE* f = fopen(LOG_FILE, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open Log for reading");
        return ESP_FAIL;
    }
    rewind(f);
    for (int a=0;a<nlines;a++)
    {
	    if(fgets(buf, 500, f)!=NULL)
            printf("[%d]%s",a,buf);
        else
            break;
    }
    fclose(f);
    if(buf)
        free(buf);

    return ESP_OK;
}

static void read_flash()
{
	esp_err_t q ;
	size_t largo;

	if(xSemaphoreTake(flashSem, portMAX_DELAY))
	{
		q = nvs_open("config", NVS_READWRITE, &nvshandle);
		if(q!=ESP_OK)
		{
			ESP_LOGE(MESH_TAG,"Error opening NVS Read File %x",q);
			xSemaphoreGive(flashSem);
			return;
		}

		largo=sizeof(theConf);
		q=nvs_get_blob(nvshandle,"sysconf",(void*)&theConf,&largo);     //very important NOT TO change to just a number due to POINTEr to length

		if (q !=ESP_OK)
			ESP_LOGE(MESH_TAG,"Error read %x largo %d aqui %d",q,largo,sizeof(theConf));
		// nvs_close(nvshandle);           //do not close it for future write_to_flash 
		xSemaphoreGive(flashSem);
	}
}

void write_to_flash() //save our configuration
{
	if(xSemaphoreTake(flashSem, portMAX_DELAY))
	{
		esp_err_t q ;
		// q = nvs_open("config", NVS_READWRITE, &nvshandle);
		// if(q!=ESP_OK)
		// {
		// 	ESP_LOGE(MESH_TAG,"Error opening NVS File RW %x",q);
		// 	xSemaphoreGive(flashSem);
		// 	return;
		// }
		size_t req=sizeof(theConf);
		q=nvs_set_blob(nvshandle,"sysconf",&theConf,req);
		if (q ==ESP_OK)
		{
			q = nvs_commit(nvshandle);
			if(q!=ESP_OK)
				ESP_LOGE(MESH_TAG,"Flash commit write failed %d",q);
		}
		else
			ESP_LOGE(MESH_TAG,"Fail to write flash %x",q);
		// nvs_close(nvshandle);           // do not close
		xSemaphoreGive(flashSem);
	}
}


void root_sntpget(void *pArg)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    timeval localtime;

    bzero(&timeinfo,sizeof(timeinfo));
    // memset(&timeinfo,0,sizeof(timeinfo));
    
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) 
    {
        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_setservername(0, "pool.ntp.org");
        sntp_init();

        int retry = 0;
        while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry <= SNTPTRY)
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        
        if(retry>=SNTPTRY)
        {
            ESP_LOGE(MESH_TAG,"SNTP failed retry count, using saved time");
            theConf.lastKnownDate=theBlower.getReservedDate();   
            localtime.tv_sec=theConf.lastKnownDate;
            localtime.tv_usec=0;                                      // no connection most likely
            settimeofday(&localtime,NULL);
            //let him fall thru and continue as if date was received
        }
        time(&now);
        setenv("TZ", LOCALTIME, 1);
        tzset();
        if(theConf.lastRebootTime==0)
            theConf.lastRebootTime=now;
        else
            theConf.downtime+=(uint32_t)now-theConf.lastRebootTime;

        theBlower.setReservedDate(now);

        ESP_LOGI(MESH_TAG,"SNTP Date %s",ctime((time_t*)&now));
        // this means mesh is ready also
        if (theConf.blower_mode==1 )
            xSemaphoreGive(workTaskSem);
            // we apparently are restarting for some reason else wait for activation command

            //start our Cycle calculations
        root_set_senddata_timer();
        write_to_flash();
    }
    vTaskDelete(NULL);

}

void root_reconnectTask(void *pArg)
{
    esp_err_t err;
    EventBits_t uxBits;
    mqttSender_t        mensaje;
    int retry=0;
    // in case timers are active
    // if(xTimerIsTimerActive(collectTimer))
        xTimerStop(collectTimer, 0 );
// if(xTimerIsTimerActive(sendMeterTimer))
    xTimerStop(sendMeterTimer, 0 );
    //kill MQTT Tasks
    if(mqttMgrHandle)
        vTaskDelete(mqttMgrHandle);
    if(mqttSendHandle)
        vTaskDelete(mqttSendHandle);
//erase messages and heap allocation in the messages. Mostly 1 message but can be more in case an Emergency/Lockconfirm or other async message was queued
        while(true)
        {
            if( xQueueReceive( mqttSender, &mensaje,  pdMS_TO_TICKS(MQTTSENDERWAIT) )==pdPASS)	//mqttHandle has a pointer to the original message. MUST be freed at some point
            {
                if(mensaje.msg)
                    free(mensaje.msg);
                if(mensaje.param)
                    free(mensaje.param);
            }
            else
                break;
        }

    while(true)
    {
        //try to reconnect
        delay(theConf.mqttDiscoRetry);     // retry interval
        xEventGroupClearBits(wifi_event_group,MQTT_BIT|ERROR_BIT|DISCO_BIT);        // clear bits
        if(clientCloud)
            err=esp_mqtt_client_destroy(clientCloud);
            if(err!=ESP_OK)
            {
                printf("No destroy\n");
            }
            else 
                clientCloud=NULL;
        clientCloud=root_setupMqtt();
        if(clientCloud)
        {
            err=esp_mqtt_client_start(clientCloud);
            if(err==ESP_FAIL)
            {
                ESP_LOGE(MESH_TAG,"FATAl AND FINAL alternative. Reboot %x",err);
            }
            retry++;
            uxBits=xEventGroupWaitBits(wifi_event_group, MQTT_BIT|DISCO_BIT|ERROR_BIT, pdFALSE, pdFALSE,   pdMS_TO_TICKS(10000)); //explicit Connect Bit
            if(((uxBits & MQTT_BIT) ==MQTT_BIT) )       //only mqttbit connect 
            { 
                ESP_LOGI(MESH_TAG,"Reconnection successfull...good job");
                char *tmp=(char*)calloc(1,100);
                if(tmp)
                {
                    sprintf(tmp,"Recconection successful after %d tries...good job",retry);
                    writeLog(tmp);
                    free(tmp);
                }
                //we are connected and subscribed YEAH
                err=esp_mqtt_client_disconnect(clientCloud);
                xEventGroupWaitBits(wifi_event_group, DISCO_BIT|ERROR_BIT, pdFALSE, pdFALSE,   portMAX_DELAY); //explicit disConnect Bit
                // restart our system tasks
                err=esp_mqtt_client_stop(clientCloud);
                root_mqtt_app_start();              // start the mqtt tasks
                delay(2000);                        // breathing space haha
                xTimerStart(collectTimer, 0 );      // start the mesh comms
                recoTaskHandle=NULL;                // in case we are called to duty again... before calling xTaskDelete
                vTaskDelete(recoTaskHandle);        // ourselves...good job :-)
            }
        }
        else
        {
            printf("No setup\n");
            delay(1000);
        }
    }
}


static void root_mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    // ESP_LOGD(MESH_TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    int err,cualq;
    mqttMsg_t mqttHandle;
    char *msg;
    char    eltopic[60];

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_BEFORE_CONNECT:
    // printf("Before connect\n");
        break;
    case MQTT_EVENT_CONNECTED:
        // ESP_LOGI(MESH_TAG, "MQTT_EVENT_CONNECTED");
        err=esp_mqtt_client_subscribe(client,cmdQueue, 0);
        // err=esp_mqtt_client_subscribe(client,cmdBroadcast, 0);       //if a broadcast strategy active
        mqttf=true;                         //service IS active
        sendMeterf=false;                   //no message are pending
        break;
    case MQTT_EVENT_DISCONNECTED:
        // ESP_LOGI(MESH_TAG, "MQTT_EVENT_DISCONNECTED");
        xEventGroupSetBits(wifi_event_group, DISCO_BIT);         // used as a CONNECT Mqtt and Subscribe in One
        xEventGroupClearBits(wifi_event_group,MQTT_BIT);
        delay(100);
        mqttf=false;  
        sendMeterf=false; 
        break;

    case MQTT_EVENT_SUBSCRIBED:
        // ESP_LOGI(MESH_TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        xEventGroupSetBits(wifi_event_group, MQTT_BIT);         // used as a CONNECT Mqtt and Subscribe in One
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        // ESP_LOGI(MESH_TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
#ifdef MQTTSUB
        xEventGroupSetBits(wifi_event_group, PUB_BIT|DONE_BIT);//message sent bit
#endif
        mqttf=false;
        break;
    case MQTT_EVENT_PUBLISHED:
        if((theConf.debug_flags >> dMQTT) & 1U)  
            ESP_LOGI(MESH_TAG, "%SMQTT_EVENT_PUBLISHED, msg_id=%d",DBG_MQTT, event->msg_id);
// #ifdef MQTTSUB
//             esp_mqtt_client_unsubscribe(client, cmdQueue);//bit is set by unsubscribe
// #else
            xEventGroupSetBits(wifi_event_group, PUB_BIT);//message sent bit
// #endif
            break;
    case MQTT_EVENT_DATA:
            bzero(&mqttHandle,sizeof(mqttHandle));
            msg=(char*)calloc(event->data_len,1);     
            memcpy(msg,event->data,event->data_len);
            if((theConf.debug_flags >> dMQTT) & 1U)  
                ESP_LOGI(MESH_TAG, "%sMqtt Msg [%s]",DBG_MQTT, msg);
            mqttHandle.message=(uint8_t*)msg;
            mqttHandle.msgLen=event->data_len;
            if(event->data_len)
            {
                theBlower.setStatsBytesIn(event->data_len);
                theBlower.setStatsMsgIn();
                // ESP_LOGI(MESH_TAG,"TOPIC=%.*s",event->topic_len, event->topic);
                // ESP_LOGI(MESH_TAG,"DATA=%.*s %d", event->data_len, event->data, event->data_len);
                bzero(eltopic,sizeof(eltopic));
                memcpy(eltopic,event->topic,event->topic_len);
                if(xQueueSend( mqttQ,&mqttHandle,0 )!=pdTRUE)
                    ESP_LOGE(MESH_TAG,"Cannot add msgd mqtt");  // TODO free msg 

                    // delete retained
                if(strcmp(eltopic,cmdQueue)==0)
                    esp_mqtt_client_publish(clientCloud, cmdQueue, "", 0, 0,1);//delete retained
                else
                    //hopefully he can do that and mqwtt is not already closed
                    if(strcmp(eltopic,cmdBroadcast)==0)
                        esp_mqtt_client_publish(clientCloud, cmdBroadcast, "", 0, 0,1);//delete retained VERY IMPORTANT
            }
            break;
    case MQTT_EVENT_ERROR:
    {
        ESP_LOGI(MESH_TAG, "MQTT_EVENT_ERROR");
        xEventGroupClearBits(wifi_event_group,ERROR_BIT);
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) 
        {
            if(!recoTaskHandle)
            {
                char *msg=(char*)calloc(1,100);
                if(msg)
                {
                    sprintf(msg,"MQTT ERROR Reconnect task launched");
                    writeLog(msg);
                    free(msg);
                }

                ESP_LOGW(MESH_TAG, "Transport error. ReconnetTask launched");
                err=esp_mqtt_client_disconnect(clientCloud);
                if(xTaskCreate(root_reconnectTask,"mqttreco",1024*4,NULL, 14,&recoTaskHandle)!=pdPASS)
                    ESP_LOGE(MESH_TAG,"Cannot create reconnect task");
                    //todo need to set the error bit to release mqtt sender
            }
            else
                delay(5000);
        } 
        else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) 
        {
            ESP_LOGI(MESH_TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
            char *msg=(char*)calloc(1,100);
            if(msg)
            {
                sprintf(msg,"MQTT ERROR Connection refused");
                writeLog(msg);
                free(msg);
            } 
        }
        else 
        {
            ESP_LOGW(MESH_TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
            char *msg=(char*)calloc(1,100);
            if(msg)
            {
                sprintf(msg,"MQTT ERROR Unknown %d",event->error_handle->error_type);
                writeLog(msg);
                free(msg);
            }
        }
            sendMeterf=false;                                           
            xEventGroupSetBits(wifi_event_group, ERROR_BIT);         
        break;
    }
    default:
        // ESP_LOGI(TAG, "MQTT Other event id:%d", event->event_id);
        break;
    }
}


static int findCommand(const char * cual)
{
	for (int a=0;a<MAXCMDS;a++)
	{
		if(strcmp(cmds[a].comando,cual)==0)
			return a;
	}
	return ESP_FAIL;
}

void meshSendTask(void *pArg)
{
    mqttSender_t meshMsg;
    mesh_data_t data;
    while(true)
    {
        if( xQueueReceive( meshQueue, &meshMsg, portMAX_DELAY ))	//mqttHandle has a pointer to the original message. MUST be freed at some point
        {   
            if(meshMsg.msg)
            {
                meshunion_t *intMessage=(meshunion_t*)calloc(1,meshMsg.lenMsg+4);     // the reserving of space for the union is not the sizeof union just arbitrary 
                void *p=(void*)intMessage+4;
                memcpy(p,meshMsg.msg,meshMsg.lenMsg);        //copy the message itself, offset 4 for cmd uint32
                intMessage->cmd=MESH_INT_DATA_CJSON;
                data.proto = MESH_PROTO_BIN;
                data.tos = MESH_TOS_P2P;
                data.data=(uint8_t*)intMessage;
                data.size=meshMsg.lenMsg+4;
                esp_mesh_send(meshMsg.addr, &data, meshMsg.addr==NULL? MESH_DATA_FROMDS:MESH_DATA_P2P, NULL, 0);
                free(intMessage);
                free(meshMsg.msg);
            }

        }
    }
}

void root_emergencyTask(void *pArg)
{
    mqttSender_t emergencyMsg,meshMsg;
    mesh_data_t data;

    while(true)
    {
        if( xQueueReceive( mqtt911, &emergencyMsg, portMAX_DELAY ))	//mqttHandle has a pointer to the original message. MUST be freed at some point
        {   
            if(emergencyMsg.msg)
            {  
                char *intMessage=(char *)calloc(1,emergencyMsg.lenMsg);     // the reserving of space for the union is not the sizeof union just arbitrary 
                memcpy(intMessage,emergencyMsg.msg,emergencyMsg.lenMsg);        //copy the message itself, offset 4 for cmd uint32
                free(emergencyMsg.msg);

                meshMsg.msg=intMessage;
                meshMsg.lenMsg=strlen(intMessage);
                meshMsg.addr=NULL;          // to root
                if(xQueueSend(meshQueue,&meshMsg,0)!=pdTRUE)      //will free todo 
                {
                    ESP_LOGE(MESH_TAG,"Error queueing confirm msg");
                    if(meshMsg.msg)                 // already checked but ...
                        free(meshMsg.msg);          //due to failure
                }
            }
        }
    }
}
 
//Task that handles received messages/commands from the MQTT server
// look at cmds.md for message strucutre
// Base command is an ARRAY of individual arrays
// waits in the mqttQ queue with mqttMsg_t type of message structure

void root_mqttMgr(void *pArg)
{
    mqttMsg_t           mqttHandle;
    mqttSender_t        mqttMsg;
	cJSON 	            *elcmd;

    while(true)
    {
        if( xQueueReceive( mqttQ, &mqttHandle, portMAX_DELAY ))	//mqttHandle has a pointer to the original message. MUST be freed at some point
        {
            // ESP_LOGI(MESH_TAG,"Message In MQtt %s len %d",mqttHandle.message,mqttHandle.msgLen);
            elcmd= cJSON_Parse((char*)mqttHandle.message);		//plain text to cJSON... must eventually cDelete elcmd
			if(elcmd)
			{
                cJSON *monton= 		    cJSON_GetObjectItem(elcmd,"cmdarr");
                if(monton)
                {
                    int son=cJSON_GetArraySize(monton);
                    printf("cmds in %d\n",son);
                    for (int a=0;a<son;a++)
                    {
                        cJSON *cmditem 	=cJSON_GetArrayItem(monton, a);//next item
                        if(cmditem)
                        {
                            // printf("Cmd[%d]\n",a);
                            cJSON *order= 		cJSON_GetObjectItem(cmditem,"cmd");
                            if(order)
                            {
                                ESP_LOGI(MESH_TAG,"External cmd [%s]",order->valuestring);
                                int cual=findCommand(order->valuestring);
                                if(cual>=0)
                                {
                                        (*cmds[cual].code)((void*)cmditem);	// call the cmd and wait for it to end
                                }
                                else    
                                {
                                    ESP_LOGE(MESH_TAG,"Invalid cmd received %s",order->valuestring);
                                    // mqttMsg.queue=emergencyQueue;   // fall thru  
                                    // char *message=(char*)calloc(1,strlen((char*)mqttHandle.message));   //will be freed by mqttsender at delivery confirmation
                                    // // char *message=(char*)calloc(1,strlen((char*)mqttHandle.message)+50);   //will be freed by mqttsender at delivery confirmation
                                    // sprintf(message,"Invalid Cmd [%s] in Node %d",(char*)mqttHandle.message,theConf.unitid);
                                    // mqttMsg.msg=message;
                                    // mqttMsg.lenMsg=strlen(message);
                                    // mqttMsg.code=NULL;
                                    // mqttMsg.param=NULL; 
                                    // if(xQueueSend(mqttSender,&mqttMsg,0)!=pdTRUE)      //will free message malloc
                                    // {
                                    //     ESP_LOGE(MESH_TAG,"Error mqttmgr queueing msg invalid cmd");
                                    //     if(mqttMsg.msg)
                                    //         free(mqttMsg.msg);  //due to failure
                                    // }
                                }
                            }
                            else
                                ESP_LOGE(MESH_TAG,"No order/cmd received in cmdarr[%d]",a);
                        }
                            else
                               ESP_LOGE(MESH_TAG,"Internal Error MqttMgr cmditem not obtained");

                    }
                    cJSON_Delete(elcmd);
                }
                else
                    ESP_LOGE(MESH_TAG,"No cmdarr received");
            }
            else
                ESP_LOGE(MESH_TAG,"Cmd received not parsable [%s]",mqttHandle.message);

            if(mqttHandle.message)
            {
                printf("Delete mqtt message\n");
                free(mqttHandle.message);
            }
        }
    }
}

/// to get ssl certificate 
//                                                <-------- Site/Port -->
// echo "" | openssl s_client -showcerts -connect m13.cloudmqtt.com:28747 | sed -n "1,/Root/d; /BEGIN/,/END/p" | openssl x509 -outform PEM >hive.pem

static esp_mqtt_client_handle_t root_setupMqtt()
{
    char who[20],mac[8];
    esp_base_mac_addr_get((uint8_t*)mac);
    bzero(who,sizeof(who));
    sprintf(who,"Meterserver%d",theConf.poolid);
    bzero((void*)&mqtt_cfg,sizeof(mqtt_cfg));
    mqtt_cfg.broker.address.uri=  					            theConf.mqttServer;  // pem certificate for m13.cloudmqtt.com:28747 match with hivessl.pem
    if(strlen(theConf.mqttcert)>0)
    {
        mqtt_cfg.broker.verification.certificate=                   theConf.mqttcert;
        mqtt_cfg.broker.verification.certificate_len=               theConf.mqttcertlen;
    }
    mqtt_cfg.credentials.client_id=				                who;
    mqtt_cfg.credentials.username=					            theConf.mqttUser;
    mqtt_cfg.credentials.authentication.password=		        theConf.mqttPass;
    mqtt_cfg.network.disable_auto_reconnect=                    true;       // we will manage this in recoTask
    mqtt_cfg.buffer.size=                                       MQTTBIG;         
   
    // printf("Mqtt client [%s] user [%s] pass [%s] uri [%s] \n",
    //          mqtt_cfg.credentials.client_id,mqtt_cfg.credentials.username,mqtt_cfg.credentials.authentication.password,
    //                     mqtt_cfg.broker.address.uri );

    // ! NOTICE
    // WITHOUT ssl encryption average message of 8 meters takes aprox 450ms
    // WITH ssl encryption it 1250 !!! for your information IMPORTANT
    // wss takes 2000ms

    clientCloud = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(clientCloud, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, root_mqtt_event_handler, NULL);
    return clientCloud;
}

static void root_mqtt_app_start(void)
{
        // BaseType_t xReturned;
       if( xTaskCreate(&root_mqttMgr,"mqtt",1024*10,NULL, 10, &mqttMgrHandle)!=pdPASS)      //receiving commands
            ESP_LOGE(MESH_TAG,"Fail to launch Mgr");
        if(xTaskCreate(&root_mqtt_sender,"mqttsend",1024*20,NULL, 14, &mqttSendHandle)!=pdPASS)
            ESP_LOGE(MESH_TAG,"Fail to launch Sender");
}

void mesh_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    esp_err_t err;


    mesh_addr_t id = {0,};
    static uint8_t last_layer = 0;

    switch (event_id) {
    case MESH_EVENT_STARTED: {
        esp_mesh_get_id(&id);
        // ESP_LOGI(MESH_TAG, "<MESH_EVENT_MESH_STARTED>ID:"MACSTR"", MAC2STR(id.addr));
        mesh_layer = esp_mesh_get_layer();
    }
    break;
    case MESH_EVENT_STOPPED: {
        // ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOPPED>");
        mesh_layer = esp_mesh_get_layer();
    }
    break;
    case MESH_EVENT_CHILD_CONNECTED: {
        mesh_event_child_connected_t *child_connected = (mesh_event_child_connected_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_CONNECTED>aid:%d, "MACSTR"",
                 child_connected->aid,
                 MAC2STR(child_connected->mac));
        esp_mesh_get_id(&id);
        memcpy(&id.addr,&child_connected->mac,6);
        if( esp_mesh_is_root())
        {
            err= root_send_data_to_node(id);      //send configuration to station Node including if start schedule
            if(err!=ESP_OK) 
                ESP_LOGE(MESH_TAG,"Send SSID Failed");

            logCount++;
            if(logCount+1>=theConf.totalnodes)
            {
                // if(xTimerIsTimerActive(loginTimer))
                    xTimerStop(loginTimer,0);
                if((theConf.debug_flags >> dLOGIC) & 1U) 
                    ESP_LOGI(TAG,"Login Timeout Done %d have %d",theConf.totalnodes-1,logCount);
                send_login_msg("Login Ok");
            }
        }
    }
    break;
    case MESH_EVENT_CHILD_DISCONNECTED: {
        mesh_event_child_disconnected_t *child_disconnected = (mesh_event_child_disconnected_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_DISCONNECTED>aid:%d, "MACSTR"",
                 child_disconnected->aid,
                 MAC2STR(child_disconnected->mac));
        if(sendMeterf)  //very important to ask
        {
            ESP_LOGI(MESH_TAG,"Disconnect while sending data. Retry");
            //timeout will manage to send whomever was available
            // this station NEVER sent the data so no RAM to free
        }
        if( esp_mesh_is_root())
        {
            logCount--;
            if(logCount<theConf.totalnodes-1)
                // if(!xTimerIsTimerActive(loginTimer ))
                // {
                    xTimerStart(loginTimer,0);  
                    esp_rom_printf("Login Timer restarted\n"); 
                // }

        }
    }
    break;
    case MESH_EVENT_ROUTING_TABLE_ADD: {
        mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
        ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_ADD>add %d, new:%d",
                 routing_table->rt_size_change,
                 routing_table->rt_size_new);
    }
    break;
    case MESH_EVENT_ROUTING_TABLE_REMOVE: {
        // mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
        // ESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_REMOVE>remove %d, new:%d",
                //  routing_table->rt_size_change,
                //  routing_table->rt_size_new);
    }
    break;
    case MESH_EVENT_NO_PARENT_FOUND: {
        // mesh_event_no_parent_found_t *no_parent = (mesh_event_no_parent_found_t *)event_data;
        // ESP_LOGI(MESH_TAG, "<MESH_EVENT_NO_PARENT_FOUND>scan times:%d",
                //  no_parent->scan_times);

    }
    /* TODO handler for the failure */    
    case MESH_EVENT_PARENT_CONNECTED: {
        mesh_event_connected_t *connected = (mesh_event_connected_t *)event_data;
        esp_mesh_get_id(&id);
        mesh_layer = connected->self_layer;
        memcpy(&mesh_parent_addr.addr, connected->connected.bssid, 6);
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_PARENT_CONNECTED>layer:%d-->%d, parent:"MACSTR"%s, ID:"MACSTR"",
                 last_layer, mesh_layer, MAC2STR(mesh_parent_addr.addr),
                 esp_mesh_is_root() ? "<ROOT>" :
                 (mesh_layer == 2) ? "<layer2>" : "", MAC2STR(id.addr));
        last_layer = mesh_layer;
        mesh_netifs_start(esp_mesh_is_root());   
        meshf=true;   
        }
    break;
    case MESH_EVENT_PARENT_DISCONNECTED: {
        mesh_event_disconnected_t *disconnected = (mesh_event_disconnected_t *)event_data;
        ESP_LOGI(MESH_TAG,
                 "<MESH_EVENT_PARENT_DISCONNECTED>reason:%d",
                 disconnected->reason);            
        if(esp_mesh_is_root())
        {
            hostflag=false;
            mqttf=false;
            theBlower.setStatsStaDiscos();
            char *msg=(char*)calloc(1,100);
            if(msg)
            {
                sprintf(msg,"Mesh Parent (HOST) disconnected Reason:%d",disconnected->reason);
                writeLog(msg);
                free(msg);
            }
        }
        mesh_layer = esp_mesh_get_layer();
        mesh_netifs_stop();
        meshf=false;
    }
    break;
    case MESH_EVENT_LAYER_CHANGE: {
        mesh_event_layer_change_t *layer_change = (mesh_event_layer_change_t *)event_data;
        mesh_layer = layer_change->new_layer;
        // ESP_LOGI(MESH_TAG, "<MESH_EVENT_LAYER_CHANGE>layer:%d-->%d%s",
        //          last_layer, mesh_layer,
        //          esp_mesh_is_root() ? "<ROOT>" :
        //          (mesh_layer == 2) ? "<layer2>" : "");
        last_layer = mesh_layer;
    }
    break;
    case MESH_EVENT_ROOT_ADDRESS: {
        // mesh_event_root_address_t *root_addr = (mesh_event_root_address_t *)event_data;
        // ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_ADDRESS>root address:"MACSTR"",
        //          MAC2STR(root_addr->addr));
    }
    break;
    case MESH_EVENT_VOTE_STARTED: {
        // mesh_event_vote_started_t *vote_started = (mesh_event_vote_started_t *)event_data;
        // ESP_LOGI(MESH_TAG,
        //          "<MESH_EVENT_VOTE_STARTED>attempts:%d, reason:%d, rc_addr:"MACSTR"",
        //          vote_started->attempts,
        //          vote_started->reason,
        //          MAC2STR(vote_started->rc_addr.addr));
    }
    break;
    case MESH_EVENT_VOTE_STOPPED: {
        // ESP_LOGI(MESH_TAG, "<MESH_EVENT_VOTE_STOPPED>");
        break;
    }
    case MESH_EVENT_ROOT_SWITCH_REQ: {
        // mesh_event_root_switch_req_t *switch_req = (mesh_event_root_switch_req_t *)event_data;
        // ESP_LOGI(MESH_TAG,
        //          "<MESH_EVENT_ROOT_SWITCH_REQ>reason:%d, rc_addr:"MACSTR"",
        //          switch_req->reason,
        //          MAC2STR( switch_req->rc_addr.addr));
    }
    break;
    case MESH_EVENT_ROOT_SWITCH_ACK: {
        /* new root */
        mesh_layer = esp_mesh_get_layer();
        esp_mesh_get_parent_bssid(&mesh_parent_addr);
        // ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_SWITCH_ACK>layer:%d, parent:"MACSTR"", mesh_layer, MAC2STR(mesh_parent_addr.addr));
    }
    break;
    case MESH_EVENT_TODS_STATE: {
        // mesh_event_toDS_state_t *toDs_state = (mesh_event_toDS_state_t *)event_data;
        // ESP_LOGI(MESH_TAG, "<MESH_EVENT_TODS_REACHABLE>state:%d", *toDs_state);
    }
    break;
    case MESH_EVENT_ROOT_FIXED: {
        // mesh_event_root_fixed_t *root_fixed = (mesh_event_root_fixed_t *)event_data;
        // ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_FIXED>%s",
        //          root_fixed->is_fixed ? "fixed" : "not fixed");
    }
    break;
    case MESH_EVENT_ROOT_ASKED_YIELD: {
        // mesh_event_root_conflict_t *root_conflict = (mesh_event_root_conflict_t *)event_data;
        // ESP_LOGI(MESH_TAG,
        //          "<MESH_EVENT_ROOT_ASKED_YIELD>"MACSTR", rssi:%d, capacity:%d",
        //          MAC2STR(root_conflict->addr),
        //          root_conflict->rssi,
        //          root_conflict->capacity);
    }
    break;
    case MESH_EVENT_CHANNEL_SWITCH: {
        // mesh_event_channel_switch_t *channel_switch = (mesh_event_channel_switch_t *)event_data;
        // ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHANNEL_SWITCH>new channel:%d", channel_switch->channel);
    }
    break;
    case MESH_EVENT_SCAN_DONE: {
        // mesh_event_scan_done_t *scan_done = (mesh_event_scan_done_t *)event_data;
        // ESP_LOGI(MESH_TAG, "<MESH_EVENT_SCAN_DONE>number:%d",
        //          scan_done->number);
    }
    break;
    case MESH_EVENT_NETWORK_STATE: {
        // mesh_event_network_state_t *network_state = (mesh_event_network_state_t *)event_data;
        // ESP_LOGI(MESH_TAG, "<MESH_EVENT_NETWORK_STATE>is_rootless:%d",
        //          network_state->is_rootless);
    }
    break;
    case MESH_EVENT_STOP_RECONNECTION: {
        // ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOP_RECONNECTION>");
    }
    break;
    case MESH_EVENT_FIND_NETWORK: {
        mesh_event_find_network_t *find_network = (mesh_event_find_network_t *)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_FIND_NETWORK>new channel:%d, router BSSID:"MACSTR"",
                 find_network->channel, MAC2STR(find_network->router_bssid));
    }
    break;
    case MESH_EVENT_ROUTER_SWITCH: {
        // mesh_event_router_switch_t *router_switch = (mesh_event_router_switch_t *)event_data;
        // ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROUTER_SWITCH>new router:%s, channel:%d, "MACSTR"",
        //          router_switch->ssid, router_switch->channel, MAC2STR(router_switch->bssid));
    }
    break;
    default:
        ESP_LOGI(MESH_TAG, "Mesh unknown id:%d", event_id);
        break;
    }
} 

void send_internal_emergency(char * msg, uint32_t err)
{
        mqttSender_t meshMsg;
        bzero(&meshMsg,sizeof(meshMsg));            //have to zero it for the callback and param
        cJSON *root=cJSON_CreateObject();
        if(root)
        {
            cJSON_AddStringToObject(root,"cmd","911");
            cJSON_AddStringToObject(root,"msg",msg);
            cJSON_AddNumberToObject(root,"err",err);
            char *intmsg=cJSON_PrintUnformatted(root);
            if(intmsg)
            {
                // printf("Emergency [%s]\n",intmsg);
                meshMsg.queue=emergencyQueue;
                meshMsg.msg=intmsg;
                meshMsg.lenMsg=strlen(intmsg);
                meshMsg.addr=NULL;
                if(xQueueSend(meshQueue,&meshMsg,0)!=pdPASS)
                    ESP_LOGE(MESH_TAG,"Cannot queue fram");   //queue it
            }
            cJSON_Delete(root);
        }

}

void send_login_msg(char * title)
{
    mqttSender_t        mqttMsg;
    bzero(&mqttMsg,sizeof(mqttMsg));            //have to zero it for the callback and param

    cJSON *root=cJSON_CreateObject();
    if(root)
    {
        cJSON_AddStringToObject(root,"alarm",title);
        cJSON_AddNumberToObject(root,"pool",theConf.poolid);
        cJSON_AddNumberToObject(root,"nodes",theConf.totalnodes-1);
        cJSON_AddNumberToObject(root,"logged",logCount);
        char *intmsg=cJSON_PrintUnformatted(root);
        if(intmsg)
        {  
            mqttMsg.queue=                      alarmQueue;
            mqttMsg.msg=                        intmsg;                                // freed by mqtt sender
            mqttMsg.lenMsg=                     strlen(intmsg);
            mqttMsg.code=                       NULL;           //explicit so as not to forget to null
            mqttMsg.param=                      NULL;
            if(xQueueSendFromISR(mqttSender,&mqttMsg,0)!=pdTRUE)      //will free todo from isr since its called from a timer callback ISR style 
            {
                ESP_LOGE(MESH_TAG,"Error queueing msg");
                if(mqttMsg.msg)
                    free(mqttMsg.msg);  //due to failure
            }
            else
                    //must set the wifi_event_bit SEND_MQTT_BIT, else it will just collect the message in the queue
                xEventGroupSetBits(wifi_event_group, SENDMQTT_BIT);	// Send everything now !!!!!

        }
        else
            ESP_LOGE(MESH_TAG,"No RAM for Login Timeout");
        
        cJSON_Delete(root);             
    }
    else
        ESP_LOGE(MESH_TAG,"No RAM for Login Timeout CJSON");

}

void login_timeout(TimerHandle_t timer)
{

    if((theConf.debug_flags >> dLOGIC) & 1U) 
        ESP_LOGI(TAG,"Login Timeout Expected %d have %d",theConf.totalnodes-1,logCount);
    //send a time out messsage and restart timer again
    send_login_msg("Login Timeout");


}

void init_process()
{
    esp_err_t err;

	flashSem= xSemaphoreCreateBinary();
	xSemaphoreGive(flashSem);

	tableSem= xSemaphoreCreateBinary();
	xSemaphoreGive(tableSem);

	recoSem= xSemaphoreCreateBinary();
	xSemaphoreGive(recoSem);

    scheduleSem= xSemaphoreCreateBinary();
	// xSemaphoreGive(scheduleSem);

    workTaskSem= xSemaphoreCreateBinary();
	// xSemaphoreGive(workTaskSem);

     err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
		ESP_LOGI(MESH_TAG,"No free pages erased!!!!");
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    // err=nvs_flash_init_partition("profile");
    // if (err != ESP_OK) 
    // {
	// 	ESP_LOGI(MESH_TAG,"No free pages erased Profile!!!!");
	// 	ESP_ERROR_CHECK(nvs_flash_erase_partition("profile"));
	// 	err = nvs_flash_init_partition("profile");
    // }
        //read flash        
    read_flash();
    if(theConf.centinel!=CENTINEL)
    {
        ESP_LOGI(MESH_TAG,"Centinel invalid check. Erase Config");
        erase_config();
    }
    esp_log_level_set("*",(esp_log_level_t)theConf.loglevel);   //set log level
    esp_log_level_set("mqtt_client",(esp_log_level_t)0);   //set log level
    if (theConf.test_timer_div==0)
        TEST=10;
    else
        TEST=theConf.test_timer_div;


	theConf.lastResetCode=esp_rom_get_reset_reason(0);              //store reset reason and reboot count
    theConf.bootcount++;
    write_to_flash();           
// end nvs

//lots of flags
//todo try to avoid them
// schedule counter globals for config display
    mesh_init_done=false;
    mesh_on=false;
    logCount=0; 
    pausef=schedulef=false;
    ck=ck_d=ck_h=0;
    scheduleHandle=NULL;
    wifiready=false;
    showHandle=NULL;
    oledDisp=NULL;
    meshf=false;
    mqttErrors=0;
    BASETIMER=theConf.baset;         // miliseconds 1 minute
    // mesh stuff 
    mesh_layer = -1;
    mesh_started=false;
    memset(MESH_ID,0x77,6);
    uint8_t solouno=theConf.poolid;
    memcpy(&MESH_ID[5],&solouno,1);
    esp_log_buffer_hex(MESH_TAG,&MESH_ID,6);
    mqttf=false;
    memset(&GroupID.addr ,0xff,6);
    sendMeterf=false;
    hostflag=false;
    loadedf=false;
    firstheap=false;
    clientCloud=NULL;
    lastKnowCount=0;
    if(theConf.mqttDiscoRetry==0)
        theConf.mqttDiscoRetry=MQTTRECONNECTTIME;
    
        // timers counters
    vanTimersStart=vanTimersEnd=0;

    // memory schedule data set to zero
    bzero(&scheduleData,sizeof(wschedule_t));

//cmd and info queue names derived form the Config so do it now
    sprintf(cmdQueue,"%s/%d/%s",QUEUE,theConf.poolid,MQTTCMD);
    sprintf(infoQueue,"%s/%d/%s",QUEUE,theConf.poolid,MQTTINFO);
    sprintf(alarmQueue,"%s/%d/%s",QUEUE,theConf.poolid,MQTTALARM);
    // sprintf(emergencyQueue,"%s/%s",QUEUE,MQTTEMER);
    // sprintf(cmdBroadcast,"%s/%s",QUEUE,MQTTBROADCAST);
    // sprintf(discoQueue,"%s/%s",QUEUE,MQTTDISCO);
    // sprintf(installQueue,"%s/%s",QUEUE,MQTTINSTALL);

// internal mesh commands 

    strcpy(internal_cmds[EMERGENCY],        "911");
    strcpy(internal_cmds[BOOTRESP],         "bootresp");
    strcpy(internal_cmds[SENDMETRICS],      "sendmetrics");
    strcpy(internal_cmds[METERSDATA],       "meterdata");
    strcpy(internal_cmds[PRODUCTION],       "prod");
    strcpy(internal_cmds[CONFIRMLOCK],      "confirm");
    strcpy(internal_cmds[CONFIRMLOCKERR],   "confError");
    strcpy(internal_cmds[INSTALLATION],     "install");
    strcpy(internal_cmds[REINSTALL],        "reinstall");
    strcpy(internal_cmds[CONFIRMINST],      "installconf");
    strcpy(internal_cmds[FORMAT],           "format");          //erases all fram must reinstall from scratch
    strcpy(internal_cmds[UPDATEMETER],      "update");
    strcpy(internal_cmds[ERASEMETRICS],     "erase");           //erases all metrics
    strcpy(internal_cmds[MQTTMETRICS],      "mqttmetrics");     //send all mesh metrics
    strcpy(internal_cmds[METRICRESP],       "metriscresp");     //sendhost required metrics
    strcpy(internal_cmds[SHOWDISPLAY],      "display");         //turn on the display



//external commands via mqtt
    int x=0;            //reset counter
	strcpy((char*)&cmds[x].comando,         "format");			        cmds[x].code=cmdFormat;			strcpy((char*)&cmds[x].abr,         "FRMT");		
	strcpy((char*)&cmds[++x].comando,       "netw");		            cmds[x].code=cmdNetw;			strcpy((char*)&cmds[x].abr,         "NETW");		
	strcpy((char*)&cmds[++x].comando,       "mqtt");		            cmds[x].code=cmdMQTT;			strcpy((char*)&cmds[x].abr,         "MQTT");		
	strcpy((char*)&cmds[++x].comando,       "prod");		            cmds[x].code=cmdProd;			strcpy((char*)&cmds[x].abr,         "PROD");		
	strcpy((char*)&cmds[++x].comando,       "update");		            cmds[x].code=cmdUpdate;			strcpy((char*)&cmds[x].abr,         "UPDT");		
	strcpy((char*)&cmds[++x].comando,       "erase");		            cmds[x].code=cmdEraseMetrics;   strcpy((char*)&cmds[x].abr,         "ERAS");		
	strcpy((char*)&cmds[++x].comando,       "mqttmetrics");		        cmds[x].code=cmdSendMetrics;    strcpy((char*)&cmds[x].abr,         "METR");		
    strcpy((char*)&cmds[++x].comando,       "display");		            cmds[x].code=cmdDisplay;        strcpy((char*)&cmds[x].abr,         "DISP");		
    strcpy((char*)&cmds[++x].comando,       "log");		                cmds[x].code=cmdLogs;           strcpy((char*)&cmds[x].abr,         "LOGG");		
    strcpy((char*)&cmds[++x].comando,       "reset");		            cmds[x].code=cmdReset;          strcpy((char*)&cmds[x].abr,         "REST");		
    strcpy((char*)&cmds[++x].comando,       "Panels");		            cmds[x].code=cmdPanels;         strcpy((char*)&cmds[x].abr,         "PANE");		
    strcpy((char*)&cmds[++x].comando,       "Battery");		            cmds[x].code=cmdBattery;        strcpy((char*)&cmds[x].abr,         "BATT");		
    strcpy((char*)&cmds[++x].comando,       "Sensors");		            cmds[x].code=cmdSensors;        strcpy((char*)&cmds[x].abr,         "SENS");		
    strcpy((char*)&cmds[++x].comando,       "Inverter");		        cmds[x].code=cmdInverter;       strcpy((char*)&cmds[x].abr,         "INVR");		
  
// Lock gpio PIN 
    bzero(&io_conf,sizeof(io_conf));
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;    //it will have an ext3ernal pullup anyway
    io_conf.pin_bit_mask = (1ULL<<RELAY);
    gpio_set_level((gpio_num_t)RELAY,1);        //before configuring it decide level we want it to be at start, in this case HIGH
    err=gpio_config(&io_conf);

	io_conf.pin_bit_mask = ((1ULL<<WIFILED) );     //output pins 
    gpio_set_level((gpio_num_t)WIFILED,0);          //before configuring it decide level we want it to be at start, in this case LOW
	gpio_config(&io_conf);
    

    io_conf.pin_bit_mask = ((1ULL<<BEATPIN) );     //output pins 
    gpio_set_level((gpio_num_t)BEATPIN,1);          //before configuring it decide level we want it to be at start, in this case LOW
	gpio_config(&io_conf);

    // aes init     for encryptions
    esp_aes_init( &actx );

//queues check sizes if problems

    mqtt911 = xQueueCreate( 5, sizeof( mqttMsg_t ) );
    if(!mqtt911)
        ESP_LOGE(MESH_TAG,"Failed queue 911");
    meshQueue = xQueueCreate( MAXNODES, sizeof( mqttSender_t ) );
    // meshQueue = xQueueCreate( MAXNODES, sizeof( mqttMsg_t ) );
    if(!meshQueue)
        ESP_LOGE(MESH_TAG,"Failed queue meshSend");
    mqttQ = xQueueCreate( 20, sizeof( mqttMsg_t ) );
    if(!mqttQ)
        ESP_LOGE(MESH_TAG,"Failed queue Cmd");
    mqttSender = xQueueCreate( 20, sizeof( mqttSender_t ) );
    if(!mqttSender)
        ESP_LOGE(MESH_TAG,"Failed queue Sender");
    rs485Q = xQueueCreate( 10, sizeof( rs485queue_t ) );
    if(!rs485Q)
        ESP_LOGE(MESH_TAG,"Failed queue rs485Q");



//timing stuff and timers

    uint32_t permanent_time=(theConf.totalnodes/theConf.conns)*EXPECTED_DELIVERY_TIME;
    // printf("Repeat time nodes %d conns %d permanent %d repeat %d finaltime %d\n",theConf.totalnodes,theConf.conns,permanent_time,theConf.repeat,
        permanent_time*theConf.repeat;
    if(permanent_time==0)
    {
        ESP_LOGI(MESH_TAG,"Permanent time 0...default to 500");
        permanent_time=500;
    }
    if (theConf.repeat<1 )
        theConf.repeat=1;

    sendMeterTimer=xTimerCreate("SendM",pdMS_TO_TICKS(MESHTIMEOUT),pdFALSE,NULL, []( TimerHandle_t xTimer)  
                { xEventGroupSetBits(otherGroup,REPEAT_BIT);});    // every 10secs for now -> use lambda
    
    collectTimer=xTimerCreate("Timer",pdMS_TO_TICKS(permanent_time*theConf.repeat),pdFALSE,( void * ) 0, root_collect_meter_data);    //no repeat, manually start it -> to big for lambda
    if(theConf.loginwait==0)
        theConf.loginwait=LOGINTIME;
    loginTimer=xTimerCreate("Ltim",pdMS_TO_TICKS(theConf.loginwait),pdFALSE,( void * ) 0, login_timeout);    //no repeat, manually start it -> to big for lambda
    
    confirmTimer=xTimerCreate("Confirm",pdMS_TO_TICKS(CONFIRMTIMER),pdFALSE,( void * ) 0,[] (TimerHandle_t xTimer)  
{   // lambda function
    char *cualMID;

            cJSON *root=cJSON_CreateObject();
            if(root)
            {
                cualMID = ( char* ) pvTimerGetTimerID( xTimer );

                cJSON_AddStringToObject(root,"cmd",internal_cmds[6]);
                cJSON_AddStringToObject(root,"mid",cualMID);
                cJSON_AddNumberToObject(root,"status",METER_NOT_FOUND);

                char *intmsg=cJSON_PrintUnformatted(root);
                if(intmsg)
                {
                    ESP_LOGW(MESH_TAG,"Confirm time out for [%s] ",cualMID);
                    root_send_confirmation_central(intmsg,strlen(intmsg),discoQueue);
                    free(intmsg);
                }
                cJSON_Delete(root);
            }
            else
                ESP_LOGE(MESH_TAG,"No RAM for Confirm timeout");
});

    // beatTimer=xTimerCreate("beats",pdMS_TO_TICKS(BEATTIMER),pdFALSE,( void * ) 0, [] (TimerHandle_t xTimer){theBlower.saveBlower();});    //lambda no repeat, manually start it -> use lambda



    //loggin stuff?
#ifdef LOGOPT
    logFileInit();              //log file init
    timeval localtime;

    theConf.lastKnownDate=theBlower.getReservedDate();
    localtime.tv_sec=theConf.lastKnownDate;
    localtime.tv_usec=0;                                      // no connection most likely
    settimeofday(&localtime,NULL);
#endif

//event group bits for wifi and other uses
    wifi_event_group = xEventGroupCreate();
    otherGroup = xEventGroupCreate();           //other group
}

void erase_config()
{
    ESP_LOGI(MESH_TAG,"Erase config");
    esp_wifi_restore();
    nvs_flash_erase();
    nvs_flash_init();
    bzero(&theConf,sizeof(theConf));        //init the Mother to 0 everything that is not explicitly definted
    theConf.centinel=CENTINEL;
    FILE* f = fopen(PROFILE_FILE, "w");
    fclose(f);
    // erase logifle and profiles

    //default mqtt server certificate and user/pass should be up to date 
    strcpy(theConf.mqttServer,"mqtt://64.23.180.233:1883");
    strcpy(theConf.mqttUser,"robert");
    strcpy(theConf.mqttPass,"csttpstt");
    struct limits start_limits = {90, 50, 32, 19, 31, 19, 70, 50, 70, 40, 42, 40, 42, 40, 42, 40, 42, 40, 42, 40, 42, 40, 820, 720, 850, 720, 820, 720, 820, 780, 50, 10, 5000, 0, 100, 20, 80, 20, 15, 14, 390, 340};
    theConf.milim=start_limits;
    modbSensors local_modbSensors =  {15, 42, 1.5, 1, 0, 0, -1, 20, 1, 1, 0, 0, -1, 19, 1, 1, 0, 0, -1, 17, 1, 1, 6, 8192, 0, 16, 1, 1, 2, 8192, 0, 16};
    modbInverter local_modbInverter = {10, 1, 10, 1, 2, 61530, 0, 10, 1, 1, 61518, 0, 10, 1, 1, 61517, 0, 10, 1, 2, 61528, 0, 10, 1, 1, 61526, 0, 10, 1, 1, 61527, 0, 10, 1, 2, 61522, 0, 10, 1, 2, 61520, 0, 10, 1, 1, 61518, 0, 10, 1, 1, 61517, 0};
    modbBattery local_modbBattery = {30, 3, 10, 1, 1, 276, 0, 1, 1, 1, 268, 0, 1, 1, 1, 260, 0, 1, 1, 1, 256, 0};
    modbPanels local_modbPanels = {30, 4, 10, 1, 1, 272, 0, 10, 1, 1, 271, 0, 10, 1, 1, 264, 0, 10, 1, 1, 263, 0, 1, 1, 1, 267, 0};
    theConf.modbus_inverter=local_modbInverter;
    theConf.modbus_sensors=local_modbSensors;
    theConf.modbus_battery=local_modbBattery;
    theConf.modbus_panels=local_modbPanels;

    // struct modbus mimod={7,3,6,3,5,3,4,3,16,3,1,10,11,12,13,14,2,20,21,22,23,8,80,81,82,83,84,85,86,87,88,89};
    // theConf.mimodbus=mimod;
    // strcpy(theConf.mqttServer,"mqtts://possum.lmq.cloudamqp.com:8883");
    // strcpy(theConf.mqttUser,"yavwcjrm:yavwcjrm");
    // strcpy(theConf.mqttPass,"UjKTzDJOnMN7voH-FaflNW0rP-dUXck0");
    // strcpy(theConf.mqttcert,(const char*)cert_start);       //copy default cert and len to configuration
    // theConf.mqttcertlen=cert_end-cert_start;
    // int ssllen=cert_end-cert_start;
    // memcpy(theConf.mqttcert,cert_start,ssllen);
    // theConf.mqttcertlen=ssllen;

    time((time_t*)&theConf.bornDate); 
    theConf.loglevel=           3;
    theConf.baset=              10;
    theConf.repeat=             25;                 // change before production
    theConf.totalnodes=         EXPECTED_NODES;     // a shitload of nodes
    theConf.conns=              EXPECTED_CONNS;     // big number
    strcpy(theConf.thessid,DEFAULT_SSID);
    strcpy(theConf.thepass,DEFAULT_PSWD);
    
    theConf.port=(uart_port_t)ECHO_UART_PORT;
    theConf.baud=BAUD_RATE;


    // skip sending on same kwh
    // theConf.maxSkips=           SKIPS;
    // theConf.allowSkips=         false;
    
    strcpy(theConf.kpass,DEFAULT_MESH_PASSW);       //default password
    const esp_app_desc_t *mip=esp_app_get_description();
    strcpy(theConf.lastVersion,mip->version);       //installed version as defualt last used

    // Profile default Everything is ) for the time being
    
    //nvs was initied so re-open
    esp_err_t q = nvs_open("config", NVS_READWRITE, &nvshandle);
    if(q!=ESP_OK)
        ESP_LOGE(MESH_TAG,"Error opening NVS Read File %x",q);
    write_to_flash();

}

// Routine to send mqtt messages
// VERY sensitive to timeouts in 2 places 
// first the WIFI network must be active in order for 
// MQTT service to be active
// now start manually the repeat timer, to avoid timer conflicts timeout timer and repeat timer
// so, this routine now controls when to fire the repeat timer again

// ! In general if an mqtt error disconnection or fatal is issued, this task is terminated and restarted again when recoonected
// ! Which means that timeouts and the other control programming is probably usless but good practice
void root_mqtt_sender(void *pArg)        // MQTTT data sender task
{
    mqttSender_t        mensaje;
    uint32_t            startMqtt,endMqtt;
    int                 err,whost;
    EventBits_t         uxBits;
    int                 msgid;
    time_t              now;

    xEventGroupClearBits(wifi_event_group, SENDMQTT_BIT);	// clear bit to wait on --> same as a queue wihtout queues
                                                            
    while(true)         //task is forever
    {
        alla:
        while(!hostflag)            //check if host is active...hum not solid but still...below waits on the SENDMQTT_BIT which will not be Set without WIIF
        {
            delay(300);             // if no host, no sending...wait for it display warning after X retries
            whost++;
            if(whost>10)
            {
                ESP_LOGW(MESH_TAG,"Waiting for host");
                whost=0;
            }
        }

        xEventGroupWaitBits(wifi_event_group,SENDMQTT_BIT,pdTRUE,pdFALSE,portMAX_DELAY);    //this is the starting gun, flag will be cleared on Queue needed to be evacuated
        // ESP_LOGI(MESH_TAG,"Check host: %s",hostflag?"connected":"disconnected");         //debug stuff
        startMqtt=xmillis();        
        if((theConf.debug_flags >> dMQTT) & 1U)                                                        // timer to know how long it takes to send mqtt msg
            ESP_LOGI(MESH_TAG,"%sMqttsender establish",DBG_MQTT);
        uxBits=xEventGroupClearBits(wifi_event_group, MQTT_BIT); //explicit Connect Bit
        //dynamic connect strategy
        if(!clientCloud)
            clientCloud=root_setupMqtt();           //why would this happen. Was created by the InitProcess routine
        if(clientCloud)                             //in case we cannot get a connection.... super secure programming
        {
            xEventGroupClearBits(wifi_event_group,MQTT_BIT|ERROR_BIT|DISCO_BIT);        // clear bits
            err=esp_mqtt_client_start(clientCloud);         //start a MQTT connection, wait for it in MQTT_BIT
            uxBits=xEventGroupWaitBits(wifi_event_group, MQTT_BIT|DISCO_BIT|ERROR_BIT, pdFALSE, pdFALSE,   pdMS_TO_TICKS(40000)); //explicit Connect Bit, can get stuck here
            // uxBits=xEventGroupWaitBits(wifi_event_group, MQTT_BIT|DISCO_BIT|ERROR_BIT, pdFALSE, pdFALSE,   portMAX_DELAY); //explicit Connect Bit, can get stuck here
            if(((uxBits & MQTT_BIT) ==MQTT_BIT) && err!=ESP_FAIL)       //only mqttbit connect other skip sending and mqtt client did start
            {   // great, something to do
                if((theConf.debug_flags >> dMQTT) & 1U)  
                    ESP_LOGI(MESH_TAG,"%sMqtt Sender connected",DBG_MQTT);

                while(true) //send all items in queue 
                {
                    if( xQueueReceive( mqttSender, &mensaje,  pdMS_TO_TICKS(MQTTSENDERWAIT) )==pdPASS)	//mqttHandle has a pointer to the original message. MUST be freed at some point
                    {
                        if(mensaje.msg)     
                        {   //entry has a msg to be sent
                            // if(mensaje.queue==emergencyQueue){
                            //     ESP_LOGI(MESH_TAG,"Publish sender len %d buff [%s]",mensaje.lenMsg,mensaje.msg);
                            // }
                            // ESP_LOG_BUFFER_HEX(MESH_TAG,mensaje.msg,mensaje.lenMsg);
                            msgid=esp_mqtt_client_publish(clientCloud,(char*) mensaje.queue, (char*)mensaje.msg,mensaje.lenMsg,QOS1,NORETAIN);
                            if((theConf.debug_flags >> dMQTT) & 1U)  
                                ESP_LOGI(MESH_TAG,"%sPublish msgid %x len %d",DBG_MQTT,msgid,mensaje.lenMsg);
                            if(msgid==ESP_FAIL)   //msg_id cannot be negative, means error
                            {
                                ESP_LOGE(MESH_TAG,"Publish failed %x",msgid);
                                if(mensaje.msg)
                                    free(mensaje.msg); 
                                if(mensaje.param)
                                    free(mensaje.param);    
                                    //failure implies all this, disconnect stop 
                                err=esp_mqtt_client_disconnect(clientCloud);                // added May 8/2025... if no connect no uxbit so stuck, now timedout
                                err=esp_mqtt_client_stop(clientCloud); 
                                sendMeterf=false;    //if we break reet senderflag
                                break;      //wait for next time slot....should free things up
                            }
                            else
                            {       // wait for published bit or error
                                uxBits=xEventGroupWaitBits(wifi_event_group, PUB_BIT|DISCO_BIT|ERROR_BIT, pdFALSE, pdFALSE,   pdMS_TO_TICKS(1000)); //explicit Connect Bit
                                // uxBits=xEventGroupWaitBits(wifi_event_group, PUB_BIT|DISCO_BIT|ERROR_BIT, pdFALSE, pdFALSE,   portMAX_DELAY); //explicit Connect Bit
                                if(((uxBits & PUB_BIT) !=PUB_BIT))   
                                {
                                    ESP_LOGE(MESH_TAG,"Pub failed Error or Disco or Timeout %x",uxBits);
                                    if(mensaje.msg)
                                        free(mensaje.msg); 
                                    if(mensaje.param)
                                        free(mensaje.param); 
                                    //failure implies all this, disconnect stop 
                                    err=esp_mqtt_client_disconnect(clientCloud);                // added May 8/2025... if no connect no uxbit so stuck, now timedout
                                    err=esp_mqtt_client_stop(clientCloud); 
                                    sendMeterf=false;    //if we break reet senderflag
                                    break;
                                }
                                endMqtt=xmillis();          
                                xEventGroupClearBits(wifi_event_group,PUB_BIT|DISCO_BIT|ERROR_BIT);     //could be done in firt call but explicit here
                                theBlower.setStatsMsgOut();  //increment count
                                theBlower.setStatsBytesOut(mensaje.lenMsg);
                                //see if callback given
                                if(mensaje.code)
                                {
                                    // printf("Callback %p param %ld\n",mensaje.code,mensaje.param);
                                    (*mensaje.code)((void*)mensaje.param);	// call back should be fast
                                }
                                //free msg and param MUST be a pointer to anything
                                if(mensaje.msg)
                                    free(mensaje.msg);          //BIGF responsability if not heap collapses
                                if(mensaje.param)
                                    free(mensaje.param); 
                                if((theConf.debug_flags >> dMQTT) & 1U)  
                                    ESP_LOGI(MESH_TAG,"%sMsgLen %d Msgs sent %d msec %d",DBG_MQTT,mensaje.lenMsg,theBlower.getStatsMsgOut(),endMqtt-startMqtt);
                                // if ((theBlower.getStatsMsgOut() % SAVEDATE)==0)
                                // {
                                //     time(&now);
                                //     theBlower.setReservedDate(now);
                                // }
                            }
                        }
                        else
                        {
                            ESP_LOGE(MESH_TAG,"No message to send but Process activated");        
                            if(mensaje.param)
                                free(mensaje.param);        //in case this has a pointer
                        }
                    }               
                    else        // queue empty, now close
                    {
                        sendMeterf=false;   //best way yet to sync sending and done sending 
                        err=esp_mqtt_client_disconnect(clientCloud);                    // disconnect to reduce connections required globaly as a System
                        xEventGroupWaitBits(wifi_event_group, DISCO_BIT|ERROR_BIT, pdFALSE, pdFALSE,   portMAX_DELAY); //explicit Connect Bit
                        err=esp_mqtt_client_stop(clientCloud);
                        if((theConf.debug_flags >> dMQTT) & 1U)  
                            ESP_LOGI(MESH_TAG,"%sConn closed and Queue empty",DBG_MQTT);
                        break;  //get out of send loop , all msgs were sent           
                    }
                }

            }
            else  
            {   
                // some problem, disconnect or error, in etiher case Task was released and will start loop again
                ESP_LOGE(MESH_TAG,"Did not get connect bit or error %x\n",uxBits);
                sendMeterf=false;
                err=esp_mqtt_client_disconnect(clientCloud);                // added May 8/2025... if no connect no uxbit so stuck, now timedout
                err=esp_mqtt_client_stop(clientCloud);
                while(true)
                {   //free queueed messages 
                    if( xQueueReceive( mqttSender, &mensaje,  pdMS_TO_TICKS(MQTTSENDERWAIT) )==pdPASS)	//mqttHandle has a pointer to the original message. MUST be freed at some point
                    {
                        if(mensaje.msg)
                            free(mensaje.msg);
                        if(mensaje.param)
                            free(mensaje.param);
                    }
                    else
                        break;
                }
                // delay(100);  //in case its very fast
            }
            //start again the collecting cycle
                if( xTimerStart(collectTimer, 0 ) != pdPASS )
                    ESP_LOGE(MESH_TAG,"Repeat Timer mqtt saender failed");
        }
        else    // NO connection to MQTT, but Host is active
        {
            sendMeterf=false;           //reset flag so we can try again, even if it suspicious that it could not open the connection
            ESP_LOGE(MESH_TAG,"Cannot connect to ClientCloud");
        }
        delay(100);
    }
}   //should never leave this while its a Task


/**
 * ? que sera
 * ! warning
 * * Important
 * TODO: algo
 * @param xTimer este parametro
 */
//  void root_firstCallback( TimerHandle_t xTimer )
//  {

//     int ulCount = ( uint32_t ) pvTimerGetTimerID( xTimer );
//     if(!theConf.meterconf)  
//         return;

//     // collect_meter_data(NULL);           // first time 
    
//     // collectTimer=xTimerCreate("Timer",pdMS_TO_TICKS(permanent_time*theConf.repeat),pdFALSE,( void * ) ulCount, collect_meter_data);    //no repeat, manually start it
//     // collectTimer=xTimerCreate("Timer",pdMS_TO_TICKS(permanent_time*theConf.repeat),pdTRUE,( void * ) ulCount, collect_meter_data);    // every hour for now
//     if( xTimerStart(collectTimer, 0 ) != pdPASS )
//         ESP_LOGE(MESH_TAG,"Repeat Timer failed");
//  }

void root_set_senddata_timer()
{
    // get our defined time slot based on the controllerid and expected publish time... in seconds
    // mqttSlots will be defined by default and modified by Host via any cmd message with SLOT primitive in Json msg
       
    time_t      now;
    struct tm   timeinfo;
    int         cuando=0;

    time(&now);
    // printf("Just now %lu %s",(int)now,ctime(&now));

    localtime_r(&now, &timeinfo);
    timeinfo.tm_hour=0;
    timeinfo.tm_min=0;
    timeinfo.tm_sec=0; //midnight hour
    time_t midnight = mktime(&timeinfo);
    // printf("Midnight %lu [%s]\n",(int)midnight,ctime(&midnight));
    int cycles=theConf.totalnodes/theConf.conns;
    
    int secs_from_midnight=int(now-midnight);
    // printf("Secs from midnight %d cycles %d\n",secs_from_midnight,cycles);

    int currCycle= (secs_from_midnight/EXPECTED_DELIVERY_TIME) % cycles;
    int passedCycle =(secs_from_midnight/EXPECTED_DELIVERY_TIME) / cycles;
    // printf("Passedcycles %d curcycle %d\n",passedCycle,currCycle);
    int time_to_next_cycle;

    // printf("Now %d Mid night %d secMid %d Secs to resync %d repeattime %d secs\n",(int)now,
        //    (int)midnight,secs_from_midnight,time_to_next_cycle*theConf.baset,theConf.totalnodes/theConf.conns*EXPECTED_DELIVERY_TIME);
    
    if(esp_mesh_is_root())
    {
        // if(theConf.baset<1)
            theConf.baset=10;
        firstTimer=xTimerCreate("Timer",pdMS_TO_TICKS(100*theConf.baset),pdFALSE,( void * ) 10,
        // firstTimer=xTimerCreate("Timer",pdMS_TO_TICKS(time_to_next_cycle*theConf.baset),pdFALSE,( void * ) 10,
        [](TimerHandle_t xTimer)
        {
            // first timer called, we are now in our alloted time cycle
            int ulCount = ( uint32_t ) pvTimerGetTimerID( xTimer );
            if(!theConf.meterconf)  
                return;

            root_collect_meter_data(NULL);              // do this cycle call directly, dont skip it then start timer for repeat frequency
            if( xTimerStart(collectTimer, 0 ) != pdPASS )
                ESP_LOGE(MESH_TAG,"Repeat Timer failed");
        }
        );   //will be called once Mesh  established
        if( xTimerStart(firstTimer, 0 ) != pdPASS )
        {
            mqttSender_t emergencyMsg;

            ESP_LOGE(MESH_TAG,"First Timer failed F A T A L");            // deadly!!!! will never send anything
            char *mensa=(char*)calloc(1,200);
            if(!mensa)
            {
                ESP_LOGE(MESH_TAG,"First Timer no emergency ram");            
                esp_restart();                                          //really screwed, try restarting 
            }
            sprintf(mensa,"First Timer failed Node %d",theConf.poolid);
            emergencyMsg.msg=mensa;
            emergencyMsg.lenMsg=strlen(mensa);
            if(xQueueSend(mqtt911,&emergencyMsg,0)!=pdPASS)
            {
                ESP_LOGE(MESH_TAG,"Cannot send emergency message first timer");
                free(mensa);        //for consistency's sake
                writeLog(mensa);
                delay(1000);
                esp_restart();      //screwed, now we restart and try again
            }
        }
    }
}

void post_root()
{
    xTaskCreate(&root_sntpget,"sntp",4096,NULL, 10, NULL); 	        // get real time
}

// void check_installation()
// {
//     mesh_data_t         data;
//     mqttSender_t meshMsg;

//     if(theConf.meterconf==1)        //for redundancy
//     {
//         //queue msg to meshsender that we have been installed so it can be relayed to Central wioth its conditions
//         if(strlen(theConf.instMsg)>10)
//         {
//             char *intmsg=(char*)calloc(1,theConf.instMsglen+4);
//             if(intmsg)
//             {
//                 memcpy(intmsg,theConf.instMsg,theConf.instMsglen);
//                 meshMsg.msg=intmsg;
//                 meshMsg.lenMsg=strlen(intmsg);
//                 meshMsg.addr=NULL;          // to root
//                 if(xQueueSend(meshQueue,&meshMsg,0)!=pdTRUE)      //will free todo 
//                 {
//                     ESP_LOGE(MESH_TAG,"Error queueing confirm msg");
//                     if(meshMsg.msg)                 // already checked but ...
//                         free(meshMsg.msg);          //due to failure
//                 }
//             }
//         }
//     }
// }


void ip_event_handler(void *arg, esp_event_base_t event_base,int32_t event_id, void *event_data)
{
    modbus_sensor_type_t  sensor;

    if (event_id ==IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(MESH_TAG, "<IP_EVENT_STA_GOT_IP>IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_current_ip.addr = event->ip_info.ip.addr;
        theBlower.setStatsStaConns();
        esp_netif_t *netif = event->esp_netif;
        esp_netif_dns_info_t dns;
        ESP_ERROR_CHECK(esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns));
        mesh_netif_start_root_ap(esp_mesh_is_root(), dns.ip.u_addr.ip4.addr);
        hostflag=true;
        gpio_set_level((gpio_num_t)WIFILED,1);
        char * prompt=(char*)calloc(1,40);

        if (esp_mesh_is_root() && !loadedf) 
        {
            loadedf=true;
            meshf=true;
            root_mqtt_app_start();
            post_root();
            xTaskCreate(&root_emergencyTask,"e911",2048,NULL, 5, NULL);
            xTaskCreate(&blinkRoot,"root",1024,(void*)400, 5, &blinkHandle);
            if(theConf.totalnodes>0)
                xTimerStart(loginTimer,0);          // start the login timer if more that 0 nodes
            // sprintf(prompt,"R%s",theBlower.getMID());
            // showLVGL(prompt,10000,3);   	
            launch_sensors();
        }
        else
        {
            ESP_LOGE(MESH_TAG,"IP child");	
            launch_sensors();
            // sprintf(prompt,"N%s",theBlower.getMID());
            // showLVGL(prompt,10000,3);   
        } 
        // // esp_console_ xTaskCreate(&rs485_task,"modbus",1024*10,NULL, 5, NULL); repl_t* veamos=repl;    //new structure type
        // create a shitload of undefined and weird problems DO NOT DO THIS
        // void * dale=(void*)repl;          //make it byte oriented
        // // void * dale=(void*)veamos;          //make it byte oriented
        // sprintf(prompt,"%s%d-%s>",esp_mesh_is_root()?"Root":"Node",theConf.controllerid,theBlower.getMID());

        // esp_console_setup_prompt(prompt,(esp_console_repl_com_t*)dale);

        // if(theConf.meterconf==1)
        //     check_installation();

        xTaskCreate(&meshSendTask,"msend",1024*5,NULL, 10, NULL); 	            //now that its active, you can send any queued messages    

    }
    if(event_id ==IP_EVENT_STA_LOST_IP)
    {
       ESP_LOGI(MESH_TAG,"Lost ip");    //never gets called but Parent disconnect does
        hostflag=false;        
        gpio_set_level((gpio_num_t)WIFILED,0);
        char *tmp=(char*)calloc(1,100);
        if (tmp)
        {
            sprintf(tmp,"Lost ip as %s",esp_mesh_is_root()?"Roo9t":"Node");
            writeLog(tmp);
            free(tmp);
        }
    }

}

void blinkSSID(void *pArg)
{
    uint32_t cuanto=(uint32_t)pArg;
    while(true)
    {
        gpio_set_level((gpio_num_t)WIFILED,1);
        gpio_set_level((gpio_num_t)BEATPIN,0);
        delay(cuanto);
        gpio_set_level((gpio_num_t)WIFILED,0);
        gpio_set_level((gpio_num_t)BEATPIN,1);
        delay(cuanto);
    }
}

static void wifi_event_handler_ap(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        // wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        // ESP_LOGI(MESH_TAG, "station "MACSTR" join, AID=%d",
        //          MAC2STR(event->mac), event->aid);
        xTaskCreate(&blinkSSID,"bssid",4096,(void*)SSIDBLINKTIME, 5, &ssidHandle); 	        //SSID Blink

    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        // wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        // ESP_LOGI(MESH_TAG, "station "MACSTR" leave, AID=%d",
        //          MAC2STR(event->mac), event->aid);// test remote
    if(ssidHandle)
    {
        gpio_set_level((gpio_num_t)BEATPIN,1);
        vTaskDelete(ssidHandle);
        ssidHandle=NULL;
    }
    }
    else if (event_id == WIFI_EVENT_STA_START)
        esp_wifi_connect();
}

void wifi_init_network()
{
    uint8_t         mac[6];
    wifi_config_t   wifi_config;

    bzero(&wifi_config,sizeof(wifi_config));

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler_ap,
                                                        NULL,
                                                        NULL));

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B));        // MUST BE
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    esp_wifi_get_mac(WIFI_IF_AP,mac);
    bzero(apssid,sizeof(apssid));
    sprintf(apssid,"%s%02X%02X%02X\0",APPNAME, mac[3],mac[4],mac[5]);
    // printf("%s%02X%02X%02X%02X%02X%02X\0",APPNAME, mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    strcpy(appsw,DEFAULT_MESH_PASSW);   //Data is protected via Challenge Id. This passw should be "irrelevant"
    // printf("Config AP [%s] Passw [%s]\n",apssid,appsw);
    memcpy(&wifi_config.ap.ssid,apssid,strlen(apssid));
    memcpy(&wifi_config.ap.password,appsw,strlen(appsw));
    wifi_config.ap.ssid_len = strlen(apssid);
    wifi_config.ap.channel = 4;
    wifi_config.ap.max_connection = 4;
    // wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    //start web server
    xTaskCreate(&start_webserver,"webs",1024*10,NULL, 5, NULL); 	       
// printf("returning wifiinit\n");
}

void meter_configure()
{
    mqttSender_t    mensaje;
    char            tmp[20];

    // if(theConf.ptch)
    // {
        // printf("Start timer\n");
        // if( xTimerStart(reconfTimer, 0 ) != pdPASS )
        //     ESP_LOGI(MESH_TAG,"Repeat Timer failed to start");
    // }
    wifi_init_network();
    if(theConf.meterconf==0)
    {
    // seed the CID base code for Configuration
        int cid= (esp_random() % 99999999);
        sprintf(tmp,"%s %d",apssid,cid);

        printf("\nSSID %s\n",tmp);
        showLVGL(tmp,600000,1);     
        theConf.cid=cid;
        write_to_flash();
    }
    else
    {
        showLVGL((char*)"CONF",600000,1); 
	}
    
    while(true)
    {
        delay(1000);        // Webserver will restart after configuration or timeout
    }
}

esp_err_t mesh_enable(void) {

    
  esp_err_t err = ESP_OK;
    char            missid[30],mipassw[18];
mesh_addr_t mesh_self_addr;
esp_rom_printf("Start Mesh\n");
        if(strlen(theConf.thessid)==0)
    {
        strcpy(missid,DEFAULT_MESH_SSID);
        strcpy(mipassw,DEFAULT_MESH_PASSW); //8 chars at least
        strcpy(theConf.thessid,DEFAULT_MESH_SSID);
        strcpy(theConf.thepass,DEFAULT_MESH_PASSW);
        write_to_flash();
    }
    else
    {
        strcpy(missid,theConf.thessid);
        strcpy(mipassw,theConf.thepass);
    }
esp_rom_printf("SSID [%s] len %d Pass [%s] len %d\n",missid,strlen(missid),mipassw,strlen(mipassw));
// theConf.masternode=true;
  if (!mesh_on) {
    if (mesh_init_done == false) {
      ESP_LOGW(MESH_TAG, "Initializing mesh network");
    } else {
      ESP_LOGW(MESH_TAG, "Enabling mesh network");
    }
    if (mesh_init_done == false) {
    //   disableCore0WDT();
      ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_init());
      ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_loop_create_default());
      ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_create_default_wifi_mesh_netifs(NULL, NULL));
      wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
      ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_init(&config));
      ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));
      ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_storage(WIFI_STORAGE_RAM));
      ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_mode(WIFI_MODE_APSTA));
      ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_ps(WIFI_PS_NONE));
    }
    // ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_country(&wifi_country));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_LR));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_start());
    uint8_t i = 72;
    while (esp_wifi_set_max_tx_power(i) == ESP_OK) {
      i++;
    }

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_mesh_init());
    if (theConf.masternode) {
      ESP_ERROR_CHECK_WITHOUT_ABORT(esp_mesh_set_type(MESH_ROOT));
    } else {
      ESP_ERROR_CHECK_WITHOUT_ABORT(esp_mesh_fix_root(true));
      ESP_ERROR_CHECK_WITHOUT_ABORT(esp_mesh_set_group_id(&GroupID, 1));
    }
    if (mesh_init_done == false) {
      ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, NULL));
    }
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_mesh_set_topology(MESH_TOPO_CHAIN));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_mesh_set_max_layer(10));
    // ESP_ERROR_CHECK_WITHOUT_ABORT(esp_mesh_set_max_layer(mesh_settings.max_layer));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_mesh_set_vote_percentage(1));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_mesh_set_xon_qsize(16));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_mesh_disable_ps());
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_mesh_set_ap_assoc_expire(10));
    mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
    // memcpy((uint8_t *)&cfg.mesh_id, mesh_settings.id, 6);
        memcpy((uint8_t *) &cfg.mesh_id, MESH_ID, 6);       //was setup in init_vars

    cfg.channel = 0;
    // cfg.channel = mesh_settings.channel;
    cfg.allow_channel_switch = true;
    ESP_ERROR_CHECK(esp_mesh_set_ap_authmode((wifi_auth_mode_t)CONFIG_MESH_AP_AUTHMODE));
    // ESP_ERROR_CHECK_WITHOUT_ABORT(esp_mesh_set_ap_authmode(mesh_settings.auth_mode));

    // esp_mesh_set_ie_crypto_key(mesh_settings.crypt_key, strlen(mesh_settings.crypt_key));
    cfg.mesh_ap.max_connection = 10;
    // cfg.mesh_ap.max_connection = mesh_settings.max_conn;
    // memcpy((uint8_t *)&cfg.mesh_ap.password, mesh_settings.ap_pass, strlen(mesh_settings.ap_pass));

        cfg.router.ssid_len =strlen( missid);
    memcpy((uint8_t *) &cfg.router.ssid, missid, cfg.router.ssid_len);
    memcpy((uint8_t *) &cfg.router.password, mipassw,strlen(mipassw));
        memcpy((uint8_t *)&cfg.mesh_ap.password, mipassw, strlen(mipassw));


    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_mesh_set_config(&cfg));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_mesh_start());
    if (mesh_init_done == false) {
    //   xTaskCreate(&get_message, "get_message", 9216, NULL, 5, &get_message_handle);
      ESP_LOGI(MESH_TAG, "Created mesh receive task");
    //   enableCore0WDT();
      ESP_ERROR_CHECK_WITHOUT_ABORT(esp_read_mac(mesh_self_addr.addr,(esp_mac_type_t) 0));
    } else {
    //   ESP_ERROR_CHECK_WITHOUT_ABORT(esp_mesh_comm_p2p_start());
    }
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_mesh_set_self_organized(true, false));
    mesh_on = true;
    if (mesh_init_done == false) {
      if (esp_mesh_is_root()) {
        // memcpy(&mesh_root_addr, &mesh_self_addr, sizeof(mesh_addr_t));
        // memcpy(&node_status[0].address, &mesh_self_addr, sizeof(mesh_addr_t));
        // node_status[0].exists = true;
        // node_status[0].connected = true;
        // node_status[0].current_version = SOFTWARE_VERSION;
        // node_status[0].uptime = (uint32_t)(esp_timer_get_time() / 1000000ULL);
        // node_status[0].mesh_layer = esp_mesh_get_layer();
      }
      mesh_init_done = true;
    }
    uint8_t wifi_protocol_ap;
    uint8_t wifi_protocol_sta;
    esp_wifi_get_protocol(WIFI_IF_AP, &wifi_protocol_ap);
    esp_wifi_get_protocol(WIFI_IF_STA, &wifi_protocol_sta);
    mesh_addr_t id;
    esp_mesh_get_id(&id);
    int8_t max_tx_power;
    esp_wifi_get_max_tx_power(&max_tx_power);

    ESP_LOGI(MESH_TAG, "Mesh starts successfully");
    ESP_LOGI(MESH_TAG, "Mesh type:                                  %s", (esp_mesh_is_root()) ? "Root" : "Node");
    ESP_LOGI(MESH_TAG, "Mesh ID:                       " MACSTR "", MAC2STR(id.addr));
    ESP_LOGI(MESH_TAG, "Station MAC:                   " MACSTR "", MAC2STR(mesh_self_addr.addr));
    ESP_LOGI(MESH_TAG, "Channel:                                       %d", cfg.channel);
    ESP_LOGI(MESH_TAG, "Max. connected stations:                       %d", cfg.mesh_ap.max_connection);
    ESP_LOGI(MESH_TAG, "STA protocol:                         %s%s%s%s", (wifi_protocol_sta == 1) ? "802.11b" : "", (wifi_protocol_sta == 2) ? "802.11g" : "", (wifi_protocol_sta == 4) ? "802.11n" : "", (wifi_protocol_sta == 8) ? "Long Range" : "");
    ESP_LOGI(MESH_TAG, "AP protocol:                          %s%s%s%s", (wifi_protocol_ap == 1) ? "802.11b" : "", (wifi_protocol_ap == 2) ? "802.11g" : "", (wifi_protocol_ap == 4) ? "802.11n" : "", (wifi_protocol_ap == 8) ? "Long Range" : "");
    ESP_LOGI(MESH_TAG, "Max. layer:                                  %d", esp_mesh_get_max_layer());
    ESP_LOGI(MESH_TAG, "Topology:                                  %s", (esp_mesh_get_topology()) ? "Chain" : "Tree");
    ESP_LOGI(MESH_TAG, "Root fixed:                                  %s", (esp_mesh_is_root_fixed()) ? "Yes" : "No");
    ESP_LOGI(MESH_TAG, "Power save enabled:                           %s", (esp_mesh_is_ps_enabled()) ? "Yes" : "No");
    ESP_LOGI(MESH_TAG, "Max. TX power:                          %.1f dBm", (float)max_tx_power * 0.25);
    ESP_LOGI(MESH_TAG, "Free heap:                                 %d", esp_get_free_heap_size());
    // ESP_LOGI(MESH_TAG, "MF ID:      %s", basic_settings.mf_id);
    // ESP_LOGI(MESH_TAG, "MF key:     %s", basic_settings.mf_key);
    // ESP_LOGI(MESH_TAG, "MF chan:    %s", basic_settings.mf_channel);
  } else {
    ESP_LOGW(MESH_TAG, "Mesh network already enabled!");
  }
  return err;
}

void start_mesh()
{
    int             err;
    char            missid[30],mipassw[18];

    // MESH sequence start
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    /*  crete network interfaces for mesh (only station instance saved for further manipulation, soft AP instance ignored */
    ESP_ERROR_CHECK(mesh_netifs_init(mesh_manager));
    wifi_init_config_t configg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&configg));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,IP_EVENT_STA_LOST_IP, &ip_event_handler, NULL));
    if(!theConf.masternode)
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_LR));
    // ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_start());
    if(strlen(theConf.thessid)==0)
    {
        strcpy(missid,DEFAULT_MESH_SSID);
        strcpy(mipassw,DEFAULT_MESH_PASSW); //8 chars at least
        strcpy(theConf.thessid,DEFAULT_MESH_SSID);
        strcpy(theConf.thepass,DEFAULT_MESH_PASSW);
        write_to_flash();
    }
    else
    {
        strcpy(missid,theConf.thessid);
        strcpy(mipassw,theConf.thepass);
    }
// nada
// printf("Missid [%s] pass [%s]\n",missid,mipassw);
        /*  mesh initialization */
    ESP_ERROR_CHECK(esp_mesh_init());
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mesh_set_max_layer(CONFIG_MESH_MAX_LAYER));
    ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(1));
    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(10));
    ESP_ERROR_CHECK(esp_mesh_send_block_time(30000));       
    ESP_ERROR_CHECK(esp_mesh_disable_ps());
    mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
    memcpy((uint8_t *) &cfg.mesh_id, MESH_ID, 6);       //was setup in init_vars
esp_log_buffer_hex("MESHID",&cfg.mesh_id,6);

    cfg.channel = 0;        //has to be 0 NO IDEA WHY -> all have to have the same Channel
    
    //Router credentials STA
    cfg.router.ssid_len =strlen( missid);
    memcpy((uint8_t *) &cfg.router.ssid, missid, cfg.router.ssid_len);
    memcpy((uint8_t *) &cfg.router.password, mipassw,strlen(mipassw));
   
   // softAP credientials
    ESP_ERROR_CHECK(esp_mesh_set_ap_authmode((wifi_auth_mode_t)CONFIG_MESH_AP_AUTHMODE));
    cfg.mesh_ap.max_connection = CONFIG_MESH_AP_CONNECTIONS;
    cfg.mesh_ap.nonmesh_max_connection = CONFIG_MESH_NON_MESH_AP_CONNECTIONS;
    memcpy((uint8_t *) &cfg.mesh_ap.password, DEFAULT_MESH_PASSW,8);            //Only password required
   err=esp_mesh_set_config(&cfg);
    if (err)
    {
        ESP_LOGE(MESH_TAG,"Init Mesh err %x");
        if(err==0x4008)
        {
            ESP_LOGE(MESH_TAG,"pasword len error");
            erase_config();
            esp_restart();
        }
    }
    /* mesh start */
    // Broadcast address setup
    //set a global flag that MESH has started. No esp_mesh_get-running or similar call so use flag
    esp_err_t errReturn = esp_mesh_set_group_id(&GroupID, 1);
    if(errReturn)
        ESP_LOGI(MESH_TAG,"Failed to resgister Mesh Broadcast");

     esp_mesh_set_topology(MESH_TOPO_CHAIN);
    ESP_ERROR_CHECK(esp_mesh_set_ie_crypto_funcs(NULL));
    if(theConf.masternode)
        {

        }
    // extremely important for MOTA else lost packages
    // int xon=esp_mesh_get_xon_qsize();
    // esp_mesh_set_xon_qsize(xon*10);      // 100 ms delay sustained
    
    ESP_ERROR_CHECK(esp_mesh_start());

    mesh_started=true;  //for kbd and other aux routines
    ESP_LOGI(MESH_TAG, "mesh starts successfully, heap:%d, %s\n",  esp_get_free_heap_size(),
             esp_mesh_is_root_fixed() ? "root fixed" : "root not fixed");
}


time_t get_today_midnight() {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    // Set time components to midnight
    tm_info->tm_hour = 0;
    tm_info->tm_min = 0;
    tm_info->tm_sec = 0;
    
    return mktime(tm_info);
}
void find_cycle_day(uint8_t * ciclo,uint8_t*dia)
{
    // check profile and day set
    printf("Find Cycle %d Dia %d\n",theConf.activeProfile,theConf.dayCycle);
    if(theConf.activeProfile>=0 && theConf.dayCycle>=0)
    {
        //given the day, find the current cycle and day to start
        for (int a=0;a<theConf.profiles[theConf.activeProfile].numCycles;a++)
        {
            if(theConf.profiles[theConf.activeProfile].cycle[a].day+theConf.profiles[theConf.activeProfile].cycle[a].duration>theConf.dayCycle)
            {
                //in this cyycle a
                //days to skip start+duracion - daycycle
                *dia=theConf.dayCycle-theConf.profiles[theConf.activeProfile].cycle[a].day;
                // *dia=theConf.profiles[theConf.activeProfile].cycle[a].duration-(theConf.dayCycle-theConf.profiles[theConf.activeProfile].cycle[a].day);
                *ciclo=a;
                    printf("Found it Cycle %d Dia %d\n",*ciclo,*dia);

                return;
            }
        }
        *ciclo=0;
        *dia=0;
    }
    ESP_LOGE(TAG,"Invalid Profile %d or Day Start %d",theConf.activeProfile,theConf.dayCycle);
}

 void start_schedule_timers(void * pArg)
 {
    time_t now = time(NULL);
    time_t midn=get_today_midnight();
    time_t son,sone,starttime,endtime;
    char aca[20];
    uint8_t cyclestart,daystart;
    int desde=0;

    while(true)
    {
        if(xSemaphoreTake(workTaskSem, portMAX_DELAY))	// start the active profile
        {
            ESP_LOGW(TAG,"Started Production cycles");
            find_cycle_day(&cyclestart,&daystart);
            schedulef=true;         // active for all production cmds
            if((theConf.debug_flags >> dSCH) & 1U)
                ESP_LOGW(TAG,"%sStart Cycle %d Start Day %d",DBG_SCH,cyclestart,daystart);
            for (ck=cyclestart;ck<theConf.profiles[0].numCycles;ck++)       // cycles
            {
                if (ck==cyclestart) 
                    desde=daystart;
                else
                    desde=0;

                scheduleData.currentCycle=ck;      // save cycle for status reporting

                for (ck_d=desde; ck_d<theConf.profiles[0].cycle[ck].duration;ck_d++)    //  days
                {
                    scheduleData.currentDay=ck_d;           // save day for status reporting

                    if((theConf.debug_flags >> dSCH) & 1U)  
                    for (int ck_h=0;ck_h<theConf.profiles[0].cycle[ck].numHorarios;ck_h++)      // hours
                    {
                        while(pausef)
                            delay(1000);
                        scheduleData.currentHorario=ck_h;      //save horario for status reporting
                        scheduleData.currentStartHour=theConf.profiles[0].cycle[ck].horarios[ck_h].hourStart;
                        scheduleData.currentEndHour=theConf.profiles[0].cycle[ck].horarios[ck_h].horarioLen;

                        starttime=midn+theConf.profiles[0].cycle[ck].horarios[ck_h].hourStart*3600;    //in seconds
                        endtime=starttime+theConf.profiles[0].cycle[ck].horarios[ck_h].horarioLen*3600;    //in seconds
                        if((theConf.debug_flags >> dSCH) & 1U)
                        {
                            sprintf(aca,"%sC-%d D-%d %d",DBG_SCH,ck,ck_d,theConf.profiles[0].cycle[ck].horarios[ck_h].horarioLen*3600);
                            ESP_LOGI(TAG,"%s",aca);
                        }
                        // printf("Start[%d] %ld %s",h,(long)starttime,ctime(&starttime));
                        // printf("End[%d] %ld %s",h,(long)endtime,ctime(&endtime));
                        // printf("Now %ld %s",(long)now,ctime(&now));
                        if(starttime<now)
                        {
                            if((theConf.debug_flags >> dSCH) & 1U)
                                ESP_LOGI(TAG,"%sStart already happened %ld %ld",DBG_SCH,(long)starttime,(long)now);
                            if(endtime<now)
                            {
                                if((theConf.debug_flags >> dSCH) & 1U)       
                                    ESP_LOGI(TAG,"%SEnd already happende. Skip this schedule %ld %ld",DBG_SCH,(long)endtime,(long)now);
                            }
                            else
                            {
                                son=endtime-now;
                                // printf("Turn on blower inmmediately and wait for %d secs %d\n",(long)son);
                                            //set start timers start and end  off all hours
                                end_timers[vanTimersEnd]=xTimerCreate(NULL,pdMS_TO_TICKS(son),pdFALSE,(ck_h==theConf.profiles[0].cycle[ck].numHorarios-1)?(void*)0xFFFFFFFF:( void * ) vanTimersEnd, blower_end);
                                xTimerStart(end_timers[vanTimersEnd],10);
                                vanTimersEnd++;
                            }
                        }
                        else
                        {
                            son=starttime-now;
                            printf("Start timer in %ld secs %ld %ld millis %ld\n",(long)son,(long)starttime,(long)now);
                            sone=endtime-now;
                            printf("End timer in %d sec %ld %ld\n",(long)sone,(long)endtime,(long)now);

                            start_timers[vanTimersStart]=xTimerCreate(NULL,pdMS_TO_TICKS(son/TEST),pdFALSE,(void*)vanTimersStart, blower_start);
                            if(!start_timers[vanTimersStart])
                            {
                                ESP_LOGE(MESH_TAG,"Start Timer not created %d",vanTimersStart);
                                //do something drastic
                            }
                            // for simulation, end timers takes longer to see the effect of blower status reporting
                            end_timers[vanTimersEnd]=xTimerCreate(NULL,pdMS_TO_TICKS(sone),pdFALSE,ck_h==theConf.profiles[0].cycle[0].numHorarios-1?(void*)0xFFFFFFFF:( void * ) vanTimersEnd, blower_end);
                            // end_timers[vanTimersEnd]=xTimerCreate(NULL,pdMS_TO_TICKS(sone/TEST),pdFALSE,ck_h==theConf.profiles[0].cycle[0].numHorarios-1?(void*)0xFFFFFFFF:( void * ) vanTimersEnd, blower_end);
                            if(!end_timers[vanTimersEnd])
                            {
                                ESP_LOGE(MESH_TAG,"End Timer not created %d",vanTimersEnd);
                                //do something drastic
                            }
                            // start them 
                            if(xTimerStart(start_timers[vanTimersStart],10)!=pdPASS)
                            {
                                ESP_LOGE(MESH_TAG,"FATAL could not start Start Timer %d",vanTimersStart);
                                //do something drastic
                            }
                            if(xTimerStart(end_timers[vanTimersEnd],10)!=pdPASS)        /// allow 10 ticks for start SUPER IMPORTANT
                            {
                                ESP_LOGE(MESH_TAG,"FATAL could not start End Timer %d",vanTimersEnd);
                                //do something drastic
                            }
                            vanTimersStart++;
                            vanTimersEnd++;
                            if (vanTimersEnd>MAXHORARIOS)
                                vanTimersEnd=0;
                            if (vanTimersStart>MAXHORARIOS)
                                vanTimersStart=0;
                        }


                    } // horarios
                    if(xSemaphoreTake(scheduleSem, portMAX_DELAY))	
                    {
                        if((theConf.debug_flags >> dSCH) & 1U)
                            ESP_LOGI(TAG,"%sDay ended",DBG_SCH);
                        time_t nnow=time(NULL);     // get our current date
                        struct tm *tm_info = localtime(&nnow);
                        struct tm *tm_infofirst = localtime(&now);  // this is the date we started with
                        if((theConf.debug_flags >> dSCH) & 1U)
                        {
                            ESP_LOGI(TAG,"%sFirst Now %s",DBG_SCH,ctime(&now));
                            ESP_LOGI(TAG,"%sNew day %s",DBG_SCH,ctime(&nnow));
                        }
                        if(tm_info->tm_mday==tm_infofirst->tm_mday && tm_info->tm_mon==tm_infofirst->tm_mon)
                        {
                            if((theConf.debug_flags >> dSCH) & 1U)
                                ESP_LOGI(TAG,"%sSame day and month",DBG_SCH); // need to get to next day midnight
                        // }
                        // else
                        //     {
                                tm_infofirst->tm_mday++;
                                time_t newn=mktime(tm_infofirst);
                                tm_info = localtime(&newn);
                                tm_info->tm_hour=tm_info->tm_min=tm_info->tm_sec=0;
                                time_t nextd=mktime(tm_info);
                                // printf("New day Mes %d Day %d Year %d\n",tm_info->tm_mon,tm_info->tm_mday,tm_info->tm_year);
                                if((theConf.debug_flags >> dSCH) & 1U)
                                {
                                    ESP_LOGI(TAG,"%sNew day %s",DBG_SCH,ctime(&nextd));
                                    ESP_LOGI(TAG,"%sDistance to midnight next day %d",DBG_SCH,long(nextd-now));
                                }

                            }
                        for (int a=0; a<vanTimersStart;a++)
                            xTimerDelete(start_timers[a],0);
                        for (int a=0; a<vanTimersEnd;a++)
                            xTimerDelete(end_timers[a],0);
                        vanTimersStart=vanTimersEnd=0;  //reset count
                        bzero(&start_timers,sizeof(start_timers));
                        bzero(&end_timers,sizeof(end_timers));
                    }
                    theConf.work_day=ck_d; //last day processed
                    theConf.work_cycle=ck;
                    theConf.dayCycle++;     // for recovery
                    if((theConf.debug_flags >> dSCH) & 1U)
                        ESP_LOGI(TAG,"%sSave day %d\n",DBG_SCH,theConf.dayCycle);
                    write_to_flash();
                } //days


            } //cycles
            ESP_LOGW(TAG,"Production cycle ended");
            theConf.blower_mode=0;              // done Production Cycle... wait for next activation
            // theConf.bleboot=BLE_MODE;            //set ble mode to restart production cycle
            theConf.dayCycle=0;                 // reset staring day
            write_to_flash();
            schedulef=false;
        } // start process semaphore
    } //task loop forever
} 


void app_spiffs_log(void)
{
    ESP_LOGI(TAG, "Initializing SPIFF LogS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = "profile",
      .max_files =2,
      .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }


    ESP_LOGI(TAG, "Performing SPIFFS_check().");
    ret = esp_spiffs_check(conf.partition_label);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
        return;
    } else {
        ESP_LOGI(TAG, "SPIFFS_check() successful");
    }


    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
        esp_spiffs_format(conf.partition_label);
        return;
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    // Check consistency of reported partition size info.
    if (used > total) {
        ESP_LOGW(TAG, "Number of used bytes cannot be larger than total. Performing SPIFFS_check().");
        ret = esp_spiffs_check(conf.partition_label);
        // Could be also used to mend broken files, to clean unreferenced pages, etc.
        // More info at https://github.com/pellepl/spiffs/wiki/FAQ#powerlosses-contd-when-should-i-run-spiffs_check
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
            return;
        } else {
            ESP_LOGI(TAG, "SPIFFS_check() successful");
        }
    }

}
void blower_start(TimerHandle_t xTimer)
{
    uint32_t ulCount = ( uint32_t ) pvTimerGetTimerID( xTimer );
    gpio_set_level((gpio_num_t)BEATPIN,0);          //before configuring it decide level we want it to be at start, in this case LOW

    // esp_rom_printf("Started Timer %d\n",ulCount);
    elapsed[ulCount]=xmillis();
    scheduleData.status=BLOWERON;

}
void blower_end(TimerHandle_t xTimer)
{
    uint32_t ulCount = ( uint32_t ) pvTimerGetTimerID( xTimer );
    // esp_rom_printf("Timer Ended %d Elapsed %ld\n",ulCount,xmillis()-elapsed[ulCount]);
    elapsed[ulCount]=0;
    gpio_set_level((gpio_num_t)BEATPIN,1);          //before configuring it decide level we want it to be at start, in this case LOW

    if (ulCount==0xFFFFFFFF)
    {
        // if((theConf.debug_flags >> dSCH) & 1U)  
        //     ESP_LOGI(TAG,"Last end timer...set for Wait For Start\n");
        xSemaphoreGive(scheduleSem);
        scheduleData.status=BLOWEROFF;        // definitly off now
    }
    else
        scheduleData.status=BLOWERNEXT;           // waiting for next hour

}

void app_main(void)
{
    esp_err_t ret;

    init_process();  
    app_spiffs_log();
    gdispf=false;
    // #ifdef DISPLAY
    //     ret=init_lcd();
    //     if(ret==ESP_OK)
    //     {
    //         xTaskCreate(&displayManager,"dMgr",1024*4,NULL, 5, &dispHandle); 	       
    //         gdispf=true;
    //     } 
    // #endif
    //log boot
    char *msg=(char*)calloc(1,100);
    if(msg)
    {    
        const esp_app_desc_t *mip=esp_app_get_description();
        sprintf(msg,"Booting Node %d Count %d Version %s Reason %d",theConf.poolid,theConf.bootcount,mip->version,theConf.lastResetCode);
        writeLog(msg);
        free(msg);
    }

    theBlower.initBlower();        //load the blower driver 

//keyboard commands

#ifdef DEBB
    xTaskCreate(&kbd,"kbd",1024*10,NULL, 5, NULL); 	        // keyboard commands
#endif



printf("PV PAnel %d Battery %d Energy %d Solar System %d SolarPad %d Union %d SmpMsg %d timet %d float %d Config %d\n",sizeof(pvPanel_t),sizeof(battery_t),
sizeof(energy_t),sizeof(solarSystem_t),sizeof(solarDef_t),sizeof(meshunion_t),sizeof(shrimpMsg_t),sizeof(time_t),sizeof(float),
sizeof(theConf) ); 

    if(theConf.meterconf==0  )  // is this meter NOT configured
    {
        //no, start the configuration phase
        //need to format fram to assure that new frrams are not with junk
        ESP_LOGW(MESH_TAG,"Formatting Fram new configuration");
        theBlower.deinit();
        theBlower.format();
        xTaskCreate(&blinkConf,"displ",1024,(void*)800, 5, &configureHandle); 	        //blink we are not configured
        reconfTimer=xTimerCreate("Reconf",pdMS_TO_TICKS(300000),pdFALSE,( void * ) 0, [] ( TimerHandle_t xTimer){esp_restart();});   //monitor activity and timeout if no work done-> use lambda
        meter_configure();      //start the STA for access to Router directly.
        vTaskDelete(configureHandle);   //stop the blinking now we are configured
        // theConf.meterconf=1;
        write_to_flash();
    }


    if(theConf.meterconf>2)
    {
        // allow for meter configuration without without erasing meter data
        xTaskCreate(&blinkConf,"displ",1024,(void*)800, 5, &configureHandle); 	        //blink we are not configured
        // reconfTimer=xTimerCreate("Reconf",pdMS_TO_TICKS(300000),pdFALSE,( void * ) 0, [] ( TimerHandle_t xTimer){esp_restart();});   //monitor activity and tiemout if no work done-> use lambda
        meter_configure();
    }
// reset reason code at https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/misc_system_api.html
        // if(theConf.bleboot==BLE_MODE && theConf.masternode)
        // {
        //     xTaskCreate(nimble_test,"nimble",1024*20,NULL, 14,NULL);      
        //     while(true)
        //         vTaskDelay(100);    
        // }
    
    xTaskCreate(&start_schedule_timers,"sched",1024*10,NULL, 5, &scheduleHandle); 	       

    xTaskCreate(&root_timer,"reptimer",1024*8,NULL, 5, NULL); 	        
    xTaskCreate(&rs485_task_manager,"modbus",1024*10,NULL, 5, NULL); 	            // start the modbus task   
    // delay(2000);
    // launch_sensors();
            // start the modbus task   
// the internal mesh is now going to start and begin all the main flow from its gotIp event manager
    showLVGL((char*)"MESH",10000,3);   

    if(!theConf.masternode)     // allow master to boot
    {
        ESP_LOGW(TAG,"Leaf -> Waiting for Root to start");
        delay(30000);
    }

    start_mesh();
    theConf.loginwait=20000;
    // mesh_enable();
// schedule timer will be started or not by sntp if root or when child connected by mesh if it was active and crash/power down
    
ESP_LOGI(MESH_TAG,"Heap Free APP %d",esp_get_free_heap_size());
// if system has resetted and restarting, the sntpget routine will be in charge of starting the schedule timer again
// the mesh connecting child will be order to start also if theConf.blower_mode=1; This is set to 0 when Cycle done see start shcedule timer

}