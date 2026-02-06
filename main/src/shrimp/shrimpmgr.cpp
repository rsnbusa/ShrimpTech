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

// Custom logging function with custom timestamp
void my_log(const char *color,const char* tag, const char* format, ...) {
    // Generate custom timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm* tm_info = localtime(&tv.tv_sec);
    
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%d-%m %H:%M:%S", tm_info);
    
    // Print custom timestamp
    printf("%s[%s]%s %s: ", color,timestamp, RESETC, tag);
    
    // Print the actual message
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
}

/**
 * @brief Format current time for log entry
 * 
 * Gets current time and formats it without trailing newline.
 * 
 * @param time_buf Buffer to store formatted time (must be at least 26 chars)
 * @return Pointer to formatted time string, or NULL if ctime fails
 */
char* format_log_time(time_t now,char *time_buf, size_t buf_size)
{
    char *time_str = ctime(&now);
    
    if (time_str && buf_size > 0) {
        strncpy(time_buf, time_str, buf_size - 1);
        time_buf[buf_size - 1] = '\0';
        time_buf[strcspn(time_buf, "\n")] = '\0';  // Remove newline
        return time_buf;
    }
    return NULL;
}

void ds18b20_task(void *pArg)
{
    // install 1-wire bus
    onewire_bus_handle_t bus = NULL;
    onewire_bus_config_t bus_config = {
        .bus_gpio_num = EXAMPLE_ONEWIRE_BUS_GPIO,
        .flags = {
            .en_pull_up = true, // enable the internal pull-up resistor in case the external device didn't have one
        }
    };
    onewire_bus_rmt_config_t rmt_config = {
        .max_rx_bytes = 10, // 1byte ROM command + 8byte ROM number + 1byte device command
    };
    ESP_ERROR_CHECK(onewire_new_bus_rmt(&bus_config, &rmt_config, &bus));
    MESP_LOGI(TAG, "1-Wire bus installed on GPIO%d", EXAMPLE_ONEWIRE_BUS_GPIO);

    int ds18b20_device_num = 0;
    ds18b20_device_handle_t ds18b20s[EXAMPLE_ONEWIRE_MAX_DS18B20];
    onewire_device_iter_handle_t iter = NULL;
    onewire_device_t next_onewire_device;
    esp_err_t search_result = ESP_OK;

    // create 1-wire device iterator, which is used for device search
    ESP_ERROR_CHECK(onewire_new_device_iter(bus, &iter));
    MESP_LOGI(TAG, "Device iterator created, start searching...");
    do {
        search_result = onewire_device_iter_get_next(iter, &next_onewire_device);
        if (search_result == ESP_OK) { // found a new device, let's check if we can upgrade it to a DS18B20
            ds18b20_config_t ds_cfg = {};
            onewire_device_address_t address;
            // check if the device is a DS18B20, if so, return the ds18b20 handle
            if (ds18b20_new_device_from_enumeration(&next_onewire_device, &ds_cfg, &ds18b20s[ds18b20_device_num]) == ESP_OK) {
                ds18b20_get_device_address(ds18b20s[ds18b20_device_num], &address);
                MESP_LOGI(TAG, "Found a DS18B20[%d], address: %016llX", ds18b20_device_num, address);
                ds18b20_device_num++;
                if (ds18b20_device_num >= EXAMPLE_ONEWIRE_MAX_DS18B20) {
                    MESP_LOGI(TAG, "Max DS18B20 number reached, stop searching...");
                    break;
                }
            } else {
                MESP_LOGI(TAG, "Found an unknown device, address: %016llX", next_onewire_device.address);
            }
        }
    } while (search_result != ESP_ERR_NOT_FOUND);
    ESP_ERROR_CHECK(onewire_del_device_iter(iter));
    MESP_LOGI(TAG, "Searching done, %d DS18B20 device(s) found", ds18b20_device_num);


    while (1) {
        vTaskDelay(pdMS_TO_TICKS(60000));

        // trigger temperature conversion for all sensors on the bus
        ESP_ERROR_CHECK(ds18b20_trigger_temperature_conversion_for_all(bus));
        for (int i = 0; i < ds18b20_device_num; i ++) {
            ESP_ERROR_CHECK(ds18b20_get_temperature(ds18b20s[i], &temperature));
            if((theConf.debug_flags >> dTEMP) & 1U)
                MESP_LOGI(TAG, "%s Temp[%d]: %.2fC%s", DBG_TEMP, i, temperature,RESETC);
        }
    }
}

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
                (void*)&theConf.modbus_battery,DBG_BATTERY,(void*)&batteryData,sizeof(batteryData),&print_battery_data);

            // Panels Modbus Device
            modbus_sensor_type_t *panels=setModbusSensor((char*)"Panels",5,4,
                (void*)&theConf.modbus_panels,DBG_PANELS,(void*)&pvPanelData,sizeof(pvPanelData),&print_panel_data);

            // Energy Modbus Device
            modbus_sensor_type_t *energy=setModbusSensor((char*)"Energy",10,4,
                (void*)&theConf.modbus_inverter,DBG_ENERGY,(void*)&energyData,sizeof(energyData),&print_energy_data);

            // Sensors Modbus Device
            modbus_sensor_type_t *sensorDev=setModbusSensor((char*)"Sensors",5,5,
                (void*)&theConf.modbus_sensors,DBG_SENSORS,(void*)&sensorData,sizeof(sensorData),&print_sensor_data);

            if(battery)
                xTaskCreate(&generic_modbus_task,battery->modbus_sensor_name,1024*4,(void*)battery, 5, NULL); 
            else
                MESP_LOGE(TAG, "Failed to create Battery modbus task due to memory allocation failure"); 

            if(panels)  
                xTaskCreate(&generic_modbus_task,panels->modbus_sensor_name,1024*4,(void*)panels, 5, NULL); 
            else
                MESP_LOGE(TAG, "Failed to create Panels modbus task due to memory allocation failure"); 

            if(energy)
                xTaskCreate(&generic_modbus_task,energy->modbus_sensor_name,1024*4,(void*)energy, 5, NULL); 
            else
                MESP_LOGE(TAG, "Failed to create Energy modbus task due to memory allocation failure"); 

            if(sensorDev)
                xTaskCreate(&generic_modbus_task,sensorDev->modbus_sensor_name,1024*4,(void*)sensorDev, 5, NULL); 
            else
                MESP_LOGE(TAG, "Failed to create Sensors modbus task due to memory allocation failure"); 
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
	_lock_t               lvgl_api_lock;
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
                MESP_LOGE("LVGL","Failed to allocate memory for label text");
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
                    MESP_LOGE("LVGL","Failed to allocate show_lvgl_t structure");
                    free(text);  // Free text if themsg allocation fails
                }
            }
            else
            {
                MESP_LOGE("LVGL","Failed to allocate memory for text");
            }
        } 
    }
    // else
    //     MESP_LOGI(MESH_TAG,"Showlvgl no disp\n");
}

// AES encryption/decryption functions moved to crypto_utils.c
// delay() and xmillis() functions moved to time_utils.c
// blinkRoot() and blinkConf() functions moved to led_utils.cpp

// uint32_t xmillisFromISR()
// {
// 	return pdTICKS_TO_MS(xTaskGetTickCountFromISR());
// }


// pending to iumplemnent loigic to tiuem out and account for confirmation from child nodes

void schedule_timeout(TimerHandle_t xTimer)
{
    esp_rom_printf("Schedule Timeout \n");

}   

/**
 * @brief Find the index of an internal command by name
 * 
 * @param cual Command name to search for
 * @return int Index of the command in internal_cmds array, ESP_FAIL if not found
 */
int findInternalCmds(const char * cual)
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
                MESP_LOGW(MESH_TAG,"Free RAM in get route %d\n",a);
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
int root_load_routing_table_mac(mesh_addr_t *who, void *ladata)
{
    if (!who || !ladata) {
        MESP_LOGE(MESH_TAG, "load routing missing args");
        return ESP_ERR_INVALID_ARG;
    }

    meshunion_t *aNode = (meshunion_t*)ladata;
    print_blower("load routing", &aNode->nodedata.solarData.solarSystem, true);

    if (!xSemaphoreTake(tableSem, portMAX_DELAY)) {
        MESP_LOGE(MESH_TAG, "load routing failed to take tableSem");
        return ESP_FAIL;
    }

    for (int a = 0; a < masterNode.existing_nodes; a++) {
        if (MAC_ADDR_EQUAL(masterNode.theTable.big_table[a].addr, who->addr)) {
            if (masterNode.theTable.thedata[a])
                free(masterNode.theTable.thedata[a]);           // erase old data

            masterNode.theTable.thedata[a] = ladata;            // new pointer ownership transferred
            xSemaphoreGive(tableSem);
            return ESP_OK;
        }
    }

    xSemaphoreGive(tableSem);
    MESP_LOGE(MESH_TAG, "Did not find mac addr " MACSTR, who->addr);
    return ESP_FAIL;
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
    if (!mid || mid[0] == '\0') {
        MESP_LOGE(MESH_TAG, "Confirm message missing mid");
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        MESP_LOGE(MESH_TAG, "No RAM creating confirm message");
        return ESP_FAIL;
    }

    cJSON_AddStringToObject(root, "cmd", internal_cmds[CONFIRMLOCK]);
    cJSON_AddStringToObject(root, "mid", mid);
    cJSON_AddNumberToObject(root, "state", lstate);

    char *intmsg = cJSON_PrintUnformatted(root);
    if (!intmsg) {
        MESP_LOGE(MESH_TAG, "No RAM building confirm message");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    mqttSender_t meshMsg = {0};
    meshMsg.msg    = intmsg;
    meshMsg.lenMsg = strlen(intmsg);
    meshMsg.queue  = NULL;
    meshMsg.addr   = NULL;

    if (xQueueSend(meshQueue, &meshMsg, 0) != pdTRUE) {  // queue frees msg
        MESP_LOGE(MESH_TAG, "Error queueing confirm msg");
        free(meshMsg.msg);
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    MESP_LOGI(MESH_TAG, "Confirmation queued for Meter [%s]", mid);
    cJSON_Delete(root);
    return ESP_OK;
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
    if (!mid || mid[0] == '\0') {
        MESP_LOGE(MESH_TAG, "Metrics message missing mid");
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        MESP_LOGE(MESH_TAG, "No RAM creating metrics response");
        return ESP_FAIL;
    }

    const esp_app_desc_t *mip = esp_app_get_description();
    if (mip)
        cJSON_AddStringToObject(root, "version", mip->version);

    cJSON_AddStringToObject(root, "cmd", internal_cmds[METRICRESP]);
    cJSON_AddStringToObject(root, "mid", mid);

    char time_buf[20] = {0};
    time_t lastup = theBlower.getLastUpdate();
    struct tm tm_info;
    if (localtime_r(&lastup, &tm_info) && strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_info) > 0)
        cJSON_AddStringToObject(root, "update", time_buf);

    char *intmsg = cJSON_PrintUnformatted(root);
    if (!intmsg) {
        MESP_LOGE(MESH_TAG, "No RAM building metrics message");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    mqttSender_t meshMsg = {0};
    meshMsg.msg    = intmsg;
    meshMsg.lenMsg = strlen(intmsg);
    meshMsg.queue  = discoQueue;
    meshMsg.addr   = NULL;

    if (xQueueSend(meshQueue, &meshMsg, 0) != pdTRUE) {      // queue frees msg
        MESP_LOGE(MESH_TAG, "Error queueing metrics msg");
        free(meshMsg.msg);
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    MESP_LOGI(MESH_TAG, "Metrics response queued for Meter [%s]", mid);
    cJSON_Delete(root);
    return ESP_OK;
}

int turn_display(char *cmd)
{
    if (!gdispf) {
        MESP_LOGW(MESH_TAG, "Display disabled; ignoring turn_display");
        return ESP_FAIL;
    }

    if (!cmd) {
        MESP_LOGE(MESH_TAG, "Turn display called with null cmd");
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *elcmd = cJSON_Parse(cmd);
    if (!elcmd) {
        MESP_LOGE(MESH_TAG, "Turn display: command not parsable");
        return ESP_FAIL;
    }

    cJSON *cualm = cJSON_GetObjectItem(elcmd, "mid");
    if (!cJSON_IsString(cualm) || cualm->valuestring == nullptr) {
        MESP_LOGE(MESH_TAG, "Display cmd missing mid");
        cJSON_Delete(elcmd);
        return ESP_ERR_INVALID_ARG;
    }

    // if (strcmp(cualm->valuestring, theBlower.getMID()) == 0)
    if (true) {
        cJSON *ltime = cJSON_GetObjectItem(elcmd, "time");
        if (!cJSON_IsNumber(ltime)) {
            MESP_LOGE(MESH_TAG, "Display cmd missing time");
            esp_lcd_panel_disp_on_off(panel_handle, false);
            cJSON_Delete(elcmd);
            return ESP_ERR_INVALID_ARG;
        }

        const int duration_ms = ltime->valueint * 1000;
        MESP_LOGW(MESH_TAG, "Display MId [%s me] Time [%d]", cualm->valuestring, ltime->valueint);

        if (!showHandle && duration_ms > 0) {
            esp_lcd_panel_disp_on_off(panel_handle, true);

            const BaseType_t task_ok = xTaskCreate(&showData, "sdata", 1024 * 3, NULL, 5, &showHandle);
            if (task_ok != pdPASS) {
                MESP_LOGE(MESH_TAG, "Failed to create display task");
                esp_lcd_panel_disp_on_off(panel_handle, false);
                cJSON_Delete(elcmd);
                return ESP_FAIL;
            }

            dispTimer = xTimerCreate("DispT", pdMS_TO_TICKS(duration_ms), pdFALSE, NULL, [](TimerHandle_t /*xTimer*/) {
                vTaskDelete(showHandle);
                showHandle = NULL;
                esp_lcd_panel_disp_on_off(panel_handle, false);
            });

            if (!dispTimer) {
                MESP_LOGE(MESH_TAG, "Failed to create display timer");
                vTaskDelete(showHandle);
                showHandle = NULL;
                esp_lcd_panel_disp_on_off(panel_handle, false);
                cJSON_Delete(elcmd);
                return ESP_FAIL;
            }

            if (xTimerStart(dispTimer, 0) != pdPASS) {
                MESP_LOGE(MESH_TAG, "Failed to start display timer");
                xTimerDelete(dispTimer, 0);
                dispTimer = NULL;
                vTaskDelete(showHandle);
                showHandle = NULL;
                esp_lcd_panel_disp_on_off(panel_handle, false);
                cJSON_Delete(elcmd);
                return ESP_FAIL;
            }
        } else if (duration_ms == 0) {
            esp_lcd_panel_disp_on_off(panel_handle, false);
            if (showHandle)
                vTaskDelete(showHandle);
            showHandle = NULL;
            if (dispTimer)
                xTimerStop(dispTimer, 0);
        }
    }

    cJSON_Delete(elcmd);
    return ESP_OK;
}

int check_my_metrics(char *cmd, char *mid)
{
    if (cmd == nullptr || mid == nullptr) {
        MESP_LOGE(MESH_TAG, "Metrics cmd or output buffer is null");
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *elcmd = cJSON_Parse(cmd);
    if (!elcmd) {
        MESP_LOGE(MESH_TAG, "Metrics internal error: command not parsable");
        return ESP_FAIL;
    }

    cJSON *mid_item = cJSON_GetObjectItem(elcmd, "mid");
    if (!cJSON_IsString(mid_item) || mid_item->valuestring == nullptr) {
        MESP_LOGE(MESH_TAG, "Metrics cmd missing mid field");
        cJSON_Delete(elcmd);
        return ESP_ERR_INVALID_ARG;
    }

    strcpy(mid, mid_item->valuestring);
    // if (strcmp(mid_item->valuestring, theBlower.getMID()) == 0)
    if (true) {
        MESP_LOGW(MESH_TAG, "MId [%s me] Metrics", mid_item->valuestring);
        send_metrics_message(mid_item->valuestring);
    }

    cJSON_Delete(elcmd);
    return ESP_OK;
}

int update_my_meter(char *cmd)
{
    if (!cmd) {
        MESP_LOGE(MESH_TAG, "Update meter called with null cmd");
        return ESP_FAIL;
    }

    cJSON *elcmd = cJSON_Parse(cmd);
    if (!elcmd) {
        MESP_LOGE(MESH_TAG, "Update Internal error not parsable");
        return ESP_FAIL;
    }

    cJSON *cualm   = cJSON_GetObjectItem(elcmd, "mid");       // the meter
    cJSON *bpk     = cJSON_GetObjectItem(elcmd, "bpk");       // bpk value
    cJSON *ks      = cJSON_GetObjectItem(elcmd, "ks");        // kwh start
    cJSON *k       = cJSON_GetObjectItem(elcmd, "k");         // kwh life
    cJSON *sk      = cJSON_GetObjectItem(elcmd, "sk");        // skip active y/n
    cJSON *skn     = cJSON_GetObjectItem(elcmd, "skn");       // skip count
    cJSON *ver     = cJSON_GetObjectItem(elcmd, "ver");       // version for possible OTA
    cJSON *baset   = cJSON_GetObjectItem(elcmd, "baset");     // base time for development
    cJSON *repeatt = cJSON_GetObjectItem(elcmd, "repeat");    // repeat time for development

    if (!cualm) {
        MESP_LOGE(MESH_TAG, "Update Cmd missing required param");
        cJSON_Delete(elcmd);
        return ESP_FAIL;
    }

    // if(strcmp(theBlower.getMID(),cualm->valuestring)==0)
    if (true) {   // its for me

        if (bpk && k && ks) {
            if (k->valueint < ks->valueint) {
                MESP_LOGW(MESH_TAG, "Update kwh [%d} < Kwstart [%d}", k->valueint, ks->valueint);
                cJSON_Delete(elcmd);
                return ESP_FAIL;
            }

            theBlower.eraseBlower();           // erase all metrics given new ks and k
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
        if (baset)
            theConf.baset = baset->valueint;
        if (repeatt)
            theConf.repeat = repeatt->valueint;

        if (sk || skn || baset || repeatt)
            write_to_flash();

        if (baset || repeatt)
            esp_restart();      // for timers need to reboot

    } else {
        MESP_LOGI(MESH_TAG, "Update It wasnt for me!");
    }

    cJSON_Delete(elcmd);
    return ESP_OK;
}

// internal Mesh network message, we use cjson since no big limit
// used to give to all connecting nodes certain "global" system parameter
// Date, last known ssid and passw, time slot, mqtt params and Application location params like prov, canton,etc
// Only Root sends this message to connecting Child

esp_err_t root_send_data_to_node(mesh_addr_t thismac)
{
    mesh_data_t  data       = {0};
    mqttSender_t meshMsg    = {0};
    time_t       now;

    time(&now);

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        MESP_LOGE(MESH_TAG, "No RAM creating boot response");
        return ESP_FAIL;
    }

    cJSON_AddStringToObject(root, "cmd", internal_cmds[BOOTRESP]);
    cJSON_AddNumberToObject(root, "time", (uint32_t)now);
    cJSON_AddNumberToObject(root, "profile", theConf.activeProfile);
    cJSON_AddNumberToObject(root, "day", theConf.dayCycle);
    cJSON_AddNumberToObject(root, "timermux", theConf.test_timer_div);
    cJSON_AddNumberToObject(root, "start", theBlower.getScheduleStatus());  // determines node start

    char *intmsg = cJSON_PrintUnformatted(root);
    if (!intmsg) {
        MESP_LOGE(MESH_TAG, "send data node no RAM");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    meshMsg.msg    = intmsg;
    meshMsg.lenMsg = strlen(intmsg);
    meshMsg.addr   = &thismac;

    if (xQueueSend(meshQueue, &meshMsg, 0) != pdPASS)
        MESP_LOGE(MESH_TAG, "Cannot queue fram");   // queue takes ownership and frees msg

    cJSON_Delete(root);

    delay(500);   // let time to node to process first message

    // now send the theConf modbus configurations binary data
    int big = sizeof(theConf.limits) + sizeof(theConf.modbus_battery) +
              sizeof(theConf.modbus_panels) +
              sizeof(theConf.modbus_inverter) +
              sizeof(theConf.modbus_sensors);
    printf("Big Modbus data to send %d\n", big);
    
    meshunion_t *intMessage = (meshunion_t*)calloc(1, big + 4);     // allocate payload + cmd header
    if (!intMessage) {
        MESP_LOGE(MESH_TAG, "No RAM for modbus config send");
        return ESP_FAIL;
    }

    void *p = (void*)intMessage + 4;
    memcpy(p, &theConf.milim, big);        // copy the message itself, offset 4 for cmd uint32 start offset is milim
    intMessage->cmd = MESH_INT_DATA_MODBUS;

    data.proto = MESH_PROTO_BIN;
    data.tos   = MESH_TOS_P2P;
    data.data  = (uint8_t*)intMessage;
    data.size  = big + 4;

    int err = esp_mesh_send(meshMsg.addr, &data, meshMsg.addr == NULL ? MESH_DATA_FROMDS : MESH_DATA_P2P, NULL, 0);
    free(intMessage);

    if (err) {
        MESP_LOGE(MESH_TAG, "Cannot send modbus config err=0x%x", err);
        return err;
    }

    return ESP_OK;
}   

int root_sendNodeACK(void *parg)
{
    mesh_addr_t *from = (mesh_addr_t*)parg;
    if (!from) {
        MESP_LOGE(MESH_TAG, "sendNodeACK missing source address");
        return ESP_FAIL;
    }

    char *mensaje = (char*)calloc(1, 32);
    if (!mensaje) {
        MESP_LOGE(MESH_TAG, "No RAM for sendNodeACK msg");
        return ESP_FAIL;
    }

    strcpy(mensaje, "{\"cmd\":\"installconf\"}");

    mqttSender_t meshMsg = {0};
    meshMsg.msg   = mensaje;
    meshMsg.lenMsg= strlen(mensaje);
    meshMsg.queue = NULL;
    meshMsg.addr  = from;      //to specific node

    if (xQueueSend(meshQueue, &meshMsg, 0) != pdTRUE) {  //will free mensaje INTERNAL Message
        MESP_LOGE(MESH_TAG, "Error queueing sendNodeACK msg");
        free(meshMsg.msg);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t root_send_confirmation_central(char *msg, uint16_t size, char *cualQ)
{
    if (!msg || !cualQ || size == 0) {
        MESP_LOGE(MESH_TAG, "Invalid confirm msg input");
        return ESP_FAIL;
    }

    char *confmsg = (char*)calloc(1, size);
    if (!confmsg) {
        MESP_LOGE(MESH_TAG, "No RAM for confirm msg");
        return ESP_FAIL;
    }

    memcpy(confmsg, msg, size);

    mqttSender_t mqttMsg = {0};
    mqttMsg.queue  = cualQ;
    mqttMsg.msg    = confmsg;  // freed by mqtt sender
    mqttMsg.lenMsg = size;
    mqttMsg.code   = NULL;
    mqttMsg.param  = 0;

    if (xQueueSend(mqttSender, &mqttMsg, 0) != pdTRUE) {
        MESP_LOGE(MESH_TAG, "Error queueing confirm msg %s", cualQ);
        free(confmsg);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t root_send_confirmation_central_cb(char *msg, char *cualQ, mesh_addr_t *from)
{
    if (!msg || !cualQ || !from) {
        MESP_LOGE(MESH_TAG, "Invalid confirm cb input");
        return ESP_FAIL;
    }

    const size_t msg_len = strlen(msg);
    if (msg_len == 0) {
        MESP_LOGE(MESH_TAG, "Confirm cb message is empty");
        free(msg);
        return ESP_FAIL;
    }

    mesh_addr_t *localcopy = (mesh_addr_t*)calloc(1, sizeof(mesh_addr_t));
    if (!localcopy) {
        MESP_LOGE(MESH_TAG, "No RAM for confirm cb addr");
        free(msg);
        return ESP_FAIL;
    }
    memcpy(localcopy, from, sizeof(mesh_addr_t));

    mqttSender_t mqttMsg = {0};
    mqttMsg.queue  = cualQ;
    mqttMsg.msg    = msg;  // freed by mqtt sender
    mqttMsg.lenMsg = msg_len;
    mqttMsg.code   = root_sendNodeACK;
    mqttMsg.param  = (uint32_t*)localcopy;

    if (xQueueSend(mqttSender, &mqttMsg, 0) != pdTRUE) {
        MESP_LOGE(MESH_TAG, "Error queueing confirm msg %s", cualQ);
        free(mqttMsg.msg);
        free(localcopy);
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t root_average_Solar(solarSystem_t *accum, solarSystem_t *src, uint8_t count)
{
    if (!accum || !src || count == 0) {
        MESP_LOGE(MESH_TAG, "Invalid args to root_average_Solar");
        return ESP_FAIL;
    }

    accum->pvPanel.chargeCurr     += src->pvPanel.chargeCurr / count;
    accum->pvPanel.pv1Volts       += src->pvPanel.pv1Volts / count;
    accum->pvPanel.pv2Volts       += src->pvPanel.pv2Volts / count;
    accum->pvPanel.pv1Amp         += src->pvPanel.pv1Amp / count;
    accum->pvPanel.pv2Amp         += src->pvPanel.pv2Amp / count;

    accum->battery.batSOC         += src->battery.batSOC / count;
    accum->battery.batSOH         += src->battery.batSOH / count;
    accum->battery.batteryCycleCount += src->battery.batteryCycleCount / count;
    accum->battery.batBmsTemp     += src->battery.batBmsTemp / count;

    accum->energy.batChgAHToday   += src->energy.batChgAHToday / count;
    accum->energy.batDischgAHToday+= src->energy.batDischgAHToday / count;
    accum->energy.batChgAHTotal   += src->energy.batChgAHTotal / count;
    accum->energy.batDischgAHTotal+= src->energy.batDischgAHTotal / count;
    accum->energy.generateEnergyToday += src->energy.generateEnergyToday / count;
    accum->energy.usedEnergyToday += src->energy.usedEnergyToday / count;
    accum->energy.gLoadConsumLineTotal += src->energy.gLoadConsumLineTotal / count;
    accum->energy.batChgkWhToday  += src->energy.batChgkWhToday / count;
    accum->energy.batDischgkWhToday += src->energy.batDischgkWhToday / count;
    accum->energy.genLoadConsumToday += src->energy.genLoadConsumToday / count;

    accum->sensors.DO             += src->sensors.DO / count;
    accum->sensors.PH             += src->sensors.PH / count;
    accum->sensors.WTemp          += src->sensors.WTemp / count;
    accum->sensors.ATemp          += src->sensors.ATemp / count;
    accum->sensors.AHum           += src->sensors.AHum / count;

    return ESP_OK;
}

esp_err_t root_send_collected_nodes(uint32_t cuantos)        //root only
{
    if (cuantos == 0) {
        MESP_LOGW(MESH_TAG, "No nodes to send");
        sendMeterf = false;
        if (xTimerStart(collectTimer, 0) != pdPASS)
            MESP_LOGE(MESH_TAG, "Repeat Timer collected failed");
        return ESP_OK;
    }

    mqttSender_t mqttMsg;
    solarSystem_t *solarPad = (solarSystem_t*)calloc(1, sizeof(solarSystem_t));
    if (!solarPad) {
        MESP_LOGE(MESH_TAG, "No RAM solarPad");
        sendMeterf = false;
        if (xTimerStart(collectTimer, 0) != pdPASS)
            MESP_LOGE(MESH_TAG, "Repeat Timer collected failed");
        return ESP_FAIL;
    }

    const uint32_t hp = esp_get_free_heap_size();
    if ((theConf.debug_flags >> dMESH) & 1U)
        MESP_LOGW(MESH_TAG, "%sCollected NODES heap %d nodes %d", DBG_MESH, hp, cuantos);

    int valid_nodes = 0;
    for (uint32_t a = 0; a < cuantos; a++) {
        if (masterNode.theTable.thedata[a])
            valid_nodes++;
        else
            MESP_LOGE(MESH_TAG, "Error null data on node %d " MACSTR, a, MAC2STR(masterNode.theTable.big_table[a].addr));
    }

    if (valid_nodes == 0) {
        MESP_LOGE(MESH_TAG, "No valid node data to aggregate");
        free(solarPad);
        sendMeterf = false;
        if (xTimerStart(collectTimer, 0) != pdPASS)
            MESP_LOGE(MESH_TAG, "Repeat Timer collected failed");
        return ESP_FAIL;
    }

    for (uint32_t a = 0; a < cuantos; a++) {
        if (!masterNode.theTable.thedata[a])
            continue;

        meshunion_t *meshMsg = (meshunion_t*)masterNode.theTable.thedata[a];
        solarSystem_t *solar = &meshMsg->nodedata.solarData.solarSystem;
        root_average_Solar(solarPad, solar, valid_nodes);
    }

    shrimpMsg_t *shmsg = (shrimpMsg_t*)calloc(1, sizeof(shrimpMsg_t));
    if (!shmsg) {
        MESP_LOGE(MESH_TAG, "No ram shmsg");
        sendMeterf = false;
        if (xTimerStart(collectTimer, 0) != pdPASS)
            MESP_LOGE(MESH_TAG, "Repeat Timer collected failed");
        free(solarPad);
        return ESP_FAIL;
    }

    memcpy(&shmsg->poolAvgMetrics, solarPad, sizeof(solarSystem_t));
    wschedule_t *scheduleData = theBlower.getSchedulePtr();
    if (scheduleData)
        memcpy(&shmsg->poolAvgMetrics.wschedule, scheduleData, sizeof(wschedule_t));

    print_blower("Root Average Solar Data", &shmsg->poolAvgMetrics, false);
    shmsg->msgTime    = time(NULL);
    shmsg->poolid     = theConf.poolid;
    shmsg->countnodes = valid_nodes;
    shmsg->centinel   = 0x12345678;
    shmsg->lim_errs   = globalErrors;

    free(solarPad);

    if (root_delete_routing_table() != ESP_OK) {
        MESP_LOGE(MESH_TAG, "Error deleting Routing table");
        free(shmsg);
        sendMeterf = false;
        if (xTimerStart(collectTimer, 0) != pdPASS)
            MESP_LOGE(MESH_TAG, "Repeat Timer collected failed");
        return ESP_FAIL;
    }

    mqttMsg.queue = infoQueue;
    mqttMsg.msg   = (char*)shmsg;  // freed by mqtt sender
    mqttMsg.lenMsg = sizeof(shrimpMsg_t);
    mqttMsg.code  = NULL;
    mqttMsg.param = NULL;

    if (xQueueSend(mqttSender, &mqttMsg, 0) != pdTRUE) {
        MESP_LOGE(MESH_TAG, "Error queueing msg");
        if (mqttMsg.msg)
            free(mqttMsg.msg);
        sendMeterf = false;
        if (xTimerStart(collectTimer, 0) != pdPASS)
            MESP_LOGE(MESH_TAG, "Repeat Timer collected mqttsender failed");
        return ESP_FAIL;
    }

    //must set the wifi_event_bit SEND_MQTT_BIT, else it will just collect the message in the queue
    xEventGroupSetBits(wifi_event_group, SENDMQTT_BIT); // Send everything now !!!!!

    return ESP_OK;
}

// works with a timer in case an expected node never sends the meter data
// Gets messages sent from other Child Nodes with their meter reading. When last one "counting_nodes" is received send the Data to MQTT HQ
// using Mqtts segmenting capabilities 
// ladata is  meshunion_t * myNode
esp_err_t root_check_incoming_meters(mesh_addr_t *who, void* ladata)
{
    if (root_load_routing_table_mac(who, ladata) != ESP_OK)
        return ESP_FAIL;  // failed to store incoming meter data

    if (counting_nodes > 0)
        counting_nodes--;
    else
        MESP_LOGW(MESH_TAG, "counting_nodes underflow while processing incoming meters");

    if (counting_nodes > 0)
        return ESP_OK;  // still waiting for more nodes

    if (xTimerStop(sendMeterTimer, 0) != pdPASS)
        MESP_LOGE(MESH_TAG, "Failed to stop SendMeter Timer");

    esp_err_t err = root_send_collected_nodes(masterNode.existing_nodes);
    if (err != ESP_OK) {
        MESP_LOGE(MESH_TAG, "Error sending mqtt msg");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void root_timer(void *parg)
{
    (void)parg;

    while (true) {
        // wait forever; flag is cleared on exit
        xEventGroupWaitBits(otherGroup, REPEAT_BIT, pdTRUE, pdFALSE, portMAX_DELAY);

        const int pending = masterNode.existing_nodes - counting_nodes;
        MESP_LOGW(MESH_TAG, "Mesh Timeout. Will send only %d nodes", pending);

        for (int a = 0; a < masterNode.existing_nodes; a++) {
            if (!masterNode.theTable.thedata[a]) {
                esp_rom_printf("Node not sending %d of %d" MACSTR " \n", a, masterNode.existing_nodes,
                               MAC2STR(masterNode.theTable.big_table[a].addr));
            } else {
                esp_rom_printf("Valid answer %d of %d" MACSTR " \n", a, masterNode.existing_nodes,
                               MAC2STR(masterNode.theTable.big_table[a].addr));
            }
        }

        if (sendMeterf) {
            if (pending > 0) {
                if (root_send_collected_nodes(pending) != ESP_OK)
                    MESP_LOGE(MESH_TAG, "Error sending meter timeout msg");
            } else {
                sendMeterf = false;  // reset flag on error/empty
            }
        }

        if (xTimerStart(collectTimer, 0) != pdPASS)
            MESP_LOGE(MESH_TAG, "Repeat Timer mqtt saender failed");
    }
}


void root_collect_meter_data(TimerHandle_t algo)
{
    if (!esp_mesh_is_root())
        return;

    if (sendMeterf) {
        MESP_LOGW(MESH_TAG, "Collect called without previous finished; resetting flag");
        sendMeterf = false;
        return;
    }

    if (get_routing_table() != ESP_OK) {
        MESP_LOGE(MESH_TAG, "Could not get routing table");
        sendMeterf = false;
        return;
    }

    meshunion_t *intMessage = (meshunion_t*)calloc(1, sizeof(meshunion_t));
    if (!intMessage) {
        MESP_LOGE(MESH_TAG, "Could not calloc collect meter message");
        sendMeterf = false;
        return;
    }

    const char payload[] = "{\"cmd\":\"sendmetrics\"}";
    const size_t payload_len = strlen(payload);

    intMessage->cmd = MESH_INT_DATA_CJSON;
    strncpy(intMessage->parajson, payload, payload_len);
    intMessage->parajson[payload_len] = '\0';

    mesh_data_t data = {0};
    data.data   = (uint8_t*)intMessage;
    data.size   = payload_len + 4;  // payload + cmd header
    data.proto  = MESH_PROTO_BIN;
    data.tos    = MESH_TOS_P2P;

    sendMeterf = true;  // now in sendmeter mode so start timer on this message to nodes
    if (xTimerStart(sendMeterTimer, 0) != pdPASS) {
        MESP_LOGE(MESH_TAG, "SendMeter Timer start failed");
        sendMeterf = false;
        free(intMessage);
        return;
    }

    int err = esp_mesh_send(&GroupID, &data, MESH_DATA_P2P, NULL, MESH_OPT_SEND_GROUP); // broadcast msg to mesh
    if (err) {
        MESP_LOGE(MESH_TAG, "Broadcast failed. err=0x%x", err);
        root_delete_routing_table();
        sendMeterf = false;
    }

    free(intMessage);
}

void set_sta_cmd(char *cjText)      //message from Root giving stations ids and passwords and other stuff
{
    if (!cjText) {
        MESP_LOGE(MESH_TAG, "Invalid Station cjson: null input");
        return;
    }

    cJSON *elcmd = cJSON_Parse(cjText);
    if (!elcmd) {
        MESP_LOGE(MESH_TAG, "Invalid Station cjson [%s]", cjText);
        return;
    }

    cJSON *cjtime   = cJSON_GetObjectItem(elcmd, "time");
    cJSON *prof     = cJSON_GetObjectItem(elcmd, "profile");
    cJSON *dayp     = cJSON_GetObjectItem(elcmd, "day");
    cJSON *mux      = cJSON_GetObjectItem(elcmd, "timermux");
    cJSON *schedule = cJSON_GetObjectItem(elcmd, "start");

    if (!(cJSON_IsNumber(cjtime) && cJSON_IsNumber(prof) && cJSON_IsNumber(dayp) && cJSON_IsNumber(schedule))) {
        MESP_LOGE(MESH_TAG, "Station parameters missing or invalid");
        cJSON_Delete(elcmd);
        return;
    }

    // set time
    setenv("TZ", LOCALTIME, 1);
    tzset();

    struct timeval now = {0};
    now.tv_sec = cjtime->valueint;
    settimeofday(&now, NULL);

    time_t nowt;
    time(&nowt);
    if ((theConf.debug_flags >> dMESH) & 1U)
        MESP_LOGI(MESH_TAG, "%sGot new Time %s", DBG_MESH, ctime(&nowt));

    const int newProfile = prof->valueint;
    const int newDay     = dayp->valueint;
    const int newMux     = mux ? mux->valueint : theConf.test_timer_div;
    const bool startNow  = schedule->valueint > 0;

    theConf.activeProfile = newProfile;
    theConf.dayCycle      = newDay;
    theConf.test_timer_div = newMux;

    if ((theConf.debug_flags >> dMESH) & 1U)
        MESP_LOGI(MESH_TAG, "%sStation got Profile %d Day %d Mux %d Restart %s", DBG_MESH,
                 newProfile, newDay, newMux, startNow ? "Yes" : "No");
    if (startNow) {
        xSemaphoreGive(workTaskSem);    //activate task
        theBlower.setScheduleStatus(BLOWERON);
    }

    write_to_flash();      
    cJSON_Delete(elcmd);
}

void wifi_send_meter_data(TimerHandle_t algo)
{
    if (!clientCloud) {
        MESP_LOGE(MESH_TAG, "wifi_send_meter_data: MQTT client not initialized");
        return;
    }

    shrimpMsg_t *shmsg = (shrimpMsg_t*)calloc(1, sizeof(shrimpMsg_t));
    if (!shmsg) {
        MESP_LOGE(MESH_TAG, "No RAM for shrimpMsg_t");
        return;
    }

    // copy theBlower solar data into shrimp message to send to MAIN HOST via MQTT HQ
    memcpy(&shmsg->poolAvgMetrics, theBlower.getSolarSystem(), sizeof(solarSystem_t));
    // update the current schedule data that is lost during averaging
    wschedule_t *scheduleData = theBlower.getSchedulePtr();
    if (scheduleData)
        memcpy(&shmsg->poolAvgMetrics.wschedule, scheduleData, sizeof(wschedule_t));

    print_blower("Root Average Solar Data", &shmsg->poolAvgMetrics, false);
    shmsg->msgTime   = time(NULL);
    shmsg->poolid    = theConf.poolid;
    shmsg->countnodes= 1;                //only 1 node in wifi mode
    shmsg->centinel  = 0x12345678;       // our sentinel
    shmsg->lim_errs  = globalErrors;     // whatever errors the Blower has recorded

    int msgid = esp_mqtt_client_publish(clientCloud, (char*)infoQueue, (char*)shmsg,
                                        sizeof(shrimpMsg_t), QOS1, NORETAIN);
    if (msgid < 0) {
        MESP_LOGE(MESH_TAG, "Error publishing meter data to MQTT HQ");
        free(shmsg);
        return;
    }

    free(shmsg);
}

esp_err_t send_datos_to_root()
{
    // this is binary data and comes with the required header
    meshunion_t *myNode = sendData(true);
    if (!myNode) {
        MESP_LOGE(MESH_TAG, "Error send datos root msg");
        return ESP_FAIL;
    }

    print_blower("send datos root", &myNode->nodedata.solarData.solarSystem, false);

    if (theConf.delay_mesh > 0)
        delay(theConf.delay_mesh * theConf.unitid * SENDMUX);  // trying to avoid congestion UNIT id *ms

    mesh_data_t datas = {0};
    datas.data  = (uint8_t*)myNode;
    datas.size  = sizeof(meshunion_t);
    datas.proto = MESH_PROTO_BIN;
    datas.tos   = MESH_TOS_P2P;

    esp_err_t err = esp_mesh_send(NULL, &datas, MESH_DATA_P2P, NULL, 1);
    if (err) {
        MESP_LOGE(MESH_TAG, "Send Meters failed %x", err);
        free(myNode);
        return err;
    }

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
//                 MESP_LOGW(MESH_TAG,"Format Meter Erase %s\n",reconf->valuestring);
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
//             MESP_LOGW(MESH_TAG,"Format MID [%s] sent to [%s] Fram and Erase configuration. Virgin chip",theBlower.getMID(),mid->valuestring);
//             delay(1000);
//             esp_restart();                
//         }
//     }

// }
err_t root_mesh_broadcast_msg(char *msg)        // csjon format only
{
    if (!msg) {
        MESP_LOGE(MESH_TAG, "Broadcast message is null");
        return ESP_FAIL;
    }

    size_t msg_len = strlen(msg);
    if (msg_len == 0) {
        MESP_LOGW(MESH_TAG, "Broadcast message is empty");
        return ESP_FAIL;
    }

    // Reserve extra bytes for header and string terminator
    meshunion_t *intMessage = (meshunion_t*)calloc(1, msg_len + 10);
    if (!intMessage) {
        MESP_LOGE(MESH_TAG, "Root broadcast no RAM");
        return ESP_FAIL;
    }

    intMessage->cmd = MESH_INT_DATA_CJSON;
    // Safe copy with explicit bound
    strncpy(intMessage->parajson, msg, msg_len);
    intMessage->parajson[msg_len] = '\0';

    mesh_data_t data;
    data.data   = (uint8_t*)intMessage;
    data.size   = msg_len + 4;  // payload + cmd header
    data.proto  = MESH_PROTO_BIN;
    data.tos    = MESH_TOS_P2P;

    //send a Broadcast Message to all nodes to send their data
    int err = esp_mesh_send(&GroupID, &data, MESH_DATA_P2P, NULL, MESH_OPT_SEND_GROUP);
    if (err) {
        MESP_LOGE(MESH_TAG, "Broadcast failed. err=0x%x", err);
        free(intMessage);
        return err;
    }

    free(intMessage);
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

/**
 * @brief Handle binary meter data from child nodes (root only)
 * 
 * Allocates buffer for meter data, copies it, and processes incoming
 * metrics from child nodes. Buffer ownership transfers to routing table.
 * 
 * @param from Source node address
 * @param data Mesh data containing meter information
 * @return true if processed successfully, false on error
 */
bool handle_meter_data(mesh_addr_t *from, mesh_data_t *data)
{
    meshunion_t *aNode = (meshunion_t*)calloc(data->size, 1);
    if (!aNode) {
        MESP_LOGE(MESH_TAG, "Failed to allocate %d bytes for meter data", data->size);
        return false;
    }

    memcpy(aNode, data->data, data->size);
    
    if (root_check_incoming_meters(from, aNode) != ESP_OK) {
        MESP_LOGE(MESH_TAG, "Error processing incoming meter data from " MACSTR, MAC2STR(from->addr));
        free(aNode);
        return false;
    }

    // NOTE: aNode ownership transferred to routing table - do not free here
    return true;
}

/**
 * @brief Handle Modbus configuration data from root (child nodes only)
 * 
 * Receives Modbus configuration parameters from root node and updates
 * local configuration. Currently placeholder for future implementation.
 * 
 * @param data Mesh data containing Modbus configuration
 * @return true if processed successfully, false on error
 */
bool handle_modbus_config(mesh_data_t *data)
{
    meshunion_t *aNode = (meshunion_t*)calloc(data->size, 1);
    if (!aNode) {
        MESP_LOGE(MESH_TAG, "Failed to allocate %d bytes for Modbus config", data->size);
        return false;
    }

    memcpy(aNode, data->data, data->size);
    
    if ((theConf.debug_flags >> dMESH) & 1U)
        MESP_LOGI(MESH_TAG, "Child node received Modbus configuration from root");

    // TODO: Process and apply Modbus configuration data
    // Currently placeholder - implement configuration update logic
    
    free(aNode);
    return true;
}

/**
 * @brief Process binary mesh messages
 * 
 * Dispatcher for binary mesh messages based on message type.
 * Routes to appropriate handler for meter data or Modbus configuration.
 * 
 * @param from Source mesh address
 * @param data Mesh data structure containing binary payload
 * @param reserved Message type identifier (first 4 bytes of data)
 */
void process_binary_mesh_msg(mesh_addr_t *from, mesh_data_t *data, uint32_t reserved)
{
    // Root node: handle meter data from child nodes
    if (esp_mesh_is_root() && reserved == MESH_INT_DATA_BIN) {
        handle_meter_data(from, data);
        return;
    }

    // Child nodes: handle Modbus configuration from root
    if (!esp_mesh_is_root() && reserved == MESH_INT_DATA_MODBUS) {
        handle_modbus_config(data);
        return;
    }

    // Unknown binary message type
    if ((theConf.debug_flags >> dMESH) & 1U)
        MESP_LOGW(MESH_TAG, "Unknown binary message type: 0x%08x", reserved);
}

/**
 * @brief Log production order action
 * 
 * Helper to log production order events with consistent formatting.
 * 
 * @param order Order name (start, stop, pause, resume)
 */
void log_production_order(const char *order)
{
    if ((theConf.debug_flags >> dMESH) & 1U)
        MESP_LOGI(MESH_TAG, "%sMesh Production Cmd order to %s", DBG_MESH, order);

    char *log_msg = (char*)calloc(200, 1);
    if (!log_msg) {
        MESP_LOGE(TAG, "No RAM for production log msg");
        return;
    }

    snprintf(log_msg, 200, "Mesh Production Cmd order to %s", order);
    writeLog(log_msg);
    free(log_msg);
}

/**
 * @brief Handle production start order
 */
void handle_production_start(void)
{
    log_production_order("Start");
    xSemaphoreGive(workTaskSem);
}

/**
 * @brief Handle production stop order
 */
void handle_production_stop(void)
{
    log_production_order("Stop");
    
    if (scheduleHandle) {
        vTaskDelete(scheduleHandle);
    }
    xTaskCreate(&start_schedule_timers, "sched", 1024 * 10, NULL, 5, &scheduleHandle);
    schedulef = false;
}

/**
 * @brief Handle production pause order
 */
void handle_production_pause(void)
{
    log_production_order("Pause");
    pausef = true;
}

/**
 * @brief Handle production resume order
 */
void handle_production_resume(void)
{
    log_production_order("Resume");
    pausef = false;
}

/**
 * @brief Update production configuration from command
 * 
 * Extracts profile, day cycle, and timer mux settings from cJSON
 * and writes to flash storage.
 * 
 * @param prof Profile ID
 * @param pday Day cycle
 * @param pmux Timer multiplexer
 */
void update_production_config(cJSON *prof, cJSON *pday, cJSON *pmux)
{
    theConf.activeProfile = prof->valueint;
    theConf.dayCycle = pday->valueint;
    theConf.test_timer_div = pmux->valueint;
    write_to_flash();

    if ((theConf.debug_flags >> dMESH) & 1U)
        MESP_LOGI(TAG, "%sMesh Production Config: Profile %d Day %d Mux %d",
                 DBG_MESH, prof->valueint, pday->valueint, pmux->valueint);
}

/**
 * @brief Process production command from mesh
 * 
 * Handles production control commands (start, stop, pause, resume)
 * for child nodes. Updates configuration and executes corresponding action.
 * 
 * @param elcmd cJSON command object containing production parameters
 */
void child_production(cJSON *elcmd)
{
    cJSON *prof = cJSON_GetObjectItem(elcmd, "prof");
    cJSON *pday = cJSON_GetObjectItem(elcmd, "day");
    cJSON *pmux = cJSON_GetObjectItem(elcmd, "mux");
    cJSON *order = cJSON_GetObjectItem(elcmd, "order");

    if (!prof || !pday || !pmux || !order) {
        if ((theConf.debug_flags >> dMESH) & 1U)
            MESP_LOGI(MESH_TAG, "%sProduction missing parameter", DBG_MESH);
        return;
    }

    const char *order_str = order->valuestring;
    
    if ((theConf.debug_flags >> dMESH) & 1U)
        MESP_LOGI(MESH_TAG, "%sMesh Production Cmd Profile %d Day %d Mux %d Order %s",
                 DBG_MESH, prof->valueint, pday->valueint, pmux->valueint, order_str);

    update_production_config(prof, pday, pmux);

    // Dispatch to appropriate handler based on order
    if (strcmp(order_str, "start") == 0) {
        handle_production_start();
    } else if (strcmp(order_str, "stop") == 0) {
        handle_production_stop();
    } else if (strcmp(order_str, "pause") == 0) {
        handle_production_pause();
    } else if (strcmp(order_str, "resume") == 0) {
        handle_production_resume();
    } else {
        MESP_LOGW(MESH_TAG, "%sUnknown production order: %s", DBG_MESH, order_str);
    }
}

int findPoolNode(mesh_addr_t *este)
{
    for (int a=0;a<poolNodesTable.existing_nodes;a++)
    {
         if (MAC_ADDR_EQUAL(poolNodesTable.address_table[a].addr, este->addr)) 
            return a;
    }
    return ESP_FAIL;
}

/**
 * @brief Parse mesh data payload into cJSON object
 * 
 * Validates data size, extracts command type, and parses JSON payload.
 * Advances data pointer past header and null-terminates before parsing.
 * 
 * @param data Mesh data structure (modified: pointer advanced, size reduced)
 * @return Parsed cJSON object, or NULL on error
 */
cJSON* parse_mesh_json_data(mesh_data_t *data)
{
    if (data->size < 5) {  // Need at least 4-byte header + 1 byte data
        MESP_LOGE(MESH_TAG, "Mesh data too small: %d bytes", data->size);
        return NULL;
    }

    data->data += 4;  // Skip command header
    data->size -= 4;
    data->data[data->size] = 0;  // Null terminate

    cJSON *parsed = cJSON_Parse((char*)data->data);
    if (!parsed) {
        MESP_LOGE(MESH_TAG, "Failed to parse mesh JSON: %s", (char*)data->data);
    }
    return parsed;
}

/**
 * @brief Handle SCHEDULE command from mesh node
 * 
 * Processes schedule confirmation from pool nodes. Root node tracks
 * confirmations and stops schedule timer when all nodes respond.
 */
void handle_schedule_command(cJSON *elcmd, mesh_addr_t *from)
{
    if (!esp_mesh_is_root())
        return;

    int cualp = findPoolNode(from);
    cJSON *cualunit = cJSON_GetObjectItem(elcmd, "unit");
    cJSON *cualcycle = cJSON_GetObjectItem(elcmd, "cycle");
    cJSON *cualday = cJSON_GetObjectItem(elcmd, "day");
    cJSON *cualhora = cJSON_GetObjectItem(elcmd, "hora");

    if (cualunit && cualcycle && cualday && cualhora) {
        MESP_LOGI(TAG, "Schedule Start Unit %d Cycle %d Day %d horario %d Pool %d " MACSTR,
                 cualunit->valueint, cualcycle->valueint, cualday->valueint,
                 cualhora->valueint, cualp, MAC2STR(from->addr));

        if (cualp >= 0)
            poolNodesTable.existing_nodes--;

        if (poolNodesTable.existing_nodes == 0) {
            MESP_LOGI(TAG, "All pool nodes confirmed schedule start");
            xTimerStop(scheduleTimer, 0);
        }
    }
}

/**
 * @brief Handle METRICRESP command from mesh node
 * 
 * Allocates buffer, copies metric data, and queues for MQTT transmission.
 */
void handle_metricresp_command(cJSON *elcmd, mesh_data_t *data, mesh_addr_t *from)
{
    if ((theConf.debug_flags >> dMESH) & 1U)
        MESP_LOGI(MESH_TAG, "Mesh Metrics respond cmd");

    char *msg = (char*)calloc(1, data->size + 1);
    if (!msg) {
        MESP_LOGE(MESH_TAG, "Error RAM Response metrics mesh mgr");
        return;
    }

    memcpy(msg, data->data, data->size);
    if (root_send_confirmation_central_cb(msg, discoQueue, from) == ESP_FAIL) {
        MESP_LOGE(MESH_TAG, "Error sending metrics response to Mqtt Mgr");
    }
}

/**
 * @brief Dispatch mesh command to appropriate handler
 * 
 * Routes validated mesh commands to their handlers based on command type.
 * Ensures consistent cleanup and error handling across all cases.
 * 
 * @param cualf Command index from findInternalCmds
 * @param elcmd Parsed cJSON command object
 * @param from Source mesh address
 * @param data Mesh data structure
 */
void dispatch_mesh_command(int cualf, cJSON *elcmd, mesh_addr_t *from, mesh_data_t *data)
{
    char cualMID[20];
    esp_err_t err;

    switch (cualf) {
    case SCHEDULE:
        handle_schedule_command(elcmd, from);
        break;

    case BOOTRESP:
        if (!esp_mesh_is_root())
            set_sta_cmd((char*)data->data);
        break;

    case SENDMETRICS:
        err = send_datos_to_root();
        if (err)
            MESP_LOGE(MESH_TAG, "Error send Meters %d", err);
        break;

    case PRODUCTION:
        if ((theConf.debug_flags >> dMESH) & 1U)
            MESP_LOGI(MESH_TAG, "%sMesh Production Cmd", DBG_MESH);
        if (!esp_mesh_is_root())
            child_production(elcmd);
        break;

    case UPDATEMETER:
        if ((theConf.debug_flags >> dMESH) & 1U)
            MESP_LOGI(MESH_TAG, "Mesh Update Mesh Cmd");
        update_my_meter((char*)data->data);
        break;

    case FORMAT:
        if ((theConf.debug_flags >> dMESH) & 1U)
            MESP_LOGI(MESH_TAG, "Mesh Format cmd");
        // format_my_meter(elcmd);
        break;

    case ERASEMETRICS:
        if ((theConf.debug_flags >> dMESH) & 1U)
            MESP_LOGI(MESH_TAG, "Mesh Erase Metrics cmd");
        // erase_my_meter(elcmd);
        break;

    case MQTTMETRICS:
        if ((theConf.debug_flags >> dMESH) & 1U)
            MESP_LOGI(MESH_TAG, "Mesh Metrics requested cmd");
        check_my_metrics((char*)data->data, cualMID);
        break;

    case METRICRESP:
        handle_metricresp_command(elcmd, data, from);
        break;

    case SHOWDISPLAY:
        if ((theConf.debug_flags >> dMESH) & 1U)
            MESP_LOGI(MESH_TAG, "Mesh Display cmd");
        turn_display((char*)data->data);
        break;

    default:
        if ((theConf.debug_flags >> dMESH) & 1U)
            MESP_LOGE(MESH_TAG, "%sMesh Internal not found (case %d)", DBG_MESH, cualf);
        break;
    }
}

//here we received all mesh traffic data
void mesh_manager(mesh_addr_t *from, mesh_data_t *data)
{
    cJSON *elcmd;
    uint32_t reserved;

    // msg from the Mesh has to have a defined structure with the first 4 bytes identifying
    // the msg type:
    // BINARY
        // MESH_INT_DATA_BIN -> binary meter msg from nodes
        // MESH_MOTA_START -> Mesh OTA start
        // MESH_MOTA_DATA -> Mesh OTA data in
        // MESH_MOTA_DATA_END -> Mesh OTA end without data
        // MESH_MOTA_DATA_REC -> Mesh OTA recovery data in
        // MESH_MOTA_DATA_REC_END -> Mesh OTA data in and end of recovery

    // MESH_INT_DATA_CJSON -> Mesh internal cmds text (cJSON)

    if (data->size == 0) {
        MESP_LOGE(MESH_TAG, "Meshmgr got 0 len");
        return;
    }

    // Identify the msg type from header
    memcpy(&reserved, data->data, 4);

    if (reserved != MESH_INT_DATA_CJSON) {
        process_binary_mesh_msg(from, data, reserved);
        return;
    }

    // Parse JSON command
    elcmd = parse_mesh_json_data(data);
    if (!elcmd)
        return;

    // All msgs MUST have a cmd field
    cJSON *cualm = cJSON_GetObjectItem(elcmd, "cmd");
    if (!cualm) {
        MESP_LOGI(MESH_TAG, "MeshMgr Internal cJSON but no CMD found");
        esp_log_buffer_hex(MESH_TAG, data->data, 60);
        cJSON_Delete(elcmd);
        return;
    }

    if ((theConf.debug_flags >> dMESH) & 1U)
        MESP_LOGI(MESH_TAG, "%sfind internal cmd %s", DBG_MESH, cualm->valuestring);

    int cualf = findInternalCmds(cualm->valuestring);
    if (cualf >= 0) {
        dispatch_mesh_command(cualf, elcmd, from, data);
    } else {
        MESP_LOGW(MESH_TAG, "Mesh cmd not found %s", cualm->valuestring);
    }

    cJSON_Delete(elcmd);
}



err_t write_log(char *LTAG, char *que)
{
    char time_buf[32];
    time_t now = time(NULL);
    char *time_str = format_log_time(now,time_buf, sizeof(time_buf));
    if (!time_str) {
        MESP_LOGE(TAG, "Failed to format log time");
        return ESP_FAIL;
    }

    FILE *f = fopen(LOG_FILE, "a");
    if (!f) {
        MESP_LOGE(TAG, "Failed to open Log for writing");
        return ESP_FAIL;
    }

    char *log_entry = (char *)calloc(1000, 1);
    if (!log_entry) {
        MESP_LOGE(TAG, "No RAM for writing log entry");
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
    char *buf = (char*)calloc(1000, 1);
    if (!buf) {
        MESP_LOGE(TAG, "Failed RAM allocation for log reading");
        return ESP_FAIL;
    }

    FILE *f = fopen(LOG_FILE, "r");
    if (!f) {
        MESP_LOGE(TAG, "Failed to open Log for reading");
        free(buf);
        return ESP_FAIL;
    }

    for (int a = 0; a < nlines; a++) {
        if (fgets(buf, 500, f) != NULL)
            printf("[%d]%s", a, buf);
        else
            break;
    }

    fclose(f);
    free(buf);

    return ESP_OK;
}

/**
 * @brief Read configuration from NVS flash storage
 * 
 * Thread-safe read of theConf structure from NVS.
 * Opens NVS handle, reads blob, and logs any errors.
 */
void read_flash()
{
    if (!xSemaphoreTake(flashSem, portMAX_DELAY))
        return;

    esp_err_t err = nvs_open("config", NVS_READWRITE, &nvshandle);
    if (err != ESP_OK) {
        MESP_LOGE(MESH_TAG, "Error opening NVS Read File %x", err);
        xSemaphoreGive(flashSem);
        return;
    }

    size_t size = sizeof(theConf);
    err = nvs_get_blob(nvshandle, "sysconf", (void*)&theConf, &size);
    
    if (err != ESP_OK)
        MESP_LOGE(MESH_TAG, "Error read %x size %d expected %d", err, size, sizeof(theConf));

    // Keep NVS handle open for future write_to_flash calls
    xSemaphoreGive(flashSem);
}

/**
 * @brief Write configuration to NVS flash storage
 * 
 * Thread-safe write of theConf structure to NVS.
 * Saves blob and commits changes. Handle remains open.
 */
void write_to_flash()
{
    if (!xSemaphoreTake(flashSem, portMAX_DELAY))
        return;

    esp_err_t err = nvs_set_blob(nvshandle, "sysconf", &theConf, sizeof(theConf));
    
    if (err == ESP_OK) {
        err = nvs_commit(nvshandle);
        if (err != ESP_OK)
            MESP_LOGE(MESH_TAG, "Flash commit write failed %d", err);
    } else {
        MESP_LOGE(MESH_TAG, "Fail to write flash %x", err);
    }

    // Keep NVS handle open
    xSemaphoreGive(flashSem);
}


/**
 * @brief Initialize SNTP and wait for time sync
 * 
 * Sets up SNTP client with NTP server and waits for successful sync.
 * Falls back to saved time if sync fails after SNTPTRY attempts.
 * 
 * @param localtime Pointer to timeval structure to receive fallback time
 * @return true if sync successful, false if using fallback time
 */
bool sntp_sync_with_fallback(timeval *localtime)
{
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

    int retry = 0;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry <= SNTPTRY)
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    if(retry >= SNTPTRY)
    {
        MESP_LOGE(MESH_TAG, "SNTP failed retry count, using saved time");
        theConf.lastKnownDate = theBlower.getReservedDate();   
        localtime->tv_sec = theConf.lastKnownDate;
        localtime->tv_usec = 0;
        settimeofday(localtime, NULL);
        return false;
    }
    return true;
}

/**
 * @brief Update system time tracking and configuration
 * 
 * Records reboot time, calculates downtime, saves to flash,
 * and updates blower with new timestamp.
 * 
 * @param now Current time_t
 */
void update_system_time_tracking(time_t now)
{
    setenv("TZ", LOCALTIME, 1);
    tzset();
    
    if(theConf.lastRebootTime == 0)
        theConf.lastRebootTime = now;
    else
        theConf.downtime += (uint32_t)now - theConf.lastRebootTime;

    theBlower.setReservedDate(now);
    write_to_flash();
}

/**
 * @brief Start blower and data collection timers if ready
 */
void start_blower_if_ready(void)
{
    wschedule_t* scheduleData = theBlower.getSchedulePtr();
    if (theBlower.getScheduleStatus() < BLOWERCROP)        // parked is the only status that does not start blower
   {
        schedule_restartf=true;

        char *aca=(char*)calloc(100,1);
        if(aca)
        {
            sprintf(aca,"Restarting Blower apparent PF Cycle %d Day %d",scheduleData->currentCycle,scheduleData->currentDay);

        // get last schedule
            if ((theConf.debug_flags >> dSCH) & 1U)
                MESP_LOGI(MESH_TAG, "%s%s", DBG_SCH,aca);
                write_log("SYSTEM", aca);
                free(aca);
        }
         xSemaphoreGive(workTaskSem);
   }  
   else
        schedule_restartf=false;
  
    if (theConf.wifi_mode != WIFI_MESH)
        root_set_senddata_timer();
}

void root_sntpget(void *pArg)
{
    time_t now;
    struct tm timeinfo;
    timeval localtime;
    
    time(&now);
    localtime_r(&now, &timeinfo);
    memset(&timeinfo, 0, sizeof(timeinfo));
    
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) 
    {
        sntp_sync_with_fallback(&localtime);
        time(&now);
        
        MESP_LOGI(MESH_TAG, "SNTP Date %s", ctime((time_t*)&now));
        update_system_time_tracking(now);

        xTaskCreate(&start_schedule_timers,"sched",1024*10,NULL, 5, &scheduleHandle); 	       
        if(theConf.doParms.docontrol)
            xTaskCreate(&PIDController,"PID",1024*10,NULL, 5, NULL); 	            // start the {PID} task  

        start_blower_if_ready();
    }
    vTaskDelete(NULL);
}

// Forward declarations and implementations for Time/SNTP and MQTT helper functions
/**
 * @brief Clear all pending messages from MQTT sender queue
 * 
 * Drains the mqttSender queue and frees all message buffers and parameters.
 * Used during reconnection to clean up partial/stale messages.
 */
void clear_mqtt_sender_queue(void)
{
    mqttSender_t mensaje;
    
    while(true)
    {
        if(xQueueReceive(mqttSender, &mensaje, pdMS_TO_TICKS(MQTTSENDERWAIT)) == pdPASS)
        {
            if(mensaje.msg)
                free(mensaje.msg);
            if(mensaje.param)
                free(mensaje.param);
        }
        else
            break;
    }
}

/**
 * @brief Attempt one MQTT reconnection cycle
 * 
 * Destroys old client, creates new one, starts it, and waits for connection.
 * On successful connection, restarts all MQTT tasks and timers.
 * 
 * @param retry Pointer to retry counter to increment
 * @return true if reconnection successful and tasks restarted, false otherwise
 */
bool mqtt_reconnect_attempt(int *retry)
{
    esp_err_t err;
    EventBits_t uxBits;
    
    if(clientCloud)
    {
        err = esp_mqtt_client_destroy(clientCloud);
        if(err != ESP_OK)
            MESP_LOGD(MESH_TAG, "MQTT client destroy failed");
        else
            clientCloud = NULL;
    }
    
    clientCloud = root_setupMqtt();
    if(!clientCloud)
    {
        MESP_LOGD(MESH_TAG, "MQTT client setup failed");
        return false;
    }
    
    err = esp_mqtt_client_start(clientCloud);
    if(err == ESP_FAIL)
    {
        MESP_LOGE(MESH_TAG, "MQTT client start failed");
        return false;
    }
    
    (*retry)++;
    uxBits = xEventGroupWaitBits(wifi_event_group, MQTT_BIT|DISCO_BIT|ERROR_BIT, pdFALSE, pdFALSE,
                                  pdMS_TO_TICKS(10000));
    
    if((uxBits & MQTT_BIT) != MQTT_BIT)
        return false;  // Did not connect
    
    // Connection successful - restart tasks and timers
    MESP_LOGI(MESH_TAG, "Reconnection successful after %d tries", *retry);
    
    char *tmp = (char*)calloc(1, 100);
    if(tmp)
    {
        snprintf(tmp, 100, "Reconnection successful after %d tries", *retry);
        writeLog(tmp);
        free(tmp);
    }
    
    err = esp_mqtt_client_disconnect(clientCloud);
    xEventGroupWaitBits(wifi_event_group, DISCO_BIT|ERROR_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    
    err = esp_mqtt_client_stop(clientCloud);
    root_mqtt_app_start();
    delay(2000);
    xTimerStart(collectTimer, 0);
    
    recoTaskHandle = NULL;
    vTaskDelete(recoTaskHandle);
    
    return true;
}

void root_reconnectTask(void *pArg)
{
    esp_err_t err;
    mqttSender_t mensaje;
    int retry = 0;
    
    xTimerStop(collectTimer, 0);
    xTimerStop(sendMeterTimer, 0);
    
    if(mqttMgrHandle)
        vTaskDelete(mqttMgrHandle);
    if(mqttSendHandle)
        vTaskDelete(mqttSendHandle);
    
    clear_mqtt_sender_queue();

    while(true)
    {
        delay(theConf.mqttDiscoRetry);
        xEventGroupClearBits(wifi_event_group, MQTT_BIT|ERROR_BIT|DISCO_BIT);
        
        if(!mqtt_reconnect_attempt(&retry))
        {
            delay(1000);
        }
    }
}


void root_mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

/**
 * @brief Log MQTT error message to system log
 * 
 * Creates log entry with specific error type and optional error code.
 * 
 * @param error_msg Error message suffix
 * @param error_code Optional error code (can be NULL)
 */
void log_mqtt_error(const char *error_msg, const char *error_code)
{
    char *msg = (char*)calloc(1, 100);
    if(msg)
    {
        snprintf(msg, 100, "MQTT ERROR %s%s%s", error_msg, error_code ? " " : "", error_code ? error_code : "");
        writeLog(msg);
        free(msg);
    }
}

/**
 * @brief Handle TCP transport error and initiate reconnection
 * 
 * Launches reconnection task if not already running.
 */
void handle_mqtt_tcp_error(void)
{
    if(!recoTaskHandle)
    {
        log_mqtt_error("Reconnect task launched", NULL);
        MESP_LOGW(MESH_TAG, "Transport error. ReconnectTask launched");
        esp_mqtt_client_disconnect(clientCloud);
        
        if(xTaskCreate(root_reconnectTask, "mqttreco", 1024*4, NULL, 14, &recoTaskHandle) != pdPASS)
            MESP_LOGE(MESH_TAG, "Cannot create reconnect task");
    }
    else
        delay(5000);
}

/**
 * @brief Handle MQTT data reception
 * 
 * Processes incoming MQTT message: allocates buffer, copies data,
 * updates statistics, and queues for processing.
 */
void handle_mqtt_data_event(esp_mqtt_event_handle_t event)
{
    if(event->data_len == 0)
    {
        // MESP_LOGE(MESH_TAG, "Empty MQTT message received");
        return;
    }
    
    mqttMsg_t mqttHandle;
    memset(&mqttHandle, 0, sizeof(mqttHandle));
    
    char *msg = (char*)calloc(event->data_len+10, 1);   // give him some space +10
    if(!msg)
    {
        MESP_LOGE(MESH_TAG, "No RAM for MQTT message");
        return;
    }
    
    memcpy(msg, event->data, event->data_len);
    
    if((theConf.debug_flags >> dMQTT) & 1U)
    {
        msg[event->data_len] = 0;
        MESP_LOGI(MESH_TAG, "%sMqtt Msg [%s]", DBG_MQTT, msg);
    }
    
    mqttHandle.message = (uint8_t*)msg;
    mqttHandle.msgLen = event->data_len;
    
    theBlower.setStatsBytesIn(event->data_len);
    theBlower.setStatsMsgIn();
    
    char eltopic[60];
    memset(eltopic, 0, sizeof(eltopic));
    memcpy(eltopic, event->topic, event->topic_len);
    
    if(xQueueSend(mqttQ, &mqttHandle, 0) != pdTRUE)
    {
        MESP_LOGE(MESH_TAG, "Cannot queue MQTT message");
        free(msg);
        return;
    }
    
    // Delete retained messages
    if(strcmp(eltopic, cmdQueue) == 0)
        esp_mqtt_client_publish(clientCloud, cmdQueue, "", 0, 0, 1);
    else if(strcmp(eltopic, cmdBroadcast) == 0)
        esp_mqtt_client_publish(clientCloud, cmdBroadcast, "", 0, 0, 1);
}

/**
 * @brief Handle MQTT connection event
 * 
 * Subscribes to command queue and sets connection flags.
 */
void handle_mqtt_connected_event(void)
{
    int err = esp_mqtt_client_subscribe(clientCloud, cmdQueue, 0);
    if(err == 0 && (theConf.debug_flags >> dMQTT) & 1U)
        MESP_LOGE(MESH_TAG, "%sSubscription Ok to %s", DBG_MQTT, cmdQueue);
    
    mqttf = true;
    sendMeterf = false;
}

/**
 * @brief Handle MQTT disconnection event
 * 
 * Clears connection flags and sets disconnect event bit.
 */
void handle_mqtt_disconnected_event(void)
{
    xEventGroupSetBits(wifi_event_group, DISCO_BIT);
    xEventGroupClearBits(wifi_event_group, MQTT_BIT);
    delay(100);
    mqttf = false;
    sendMeterf = false;
}

void root_mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_BEFORE_CONNECT:
        break;
    case MQTT_EVENT_CONNECTED:
        handle_mqtt_connected_event();
        break;
    case MQTT_EVENT_DISCONNECTED:
        handle_mqtt_disconnected_event();
        break;
    case MQTT_EVENT_SUBSCRIBED:
        xEventGroupSetBits(wifi_event_group, MQTT_BIT);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
#ifdef MQTTSUB
        xEventGroupSetBits(wifi_event_group, PUB_BIT|DONE_BIT);
#endif
        mqttf = false;
        break;
    case MQTT_EVENT_PUBLISHED:
        if((theConf.debug_flags >> dMQTT) & 1U)
            MESP_LOGI(MESH_TAG, "%sMQTT_EVENT_PUBLISHED, msg_id=%d", DBG_MQTT, event->msg_id);
        xEventGroupSetBits(wifi_event_group, PUB_BIT);
        break;
    case MQTT_EVENT_DATA:
        handle_mqtt_data_event(event);
        break;
    case MQTT_EVENT_ERROR:
        MESP_LOGI(MESH_TAG, "MQTT_EVENT_ERROR");
        xEventGroupClearBits(wifi_event_group, ERROR_BIT);
        
        if(event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            handle_mqtt_tcp_error();
        }
        else if(event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED)
        {
            MESP_LOGI(MESH_TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
            log_mqtt_error("Connection refused", NULL);
        }
        else
        {
            MESP_LOGW(MESH_TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
            char error_code[20];
            snprintf(error_code, sizeof(error_code), "0x%x", event->error_handle->error_type);
            log_mqtt_error("Unknown type", error_code);
        }
        
        sendMeterf = false;
        xEventGroupSetBits(wifi_event_group, ERROR_BIT);
        break;
    default:
        break;
    }
}


int findCommand(const char * cual)
{
	for (int a=0;a<MAXCMDS;a++)
	{
		if(strcmp(cmds[a].comando,cual)==0)
			return a;
	}
	return ESP_FAIL;
}

/**
 * @brief Process a single command item from MQTT command array
 * 
 * Extracts the command name, looks up its handler, and executes it.
 * Logs errors if command not found or missing cmd field.
 * 
 * @param cmditem cJSON object containing the command
 * @return void
 */
void process_mqtt_command_item(cJSON *cmditem)
{
	cJSON *order = cJSON_GetObjectItem(cmditem, "cmd");
	if (!order)
	{
		MESP_LOGE(MESH_TAG, "No order/cmd received in command item");
		return;
	}

	MESP_LOGI(MESH_TAG, "%sExternal cmd [%s]", DBG_XCMDS, order->valuestring);
	int cual = findCommand(order->valuestring);
	
	if (cual >= 0)
	{
		(*cmds[cual].code)((void *)cmditem);	// call the cmd and wait for it to end
	}
	else
	{
		MESP_LOGE(MESH_TAG, "Invalid cmd received %s", order->valuestring);
	}
}

/**
 * @brief Process MQTT command array from message
 * 
 * Iterates through cmdarr array in cJSON and dispatches each command
 * to the command handler. Handles parse errors and missing arrays.
 * 
 * @param elcmd Parsed cJSON object containing cmdarr
 * @return void
 */
void process_mqtt_command_array(cJSON *elcmd)
{
	cJSON *monton = cJSON_GetObjectItem(elcmd, "cmdarr");
	if (!monton)
	{
		MESP_LOGE(MESH_TAG, "No cmdarr received");
		return;
	}

	int son = cJSON_GetArraySize(monton);
	for (int a = 0; a < son; a++)
	{
		cJSON *cmditem = cJSON_GetArrayItem(monton, a);
		if (cmditem)
		{
			process_mqtt_command_item(cmditem);
		}
		else
		{
			MESP_LOGE(MESH_TAG, "Internal Error MqttMgr cmditem not obtained");
		}
	}
}

void meshSendTask(void *pArg)
{
    mqttSender_t meshMsg;
    mesh_data_t data;
    
    while(true)
    {
        if(xQueueReceive(meshQueue, &meshMsg, portMAX_DELAY))
        {   
            if(meshMsg.msg)
            {
                meshunion_t *intMessage = (meshunion_t*)calloc(1, meshMsg.lenMsg + 4);
                if (!intMessage)
                {
                    MESP_LOGE(MESH_TAG, "No RAM for mesh message");
                    free(meshMsg.msg);
                    continue;
                }

                void *p = (void*)intMessage + 4;
                memcpy(p, meshMsg.msg, meshMsg.lenMsg);
                intMessage->cmd = MESH_INT_DATA_CJSON;
                
                data.proto = MESH_PROTO_BIN;
                data.tos = MESH_TOS_P2P;
                data.data = (uint8_t*)intMessage;
                data.size = meshMsg.lenMsg + 4;
                
                esp_err_t err = esp_mesh_send(meshMsg.addr, &data, meshMsg.addr == NULL ? MESH_DATA_FROMDS : MESH_DATA_P2P, NULL, 0);
                if (err != ESP_OK)
                    MESP_LOGE(MESH_TAG, "Mesh send failed: %x", err);
                
                free(intMessage);
                free(meshMsg.msg);
            }
        }
    }
}

void root_emergencyTask(void *pArg)
{
    mqttSender_t emergencyMsg, meshMsg;

    while(true)
    {
        if(xQueueReceive(mqtt911, &emergencyMsg, portMAX_DELAY))
        {   
            if(emergencyMsg.msg)
            {  
                char *intMessage = (char *)calloc(1, emergencyMsg.lenMsg);
                if (!intMessage)
                {
                    MESP_LOGE(MESH_TAG, "No RAM for emergency message");
                    free(emergencyMsg.msg);
                    continue;
                }

                memcpy(intMessage, emergencyMsg.msg, emergencyMsg.lenMsg);
                free(emergencyMsg.msg);

                memset(&meshMsg, 0, sizeof(meshMsg));
                meshMsg.msg = intMessage;
                meshMsg.lenMsg = strlen(intMessage);
                meshMsg.addr = NULL;  // to root
                
                if(xQueueSend(meshQueue, &meshMsg, 0) != pdTRUE)
                {
                    MESP_LOGE(MESH_TAG, "Error queueing emergency msg");
                    free(meshMsg.msg);  // free on failure
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
    mqttMsg_t mqttHandle;
	cJSON *elcmd;

    while(true)
    {
        if(xQueueReceive(mqttQ, &mqttHandle, portMAX_DELAY))
        {
            elcmd = cJSON_Parse((char*)mqttHandle.message);
			if(elcmd)
			{
				process_mqtt_command_array(elcmd);
                cJSON_Delete(elcmd);
            }
            else
                MESP_LOGE(MESH_TAG, "Cmd received not parsable [%s]", mqttHandle.message);

            if(mqttHandle.message)
            {
                free(mqttHandle.message);
            }
        }
    }
}

/// to get ssl certificate 
//                                                <-------- Site/Port -->
// echo "" | openssl s_client -showcerts -connect m13.cloudmqtt.com:28747 | sed -n "1,/Root/d; /BEGIN/,/END/p" | openssl x509 -outform PEM >hive.pem

void setup_mqtt_config(void)
{
    char who[20];
    
    memset(who, 0, sizeof(who));
    memset(&mqtt_cfg, 0, sizeof(mqtt_cfg));
    
    snprintf(who, sizeof(who), "Meterserver%d", theConf.poolid);
    
    mqtt_cfg.broker.address.uri = theConf.mqttServer;
    mqtt_cfg.credentials.client_id = who;
    mqtt_cfg.credentials.username = theConf.mqttUser;
    mqtt_cfg.credentials.authentication.password = theConf.mqttPass;
    mqtt_cfg.network.disable_auto_reconnect = true;  // we will manage this in recoTask
    mqtt_cfg.buffer.size = MQTTBIG;
    
    // Configure certificate if present
    if (strlen(theConf.mqttcert) > 0)
    {
        mqtt_cfg.broker.verification.certificate = theConf.mqttcert;
        mqtt_cfg.broker.verification.certificate_len = theConf.mqttcertlen;
    }
}

esp_mqtt_client_handle_t root_setupMqtt(void)
{
    setup_mqtt_config();
    
    clientCloud = esp_mqtt_client_init(&mqtt_cfg);
    if (!clientCloud)
    {
        MESP_LOGE(MESH_TAG, "Failed to initialize MQTT client");
        return NULL;
    }
    
    esp_mqtt_client_register_event(clientCloud, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, root_mqtt_event_handler, NULL);
    return clientCloud;
}

bool launch_mqtt_manager_task(void)
{
    if (xTaskCreate(&root_mqttMgr, "mqtt", 1024*10, NULL, 10, &mqttMgrHandle) != pdPASS)
    {
        MESP_LOGE(MESH_TAG, "Failed to launch MQTT manager task");
        return false;
    }
    return true;
}

bool launch_mqtt_sender_task(void)
{
    if (xTaskCreate(&root_mqtt_sender, "mqttsend", 1024*20, NULL, 14, &mqttSendHandle) != pdPASS)
    {
        MESP_LOGE(MESH_TAG, "Failed to launch MQTT sender task");
        return false;
    }
    return true;
}

void root_mqtt_app_start(void)
{
    launch_mqtt_manager_task();
    launch_mqtt_sender_task();
}

uint8_t last_mesh_layer = 0;

void handle_child_connected(mesh_event_child_connected_t *child_connected)
{
    mesh_addr_t id = {0};
    esp_mesh_get_id(&id);
    memcpy(&id.addr, &child_connected->mac, 6);

    MESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_CONNECTED>aid:%d, " MACSTR "",
             child_connected->aid, MAC2STR(child_connected->mac));

    if (!esp_mesh_is_root())
        return;

    esp_err_t err = root_send_data_to_node(id);  // send configuration to station Node including if start schedule
    if (err != ESP_OK)
        MESP_LOGE(MESH_TAG, "Send SSID Failed");

    logCount++;
    if (logCount + 1 >= theConf.totalnodes)
    {
        xTimerStop(loginTimer, 0);  // stop any pending login timeout
        if ((theConf.debug_flags >> dLOGIC) & 1U)
            MESP_LOGI(TAG, "Login Timeout Done %d have %d", theConf.totalnodes - 1, logCount);
        send_login_msg("Login Ok");
    }
}

void handle_child_disconnected(mesh_event_child_disconnected_t *child_disconnected)
{
    MESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_DISCONNECTED>aid:%d, " MACSTR "",
             child_disconnected->aid, MAC2STR(child_disconnected->mac));

    if (sendMeterf)
        MESP_LOGI(MESH_TAG, "Disconnect while sending data. Retry");

    if (!esp_mesh_is_root())
        return;

    logCount--;
    if (logCount < theConf.totalnodes - 1)
    {
        xTimerStart(loginTimer, 0);
        esp_rom_printf("Login Timer restarted\n");
    }
}

void handle_parent_connected(mesh_event_connected_t *connected)
{
    mesh_addr_t id = {0};
    esp_mesh_get_id(&id);

    mesh_layer = connected->self_layer;
    memcpy(&mesh_parent_addr.addr, connected->connected.bssid, 6);

    MESP_LOGI(MESH_TAG,
             "<MESH_EVENT_PARENT_CONNECTED>layer:%d-->%d, parent:" MACSTR "%s, ID:" MACSTR "",
             last_mesh_layer, mesh_layer, MAC2STR(mesh_parent_addr.addr),
             esp_mesh_is_root() ? "<ROOT>" : (mesh_layer == 2) ? "<layer2>" : "", MAC2STR(id.addr));

    last_mesh_layer = mesh_layer;
    mesh_netifs_start(esp_mesh_is_root());
    meshf = true;
}

void handle_parent_disconnected(mesh_event_disconnected_t *disconnected)
{
    MESP_LOGI(MESH_TAG, "<MESH_EVENT_PARENT_DISCONNECTED>reason:%d", disconnected->reason);

    if (esp_mesh_is_root())
    {
        hostflag = false;
        mqttf = false;
        theBlower.setStatsStaDiscos();

        char *msg = (char *)calloc(1, 100);
        if (msg)
        {
            sprintf(msg, "Mesh Parent (HOST) disconnected Reason:%d", disconnected->reason);
            writeLog(msg);
            free(msg);
        }
    }

    mesh_layer = esp_mesh_get_layer();
    mesh_netifs_stop();
    meshf = false;
}

void mesh_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    switch (event_id) {
    case MESH_EVENT_STARTED: {
        mesh_addr_t id = {0};
        esp_mesh_get_id(&id);
        // MESP_LOGI(MESH_TAG, "<MESH_EVENT_MESH_STARTED>ID:"MACSTR"", MAC2STR(id.addr));
        mesh_layer = esp_mesh_get_layer();
    }
    break;
    case MESH_EVENT_STOPPED: {
        // MESP_LOGI(MESH_TAG, "<MESH_EVENT_STOPPED>");
        mesh_layer = esp_mesh_get_layer();
    }
    break;
    case MESH_EVENT_CHILD_CONNECTED: {
        handle_child_connected((mesh_event_child_connected_t *)event_data);
    }
    break;
    case MESH_EVENT_CHILD_DISCONNECTED: {
        handle_child_disconnected((mesh_event_child_disconnected_t *)event_data);
    }
    break;
    case MESH_EVENT_ROUTING_TABLE_ADD: {
        mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
        MESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_ADD>add %d, new:%d",
                 routing_table->rt_size_change,
                 routing_table->rt_size_new);
    }
    break;
    case MESH_EVENT_ROUTING_TABLE_REMOVE: {
        // mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
        // MESP_LOGW(MESH_TAG, "<MESH_EVENT_ROUTING_TABLE_REMOVE>remove %d, new:%d",
        //          routing_table->rt_size_change,
        //          routing_table->rt_size_new);
    }
    break;
    case MESH_EVENT_NO_PARENT_FOUND: {
        // mesh_event_no_parent_found_t *no_parent = (mesh_event_no_parent_found_t *)event_data;
        // MESP_LOGI(MESH_TAG, "<MESH_EVENT_NO_PARENT_FOUND>scan times:%d",
        //          no_parent->scan_times);

    }
    /* TODO handler for the failure */    
    case MESH_EVENT_PARENT_CONNECTED: {
        handle_parent_connected((mesh_event_connected_t *)event_data);
    }
    break;
    case MESH_EVENT_PARENT_DISCONNECTED: {
        handle_parent_disconnected((mesh_event_disconnected_t *)event_data);
    }
    break;
    // case MESH_EVENT_LAYER_CHANGE: {
    //     mesh_event_layer_change_t *layer_change = (mesh_event_layer_change_t *)event_data;
    //     mesh_layer = layer_change->new_layer;
    //     // MESP_LOGI(MESH_TAG, "<MESH_EVENT_LAYER_CHANGE>layer:%d-->%d%s",
    //     //          last_layer, mesh_layer,
    //     //          esp_mesh_is_root() ? "<ROOT>" :
    //     //          (mesh_layer == 2) ? "<layer2>" : "");
    //     last_mesh_layer = mesh_layer;
    // }
    // break;
    // case MESH_EVENT_ROOT_ADDRESS: {
    //     // mesh_event_root_address_t *root_addr = (mesh_event_root_address_t *)event_data;
    //     // MESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_ADDRESS>root address:"MACSTR"",
    //     //          MAC2STR(root_addr->addr));
    // }
    // break;
    // case MESH_EVENT_VOTE_STARTED: {
    //     // mesh_event_vote_started_t *vote_started = (mesh_event_vote_started_t *)event_data;
    //     // MESP_LOGI(MESH_TAG,
    //     //          "<MESH_EVENT_VOTE_STARTED>attempts:%d, reason:%d, rc_addr:"MACSTR"",
    //     //          vote_started->attempts,
    //     //          vote_started->reason,
    //     //          MAC2STR(vote_started->rc_addr.addr));
    // }
    // break;
    // case MESH_EVENT_VOTE_STOPPED: {
    //     // MESP_LOGI(MESH_TAG, "<MESH_EVENT_VOTE_STOPPED>");
    //     break;
    // }
    // case MESH_EVENT_ROOT_SWITCH_REQ: {
    //     // mesh_event_root_switch_req_t *switch_req = (mesh_event_root_switch_req_t *)event_data;
    //     // MESP_LOGI(MESH_TAG,
    //     //          "<MESH_EVENT_ROOT_SWITCH_REQ>reason:%d, rc_addr:"MACSTR"",
    //     //          switch_req->reason,
    //     //          MAC2STR( switch_req->rc_addr.addr));
    // }
    // break;
    // case MESH_EVENT_ROOT_SWITCH_ACK: {
    //     /* new root */
    //     mesh_layer = esp_mesh_get_layer();
    //     esp_mesh_get_parent_bssid(&mesh_parent_addr);
    //     // MESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_SWITCH_ACK>layer:%d, parent:"MACSTR"", mesh_layer, MAC2STR(mesh_parent_addr.addr));
    // }
    // break;
    // case MESH_EVENT_TODS_STATE: {
    //     // mesh_event_toDS_state_t *toDs_state = (mesh_event_toDS_state_t *)event_data;
    //     // MESP_LOGI(MESH_TAG, "<MESH_EVENT_TODS_REACHABLE>state:%d", *toDs_state);
    // }
    // break;
    // case MESH_EVENT_ROOT_FIXED: {
    //     // mesh_event_root_fixed_t *root_fixed = (mesh_event_root_fixed_t *)event_data;
    //     // MESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_FIXED>%s",
    //     //          root_fixed->is_fixed ? "fixed" : "not fixed");
    // }
    // break;
    // case MESH_EVENT_ROOT_ASKED_YIELD: {
    //     // mesh_event_root_conflict_t *root_conflict = (mesh_event_root_conflict_t *)event_data;
    //     // MESP_LOGI(MESH_TAG,
    //     //          "<MESH_EVENT_ROOT_ASKED_YIELD>"MACSTR", rssi:%d, capacity:%d",
    //     //          MAC2STR(root_conflict->addr),
    //     //          root_conflict->rssi,
    //     //          root_conflict->capacity);
    // }
    // break;
    // case MESH_EVENT_CHANNEL_SWITCH: {
    //     // mesh_event_channel_switch_t *channel_switch = (mesh_event_channel_switch_t *)event_data;
    //     // MESP_LOGI(MESH_TAG, "<MESH_EVENT_CHANNEL_SWITCH>new channel:%d", channel_switch->channel);
    // }
    // break;
    // case MESH_EVENT_SCAN_DONE: {
    //     // mesh_event_scan_done_t *scan_done = (mesh_event_scan_done_t *)event_data;
    //     // MESP_LOGI(MESH_TAG, "<MESH_EVENT_SCAN_DONE>number:%d",
    //     //          scan_done->number);
    // }
    // break;
    // case MESH_EVENT_NETWORK_STATE: {
    //     // mesh_event_network_state_t *network_state = (mesh_event_network_state_t *)event_data;
    //     // MESP_LOGI(MESH_TAG, "<MESH_EVENT_NETWORK_STATE>is_rootless:%d",
    //     //          network_state->is_rootless);
    // }
    // break;
    // case MESH_EVENT_STOP_RECONNECTION: {
    //     // MESP_LOGI(MESH_TAG, "<MESH_EVENT_STOP_RECONNECTION>");
    // }
    // break;
    // case MESH_EVENT_FIND_NETWORK: {
    //     mesh_event_find_network_t *find_network = (mesh_event_find_network_t *)event_data;
    //     MESP_LOGI(MESH_TAG, "<MESH_EVENT_FIND_NETWORK>new channel:%d, router BSSID:"MACSTR"",
    //              find_network->channel, MAC2STR(find_network->router_bssid));
    // }
    // break;
    // case MESH_EVENT_ROUTER_SWITCH: {
    //     // mesh_event_router_switch_t *router_switch = (mesh_event_router_switch_t *)event_data;
    //     // MESP_LOGI(MESH_TAG, "<MESH_EVENT_ROUTER_SWITCH>new router:%s, channel:%d, "MACSTR"",
    //     //          router_switch->ssid, router_switch->channel, MAC2STR(router_switch->bssid));
    // }
    // break;
    default:
        MESP_LOGI(MESH_TAG, "Mesh unknown id:%d", event_id);
        break;
    }
} 

// Emergency/login helpers build JSON payloads and manage queue ownership in one place
bool enqueue_mesh_emergency(const char *msg, uint32_t err)
{
    mqttSender_t meshMsg;
    memset(&meshMsg, 0, sizeof(meshMsg));

    cJSON *root = cJSON_CreateObject();
    if(!root)
        return false;

    cJSON_AddStringToObject(root, "cmd", "911");
    cJSON_AddStringToObject(root, "msg", msg);
    cJSON_AddNumberToObject(root, "err", err);

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if(!payload)
        return false;

    meshMsg.queue = emergencyQueue;
    meshMsg.msg = payload;
    meshMsg.lenMsg = strlen(payload);
    meshMsg.addr = NULL;

    if(xQueueSend(meshQueue, &meshMsg, 0) != pdPASS)
    {
        MESP_LOGE(MESH_TAG, "Cannot queue emergency frame");
        free(payload);
        return false;
    }

    return true;
}

char *build_login_payload(const char *title)
{
    cJSON *root = cJSON_CreateObject();
    if(!root)
        return NULL;

    cJSON_AddStringToObject(root, "alarm", title);
    cJSON_AddNumberToObject(root, "pool", theConf.poolid);
    cJSON_AddNumberToObject(root, "nodes", theConf.totalnodes - 1);
    cJSON_AddNumberToObject(root, "logged", logCount);

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return payload;
}

void send_internal_emergency(char * msg, uint32_t err)
{
    enqueue_mesh_emergency(msg, err);
}

void send_login_msg(char * title)
{
    mqttSender_t mqttMsg;
    memset(&mqttMsg, 0, sizeof(mqttMsg));

    char *payload = build_login_payload(title);
    if(!payload)
    {
        MESP_LOGE(MESH_TAG, "No RAM for Login Timeout payload");
        return;
    }

    mqttMsg.queue = alarmQueue;
    mqttMsg.msg = payload;                    // freed by mqtt sender
    mqttMsg.lenMsg = strlen(payload);
    mqttMsg.code = NULL;
    mqttMsg.param = NULL;

    if(xQueueSendFromISR(mqttSender, &mqttMsg, 0) != pdTRUE)
    {
        MESP_LOGE(MESH_TAG, "Error queueing msg");
        free(mqttMsg.msg);
    }
    else
    {
        // must set the wifi_event_bit SEND_MQTT_BIT, else it will just collect the message in the queue
        xEventGroupSetBits(wifi_event_group, SENDMQTT_BIT);
    }
}

void login_timeout(TimerHandle_t timer)
{
    // Mesh event helpers keep the switch lean and centralize logging/state updates

    if((theConf.debug_flags >> dLOGIC) & 1U) 
        MESP_LOGI(TAG,"Login Timeout Expected %d have %d",theConf.totalnodes-1,logCount);
    //send a time out messsage and restart timer again
    send_login_msg("Login Timeout");
}

/**
 * @brief Initialize system semaphores
 * 
 * Creates and initializes binary semaphores for flash access,
 * table management, reconnection handling, and work scheduling
 */
void init_system_semaphores(void)
{
    flashSem = xSemaphoreCreateBinary();
    xSemaphoreGive(flashSem);

    tableSem = xSemaphoreCreateBinary();
    xSemaphoreGive(tableSem);

    recoSem = xSemaphoreCreateBinary();
    xSemaphoreGive(recoSem);

    scheduleSem = xSemaphoreCreateBinary();
    // xSemaphoreGive(scheduleSem);  // Intentionally not given initially

    workTaskSem = xSemaphoreCreateBinary();
    // xSemaphoreGive(workTaskSem);  // Intentionally not given initially
}

/**
 * @brief Initialize NVS flash storage
 * 
 * Initializes the non-volatile storage, handling cases where
 * partitions need to be erased and reinitialized
 */
void init_nvs_flash(void)
{
    esp_err_t err = nvs_flash_init();
    
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        MESP_LOGI(MESH_TAG, "NVS pages need recovery, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    
    ESP_ERROR_CHECK(err);
}

/**
 * @brief Load and validate configuration from flash
 * 
 * Reads configuration from flash storage and validates the centinel
 * value. Erases and resets if validation fails.
 */
void load_and_validate_config(void)
{
    read_flash();
    
    if (theConf.centinel != CENTINEL) {
        MESP_LOGI(MESH_TAG, "Invalid centinel check. Erasing config...");
        erase_config();
    }
    
    // Validate and set logging level
    esp_log_level_set("*", (esp_log_level_t)theConf.loglevel);
    esp_log_level_set("mqtt_client", (esp_log_level_t)0);
    
    // Set default values for uninitialized fields
    if (theConf.test_timer_div == 0) {
        theConf.test_timer_div = 10;
    }
}

/**
 * @brief Update system state tracking
 * 
 * Records reset reason, increments boot count, and persists to flash
 */
void update_system_state(void)
{
    theConf.lastResetCode = esp_rom_get_reset_reason(0);
    theConf.bootcount++;
    write_to_flash();           
}

/**
 * @brief Initialize global state flags and variables
 * 
 * Sets up all the global flags used for mesh, WiFi, MQTT, and scheduling
 */
void init_global_state_flags(void)
{
    // Scheduling and mesh state
    mesh_init_done = false;
    mesh_on = false;
    logCount = 0;
    pausef = schedulef = false;
    ck = ck_d = ck_h = 0;
    scheduleHandle = NULL;
    wifiready = false;
    showHandle = NULL;
    oledDisp = NULL;
    meshf = false;
    mqttErrors = 0;
    BASETIMER = theConf.baset;
    
    // Mesh configuration
    mesh_layer = -1;
    mesh_started = false;
    memset(MESH_ID, 0x77, 6);
    uint8_t pool_id = theConf.poolid;
    memcpy(&MESH_ID[5], &pool_id, 1);
    esp_log_buffer_hex(MESH_TAG, &MESH_ID, 6);
    
    // MQTT state
    mqttf = false;
    memset(&GroupID.addr, 0xff, 6);
    sendMeterf = false;
    hostflag = false;
    loadedf = false;
    firstheap = false;
    clientCloud = NULL;
    lastKnowCount = 0;
    
    // Retry configuration
    if (theConf.mqttDiscoRetry == 0) {
        theConf.mqttDiscoRetry = MQTTRECONNECTTIME;
    }
    
    // Timer counters
    countTimersStart = countTimersEnd = 0;
}

/**
 * @brief Initialize MQTT queue topic names
 * 
 * Constructs the MQTT topic strings based on pool ID and configuration
 */
void init_mqtt_queue_names(void)
{
    snprintf(cmdQueue, sizeof(cmdQueue), "%s/%d/%s", QUEUE, theConf.poolid, MQTTCMD);
    snprintf(infoQueue, sizeof(infoQueue), "%s/%d/%s", QUEUE, theConf.poolid, MQTTINFO);
    snprintf(alarmQueue, sizeof(alarmQueue), "%s/%d/%s", QUEUE, theConf.poolid, MQTTALARM);
    snprintf(limitsQueue, sizeof(limitsQueue), "%s/%d/%s", QUEUE, theConf.poolid, MQTTLIMITS);
}
/**
 * @brief Initialize internal mesh command strings
 * 
 * Initializes lookup table for internal mesh commands
 */
void init_internal_mesh_commands(void)
{
    strncpy(internal_cmds[SCHEDULE], "schstart", sizeof(internal_cmds[SCHEDULE]) - 1);
    strncpy(internal_cmds[BOOTRESP], "bootresp", sizeof(internal_cmds[BOOTRESP]) - 1);
    strncpy(internal_cmds[SENDMETRICS], "sendmetrics", sizeof(internal_cmds[SENDMETRICS]) - 1);
    strncpy(internal_cmds[METERSDATA], "meterdata", sizeof(internal_cmds[METERSDATA]) - 1);
    strncpy(internal_cmds[PRODUCTION], "prod", sizeof(internal_cmds[PRODUCTION]) - 1);
    strncpy(internal_cmds[CONFIRMLOCK], "confirm", sizeof(internal_cmds[CONFIRMLOCK]) - 1);
    strncpy(internal_cmds[CONFIRMLOCKERR], "confError", sizeof(internal_cmds[CONFIRMLOCKERR]) - 1);
    strncpy(internal_cmds[INSTALLATION], "install", sizeof(internal_cmds[INSTALLATION]) - 1);
    strncpy(internal_cmds[REINSTALL], "reinstall", sizeof(internal_cmds[REINSTALL]) - 1);
    strncpy(internal_cmds[CONFIRMINST], "installconf", sizeof(internal_cmds[CONFIRMINST]) - 1);
    strncpy(internal_cmds[FORMAT], "format", sizeof(internal_cmds[FORMAT]) - 1);
    strncpy(internal_cmds[UPDATEMETER], "update", sizeof(internal_cmds[UPDATEMETER]) - 1);
    strncpy(internal_cmds[ERASEMETRICS], "erase", sizeof(internal_cmds[ERASEMETRICS]) - 1);
    strncpy(internal_cmds[MQTTMETRICS], "mqttmetrics", sizeof(internal_cmds[MQTTMETRICS]) - 1);
    strncpy(internal_cmds[METRICRESP], "metriscresp", sizeof(internal_cmds[METRICRESP]) - 1);
    strncpy(internal_cmds[SHOWDISPLAY], "display", sizeof(internal_cmds[SHOWDISPLAY]) - 1);
}

/**
 * @brief Register external MQTT command handlers
 * 
 * Initializes the command table with mappings between command names,
 * codes, and abbreviations for external MQTT commands.
 * Commands are initialized at index time based on their order in the registry.
 */
// External MQTT command registry helper keeps string copies safe and handlers aligned
void register_external_mqtt_commands(void)
{
    int idx = 0;

    auto set_cmd = [&](const char *name, const char *abbr, functrsn handler) {
        strncpy(cmds[idx].comando, name, sizeof(cmds[idx].comando) - 1);
        cmds[idx].comando[sizeof(cmds[idx].comando) - 1] = '\0';

        strncpy(cmds[idx].abr, abbr, sizeof(cmds[idx].abr) - 1);
        cmds[idx].abr[sizeof(cmds[idx].abr) - 1] = '\0';

        cmds[idx].code = handler;
        idx++;
    };

    set_cmd("format", "FRMT", cmdFormat);
    set_cmd("netw", "NETW", cmdNetw);
    set_cmd("mqtt", "MQTT", cmdMQTT);
    set_cmd("prod", "PROD", cmdProd);
    set_cmd("update", "UPDT", cmdUpdate);
    set_cmd("erase", "ERAS", cmdEraseMetrics);
    set_cmd("mqttmetrics", "METR", cmdSendMetrics);
    set_cmd("display", "DISP", cmdDisplay);
    set_cmd("log", "LOGG", cmdLogs);
    set_cmd("reset", "REST", cmdReset);
    set_cmd("Panels", "PANE", cmdPanels);
    set_cmd("Battery", "BATT", cmdBattery);
    set_cmd("Sensors", "SENS", cmdSensors);
    set_cmd("Inverter", "INVR", cmdInverter);
    set_cmd("limits", "LMTS", cmdLimits);
    set_cmd("rlimits", "RLIM", cmdReadLimits);
}
/**
 * @brief Configure GPIO pins for relay, LED, and heartbeat
 * 
 * Sets up output pins with appropriate initial states
 */
void init_gpio_pins(void)
{
    gpio_config_t io_conf = {};
    
    // Configure RELAY pin
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pin_bit_mask = (1ULL << RELAY);
    gpio_set_level((gpio_num_t)RELAY, 1);  // Set HIGH initially
    gpio_config(&io_conf);

    // Configure WiFi LED pin
    io_conf.pin_bit_mask = (1ULL << WIFILED);
    gpio_set_level((gpio_num_t)WIFILED, 0);  // Set LOW initially
    gpio_config(&io_conf);
    
    // Configure Heartbeat pin
    io_conf.pin_bit_mask = (1ULL << BEATPIN);
    gpio_set_level((gpio_num_t)BEATPIN, 1);  // Set HIGH initially
    gpio_config(&io_conf);
}


/**
 * @brief Initialize cryptographic engine
 * 
 * Sets up AES encryption context
 */
void init_crypto_engine(void)
{
    esp_aes_init(&actx);
}

/**
 * @brief Helper to create and validate queue
 * 
 * @param queue_ptr Pointer to store queue handle
 * @param queue_size Number of items the queue can hold
 * @param item_size Size of each queue item
 * @param queue_name Name for logging purposes
 * @return true if queue created successfully, false otherwise
 */
bool create_queue_safe(QueueHandle_t *queue_ptr, uint32_t queue_size, 
                               uint32_t item_size, const char *queue_name)
{
    *queue_ptr = xQueueCreate(queue_size, item_size);
    if (*queue_ptr == NULL) {
        MESP_LOGE(MESH_TAG, "Failed to create queue: %s", queue_name);
        return false;
    }
    return true;
}

/**
 * @brief Initialize all inter-task communication queues
 * 
 * Creates queues for MQTT, mesh, RS485, and emergency messages
 */
void init_system_queues(void)
{
    create_queue_safe(&mqtt911, 5, sizeof(mqttMsg_t), "emergency");
    create_queue_safe(&meshQueue, MAXNODES, sizeof(mqttSender_t), "mesh send");
    create_queue_safe(&mqttQ, 20, sizeof(mqttMsg_t), "MQTT command");
    create_queue_safe(&mqttSender, 20, sizeof(mqttSender_t), "MQTT sender");
    create_queue_safe(&rs485Q, 10, sizeof(rs485queue_t), "RS485");
}

/**
 * @brief Calculate permanent data delivery time
 * 
 * Computes the time slot for data transmission based on network configuration
 * 
 * @return Calculated permanent time in milliseconds, minimum 500ms
 */
uint32_t calculate_permanent_delivery_time(void)
{
    uint32_t permanent_time = (theConf.totalnodes / theConf.conns) * EXPECTED_DELIVERY_TIME;
    
    if (permanent_time == 0) {
        MESP_LOGI(MESH_TAG, "Permanent time calculated as 0, using default 500ms");
        permanent_time = 500;
    }
    
    if (theConf.repeat < 1) {
        theConf.repeat = 1;
    }
    
    return permanent_time;
}

/**
 * @brief Initialize system timers for various periodic tasks
 * 
 * Creates timers for meter collection, transmission, login timeout, and confirmation
 */
void init_system_timers(void)
{
    // Validate and set login wait time
    if (theConf.loginwait == 0) {
        theConf.loginwait = LOGINTIME;
    }
    
    // Schedule timer for internal scheduling
    scheduleTimer = xTimerCreate("scht", pdMS_TO_TICKS(500), pdFALSE, 
                                  (void*)0, schedule_timeout);
    
    // Send meter timer with lambda callback
    sendMeterTimer = xTimerCreate("SendM", pdMS_TO_TICKS(MESHTIMEOUT), pdFALSE, NULL,
        [](TimerHandle_t xTimer) {
            xEventGroupSetBits(otherGroup, REPEAT_BIT);
        }
    );
    
    // Meter collection timer (based on WiFi mode)
    // uint32_t collection_period = theConf.repeat * 25000;        // for testing every 10s
    uint32_t collection_period = theConf.repeat * 60000;        // in minutes to ms
    TimerCallbackFunction_t collection_callback = 
        (theConf.wifi_mode > 0) ? root_collect_meter_data : wifi_send_meter_data;
    BaseType_t collection_autoreload = 
        (theConf.wifi_mode > 0) ? pdFALSE : pdTRUE;
    printf("coolect %d\n", collection_period);
    collectTimer = xTimerCreate("Timer", pdMS_TO_TICKS(collection_period),
                               collection_autoreload, (void*)0, collection_callback);
    
    // Login timeout timer
    loginTimer = xTimerCreate("Ltim", pdMS_TO_TICKS(theConf.loginwait), pdFALSE,
                             (void*)0, login_timeout);
    
    // Confirmation timer (only in WiFi mode) with lambda callback
    if (theConf.wifi_mode) {
        confirmTimer = xTimerCreate("Confirm", pdMS_TO_TICKS(CONFIRMTIMER), pdFALSE,
            (void*)0,
            [](TimerHandle_t xTimer) {
                cJSON *root = cJSON_CreateObject();
                if (root) {
                    char *cualMID = (char*)pvTimerGetTimerID(xTimer);
                    cJSON_AddStringToObject(root, "cmd", internal_cmds[6]);
                    cJSON_AddStringToObject(root, "mid", cualMID);
                    cJSON_AddNumberToObject(root, "status", METER_NOT_FOUND);

                    char *intmsg = cJSON_PrintUnformatted(root);
                    if (intmsg) {
                        MESP_LOGW(MESH_TAG, "Confirm timeout for [%s]", cualMID);
                        root_send_confirmation_central(intmsg, strlen(intmsg), discoQueue);
                        free(intmsg);
                    }
                    cJSON_Delete(root);
                } else {
                    MESP_LOGE(MESH_TAG, "No RAM for Confirm timeout");
                }
            }
        );
    }
}

/**
 * @brief Initialize logging system and time synchronization
 * 
 * Sets up logging file and optionally initializes system time from stored date
 */
void init_logging_system(void)
{
#ifdef LOGOPT
    logFileInit();  // Initialize log file
    timeval localtime = {};
    
    theConf.lastKnownDate = theBlower.getReservedDate();
    localtime.tv_sec = theConf.lastKnownDate;
    localtime.tv_usec = 0;  // No connection most likely
    settimeofday(&localtime, NULL);
#endif
}

/**
 * @brief Initialize event groups for inter-task synchronization
 * 
 * Creates event groups for WiFi/MQTT state management and other async events
 */
void init_event_groups(void)
{
    wifi_event_group = xEventGroupCreate();
    if (wifi_event_group == NULL) {
        MESP_LOGE(MESH_TAG, "Failed to create WiFi event group");
    }
    
    otherGroup = xEventGroupCreate();
    if (otherGroup == NULL) {
        MESP_LOGE(MESH_TAG, "Failed to create other event group");
    }
}
/**
 * @brief Initialize DO Defaults
 * 
 * Creates Defaults for Dissolved Oxygen PID
 */
void init_DO_Defaults(void)
{
    theConf.doParms.setpoint=5.5;
    theConf.doParms.KP=1.5;
    theConf.doParms.KI=0.1;
    theConf.doParms.KD=0.01;
    theConf.doParms.docontrol=false;
    theConf.doParms.nighonly=false;
    theConf.doParms.sampletime=10;
}

/**
 * @brief Main initialization process for the system
 * 
 * Orchestrates all system initialization in the correct order:
 * 1. Semaphores and synchronization primitives
 * 2. NVS flash storage
 * 3. Configuration loading and validation
 * 4. System state initialization
 * 5. Global flags and state variables
 * 6. MQTT and command configuration
 * 7. GPIO and cryptography setup
 * 8. Queues and timers
 * 9. Logging and event groups
 */
void init_process(void)
{
    init_system_semaphores();
    init_nvs_flash();
    load_and_validate_config();
    update_system_state();
    init_global_state_flags();
    init_mqtt_queue_names();
    init_internal_mesh_commands();
    register_external_mqtt_commands();
    init_gpio_pins();
    init_crypto_engine();
    init_system_queues();
    calculate_permanent_delivery_time();
    init_system_timers();
    init_logging_system();
    init_event_groups();
    
    MESP_LOGI(MESH_TAG, "System initialization complete");
}

/**
 * @brief Initialize default modbus sensor configuration
 */
void init_default_modbus_config(void)
{
    modbSensors local_modbSensors = {15, 42, 1.5, 1, 0, 0, -1, 20, 1, 1, 0, 0, -1, 19, 1, 1, 0, 0, -1, 17, 1, 1, 6, 8192, 0, 16, 1, 1, 2, 8192, 0, 16};
    
    modbInverter local_modbInverter =  {10, 1, 10, 1, 2, 61530, 0, 10, 1, 1, 61518, 0, 10, 1, 1, 61517, 0, 10, 1, 2, 61528, 0, 10, 1, 1, 61526, 0, 10, 1, 1, 61527, 0, 10, 1, 2, 61522, 0, 10, 1, 2, 61520, 0, 10, 1, 1, 61518, 0, 10, 1, 1, 61517, 0};

    modbBattery local_modbBattery = {30, 3, 10, 1, 1, 276, 0, 1, 1, 1, 268, 0, 1, 1, 1, 260, 0, 1, 1, 1, 256, 0};

    modbPanels local_modbPanels = {30, 4, 10, 1, 1, 272, 0, 10, 1, 1, 271, 0, 10, 1, 1, 264, 0, 10, 1, 1, 263, 0, 1, 1, 1, 267, 0};
    
    theConf.modbus_sensors = local_modbSensors;
    theConf.modbus_inverter = local_modbInverter;
    theConf.modbus_battery = local_modbBattery;
    theConf.modbus_panels = local_modbPanels;
}

/**
 * @brief Initialize default network and MQTT configuration
 */
void init_default_network_config(void)
{
    // Default MQTT server certificate and credentials
    strncpy(theConf.mqttServer, DEFAULTMQTT, sizeof(theConf.mqttServer) - 1);
    strncpy(theConf.mqttUser, DEFAULTMQTTUSER, sizeof(theConf.mqttUser) - 1);
    strncpy(theConf.mqttPass, DEFAULTMQTTPASS, sizeof(theConf.mqttPass) - 1);
    
    // WiFi credentials
    strncpy(theConf.thessid, DEFAULT_SSID, sizeof(theConf.thessid) - 1);
    strncpy(theConf.thepass, DEFAULT_PSWD, sizeof(theConf.thepass) - 1);
    
    // Mesh security
    strncpy(theConf.kpass, DEFAULT_MESH_PASSW, sizeof(theConf.kpass) - 1);
    
    // Serial configuration
    theConf.port = (uart_port_t)ECHO_UART_PORT;
    theConf.baud = BAUD_RATE;
}

/**
 * @brief Initialize default limits configuration
 */
void init_default_limits(void)
{
    struct limits start_limits = {
        90, 50, 32, 19, 31, 19, 70, 50, 70, 40, 42, 40, 42, 40, 42, 40,
        42, 40, 42, 40, 42, 40, 820, 720, 850, 720, 820, 720, 820, 780,
        50, 10, 5000, 0, 100, 20, 80, 20, 15, 14, 390, 340
    };
    theConf.milim = start_limits;
}

/**
 * @brief Initialize default timing configuration
 */
void init_default_timing(void)
{
    theConf.loglevel = 3;
    theConf.baset = 10;
    theConf.repeat = 25;                            // Change before production
    theConf.totalnodes = EXPECTED_NODES;
    theConf.conns = EXPECTED_CONNS;
    bzero(&start_timers,sizeof(start_timers));
    bzero(&end_timers,sizeof(end_timers));
}

/**
 * @brief Reset all system configuration to factory defaults
 * 
 * Clears NVS flash, WiFi settings, and reinitializes all configuration
 * parameters to their default values. Also clears profile file.
 */
void erase_config(void)
{
    MESP_LOGI(MESH_TAG, "Erasing configuration to factory defaults");
    
    // Clear system storage
    esp_wifi_restore();
    nvs_flash_erase();
    nvs_flash_init();
    
    // Clear configuration structure
    bzero(&theConf, sizeof(theConf));
    theConf.centinel = CENTINEL;
    
    // Clear profile file
    FILE* f = fopen(PROFILE_FILE, "w");
    if (f != NULL) {
        fclose(f);
    } else {
        MESP_LOGW(MESH_TAG, "Failed to open profile file for clearing");
    }
    
    // Initialize all default configurations
    init_default_network_config();
    init_default_modbus_config();
    init_default_limits();
    init_default_timing();
    init_DO_Defaults();
    
    // Set creation timestamp
    time((time_t*)&theConf.bornDate);
    theConf.test_timer_div = 1;
    // Get and store current firmware version
    const esp_app_desc_t *app_info = esp_app_get_description();
    if (app_info != NULL) {
        strncpy(theConf.lastVersion, app_info->version, sizeof(theConf.lastVersion) - 1);
    }
    
    // Open NVS and persist configuration
    esp_err_t nvs_err = nvs_open("config", NVS_READWRITE, &nvshandle);
    if (nvs_err != ESP_OK) {
        MESP_LOGE(MESH_TAG, "Error opening NVS: %s", esp_err_to_name(nvs_err));
    }
    
    write_to_flash();
    MESP_LOGI(MESH_TAG, "Configuration reset to factory defaults");
}

// Routine to send mqtt messages
// VERY sensitive to timeouts in 2 places 
// first the WIFI network must be active in order for 
// MQTT service to be active
// now start manually the repeat timer, to avoid timer conflicts timeout timer and repeat timer
// so, this routine now controls when to fire the repeat timer again

// ! In general if an mqtt error disconnection or fatal is issued, this task is terminated and restarted again when recoonected
// ! Which means that timeouts and the other control programming is probably usless but good practice
/**
 * @brief Wait for MQTT host to become active
 * 
 * Polls hostflag with timeout warnings every 10 retries (3 seconds)
 */
void wait_for_mqtt_host(void)
{
    int whost = 0;
    while (!hostflag) {
        delay(300);
        whost++;
        if (whost > 10) {
            MESP_LOGW(MESH_TAG, "Waiting for host");
            whost = 0;
        }
    }
}

/**
 * @brief Free message resources
 * 
 * Safely frees both message buffer and parameter pointer
 * 
 * @param msg Message structure containing msg and param pointers
 */
void free_message_resources(mqttSender_t *msg)
{
    if (msg == NULL) {
        return;
    }
    
    if (msg->msg != NULL) {
        free(msg->msg);
        msg->msg = NULL;
    }
    
    if (msg->param != NULL) {
        free(msg->param);
        msg->param = NULL;
    }
}

/**
 * @brief Publish message and wait for acknowledgment
 * 
 * Attempts to publish MQTT message and wait for publication confirmation
 * with 1 second timeout
 * 
 * @param msg Message to publish
 * @return true if successfully published, false on failure
 */
bool publish_and_wait_ack(mqttSender_t *msg, uint32_t *endTime)
{
    if (msg == NULL || msg->msg == NULL) {
        return false;
    }
    
    int msgid = esp_mqtt_client_publish(clientCloud, (char*)msg->queue, 
                                        (char*)msg->msg, msg->lenMsg, QOS1, NORETAIN);
    
    if ((theConf.debug_flags >> dMQTT) & 1U) {
        MESP_LOGI(MESH_TAG, "%sPublish msgid %x len %d", DBG_MQTT, msgid, msg->lenMsg);
    }
    
    if (msgid == ESP_FAIL) {
        MESP_LOGE(MESH_TAG, "Publish failed %x", msgid);
        return false;
    }
    
    EventBits_t uxBits = xEventGroupWaitBits(wifi_event_group, PUB_BIT|DISCO_BIT|ERROR_BIT, 
                                              pdFALSE, pdFALSE, pdMS_TO_TICKS(1000));
    
    if ((uxBits & PUB_BIT) != PUB_BIT) {
        MESP_LOGE(MESH_TAG, "Pub failed Error or Disco or Timeout %x", uxBits);
        return false;
    }
    
    *endTime = xmillis();
    xEventGroupClearBits(wifi_event_group, PUB_BIT|DISCO_BIT|ERROR_BIT);
    return true;
}

/**
 * @brief Process and send message from queue
 * 
 * Handles publication, acknowledgment, callbacks, and cleanup for a single message
 * 
 * @param msg Message to process
 * @param startTime Start time for timing measurement
 * @return true if message sent successfully, false if connection failed
 */
bool process_queued_message(mqttSender_t *msg, uint32_t startTime)
{
    uint32_t endTime = 0;
    
    if (!msg->msg) {
        MESP_LOGE(MESH_TAG, "No message to send but Process activated");
        free_message_resources(msg);
        return true;
    }
    
    if (!publish_and_wait_ack(msg, &endTime)) {
        free_message_resources(msg);
        return false;
    }
    
    // Update statistics
    theBlower.setStatsMsgOut();
    theBlower.setStatsBytesOut(msg->lenMsg);
    
    // Execute callback if provided
    if (msg->code != NULL) {
        (*msg->code)((void*)msg->param);
    }
    
    if ((theConf.debug_flags >> dMQTT) & 1U) {
        MESP_LOGI(MESH_TAG, "%sMsgLen %d Msgs sent %d msec %d", DBG_MQTT, 
                 msg->lenMsg, theBlower.getStatsMsgOut(), endTime - startTime);
    }
    
    free_message_resources(msg);
    return true;
}

/**
 * @brief Send all queued messages from the MQTT sender queue
 * 
 * Processes the message queue until empty or connection fails
 * disco if true will disconnect after sending all messages -> used in Mesh Mode and NOT wifi_mesh
 * @return true if all messages sent, false if connection lost
 */
bool send_queued_messages(bool disco)
{
    mqttSender_t mensaje;
    uint32_t startTime = xmillis();
    
    while (true) {
        if (xQueueReceive(mqttSender, &mensaje, pdMS_TO_TICKS(MQTTSENDERWAIT)) == pdPASS) {
            if (!process_queued_message(&mensaje, startTime)) {
                return false;
            }
        } else {
            // Queue empty - disconnect and return success
            sendMeterf = false;
            if(disco)
            {
                esp_mqtt_client_disconnect(clientCloud);
                xEventGroupWaitBits(wifi_event_group, DISCO_BIT|ERROR_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
                esp_mqtt_client_stop(clientCloud);
            }
            if ((theConf.debug_flags >> dMQTT) & 1U) {
                MESP_LOGI(MESH_TAG, "%sConn closed and Queue empty", DBG_MQTT);
            }
            return true;
        }
    }
}

/**
 * @brief Establish MQTT connection and send messages
 * 
 * Handles MQTT client initialization, connection, and message transmission
 *  
 * @return true if session completed successfully
 */
bool mqtt_send_session(void)
{
    EventBits_t uxBits;
    
    if (!clientCloud) {
        clientCloud = root_setupMqtt();
        if(theConf.mesh_wifi)
            esp_mqtt_client_start(clientCloud);
    }
    
    if (clientCloud == NULL) {
        MESP_LOGE(MESH_TAG, "Cannot connect to ClientCloud");
        sendMeterf = false;
        return true;
    }
    
    if(theConf.mesh_wifi)       // in mesh mode, if wifi_mesh is used, connection is always on
    {
        // not using connection sharing, but direct connection to mqtt host
        if (!send_queued_messages(false))
        {
            MESP_LOGE(MESH_TAG, "Connection lost during sending mesh wifi");
            if (xTimerStart(collectTimer, 0) != pdPASS)                 // start the timer again.... could be setup to be repeating but disconnecting uses manual so do the same
                MESP_LOGE(MESH_TAG, "Repeat Timer mqtt sender mesh_wifi failed");
            return false;
        }

        if (xTimerStart(collectTimer, 0) != pdPASS)
            MESP_LOGE(MESH_TAG, "Repeat Timer mqtt sender mesh_wifi 2 failed");   // stat the timer again.... could be setup to be repeating but disconnecting uses manual so do the same
        return true;
    }

    xEventGroupClearBits(wifi_event_group, MQTT_BIT|ERROR_BIT|DISCO_BIT);
    int err = esp_mqtt_client_start(clientCloud);
    
    uxBits = xEventGroupWaitBits(wifi_event_group, MQTT_BIT|DISCO_BIT|ERROR_BIT, 
                                 pdFALSE, pdFALSE, pdMS_TO_TICKS(40000));
    
    if (((uxBits & MQTT_BIT) == MQTT_BIT) && err != ESP_FAIL) {
        if ((theConf.debug_flags >> dMQTT) & 1U) {
            MESP_LOGI(MESH_TAG, "%sMqtt Sender connected", DBG_MQTT);
        }
        
        if (!send_queued_messages(true)) {
            // Connection failed during sending
            esp_mqtt_client_disconnect(clientCloud);
            esp_mqtt_client_stop(clientCloud);
            xQueueReset(mqttSender);
            // clear_queued_messages();
        }
    } else {
        // Failed to connect
        MESP_LOGE(MESH_TAG, "Did not get connect bit or error %x", uxBits);
        sendMeterf = false;
        esp_mqtt_client_disconnect(clientCloud);
        esp_mqtt_client_stop(clientCloud);
        xQueueReset(mqttSender);
        // clear_queued_messages();
    }
    
    if (xTimerStart(collectTimer, 0) != pdPASS) {
        MESP_LOGE(MESH_TAG, "Repeat Timer mqtt sender failed");
    }
    
    return true;
}

/**
 * @brief MQTT sender task
 * 
 * Continuously monitors MQTT sender queue and transmits messages when host is available.
 * Manages connection lifecycle and message cleanup on failures.
 * 
 * @param pArg Task parameter (unused)
 */
void root_mqtt_sender(void *pArg)
{
    xEventGroupClearBits(wifi_event_group, SENDMQTT_BIT);
    
    while (true) {
        wait_for_mqtt_host();
        
        xEventGroupWaitBits(wifi_event_group, SENDMQTT_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
        
        uint32_t startMqtt = xmillis();
        if ((theConf.debug_flags >> dMQTT) & 1U) {
            MESP_LOGI(MESH_TAG, "%sMqttsender establish", DBG_MQTT);
        }
        
        mqtt_send_session();
        
        delay(100);
    }
}


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
//         MESP_LOGE(MESH_TAG,"Repeat Timer failed");
//  }

/**
 * @brief Calculate current meter collection cycle
 * 
 * Determines the current cycle based on time elapsed since midnight
 * and the number of nodes/connections configured
 * 
 * @param now Current time
 * @param cycles Total number of cycles per day
 * @return Current cycle index
 */
int calculate_current_cycle(time_t now, int cycles)
{
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    // Calculate midnight time
    timeinfo.tm_hour = 0;
    timeinfo.tm_min = 0;
    timeinfo.tm_sec = 0;
    time_t midnight = mktime(&timeinfo);
    
    int secs_from_midnight = (int)(now - midnight);
    int currCycle = (secs_from_midnight / EXPECTED_DELIVERY_TIME) % cycles;
    
    return currCycle;
}

/**
 * @brief Send emergency message and restart if critical operation fails
 * 
 * Used when timer initialization fails - attempts to send emergency
 * notification through MQTT queue before restarting system
 * 
 * @param context Context string describing the failure
 */
void send_emergency_and_restart(const char *context)
{
    mqttSender_t emergencyMsg;
    
    MESP_LOGE(MESH_TAG, "%s: FATAL - attempting emergency restart", context);
    
    char *mensa = (char*)calloc(1, 200);
    if (mensa == NULL) {
        MESP_LOGE(MESH_TAG, "%s: No RAM for emergency message", context);
        esp_restart();
        return;
    }
    
    sprintf(mensa, "%s failed Node %d", context, theConf.poolid);
    emergencyMsg.msg = mensa;
    emergencyMsg.lenMsg = strlen(mensa);
    
    if (xQueueSend(mqtt911, &emergencyMsg, 0) != pdPASS) {
        MESP_LOGE(MESH_TAG, "Cannot send emergency message: %s", context);
        writeLog(mensa);
        free(mensa);
        delay(1000);
    }
    
    esp_restart();
}

/**
 * @brief Create and start the meter data collection timer
 * 
 * Initializes the primary timer for periodic meter data collection.
 * Timer fires once at start of collection cycle, then schedules
 * the repeat timer for subsequent collections.
 * 
 * @return true if timer successfully created and started, false otherwise
 */
bool create_and_start_collection_timer(void)
{
    firstTimer = xTimerCreate(
        "Timer",
        pdMS_TO_TICKS(100 * theConf.baset),
        pdFALSE,
        (void*)10,
        [](TimerHandle_t xTimer) {
            // First timer called - we are now in our alloted time cycle
            if (!theConf.meterconf) {
                return;
            }
            
            root_collect_meter_data(NULL);
            if (xTimerStart(collectTimer, 0) != pdPASS) {
                MESP_LOGE(MESH_TAG, "Repeat Timer failed");
            }
        }
    );
    
    if (firstTimer == NULL) {
        MESP_LOGE(MESH_TAG, "Failed to create first timer");
        return false;
    }
    
    if (xTimerStart(firstTimer, 0) != pdPASS) {
        send_emergency_and_restart("First Timer");
        return false;
    }
    
    return true;
}

/**
 * @brief Set up root node data collection timer
 * 
 * Calculates the appropriate time slot for this node's meter data
 * transmission based on controller ID and network configuration.
 * Creates and starts the collection timer if this is the root node.
 * 
 * Timer scheduling: Divides the day into cycles based on number of
 * nodes and connections. Each node gets a defined time slot within
 * the EXPECTED_DELIVERY_TIME window.
 */
void root_set_senddata_timer()
{
    time_t now;
    struct tm timeinfo;
    
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // Ensure baset has a minimum value
    if (theConf.baset < 1) {
        theConf.baset = 10;
    }
    
    int cycles = theConf.totalnodes / theConf.conns;
    if (cycles < 1) {
        cycles = 1;
    }
    
    MESP_LOGI(MESH_TAG, "Setting senddata timer: cycles=%d, baset=%d", cycles, theConf.baset);
    
    if (esp_mesh_is_root()) {
        calculate_current_cycle(now, cycles);
        
        if (!create_and_start_collection_timer()) {
            MESP_LOGE(MESH_TAG, "Failed to create collection timer");
        }
    }
}

void post_root()
{
    xTaskCreate(&root_sntpget,"sntp",4096,NULL, 10, NULL); 	        // get real time
}

/**
 * @brief Initialize root node after IP acquisition
 * 
 * Sets up MQTT, emergency task, blink indicator, and login timer
 */
void init_root_node(void)
{
    loadedf = true;
    meshf = true;
    root_mqtt_app_start();
    post_root();
    xTaskCreate(&root_emergencyTask, "e911", 2048, NULL, 5, NULL);
    xTaskCreate(&blinkRoot, "root", 1024, (void*)400, 5, &blinkHandle);
    
    if (theConf.totalnodes > 0) {
        xTimerStart(loginTimer, 0);
    }
    
    launch_sensors();
    MESP_LOGI(MESH_TAG, "Root node initialized with %d total nodes", theConf.totalnodes);
}

/**
 * @brief Initialize child node after IP acquisition
 * 
 * Launches sensor monitoring for child node
 */
void init_child_node(void)
{
    launch_sensors();
    MESP_LOGI(MESH_TAG, "Child node initialized with sensors launched");
}

/**
 * @brief Handle IP address acquisition event
 * 
 * Updates current IP, initializes root or child node, and starts mesh send task
 * 
 * @param event IP event data containing IP information
 */
void handle_ip_got_ip(ip_event_got_ip_t *event)
{
    if (event == NULL) {
        return;
    }
    
    MESP_LOGI(MESH_TAG, "<IP_EVENT_STA_GOT_IP>IP:" IPSTR, IP2STR(&event->ip_info.ip));
    s_current_ip.addr = event->ip_info.ip.addr;
    theBlower.setStatsStaConns();
    
    // Get DNS configuration
    esp_netif_t *netif = event->esp_netif;
    esp_netif_dns_info_t dns;
    ESP_ERROR_CHECK(esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns));
    mesh_netif_start_root_ap(esp_mesh_is_root(), dns.ip.u_addr.ip4.addr);
    
    hostflag = true;
    gpio_set_level((gpio_num_t)WIFILED, 1);
    
    // Initialize appropriate node type
    if (esp_mesh_is_root() && !loadedf) {
        init_root_node();
    } else {
        init_child_node();
    }
    
    // Start mesh send task to process queued messages
    xTaskCreate(&meshSendTask, "msend", 1024*5, NULL, 10, NULL);
}

/**
 * @brief Handle IP address lost event
 * 
 * Disables indicators and logs the disconnection
 */
void handle_ip_lost_ip(void)
{
    MESP_LOGI(MESH_TAG, "Lost IP address");
    hostflag = false;
    gpio_set_level((gpio_num_t)WIFILED, 0);
    
    char *tmp = (char*)calloc(1, 100);
    if (tmp != NULL) {
        sprintf(tmp, "Lost ip as %s", esp_mesh_is_root() ? "Root" : "Node");
        writeLog(tmp);
        free(tmp);
    }
}

/**
 * @brief IP event handler for WiFi IP events
 * 
 * Processes IP acquisition and loss events for proper network initialization
 * and cleanup
 * 
 * @param arg Context argument (unused)
 * @param event_base Event base identifier
 * @param event_id IP event ID
 * @param event_data Event data (type depends on event_id)
 */
void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_id == IP_EVENT_STA_GOT_IP) {
        handle_ip_got_ip((ip_event_got_ip_t*)event_data);
    } else if (event_id == IP_EVENT_STA_LOST_IP) {
        handle_ip_lost_ip();
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

/**
 * @brief Handle WiFi AP station connection event
 * 
 * Starts SSID blink task to indicate active station connection
 */
void handle_sta_connected(void)
{
    xTaskCreate(&blinkSSID, "bssid", 4096, (void*)SSIDBLINKTIME, 5, &ssidHandle);
    MESP_LOGI(MESH_TAG, "Station connected to AP, starting SSID blink");
}

/**
 * @brief Handle WiFi AP station disconnection event
 * 
 * Stops SSID blink task and cleans up task handle
 */
void handle_sta_disconnected(void)
{
    if (ssidHandle != NULL) {
        gpio_set_level((gpio_num_t)BEATPIN, 1);
        vTaskDelete(ssidHandle);
        ssidHandle = NULL;
        MESP_LOGI(MESH_TAG, "Station disconnected from AP, stopped SSID blink");
    }
}

/**
 * @brief Handle WiFi STA start event
 * 
 * Initiates WiFi connection to mesh network
 */
void handle_sta_start(void)
{
    esp_wifi_connect();
    MESP_LOGI(MESH_TAG, "WiFi STA started, connecting to network");
}

/**
 * @brief WiFi event handler for AP mode events
 * 
 * Processes station connection/disconnection and STA start events
 * 
 * @param arg Context argument (unused)
 * @param event_base Event base identifier
 * @param event_id WiFi event ID
 * @param event_data Event data (type depends on event_id)
 */
void wifi_event_handler_ap(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        handle_sta_connected();
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        handle_sta_disconnected();
    } else if (event_id == WIFI_EVENT_STA_START) {
        handle_sta_start();
    }
}
/**
 * @brief Generate AP SSID from MAC address
 * 
 * Creates SSID in format: APPNAME + last 3 bytes of MAC in hex
 */
void generate_ap_ssid(void)
{
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_AP, mac);
    
    bzero(apssid, sizeof(apssid));
    sprintf(apssid, "%s%02X%02X%02X", APPNAME, mac[3], mac[4], mac[5]);
}

/**
 * @brief Setup WiFi AP SSID and password
 */
void setup_ap_credentials(void)
{
    generate_ap_ssid();
    strcpy(appsw, DEFAULT_MESH_PASSW);
    MESP_LOGI(TAG, "AP configured - SSID: %s", apssid);
}

/**
 * @brief Configure WiFi AP settings
 * 
 * @param wifi_config Config structure to populate
 */
void setup_wifi_ap_config(wifi_config_t *wifi_config)
{
    if (!wifi_config) {
        return;
    }
    
    bzero(wifi_config, sizeof(wifi_config_t));
    
    memcpy(&wifi_config->ap.ssid, apssid, strlen(apssid));
    memcpy(&wifi_config->ap.password, appsw, strlen(appsw));
    wifi_config->ap.ssid_len = strlen(apssid);
    wifi_config->ap.channel = 4;
    wifi_config->ap.max_connection = 4;
    wifi_config->ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
}

/**
 * @brief Initialize network interfaces and WiFi for AP mode
 * 
 * Sets up netif, event handlers, and WiFi driver in access point mode.
 * Generates SSID from MAC address and launches web server for configuration.
 */
void wifi_init_network(void)
{
    // Initialize network interfaces
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();
    
    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // Register event handler
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler_ap, NULL, NULL));
    
    // Configure WiFi parameters
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    
    // Setup AP credentials
    setup_ap_credentials();
    
    // Configure AP settings
    wifi_config_t wifi_config;
    setup_wifi_ap_config(&wifi_config);
    
    // Start WiFi
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Start web server for configuration
    xTaskCreate(&start_webserver, "webs", 1024 * 10, NULL, 5, NULL);
    MESP_LOGI(TAG, "WiFi AP initialized and web server started");
}
// WiFi STA connection state variables
bool s_wifi_sta_connected = false;
int s_wifi_sta_retry_num = 0;
#define WIFI_STA_MAXIMUM_RETRY  5

/**
 * @brief WiFi event handler for external AP connection (STA mode)
 * @param arg User data registered to the event
 * @param event_base Event base for the handler
 * @param event_id Event ID
 * @param event_data Event data
 * 
 * Handles WiFi station events including:
 * - WIFI_EVENT_STA_START: Initiates connection to external AP
 * - WIFI_EVENT_STA_DISCONNECTED: Handles disconnection and retry logic
 */
void wifi_sta_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        MESP_LOGI(MESH_TAG, "WiFi STA started, attempting to connect to external AP");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_wifi_sta_retry_num < WIFI_STA_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_wifi_sta_retry_num++;
            MESP_LOGW(MESH_TAG, "Retry connecting to external AP (attempt %d/%d)", 
                     s_wifi_sta_retry_num, WIFI_STA_MAXIMUM_RETRY);
        }
        else
        {
            MESP_LOGE(MESH_TAG, "Failed to connect to external AP after %d attempts", 
                     WIFI_STA_MAXIMUM_RETRY);
        }
        s_wifi_sta_connected = false;
    }
}

/**
 * @brief IP event handler for external AP connection (STA mode)
 * @param arg User data registered to the event
 * @param event_base Event base for the handler
 * @param event_id Event ID
 * @param event_data Event data
 * 
 * Handles IP events:
 * - IP_EVENT_STA_GOT_IP: Successfully obtained IP address from external AP
 */
void ip_sta_got_ip_handler(void* arg, esp_event_base_t event_base,
                                   int32_t event_id, void* event_data)
{
    if (event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        MESP_LOGI(MESH_TAG, "Got IP from external AP: " IPSTR, IP2STR(&event->ip_info.ip));
        MESP_LOGI(MESH_TAG, "Gateway: " IPSTR, IP2STR(&event->ip_info.gw));
        MESP_LOGI(MESH_TAG, "Netmask: " IPSTR, IP2STR(&event->ip_info.netmask));
        s_wifi_sta_retry_num = 0;
        s_wifi_sta_connected = true;
        post_root();
        xTaskCreate(&root_emergencyTask,"e911",2048,NULL, 5, NULL);
        xTaskCreate(&blinkRoot,"root",1024,(void*)400, 5, &blinkHandle);
        launch_sensors();
        if( xTaskCreate(&root_mqttMgr,"mqtt",1024*10,NULL, 10, &mqttMgrHandle)!=pdPASS)      //receiving commands
            MESP_LOGE(MESH_TAG,"Fail to launch Mgr Wifi");
        clientCloud=root_setupMqtt();
        if(clientCloud)
            esp_mqtt_client_start(clientCloud);   // start and keep it open
        else
            MESP_LOGE(TAG,"Could not start MQTT client" );
        if( xTimerStart(collectTimer, 0 ) != pdPASS )         // data collection timer
            MESP_LOGE(MESH_TAG,"WIFI Collect Timer failed");
    }
}

/**
 * @brief Initialize and connect to external WiFi AP in STA mode
 * 
 * Configures WiFi in STA mode and connects to the external access point
 * using credentials from theConf.thessid and theConf.thepass.
 * 
 * Registers event handlers for:
 * - WiFi connection/disconnection events
 * - IP address assignment (got IP event)
 * 
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t wifi_connect_external_ap(void)
{
    esp_err_t ret;
    
    // Validate SSID and password
    if (strlen(theConf.thessid) == 0)
    {
        MESP_LOGE(MESH_TAG, "WiFi SSID is empty, cannot connect to external AP");
        return ESP_FAIL;
    }
    
    // Initialize network interface if not already done
    ret = esp_netif_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        MESP_LOGE(MESH_TAG, "Failed to initialize netif: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    
    // Create default event loop if not already created
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        MESP_LOGE(MESH_TAG, "Failed to create event loop: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    
    // Create default WiFi station interface
    esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();
    if (sta_netif == NULL)
    {
        MESP_LOGE(MESH_TAG, "Failed to create WiFi STA interface");
        return ESP_FAIL;
    }
    
    // Create default WiFi AP interface
    esp_netif_t* ap_netif = esp_netif_create_default_wifi_ap();
    if (ap_netif == NULL)
    {
        MESP_LOGE(MESH_TAG, "Failed to create WiFi AP interface");
        return ESP_FAIL;
    }
    
    // Initialize WiFi with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        MESP_LOGE(MESH_TAG, "Failed to initialize WiFi: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    
    // Register WiFi event handler
    ret = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_sta_event_handler, NULL);
    if (ret != ESP_OK)
    {
        MESP_LOGE(MESH_TAG, "Failed to register WiFi event handler: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    
    // Register IP event handler for GOT_IP event
    ret = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_sta_got_ip_handler, NULL);
    if (ret != ESP_OK)
    {
        MESP_LOGE(MESH_TAG, "Failed to register IP event handler: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    
    // Configure WiFi station with SSID and password from configuration
    wifi_config_t sta_config = {};
    memset(&sta_config, 0, sizeof(wifi_config_t));
    
    strncpy((char*)sta_config.sta.ssid, theConf.thessid, sizeof(sta_config.sta.ssid) - 1);
    strncpy((char*)sta_config.sta.password, theConf.thepass, sizeof(sta_config.sta.password) - 1);
    sta_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    sta_config.sta.pmf_cfg.capable = true;
    sta_config.sta.pmf_cfg.required = false;
    
    // Configure WiFi AP with SSID "SHRIMP" and password "csttpstt"
    wifi_config_t ap_config = {};
    memset(&ap_config, 0, sizeof(wifi_config_t));
    
    strncpy((char*)ap_config.ap.ssid, "shrimp", sizeof(ap_config.ap.ssid) - 1);
    strncpy((char*)ap_config.ap.password, "csttpstt", sizeof(ap_config.ap.password) - 1);
    ap_config.ap.ssid_len = strlen("shrimp");
    ap_config.ap.channel = 0;
    ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    ap_config.ap.max_connection = 4;
    
    // Set WiFi mode to APSTA (AP + Station)
    ret = esp_wifi_set_mode(WIFI_MODE_APSTA);
    if (ret != ESP_OK)
    {
        MESP_LOGE(MESH_TAG, "Failed to set WiFi mode to APSTA: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    
    // Set WiFi STA configuration
    ret = esp_wifi_set_config(WIFI_IF_STA, &sta_config);
    if (ret != ESP_OK)
    {
        MESP_LOGE(MESH_TAG, "Failed to set WiFi STA config: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    
    // Set WiFi AP configuration
    ret = esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    if (ret != ESP_OK)
    {
        MESP_LOGE(MESH_TAG, "Failed to set WiFi AP config: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    
    // Start WiFi
    ret = esp_wifi_start();
    if (ret != ESP_OK)
    {
        MESP_LOGE(MESH_TAG, "Failed to start WiFi: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    
    MESP_LOGI(MESH_TAG, "WiFi APSTA initialization complete. AP SSID: SHRIMP, STA connecting to: %s", theConf.thessid);
    return ESP_OK;
}

/**
 * @brief Generate and store configuration ID for new meter
 * 
 * Creates random CID and displays SSID with CID for user information.
 */
void generate_meter_cid(void)
{
    int cid = esp_random() % 99999999;
    char tmp[50];
    
    sprintf(tmp, "%s %d", apssid, cid);
    MESP_LOGI(TAG, "Generated CID: %s", tmp);
    showLVGL(tmp, 600000, 1);
    
    theConf.cid = cid;
    write_to_flash();
}

/**
 * @brief Display configuration status on LVGL display
 */
void display_config_status(void)
{
    showLVGL((char*)"CONF", 600000, 1);
}

/**
 * @brief Wait for webserver configuration completion or timeout
 * 
 * Blocks indefinitely for webserver to complete configuration.
 */
void wait_for_webserver_config(void)
{
    while (true) {
        delay(1000);  // Webserver will restart after configuration
    }
}

/**
 * @brief Initialize meter and wait for configuration via web interface
 * 
 * Sets up WiFi in AP mode and waits for configuration. If meter is new
 * (meterconf==0), generates and displays a unique CID. Displays status
 * on LVGL display and blocks until webserver completes configuration.
 */
void meter_configure(void)
{
    wifi_init_network();
    
    if (theConf.meterconf == 0) {
        // New meter: generate CID
        generate_meter_cid();
    } else {
        // Already configured: show status
        display_config_status();
    }
    
    // Block until webserver completes configuration
    wait_for_webserver_config();
}

/**
 * @brief Initialize WiFi stack for mesh networking
 * 
 * Sets up network interfaces, WiFi drivers, and registers event handlers.
 */
void init_wifi_stack(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(mesh_netifs_init(mesh_manager));
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, 
                                               &ip_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP, 
                                               &ip_event_handler, NULL));
    
    if (!theConf.masternode) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR));
    }
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_LR));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_start());
}

/**
 * @brief Load or initialize mesh SSID and password credentials
 * 
 * @param ssid Buffer to store SSID (min 30 bytes)
 * @param password Buffer to store password (min 18 bytes)
 */
void load_mesh_credentials(char *ssid, char *password)
{
    if (!ssid || !password) {
        return;
    }
    
    if (strlen(theConf.thessid) == 0) {
        // Initialize with defaults
        strcpy(ssid, DEFAULT_MESH_SSID);
        strcpy(password, DEFAULT_MESH_PASSW);
        strcpy(theConf.thessid, DEFAULT_MESH_SSID);
        strcpy(theConf.thepass, DEFAULT_MESH_PASSW);
        write_to_flash();
    } else {
        // Use existing configuration
        strcpy(ssid, theConf.thessid);
        strcpy(password, theConf.thepass);
    }
}

/**
 * @brief Initialize mesh configuration structure
 * 
 * @param cfg Mesh config structure to populate
 * @param ssid Router SSID
 * @param password Router password
 */
void setup_mesh_config(mesh_cfg_t *cfg, const char *ssid, const char *password)
{
    if (!cfg || !ssid || !password) {
        return;
    }
    
    *cfg = MESH_INIT_CONFIG_DEFAULT();
    memcpy((uint8_t *)&cfg->mesh_id, MESH_ID, 6);
    
    cfg->channel = 0;  // Required: all nodes must use same channel
    cfg->router.ssid_len = strlen(ssid);
    memcpy((uint8_t *)&cfg->router.ssid, ssid, cfg->router.ssid_len);
    memcpy((uint8_t *)&cfg->router.password, password, strlen(password));
    
    // Soft AP credentials
    ESP_ERROR_CHECK(esp_mesh_set_ap_authmode((wifi_auth_mode_t)CONFIG_MESH_AP_AUTHMODE));
    cfg->mesh_ap.max_connection = CONFIG_MESH_AP_CONNECTIONS;
    cfg->mesh_ap.nonmesh_max_connection = CONFIG_MESH_NON_MESH_AP_CONNECTIONS;
    memcpy((uint8_t *)&cfg->mesh_ap.password, DEFAULT_MESH_PASSW, 8);
}

/**
 * @brief Initialize core mesh parameters and event handlers
 */
void init_mesh_parameters(void)
{
    ESP_ERROR_CHECK(esp_mesh_init());
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, 
                                               &mesh_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mesh_set_max_layer(CONFIG_MESH_MAX_LAYER));
    ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(1));
    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(10));
    ESP_ERROR_CHECK(esp_mesh_send_block_time(30000));
    ESP_ERROR_CHECK(esp_mesh_disable_ps());
}

/**
 * @brief Apply mesh configuration and handle errors
 * 
 * @param cfg Mesh configuration to apply
 * @return true if successful, false if fatal error
 */
bool apply_mesh_config(const mesh_cfg_t *cfg)
{
    if (!cfg) {
        return false;
    }
    
    esp_err_t err = esp_mesh_set_config(cfg);
    if (err != ESP_OK) {
        MESP_LOGE(MESH_TAG, "Init Mesh err %x", err);
        
        if (err == 0x4008) {
            MESP_LOGE(MESH_TAG, "Password length error");
            erase_config();
            esp_restart();
        }
        return false;
    }
    
    return true;
}

/**
 * @brief Setup mesh topology and broadcast addressing
 */
void setup_mesh_topology(void)
{
    esp_err_t err = esp_mesh_set_group_id(&GroupID, 1);
    if (err != ESP_OK) {
        MESP_LOGI(MESH_TAG, "Failed to register Mesh Broadcast");
    }
    
    ESP_ERROR_CHECK(esp_mesh_set_topology(MESH_TOPO_CHAIN));
    ESP_ERROR_CHECK(esp_mesh_set_ie_crypto_funcs(NULL));
}

/**
 * @brief Start mesh networking and finalize initialization
 */
void start_mesh(void)
{
    char ssid[30], password[18];
    
    // Initialize WiFi stack
    init_wifi_stack();
    
    // Load or initialize mesh credentials
    load_mesh_credentials(ssid, password);
    
    // Initialize mesh parameters
    init_mesh_parameters();
    
    // Setup and apply mesh configuration
    mesh_cfg_t cfg;
    setup_mesh_config(&cfg, ssid, password);
    
    esp_log_buffer_hex("MESHID", &cfg.mesh_id, 6);
    
    if (!apply_mesh_config(&cfg)) {
        return;
    }
    
    // Setup topology and broadcast
    setup_mesh_topology();
    
    // Start mesh
    ESP_ERROR_CHECK(esp_mesh_start());
    
    mesh_started = true;
    MESP_LOGI(MESH_TAG, "Mesh started successfully. Heap: %d bytes, Root: %s",
             esp_get_free_heap_size(),
             esp_mesh_is_root_fixed() ? "fixed" : "not fixed");
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

/**
 * @brief Validate active profile and day cycle configuration
 * 
 * @return true if configuration is valid, false otherwise
 */
bool is_profile_config_valid(void)
{
    if (theConf.activeProfile < 0 || theConf.dayCycle < 0 || theConf.activeProfile >= MAXPROFILES) {
        MESP_LOGE(TAG, "Invalid Profile %d or Day Start %d", theConf.activeProfile, theConf.dayCycle);
        return false;
    }
    return true;
}

/**
 * @brief Find active cycle and day within profile for recovery from power loss
 * 
 * Calculates which cycle and day within that cycle to resume based on
 * theConf.dayCycle counter. Used for resuming production after reboot.
 * 
 * @param ciclo Output: cycle number to start (0-indexed)
 * @param dia Output: day within cycle to start (0-indexed)
 */
void find_cycle_day(uint8_t *ciclo, uint8_t *dia)
{
    if (!ciclo || !dia) {
        MESP_LOGE(TAG, "Invalid output pointers");
        return;
    }
    // the initial profile and cycle is sent by the cmdProd mqtt command with these values
    // in theConf.activeProdile and theConf.dayCycle

    if (theConf.debug_flags & (1U << dSCH)) {
        MESP_LOGI(TAG, "%sFinding cycle for Profile %d Day %d", DBG_SCH,
                theConf.activeProfile, theConf.dayCycle);
    }
    
    if (!is_profile_config_valid()) {
        *ciclo = 0;
        *dia = 0;
        return;
    }
    
    // Search for cycle containing the target day
    const profile_t *profile = &theConf.profiles[theConf.activeProfile];
    for (int i = 0; i < profile->numCycles; i++) {
        const ciclo_t *cycle = &profile->cycle[i];
        int cycle_end_day = cycle->day + cycle->duration;
        
        if (cycle_end_day > theConf.dayCycle) {
            // Found containing cycle
            *dia = theConf.dayCycle - (cycle->day - 1);
            *ciclo = i;
            
            if (theConf.debug_flags & (1U << dSCH)) {
                MESP_LOGI(TAG, "%sFound Cycle %d Day %d",DBG_SCH, *ciclo, *dia);
            }
            return;
        }
    }
    
    // Day not found in any cycle, reset to start
    *ciclo = 0;
    *dia = 0;
    MESP_LOGW(TAG, "Day %d not found in profile, resetting to start", theConf.dayCycle);
}


/**
 * @brief Stop the send meter watchdog timer
 * 
 * @return true if stopped successfully, false otherwise
 */
bool stop_send_meter_timer(void)
{
    return (xTimerStop(sendMeterTimer, 0) == pdPASS);
}

/**
 * @brief Control blower hardware state
 * 
 * Sets GPIO level to turn blower on or off.
 * 
 * @param onoff true to turn on, false to turn off
 */
void turn_blower_onOff(bool onoff)
{
     if ((theConf.debug_flags >> dBLOW) & 1U) {
        MESP_LOGI(TAG, "%sBlower turned %s",DBG_BLOW, onoff ? "ON" : "OFF");
     }
    
    if(onoff)
    {
        gpio_set_level((gpio_num_t)BEATPIN, 0);
        theBlower.setScheduleStatus(BLOWERON);
    }
    else
    {
        gpio_set_level((gpio_num_t)BEATPIN, 1);
        theBlower.setScheduleStatus(BLOWEROFF);
    }
}

/**
 * @brief Check mesh node schedule and trigger data collection when complete
 * 
 * Processes routing table entries from mesh nodes. When all expected nodes
 * have reported, sends collected data via MQTT and stops the watchdog timer.
 * 
 * @param who Mesh address of reporting node
 * @param ladata Node data from routing table
 * @return ESP_OK if processed or waiting, ESP_FAIL if routing table load failed
 */
esp_err_t root_check_schedule(mesh_addr_t *who, void* ladata)
{
    if (root_load_routing_table_mac(who, ladata) != ESP_OK) {
        return ESP_FAIL;
    }
    
    counting_nodes--;
    
    // All nodes reported, send collected data
    if (counting_nodes == 0) {
        if (!stop_send_meter_timer()) {
            MESP_LOGE(MESH_TAG, "Failed to stop SendMeter Timer");
        }
        
        esp_err_t err = root_send_collected_nodes(masterNode.existing_nodes);
        if (err != ESP_OK) {
            MESP_LOGE(MESH_TAG, "Error sending mqtt msg");
            return ESP_FAIL;
        }
    }
    
    return ESP_OK;
}
/**
 * @brief Create timer context for schedule execution
 * 
 * @param cycle Cycle number
 * @param day Day number within cycle
 * @param horario Hour index
 * @param timer_num Timer array index
 * @return Allocated context or NULL on failure
 */
start_timer_ctx_t* create_timer_context(uint8_t cycle, uint8_t day, int horario, int timer_num)
{
    start_timer_ctx_t* ctx = (start_timer_ctx_t*)calloc(1, sizeof(start_timer_ctx_t));
    if (!ctx) {
        MESP_LOGE(MESH_TAG, "No ram for start timer context");
        return NULL;
    }
    
    ctx->cycle = cycle;
    ctx->day = day;
    ctx->horario = horario;
    ctx->tostart = theConf.profiles[0].cycle[cycle].horarios[horario].hourStart;
    ctx->horaslen = theConf.profiles[0].cycle[cycle].horarios[horario].horarioLen;
    ctx->pwm = theConf.profiles[0].cycle[cycle].horarios[horario].pwmDuty;
    ctx->timerNum = timer_num;
    
    return ctx;
}

/**
 * @brief Handle schedule that started in the past but hasn't ended
 * 
 * Creates immediate end timer for currently running schedule.
 * 
 * @param endtime Schedule end timestamp
 * @param now Current timestamp
 * @param ck_h Hour index
 * @param num_horarios Total horarios in cycle
 */
void handle_past_schedule_in_progress(start_timer_ctx_t * ctx,time_t endtime, time_t now, int ck
        ,int ck_day,int ck_h, int num_horarios)
{
    timer_t remaining = ((endtime - now) * 1000) / theConf.test_timer_div ;
    ctx->isLast=(ck_h == num_horarios - 1)? true : false;
    
    turn_blower_onOff(true);  // Ensure blower is on
    if ((theConf.debug_flags >> dSCH) & 1U) {
        char time_str[30],time_str2[30];
        format_log_time(endtime, time_str, 30);
        format_log_time(now, time_str2, 30);
        // MESP_LOGI(TAG, "%sScheduling Ending %d in %lld ms(%s | %lld) now %s - %lld Last %s", 
        //          DBG_SCH,countTimersEnd, remaining , time_str, endtime, time_str2, now,ctx->isLast ? "YES" : "NO");
    }  
    elapsed[countTimersEnd] = time(NULL);     // consider now as start time since its started manually. countimersstart is 1 ahead so use countendtimers
    if(remaining<=0)
    {
        MESP_LOGE(TAG, "Remaining time invalid past %lld", remaining);
        return ;
    }
    ctx->timerNum=countTimersEnd;
    uint32_t finaltime=remaining;

    end_timers[countTimersEnd] = xTimerCreate("PEND", pdMS_TO_TICKS(finaltime ), 
                                            pdFALSE, (void*)ctx, blower_end);
    if (end_timers[countTimersEnd]!=NULL) {
        xTimerStart(end_timers[countTimersEnd], 10);
        countTimersEnd++;
        countTimersStart++;
        // it was started above, so set the schedule in blower
        theBlower.setSchedule(ck,ck_day,ck_h,theConf.profiles[0].cycle[ck].horarios[ck_h].hourStart,theConf.profiles[0].cycle[ck].horarios[ck_h].horarioLen,theConf.profiles[0].cycle[ck].horarios[ck_h].pwmDuty,BLOWERON); // clear any previous schedule in blower
    }
    else
    {
        MESP_LOGE(MESH_TAG, "Past End Timer not created %d", countTimersEnd);
        free(ctx);
    }   
}

/**
 * @brief Validate and apply test timer multiplier
 * 
 * @param delay_ms Calculated delay in milliseconds
 * @return true if delay is valid, false if timer div too large
 */
bool validate_timer_delay(time_t delay_ms)
{
    if (delay_ms < 10) {
        MESP_LOGE(TAG, "MUX too big %d-%d", theConf.test_timer_div,delay_ms);
        return false;
    }
    return true;
}

/**
 * @brief Create and start schedule timers for future event
 * 
 * @param starttime Schedule start timestamp
 * @param endtime Schedule end timestamp  
 * @param now Current timestamp
 * @param ctx Timer context
 * @param ck_h Hour index
 * @param num_horarios Total horarios
 * @return true on success, false if validation failed
 */
bool create_future_timers(time_t starttime, time_t endtime, time_t now,
                                 start_timer_ctx_t* ctx, int ck_h, int num_horarios)
{
    char time_str[30],time_str2[30];

    if(starttime<=now || endtime<=now)
    {
        MESP_LOGE(MESH_TAG, "FATAL create_future_timers called with past times Start %llu Now %llu End %llu", starttime, now, endtime);
        return false;
    }

    timer_t start_delay = ((starttime - now) / theConf.test_timer_div) * 1000 ;
    timer_t end_delay = ((endtime - now) / theConf.test_timer_div) * 1000 ;
    
    if (!validate_timer_delay(start_delay) || !validate_timer_delay(end_delay)) {
        free(ctx);
        return false;
    }
    int delay_int =start_delay;
    // Create start timer
    if ((theConf.debug_flags >> dSCH) & 1U) {
        format_log_time(starttime,time_str,30);
        format_log_time(now,time_str2,30);
        MESP_LOGI(TAG, "%sScheduling timer %d Start in %d ms [ %s | %llu ] now [ %s | %llu ] Mux %ld", 
                 DBG_SCH, ctx->timerNum, delay_int, time_str,starttime, time_str2,now,theConf.test_timer_div);
                //  DBG_SCH, countTimersStart, delay_int, time_str,starttime, time_str2,now,theConf.test_timer_div);
    }
    
    start_timers[countTimersStart] = xTimerCreate("TSTART", pdMS_TO_TICKS(delay_int),
                                                pdFALSE, (void*)ctx, blower_start);
    if (start_timers[countTimersStart]==NULL) {
        MESP_LOGE(MESH_TAG, "Start Timer not created %d", countTimersStart);
        free(ctx);
        return false; 
    }
    
    // Create end timer
    delay_int =end_delay;

    ctx->isLast=(ck_h == num_horarios - 1)? true : false;
    end_timers[countTimersEnd] = xTimerCreate("TEND", pdMS_TO_TICKS(delay_int),
                                           pdFALSE, (void*) ctx, blower_end);

    if ((theConf.debug_flags >> dSCH) & 1U) {

        format_log_time(endtime,time_str,30);
        format_log_time(now,time_str2,30);
        MESP_LOGI(TAG, "%sScheduling Timer %d Ending in %d ms [%s | %llu ] now %s | [ %llu Mux %ld ] is Last %s", 
                 DBG_SCH, ctx->timerNum, delay_int, time_str, endtime, time_str2, now,theConf.test_timer_div, ctx->isLast ? "YES" : "NO" );
                //  DBG_SCH, countTimersEnd, delay_int, time_str, endtime, time_str2, now,theConf.test_timer_div, ctx->isLast ? "YES" : "NO" );
    }                                           
    if (end_timers[countTimersEnd]==NULL) { 
        MESP_LOGE(MESH_TAG, "End Timer not created %d", countTimersEnd);
        xTimerDelete(start_timers[countTimersStart], 0);
        free(ctx);
        return false;
    }
    
    // Start both timers
    // if(xTimerIsTimerActive(start_timers[countTimersStart]) == pdTRUE) 
    //     xTimerStop(start_timers[countTimersStart], portMAX_DELAY);
    if (xTimerStart(start_timers[countTimersStart], portMAX_DELAY) != pdPASS)
        MESP_LOGE(MESH_TAG, "FATAL could not start Start Timer %d", countTimersStart);

    // if(xTimerIsTimerActive(end_timers[countTimersEnd]) == pdTRUE) 
    //     xTimerStop(end_timers[countTimersEnd], portMAX_DELAY);
    if (xTimerStart(end_timers[countTimersEnd], portMAX_DELAY) != pdPASS) 
        MESP_LOGE(MESH_TAG, "FATAL could not start End Timer %d", countTimersEnd);
    
    countTimersStart++;
    countTimersEnd++;

    // should set the pool status to BLOWERON since it will be started soon It certainly is not PARKED
    // theBlower.setScheduleStatus(BLOWERON); // already done by start schedule
    if(countTimersStart>=MAXHORARIOS || countTimersEnd>=MAXHORARIOS)
    {
        MESP_LOGE(MESH_TAG, "FATAL too many timers Start %d End %d", countTimersStart,countTimersEnd);
        return false;
    }
        
    return true;
}

/**
 * @brief Process single horario (schedule slot)
 * 
 * @param ck Cycle index
 * @param ck_d Day index
 * @param ck_h Hour index
 * @param midn Midnight timestamp
 * @param now Current timestamp
 * @return true to continue, false to restart
 */
bool process_horario(uint8_t ck, uint8_t ck_d, int ck_h, time_t midn, time_t now) // this cycle,day,hour,midnight and now
{
    char  time_str[30],now_str[30];

    // while (pausef) {
    //     delay(1000);
    // }
    // create a context structure for every pair of start-end timers
    start_timer_ctx_t* ctx = create_timer_context(ck, ck_d, ck_h, countTimersEnd);  //all tiemr are in pairs
    if (!ctx) {
        return false;
    }
    
    const auto& horario = theConf.profiles[0].cycle[ck].horarios[ck_h];
    time_t starttime = time_t(midn + (uint64_t)(horario.hourStart )*3600 +(uint64_t)(horario.minutesStart)*60);
    time_t endtime = time_t(starttime + (uint64_t)( horario.horarioLen));
    int64_t son=endtime-starttime;
    int32_t son32=son;
     
    if ((theConf.debug_flags >> dSCH) & 1U) 
        MESP_LOGI(TAG, "%sC-%d D-%d H-%d %d secs %d hours Start %lld End %lld Timer %d", DBG_SCH, ck, ck_d, ck_h,
             horario.horarioLen,horario.horarioLen /3600,starttime,endtime ,countTimersEnd);
    
    // Schedule already started
    if (starttime < now) { // past schedule
        if ((theConf.debug_flags >> dSCH) & 1U) 
        {
            format_log_time(starttime, time_str, 30);
            format_log_time(now, now_str, 30);
            MESP_LOGW(TAG, "%sStart already happened %lld (%s) %lld (%s) Mux %d", DBG_SCH, starttime, time_str,now, now_str,
                theConf.test_timer_div);
        }
        // countTimersStart++;   // cannot skip start timer count due to complicated timer numbering

        if (endtime < now) 
        {
            if ((theConf.debug_flags >> dSCH) & 1U) 
            {
                format_log_time(endtime, time_str, 30);
                format_log_time(now, now_str, 30);
                    MESP_LOGW(TAG, "%sEnd already happened. Skip this schedule %lld (%s) %lld (%s) Mux %d", 
                            DBG_SCH, endtime, time_str, now, now_str, theConf.test_timer_div);
            }
            free(ctx);      // now we free ctx for this timer... its done working
        } else  // we have the end timer active so create it
            handle_past_schedule_in_progress(ctx,endtime, now, ck,ck_d,ck_h, 
                                            theConf.profiles[0].cycle[ck].numHorarios);
        return true;
    }
    
    // Future schedule
    return create_future_timers(starttime, endtime, now, ctx, ck_h,
                               theConf.profiles[0].cycle[ck].numHorarios);
}

/**
 * @brief Clean up all active timers
 */
void cleanup_all_timers()
{
    uint8_t deletedStart=0,deletedEnd=0;
    for (int i = 0; i < countTimersStart; i++) 
    {
        if(start_timers[i])
        {
            if(xTimerIsTimerActive(start_timers[i]) == pdTRUE) 
                    xTimerStop(start_timers[i], portMAX_DELAY);

            xTimerDelete(start_timers[i], portMAX_DELAY);
            deletedStart++;
        }   
        if(end_timers[i])
        {
            if(xTimerIsTimerActive(end_timers[i]) == pdTRUE) 
                xTimerStop(end_timers[i], portMAX_DELAY);

            xTimerDelete(end_timers[i], portMAX_DELAY);
            deletedEnd++;
        }
    }
    bzero(start_timers, sizeof(start_timers));
    bzero(end_timers, sizeof(end_timers));
    bzero(elapsed, sizeof(elapsed));
    
    if ((theConf.debug_flags >> dSCH) & 1U)
        MESP_LOGI(TAG, "%sStart Deleted %d End Deleted %d", DBG_SCH, deletedStart,deletedEnd);
    
    countTimersStart = countTimersEnd = 0;
}

/**
 * @brief Handle end-of-day transition and cleanup
 * 
 * @param now Initial start timestamp
 */
uint32_t handle_day_end(uint8_t workingday)
{
    char time_str1[30],time_str2[30];

    time_t current = time(NULL);
    struct tm *timeinfo = localtime(&current);
    uint8_t currentDay= timeinfo->tm_mday;

    if(countTimersEnd==0)       //no timers active
    {
        timeinfo->tm_hour = 0;
        timeinfo->tm_min = 0;
        timeinfo->tm_sec = 0;
        timeinfo->tm_mday++;  // Move to next day Zero hours
        time_t midnight = mktime(timeinfo);
        return uint32_t(midnight-current);
    }

      if (!xSemaphoreTake(scheduleSem, portMAX_DELAY)) { //wait for blower last timer given by blower end
        return 0;
        }
    
    cleanup_all_timers();

    if ((theConf.debug_flags >> dSCH) & 1U) {
        MESP_LOGI(TAG, "%sDay ended Working day %d Today %d", DBG_SCH,workingday,currentDay);
    }

    if(currentDay>workingday)
        return 0; // we have passed midnight and we are already in the next day, so start immediately
    // we are in the same day, but we have passed the scheduled end time, so we need to wait for midnight to start the next day cycle
    // time_t current = time(NULL);
    // struct tm *timeinfo = localtime(&current);
    // uint8_t currentHour= timeinfo->tm_hour;

    // if(nextHour>currentHour)
    // {
    //     if ((theConf.debug_flags >> dSCH) & 1U) {
    //         MESP_LOGI(TAG, "%sAfter current hour NH %d CH %d", DBG_SCH,nextHour,currentHour);
    //     }
    //     return 0; // we have passed midnight and next hour is still ahead of current time.
    // }
  
    // need to wait for midnight and then start normal scheduling process
   
    // Calculate midnight time
    timeinfo->tm_hour = 0;
    timeinfo->tm_min = 0;
    timeinfo->tm_sec = 0;
    timeinfo->tm_mday++;  // Move to next day Zero hours
    time_t midnight = mktime(timeinfo);
    // sanity check
    if(midnight<current)
    {
        MESP_LOGE(TAG, "%sMidnight calculation error now %ld midn %ld", DBG_SCH, (long)current, (long)midnight);
        // cleanup_all_timers();
        return 0;   
    }

    if ((theConf.debug_flags >> dSCH) & 1U) {
        format_log_time(current, time_str1, 30);
        format_log_time(midnight, time_str2, 30);
        MESP_LOGI(TAG, "%sStart Day %s Midnight %s", DBG_SCH, time_str1, time_str2);
    }
    
    return (uint32_t)(midnight - current);     //return time to wait in seconds for next day midnight 00:00:00
}


/**
 * @brief Send day cycle start notification to MQTT info queue
 * 
 * Sends a message to the info queue containing current date, pool ID, unit ID,
 * day counter, and "Started Day Cycle" status message.
 * 
 * @param ck_d Current day counter within cycle
 */
void send_start_day_host(uint8_t ck_d)
{
    time_t now;
    time(&now);
    struct tm *timeinfo = localtime(&now);
    
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        MESP_LOGE(MESH_TAG, "No ram for start day message");
        return;
    }
    
    char date_str[30];
    strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    cJSON_AddStringToObject(root, "cmd", "WORKINFO");
    cJSON_AddStringToObject(root, "date", date_str);
    cJSON_AddNumberToObject(root, "poolid", theConf.poolid);
    cJSON_AddNumberToObject(root, "unitid", theConf.unitid);
    cJSON_AddNumberToObject(root, "day", ck_d);
    cJSON_AddStringToObject(root, "msg", "Started Day Cycle");
    
    char *json_msg = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    
    if (!json_msg) {
        MESP_LOGE(MESH_TAG, "No ram for JSON print");
        return;
    }
    
    if(theConf.wifi_mode==0)
    {  
        esp_mqtt_client_publish(clientCloud, (char*)alarmQueue, (char*)json_msg, strlen(json_msg), QOS1, NORETAIN);
        free(json_msg);
        return;
    }

    mqttSender_t mensaje;
    bzero(&mensaje, sizeof(mensaje));
    mensaje.queue = alarmQueue;
    mensaje.msg = json_msg;
    mensaje.lenMsg = strlen(json_msg);
    
    if (xQueueSend(mqttSender, &mensaje, 0) != pdPASS) {
        MESP_LOGE(MESH_TAG, "Cannot queue start day message");
        free(json_msg);
        return;
    }

    xEventGroupSetBits(wifi_event_group, SENDMQTT_BIT); // Send when possible

    if ((theConf.debug_flags >> dSCH) & 1U) {
        MESP_LOGI(TAG, "Start day message queued: %s", json_msg);
    }
}


/**
 * @brief Production schedule timer management task
 * 
 * Main task loop that processes production cycles, days, and hourly schedules.
 * Creates and manages FreeRTOS timers for blower activation/deactivation.
 * Handles recovery from power loss via flash-stored day counter.
 * 
 * @param pArg Unused task parameter
 */
void start_schedule_timers(void * pArg)
{
    time_t nows;
    time(&nows);
    time_t midn = get_today_midnight();
    uint8_t cyclestart, daystart;
    char  time_str[30],mid_str[30];

    while (true)
    {
        if (!xSemaphoreTake(workTaskSem, portMAX_DELAY)) {
            continue;
        }
        format_log_time(nows, time_str, 30);

        countTimersEnd = countTimersStart = 0;
        MESP_LOGW(TAG, "%sStarted Production cycles", DBG_SCH);
        if(!schedule_restartf)
            find_cycle_day(&cyclestart, &daystart);
        else
        {
            wschedule_t* scheduleData = theBlower.getSchedulePtr();
            cyclestart = scheduleData->currentCycle;
            daystart = scheduleData->currentDay;
            schedule_restartf= false;
        }
        schedulef = true;
        if ((theConf.debug_flags >> dSCH) & 1U)
            MESP_LOGI(TAG, "%sStart Cycle %d Start Day %d midn %ld now %ld %s", DBG_SCH, cyclestart, daystart,
                    (uint32_t)midn,(uint32_t)nows,time_str);
        
        // theBlower.setSchedule(0, 0, 0, 0, 0,0,BLOWERON); // since we are strarting a schedule, set blower status to ACTIVE/STANDBY

            time_t first_cycle_day=time(NULL);
            struct tm *first_timeinfo = localtime(&first_cycle_day);

        // Process all cycles
        for (ck = cyclestart; ck < theConf.profiles[0].numCycles; ck++)
        {
            int day_offset = (ck == cyclestart) ? daystart : 0;     // when starting cycle use daystart passed by cmd else 0 start

            // Process days in cycle
            for (ck_d = day_offset; ck_d < theConf.profiles[0].cycle[ck].duration; ck_d++)
            { 
                // if(ck_d!=day_offset)    
                // {
                //     time_t right_now=time(NULL);
                //     struct tm *timeinfo = localtime(&right_now);
                //     if(first_timeinfo->tm_mday==timeinfo->tm_mday) // if we have changed day since first cycle day
                //     {
                //         first_cycle_day=right_now;
                //         first_timeinfo = localtime(&first_cycle_day);
                //         timeinfo->tm_hour = 0;
                //         timeinfo->tm_min = 0;
                //         timeinfo->tm_sec = 0;
                //         timeinfo->tm_mday++;  // Move to next day Zero hours
                //         time_t midn = mktime(timeinfo);

                //         struct timeval tv = {
                //                 .tv_sec = midn,
                //                 .tv_usec = 0
                //             };
                //         settimeofday(&tv, NULL);

                //     }
                // }
                // start here if PF
                theBlower.setSchedule(ck,ck_d,0,0,0,0,BLOWERON); // set the schedule in blower to indicate we are in active schedule mode

                // Process horarios (hourly schedules) in day
                time(&nows);        // get todays new time or it will use the one when we STARTED the SCHEDULE process
                struct tm *hoytimeinfo = localtime(&nows);
                format_log_time(nows, time_str, 30);
                MESP_LOGI(TAG, "Day %d Time %s",ck_d,time_str);
                midn = get_today_midnight();
                for (int ck_h = 0; ck_h < theConf.profiles[0].cycle[ck].numHorarios; ck_h++)
                {
                    if(ck_h==0)
                        send_start_day_host(ck_d);
                    if (!process_horario(ck, ck_d, ck_h, midn, nows))
                    {
                        printf("Failed process horario\n");
                        goto restart_schedule; // Timer validation failed, restart
                    }
                }
                // Day complete scheduled all timers, wait and cleanup
                uint8_t newhour=theConf.profiles[0].cycle[ck].horarios[0].hourStart; // first hour of next day same cycle

                uint32_t wait_next_day= handle_day_end(hoytimeinfo->tm_mday);        // the first hour of next day. boundry of end fo cycles later
                // uint32_t wait_next_day= handle_day_end(newhour);        // the first hour of next day. boundry of end fo cycles later
                // above will wait for schedulesemaphore to be free meaning last blower timer ended

                // need to wait for next day
                if ((theConf.debug_flags >> dSCH) & 1U)
                {
                    MESP_LOGI(TAG, "%sSave day %d", DBG_SCH, ck_d+1);
                    time_t new_day=time(NULL);
                    format_log_time(new_day, time_str, 30);
                    time_t nmid=new_day+wait_next_day;
                    format_log_time(nmid, mid_str, 30);

                    MESP_LOGW(TAG, "%sWait Midnight %ld (%s) New day %s", DBG_SCH, wait_next_day, mid_str, time_str);
                }
                uint32_t howmuch= (wait_next_day*1000/theConf.test_timer_div)+10000;
      
                // if (countTimersStart || countTimersEnd)
                    delay(howmuch); // in secs->ms and add 10 seconds to be sure we are in the next day
                // else
                    // delay(1000); // in secs->ms and add 10 seconds to be sure
   
                    /* for simultions only erase in production

                delay(5000); // in secs->ms and add 10 seconds to be sure we are in the next day

                time_t midnn = time(NULL);
                struct tm *timeinfo = localtime(&midnn);
                
                timeinfo->tm_hour = 0;
                timeinfo->tm_min = 0;
                timeinfo->tm_sec = 0;
                timeinfo->tm_mday++;  // Move to next day Zero hours
                time_t midn = mktime(timeinfo);

                struct timeval tv = {
                        .tv_sec = midn,
                        .tv_usec = 0
                    };
                settimeofday(&tv, NULL);*/

                time_t nuevo=time(NULL);
                format_log_time(nuevo, time_str, 30);

                if ((theConf.debug_flags >> dSCH) & 1U)
                    MESP_LOGI(TAG, "%sNew Day %d %s", DBG_SCH, ck_d+1,time_str);
            }

            if ((theConf.debug_flags >> dSCH) & 1U)
                MESP_LOGI(TAG, "%sNew Cycle %d", DBG_SCH, ck+1);
        }
                
        // All cycles complete
        MESP_LOGE(TAG, "Production cycle ended");
        theBlower.setSchedule(0, 0, 0, 0, 0,0,BLOWERCROP);
        schedulef = false;
        continue;
        
restart_schedule:
        cleanup_all_timers();       // just in case
    }
} 

/**
 * @brief Register and mount SPIFFS filesystem
 * 
 * Configures VFS for SPIFFS with automatic formatting on mount failure.
 * Uses /spiffs as base path with max 2 concurrent file handles.
 * 
 * @param partition_label Name of the SPIFFS partition to mount
 * @return true if registration and mount successful, false otherwise
 */
bool spiffs_register_and_mount(const char* partition_label)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = partition_label,
        .max_files = 2,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            MESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            MESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            MESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return false;
    }
    return true;
}

/**
 * @brief Perform filesystem consistency check
 * 
 * Validates SPIFFS partition integrity. Can mend broken files and clean
 * unreferenced pages. Should be run after power loss or unexpected shutdowns.
 * 
 * @param partition_label Name of the SPIFFS partition to check
 * @return true if check passed, false if errors detected
 * @see https://github.com/pellepl/spiffs/wiki/FAQ#powerlosses-contd-when-should-i-run-spiffs_check
 */
bool spiffs_perform_check(const char* partition_label)
{
    MESP_LOGI(TAG, "Performing SPIFFS_check()");
    esp_err_t ret = esp_spiffs_check(partition_label);
    if (ret != ESP_OK) {
        MESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
        return false;
    }
    MESP_LOGI(TAG, "SPIFFS_check() successful");
    return true;
}

/**
 * @brief Verify partition size information and handle anomalies
 * 
 * Retrieves and validates partition usage statistics. If used bytes exceed
 * total capacity (indicates corruption), triggers filesystem check.
 * Formats partition if info retrieval fails.
 * 
 * @param partition_label Name of the SPIFFS partition to verify
 * @return true if partition info valid, false if formatting required
 */
bool spiffs_verify_partition_info(const char* partition_label)
{
    size_t total = 0, used = 0;
    esp_err_t ret = esp_spiffs_info(partition_label, &total, &used);
    
    if (ret != ESP_OK) {
        MESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
        esp_spiffs_format(partition_label);
        return false;
    }
    
    MESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    
    if (used > total) {
        MESP_LOGW(TAG, "Number of used bytes cannot be larger than total. Performing SPIFFS_check()");
        return spiffs_perform_check(partition_label);
    }
    
    return true;
}

/**
 * @brief Initialize SPIFFS logging partition
 * 
 * Complete initialization sequence for SPIFFS-based logging:
 * 1. Register and mount the "profile" partition
 * 2. Perform filesystem consistency check
 * 3. Verify partition size information
 * 
 * Early return on any initialization failure.
 */
void app_spiffs_log(void)
{
    MESP_LOGI(TAG, "Initializing SPIFFS Logs");
    
    const char* partition_label = "profile";
    
    if (!spiffs_register_and_mount(partition_label)) {
        return;
    }
    
    if (!spiffs_perform_check(partition_label)) {
        return;
    }
    
    if (!spiffs_verify_partition_info(partition_label)) {
        return;
    }
}

// Blower Start is called when the start timer fires
// will turn the Blower ON
// if Mesh Mode it will send the schedule info to all child nodes (ctx has the schedule info)
// ctx has the detailes of schedule info to send to childs as we3ll as the timerNum for non Mesh Mode

/**
 * @brief Create and populate schedule start JSON message
 * 
 * @param start_timer_ctx Timer context with schedule information
 * @return Allocated JSON string or NULL on failure (caller must free)
 */
char* create_schedule_start_json(const start_timer_ctx_t *start_timer_ctx)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return NULL;
    }
    
    cJSON_AddStringToObject(root, "cmd", "schstart");
    cJSON_AddNumberToObject(root, "unit", theConf.unitid);
    cJSON_AddNumberToObject(root, "cycle", start_timer_ctx->cycle);
    cJSON_AddNumberToObject(root, "day", start_timer_ctx->day);
    cJSON_AddNumberToObject(root, "hora", start_timer_ctx->horario);
    
    char *json_msg = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    
    return json_msg;
}

/**
 * @brief Send schedule start message to mesh network
 * 
 * @param json_msg JSON formatted message (will be freed by sender)
 * @return true if queued successfully, false otherwise
 */
bool send_schedule_sync_to_mesh(char *json_msg)
{
    mqttSender_t meshMsg;
    bzero(&meshMsg, sizeof(meshMsg));
    
    meshMsg.queue = emergencyQueue;
    meshMsg.msg = json_msg;
    meshMsg.lenMsg = strlen(json_msg);
    meshMsg.addr = NULL;
    
    if (xQueueSend(meshQueue, &meshMsg, 0) != pdPASS) {
        MESP_LOGE(MESH_TAG, "Cannot queue Schedule Sync");
        free(json_msg);
        return false;
    }
    
    if ((theConf.debug_flags >> dSCH) & 1U) {
        MESP_LOGI(TAG, "Scheduled Start sent to Mesh %s", json_msg);
    }
    
    return true;
}

/**
 * @brief Synchronize schedule start with child mesh nodes
 * 
 * Root node broadcasts schedule start to all children for synchronized activation.
 * Starts watchdog timer to monitor sync completion.
 * 
 * @param start_timer_ctx Timer context with schedule parameters
 */
void sync_schedule_with_mesh(const start_timer_ctx_t *start_timer_ctx)
{
    int err = esp_mesh_get_routing_table((mesh_addr_t *)&poolNodesTable.address_table,
                                         MAXNODES * 6, &poolNodesTable.existing_nodes);
    
    if (err != ESP_OK) {
        MESP_LOGE(MESH_TAG, "Failed to get routing table %d", err);
        return;
    }
    
    poolNodesTable.existing_nodes = theConf.totalnodes;
    xTimerStart(scheduleTimer, 1);
    
    char *json_msg = create_schedule_start_json(start_timer_ctx);
    if (!json_msg) {
        MESP_LOGE(MESH_TAG, "No ram for child schedule start message");
        return;
    }
    
    send_schedule_sync_to_mesh(json_msg);
}

/**
 * @brief PID Controller for Blower will be a task launch by the Configuration Setup options setup
 *  ! launched by after we have the correc timer SNTP check that rioutine root_sntpget
 * @param pArg is void 
 * @return no return as it is a task
 * the outputVal variable is global and used by the blower control task to set the blower speed via VFD
 */
void PIDController(void *pArg)  
{
    time_t now;
    struct tm timeinfo;
    bool logged=false;

    // PID settings and gains
    setPoint=theConf.doParms.setpoint;          // Desired DO setpoint
    KP = theConf.doParms.KP;                    // Proportional gain
    KI = theConf.doParms.KI;                    // Integral gain
    KD = theConf.doParms.KD;                    // Derivative gain

    if (((theConf.debug_flags >> dDO) & 1U) && !logged)
        MESP_LOGW(TAG,"PID Controller Task Started SetPoint %.4f Kp %.4f KI %.4f KD %.4f Sample %d Night-Only: %s",setPoint,KP,KI,KD,
            theConf.doParms.sampletime,theConf.doParms.nighonly ? "Yes" : "No");

// setGains(double Kp, double Ki, double Kd);       // call this to set gains dynamcially according to the HOUR of the day
    AutoPID myPID(&doValue, &setPoint, &outputVal, OUTPUT_MIN, OUTPUT_MAX, KP, KI, KD); // Create an AutoPID object
    myPID.setBangBang(theConf.doParms.setpoint-1.0,theConf.doParms.setpoint+1.0); // Set bang-bang control thresholds
    myPID.setTimeStep(10000); // Set PID update interval to 10000ms

    // MUST BE AWARE of DO reading delays from the sensor and if any error to handle this scenerio

    while (true) {
        denuevo:
        time(&now);                     // the nightonly parameter requires knowing the current time
        localtime_r(&now, &timeinfo);
        if(theConf.doParms.nighonly)            
        {
            if(timeinfo.tm_hour>6 && timeinfo.tm_hour<18)
            {
                if (((theConf.debug_flags >> dDO) & 1U) && !logged) {
                    MESP_LOGI(TAG, "Night Only DO Control Disabled Now %02d:%02d",timeinfo.tm_hour,timeinfo.tm_min);
                    logged=true;        // display thsi only once
                }
                vTaskDelay(pdMS_TO_TICKS(60000)); // Delay for a short period
                goto denuevo;
            }
        }
        
        // choose kd according to time of day
        if(timeinfo.tm_hour>=6 && timeinfo.tm_hour<12) // day time to noon
            KD=1.5;
        else 
            if(timeinfo.tm_hour>=12 && timeinfo.tm_hour<18) // noon to evening
                KD=2.5;
            else
                KD=1.0; // night time

        doValue=sensorData.DO; // get the current DO value from sensorsData structure that mantains all sensor readings
        if (doValue==0){
            if (((theConf.debug_flags >> dDO) & 1U)) 
                MESP_LOGE(TAG, "DO reading is ZERO, cannot run PID");
            vTaskDelay(pdMS_TO_TICKS(theConf.doParms.sampletime*60*1000)); // Delay for a short period
            // shall revert to standard schedule operations for now just wait and retry 
            // myPID.run will return max Output if DO is zero which is kinda OK but not ideal
        }

        myPID.run(); // Call every loop, updates automatically at the set time interval

        // print at setpoint when at setpoint +-1 degree
        if (myPID.atSetPoint(1)) {
            if ((theConf.debug_flags >> dDO) & 1U) 
                MESP_LOGI(TAG, "At setpoint");
        }
        // send the VFD the current outputVal to set blower speed
        // VFDSetSpeed(outputVal); // to be implemented in blower control task
        if ((theConf.debug_flags >> dDO) & 1U) 
            MESP_LOGI(TAG, "KD: %.4f DO: %.2f, SetPoint: %.2f, Output: %.2f", KD,doValue, setPoint, outputVal); // Log the values

        vTaskDelay(pdMS_TO_TICKS(theConf.doParms.sampletime*60*1000)); // Delay for a short period
    }
}

/**
 * @brief Timer callback for blower start event
 * 
 * Handles schedule activation for blower operation. In mesh mode, root node
 * broadcasts start message to child nodes for synchronized operation.
 * 
 * @param xTimer Timer handle containing start_timer_ctx_t as ID
 */
void blower_start(TimerHandle_t xTimer)
{
    uint16_t currentamps,dummy;
    float dummyf;
    char time_str2[30];

    start_timer_ctx_t *start_timer_ctx = (start_timer_ctx_t *)pvTimerGetTimerID(xTimer);

    // motor power  * duration is thw kwh to be consumed in this schedule
    uint16_t energy_amps= MOTORKW * (start_timer_ctx->horaslen)  / MOTORVOLTS;   // Expected energy consumption in AH

    theBlower.getEnergy(&currentamps,&dummy,&dummy,&dummy,&dummyf,&dummyf,&dummyf,&dummyf,&dummyf,&dummyf); // get current amps form the blower

    if(energy_amps>currentamps)
        if ((theConf.debug_flags >> dSCH) & 1U) 
            MESP_LOGW(TAG, "Schedule Start Check Energy NOT OK amps Availble %d needed %d",currentamps,energy_amps); 
        // todo send mqtt msg to host Emergency/Alarm
    
    turn_blower_onOff(true);

    theBlower.setSchedule( start_timer_ctx->cycle, start_timer_ctx->day, start_timer_ctx->horario, theConf.profiles[0].cycle[start_timer_ctx->cycle].horarios[start_timer_ctx->tostart].hourStart,
            theConf.profiles[0].cycle[start_timer_ctx->cycle].horarios[start_timer_ctx->tostart].horarioLen,theConf.profiles[0].cycle[start_timer_ctx->cycle].horarios[start_timer_ctx->tostart].pwmDuty,BLOWERNEXT);

    if ((theConf.debug_flags >> dSCH) & 1U) {
        time_t now = time(NULL);
        format_log_time(now,time_str2,30);
        MESP_LOGI(TAG, "Timer %d Started Cycle %d Day %d Hora %d Start %d Horas %d PW %d Time: %s",
                 start_timer_ctx->timerNum, start_timer_ctx->cycle, start_timer_ctx->day, start_timer_ctx->horario,
                 start_timer_ctx->tostart, start_timer_ctx->horaslen, start_timer_ctx->pwm, time_str2);
    }

    elapsed[start_timer_ctx->timerNum] = time(NULL);

    if (!theConf.wifi_mode) {
        // free(start_timer_ctx);  //freed by endtimer
        return;
    }
    
    
    if (esp_mesh_is_root()) {
        sync_schedule_with_mesh(start_timer_ctx);
    }
    
    // free(start_timer_ctx); //freed by endtimer
}

void blower_end(TimerHandle_t xTimer)
{
    char time_str2[30];
    start_timer_ctx_t *ctx = (start_timer_ctx_t *)pvTimerGetTimerID(xTimer);
    uint8_t ulCount = ctx->timerNum;

    if((theConf.debug_flags >> dSCH) & 1U)  
    {
        time_t now= time(NULL);
        format_log_time(now,time_str2,30);
        MESP_LOGI(TAG,"Timer Ended %d Elapsed %ld Time: %s",ulCount,(uint32_t)(now-elapsed[ulCount]),time_str2);
    }
    elapsed[ulCount]=0;
    turn_blower_onOff(false);

    if (ctx->isLast)
    {
        if((theConf.debug_flags >> dSCH) & 1U)  
            MESP_LOGI(TAG,"Last end timer...set for Wait For Start\n");
        xSemaphoreGive(scheduleSem);
        theBlower.setScheduleStatus(BLOWEROFF);
    }
    else
        theBlower.setScheduleStatus(BLOWERNEXT);
    
    free(ctx);      // now we free ctx for this timer... its done working
}

void app_main(void)
{
    esp_err_t ret;

    init_process();  
webserverf=false;

    // printf("DO Active %d Setpoint %f KP %f KI %f KD %f Night %d\n",theConf.doParms.docontrol,theConf.doParms.setpoint,
    //     theConf.doParms.KP,theConf.doParms.KI,theConf.doParms.KD,theConf.doParms.nighonly);

    app_spiffs_log();
    // gdispf=false;
    // #ifdef DISPLAY
    //     ret=init_lcd();
    //     if(ret==ESP_OK)
    //     {
    //         xTaskCreate(&displayManager,"dMgr",1024*4,NULL, 5, &dispHandle); 	       
    //         gdispf=true;
    //     } 
    // #endif
    //log boot
    if(theConf.test_timer_div<1)
        theConf.test_timer_div=1;
    if(theConf.modbus_mux<1)
        theConf.modbus_mux=1;
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
printf("PV PAnel %d Battery %d Energy %d Solar System %d SolarPad %d Union %d SmpMsg %d timet %d float %d Config %d tickType %d\n",sizeof(pvPanel_t),sizeof(battery_t),
sizeof(energy_t),sizeof(solarSystem_t),sizeof(solarDef_t),sizeof(meshunion_t),sizeof(shrimpMsg_t),sizeof(time_t),sizeof(float),
sizeof(theConf), sizeof(TickType_t)); 

    if(theConf.meterconf==0  )  // is this meter NOT configured
    {
        //no, start the configuration phase
        //need to format fram to assure that new frrams are not with junk
        MESP_LOGW(MESH_TAG,"Formatting Fram new configuration");
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
    
    // xTaskCreate(&start_schedule_timers,"sched",1024*10,NULL, 5, &scheduleHandle); 	       

    xTaskCreate(&root_timer,"reptimer",1024*8,NULL, 5, NULL); 	        
    xTaskCreate(&rs485_task_manager,"modbus",1024*10,NULL, 5, NULL); 	            // start the modbus task  
    xTaskCreate(&ds18b20_task,"modbus",1024*3,NULL, 5, NULL); 	            // start the modbus task  
    // printf("Active %d\n",theConf.doParms.docontrol); 
 
    // delay(2000);  //used when debugging modbus to test just modbus without starting mesh/wifi
    // launch_sensors();
            // start the modbus task   
// the internal mesh is now going to start and begin all the main flow from its gotIp event manager
    showLVGL((char*)"MESH",10000,3);   

    if(!theConf.masternode && theConf.wifi_mode)     // allow master to boot
    {
        MESP_LOGW(TAG,"Leaf -> Waiting for Root to start");
        delay(30000);
    }
    MESP_LOGI(TAG,"Network mode is %s",theConf.wifi_mode?"Mesh":"WiFi");

    if(theConf.wifi_mode==WIFI_MESH)
    {
        // printf("Start wifi mode\n");
        wifi_connect_external_ap(); //start wifi which will start many ither tasks from got ip event wiht SNTP starting the scheduler
        webserverf=true;
        xTaskCreate(&start_webserver, "webs", 1024 * 10, NULL, 5, NULL);

    }
    else  
    {
        // printf("Start mesh\n");
        start_mesh();
        theConf.loginwait=20000;
    }
// schedule timer will be started or not by sntp if root or when child connected by mesh if it was active and crash/power down
    
MESP_LOGI(MESH_TAG,"APP Free Heap %d",esp_get_free_heap_size());
// if system has resetted and restarting, the sntpget routine will be in charge of starting the schedule timer again
//  In mesh mode,the mesh connecting child will be order to start also if theConf.blower_mode=1;(???) This is set to 0 when Cycle done see start shcedule timer

}