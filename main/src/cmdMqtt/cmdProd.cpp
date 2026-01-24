#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

/*
 * Handles production schedule control commands sent over mesh. Manages production profile
 * activation, day cycle configuration, and operational state (start/stop/pause/resume).
 * Broadcasts changes to all mesh nodes and updates persistent configuration.
 */

extern void writeLog(char *que);
extern void save_ota_version(char *version);

// Production command constants
#define PRODUCTION_LOG_BUFFER_SIZE 200
#define ORDER_COMMAND_MAX_LENGTH 10

// Production order types
typedef enum {
    ORDER_START = 0,
    ORDER_STOP = 1,
    ORDER_PAUSE = 2,
    ORDER_RESUME = 3,
    ORDER_CROP=4,
    ORDER_PARK=5,
    ORDER_INVALID = -1
} ProductionOrderType;

typedef struct {
    uint8_t profileIndex;
    uint8_t dayIndex;
    uint32_t muxTime;
    ProductionOrderType orderType;
    char orderCommand[ORDER_COMMAND_MAX_LENGTH];
} ProductionCommandFields;

/* Broadcasts production control message to all mesh nodes. */
void send_start_production(uint8_t profileIndex, uint8_t dayIndex, uint32_t muxTime, char* orderCommand)
{
    cJSON *root = cJSON_CreateObject();
    if (root)
    {
        cJSON_AddStringToObject(root, "cmd", internal_cmds[PRODUCTION]);
        cJSON_AddNumberToObject(root, "prof", profileIndex);
        cJSON_AddNumberToObject(root, "day", dayIndex);
        cJSON_AddNumberToObject(root, "mux", (uint32_t)muxTime);
        cJSON_AddStringToObject(root, "order", orderCommand);
        char *mensaje = cJSON_PrintUnformatted(root);
        if(!mensaje)
        {
            ESP_LOGE(MESH_TAG, "Start Production failed create unformatted msg");
            cJSON_Delete(root);
            return;
        }
        if(root_mesh_broadcast_msg(mensaje) != ESP_OK)
            ESP_LOGE(MESH_TAG, "Start Production Msg not sent");
        free(mensaje);
        cJSON_Delete(root);
    }
    else
    {
        ESP_LOGE(MESH_TAG, "Could not create root Start Production");
    }
}

/* Validates JSON payload and extracts production command parameters. */
static bool validate_production_command(cJSON *productionCommand, ProductionCommandFields *outFields)
{
    if(!productionCommand || !outFields)
    {
        ESP_LOGE(MESH_TAG, "Production command argument invalid");
        return false;
    }
    
    if(!cJSON_IsObject(productionCommand))
    {
        ESP_LOGE(MESH_TAG, "Production command payload invalid");
        return false;
    }
    
    cJSON *profileNode = cJSON_GetObjectItem(productionCommand, "prof");
    cJSON *dayNode = cJSON_GetObjectItem(productionCommand, "day");
    cJSON *muxNode = cJSON_GetObjectItem(productionCommand, "mux");
    cJSON *orderNode = cJSON_GetObjectItem(productionCommand, "order");
    
    if(!profileNode || !dayNode || !muxNode || !orderNode)
    {
        ESP_LOGE(MESH_TAG, "Production command missing required fields");
        return false;
    }
    
    if(!cJSON_IsNumber(profileNode) || !cJSON_IsNumber(dayNode) || !cJSON_IsNumber(muxNode))
    {
        ESP_LOGE(MESH_TAG, "Production command numeric parameters invalid");
        return false;
    }
    
    if(!cJSON_IsString(orderNode) || !orderNode->valuestring || strlen(orderNode->valuestring) == 0)
    {
        ESP_LOGE(MESH_TAG, "Production order parameter invalid or empty");
        return false;
    }
    
    outFields->profileIndex = profileNode->valueint - 1;
    outFields->dayIndex = dayNode->valueint - 1;
    outFields->muxTime = muxNode->valueint;

    
    if(outFields->profileIndex >= MAXPROFILES)
    {
        ESP_LOGE(MESH_TAG, "Production profile index out of range: %d (max %d)", 
                 outFields->profileIndex, MAXPROFILES - 1);
        return false;
    }
    
    if(outFields->dayIndex >= MAXDAYCYCLE)
    {
        ESP_LOGE(MESH_TAG, "Production day index out of range: %d (max %d)", 
                 outFields->dayIndex, MAXDAYCYCLE - 1);
        return false;
    }
    
    strlcpy(outFields->orderCommand, orderNode->valuestring, ORDER_COMMAND_MAX_LENGTH);
    
    if(strcmp(outFields->orderCommand, "start") == 0)
        outFields->orderType = ORDER_START;
    else if(strcmp(outFields->orderCommand, "stop") == 0)
        outFields->orderType = ORDER_STOP;
    else if(strcmp(outFields->orderCommand, "pause") == 0)
        outFields->orderType = ORDER_PAUSE;
    else if(strcmp(outFields->orderCommand, "resume") == 0)
        outFields->orderType = ORDER_RESUME;
    else if(strcmp(outFields->orderCommand, "crop") == 0)
        outFields->orderType = ORDER_CROP;
    else if(strcmp(outFields->orderCommand, "park") == 0)
        outFields->orderType = ORDER_PARK;
    else
        outFields->orderType = ORDER_INVALID;
    
    if(outFields->orderType == ORDER_INVALID)
    {
        ESP_LOGE(MESH_TAG, "Invalid production order: %s", outFields->orderCommand);
        return false;
    }
    
    return true;
}

/* Updates persistent configuration with production parameters. */
static void apply_production_config(const ProductionCommandFields *fields)
{
    theConf.dayCycle = fields->dayIndex;
    theConf.activeProfile = fields->profileIndex;
    theConf.test_timer_div = fields->muxTime;
    write_to_flash();
}

/* Handles production start operation. */
static int handle_production_start(const ProductionCommandFields *fields, char *logBuffer)
{
    if(schedulef)
    {
        if((theConf.debug_flags >> dXCMDS) & 1U)  
            ESP_LOGI(MESH_TAG, "%sCMd Prod already started %s", DBG_XCMDS, fields->orderCommand);
        return ESP_OK;
    }
    
    snprintf(logBuffer, PRODUCTION_LOG_BUFFER_SIZE, "CMd Prod Start %s", fields->orderCommand);
    if((theConf.debug_flags >> dXCMDS) & 1U)  
        ESP_LOGI(MESH_TAG, "%sCMd Prod Start %s", DBG_XCMDS, fields->orderCommand);
    writeLog(logBuffer);
    
    xSemaphoreGive(workTaskSem);
    if(theConf.wifi_mode)
        send_start_production(fields->profileIndex, fields->dayIndex, 
                         theConf.test_timer_div, (char*)fields->orderCommand);
    return ESP_OK;
}

/* Handles production stop operation. */
static int handle_production_stop(const ProductionCommandFields *fields, char *logBuffer)
{
    if(!schedulef)
    {
        if((theConf.debug_flags >> dXCMDS) & 1U)  
            ESP_LOGI(MESH_TAG, "%sCMd Prod Stop but not scheduling %s", DBG_XCMDS, fields->orderCommand);
        return ESP_OK;
    }
    
    theConf.dayCycle = 0;
    theConf.work_day=theConf.work_cycle=0;
    write_to_flash();
    
    snprintf(logBuffer, PRODUCTION_LOG_BUFFER_SIZE, "CMd Prod Stop %s", fields->orderCommand);
    writeLog(logBuffer);
    
    if((theConf.debug_flags >> dXCMDS) & 1U)  
        ESP_LOGI(MESH_TAG, "%sCMd Prod Stop %s", DBG_XCMDS, fields->orderCommand);
    
    vTaskDelete(scheduleHandle);
    xTaskCreate(&start_schedule_timers, "sched", 1024*10, NULL, 5, &scheduleHandle);

    if(theConf.wifi_mode)
        send_start_production(fields->profileIndex, fields->dayIndex, 
                         theConf.test_timer_div, (char*)fields->orderCommand);
    schedulef = false;
    theBlower.setSchedule(0, 0, 0,0,0,0,BLOWEROFF);

    // stop all timers

    for (int a=0;a<vanTimersStart;a++)
        if(start_timers[a])
            xTimerStop(start_timers[a],10);
    for (int a=0;a<vanTimersEnd;a++)
        if(end_timers[a])
            xTimerStop(end_timers[a],10);

    vanTimersStart=vanTimersEnd=0;

    // stop the blower itself
    turn_blower_onOff(false);
    return ESP_OK;
}

/* Handles production pause operation. */
static int handle_production_pause(const ProductionCommandFields *fields, char *logBuffer)
{
    if(!schedulef) 
    {
        if((theConf.debug_flags >> dXCMDS) & 1U)  
            ESP_LOGI(MESH_TAG, "%sCMd Prod Pause but not scheduling %s", DBG_XCMDS, fields->orderCommand);
        return ESP_OK;
    }
    
    if((theConf.debug_flags >> dXCMDS) & 1U)  
        ESP_LOGI(MESH_TAG, "%sCMd Prod Paused %s", DBG_XCMDS, fields->orderCommand);
    
    snprintf(logBuffer, PRODUCTION_LOG_BUFFER_SIZE, "CMd Prod Pause %s", fields->orderCommand);
    writeLog(logBuffer);

    if(theConf.wifi_mode)    
        send_start_production(fields->profileIndex, fields->dayIndex, 
                         theConf.test_timer_div, (char*)fields->orderCommand);
    pausef = true;
    return ESP_OK;
}

/* Handles production resume operation. */
static int handle_production_resume(const ProductionCommandFields *fields, char *logBuffer)
{
    if(!schedulef) 
    {
        if((theConf.debug_flags >> dXCMDS) & 1U)  
            ESP_LOGI(MESH_TAG, "%sCMd Prod Resume but not scheduling %s", DBG_XCMDS, fields->orderCommand);
        return ESP_OK;
    }
    
    if((theConf.debug_flags >> dXCMDS) & 1U)  
        ESP_LOGI(MESH_TAG, "%sCMd Prod Resumed %s", DBG_XCMDS, fields->orderCommand);
    
    snprintf(logBuffer, PRODUCTION_LOG_BUFFER_SIZE, "CMd Prod Resume %s", fields->orderCommand);
    pausef = false;

    if(theConf.wifi_mode)    
        send_start_production(fields->profileIndex, fields->dayIndex, 
                         theConf.test_timer_div, (char*)fields->orderCommand);
    writeLog(logBuffer);
    return ESP_OK;
}

/* Handles production crop operation. */
static int handle_production_crop(const ProductionCommandFields *fields, char *logBuffer)
{
    // if(!schedulef) 
    // {
    //     if((theConf.debug_flags >> dXCMDS) & 1U)  
    //         ESP_LOGI(MESH_TAG, "%sCMd Prod Crop but not scheduling %s", DBG_XCMDS, fields->orderCommand);
    //     return ESP_OK;
    // }
    
    if((theConf.debug_flags >> dXCMDS) & 1U)  
        ESP_LOGI(MESH_TAG, "%sCMd Prod set to Crop %s", DBG_XCMDS, fields->orderCommand);
    
    snprintf(logBuffer, PRODUCTION_LOG_BUFFER_SIZE, "CMd Prod Crop %s", fields->orderCommand);
    pausef = false;

    if(theConf.wifi_mode)    
        send_start_production(fields->profileIndex, fields->dayIndex, 
                         theConf.test_timer_div, (char*)fields->orderCommand);
    writeLog(logBuffer);

    theBlower.setSchedule(0, 0, 0,0,0,0,BLOWERCROP);
        //TODO should stop all timer
    return ESP_OK;
}

/* Handles production Park operation. */
static int handle_production_park(const ProductionCommandFields *fields, char *logBuffer)
{
    // if(!schedulef) 
    // {
    //     if((theConf.debug_flags >> dXCMDS) & 1U)  
    //         ESP_LOGI(MESH_TAG, "%sCMd Prod Crop but not scheduling %s", DBG_XCMDS, fields->orderCommand);
    //     return ESP_OK;
    // }
    
    if((theConf.debug_flags >> dXCMDS) & 1U)  
        ESP_LOGI(MESH_TAG, "%sCMd Prod set to Park %s", DBG_XCMDS, fields->orderCommand);
    
    snprintf(logBuffer, PRODUCTION_LOG_BUFFER_SIZE, "CMd Prod Park %s", fields->orderCommand);
    pausef = false;

    if(theConf.wifi_mode)    
        send_start_production(fields->profileIndex, fields->dayIndex, 
                         theConf.test_timer_div, (char*)fields->orderCommand);
    writeLog(logBuffer);

    theBlower.setSchedule(0, 0, 0,0,0,0,BLOWERPARK);
        //TODO should stop all timer
    return ESP_OK;
}

int cmdProd(void *argument)
{
    /*
     * Expected JSON format:
     * {"cmd":"prod","prof":1,"day":1,"mux":30,"order":"start"}
     * order values: "start", "stop", "pause", "resume","crop","park"
     */
    
    if((theConf.debug_flags >> dXCMDS) & 1U)  
        ESP_LOGI(MESH_TAG, "%sCMd Prod", DBG_XCMDS);
    
    cJSON *productionCommand = (cJSON *)argument;
    
    if(productionCommand == NULL)
    {
        ESP_LOGE(MESH_TAG, "Production command argument is NULL");
        return ESP_FAIL;
    }
    
    ProductionCommandFields fields = {0};
    if(!validate_production_command(productionCommand, &fields))
        return ESP_FAIL;
    
    apply_production_config(&fields);
    
    char *logBuffer = (char*)calloc(PRODUCTION_LOG_BUFFER_SIZE, 1);
    if(!logBuffer)
    {
        ESP_LOGE(TAG, "No RAM for CmdProd log message");
        return ESP_FAIL;
    }
    
    int result = ESP_FAIL;
    
    switch(fields.orderType)
    {
        case ORDER_START:
            result = handle_production_start(&fields, logBuffer);
            break;
            
        case ORDER_STOP:
            result = handle_production_stop(&fields, logBuffer);
            break;
            
        case ORDER_PAUSE:
            result = handle_production_pause(&fields, logBuffer);
            break;
            
        case ORDER_RESUME:
            result = handle_production_resume(&fields, logBuffer);
            break;
            
        case ORDER_CROP:
            result = handle_production_crop(&fields, logBuffer);
            break;
            
        case ORDER_PARK:
            result = handle_production_park(&fields, logBuffer);
            break;
            
        default:
            ESP_LOGE(MESH_TAG, "Unhandled production order type");
            break;
    }
    
    free(logBuffer);
    return result;
}