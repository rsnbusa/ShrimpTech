#define GLOBAL
#include "includes.h"
#include "globals.h"

/*
 * Handles Modbus battery configuration pushed over mesh. Parses the incoming JSON payload,
 * validates required fields, updates persistent Modbus battery settings, and stores them
 * to flash so the running tasks pick up the new mapping on next read cycle.
 */

extern void write_to_flash();
extern void writeLog(char* que);

#define BATTERY_LOG_BUFFER_SIZE 100

typedef struct
{
    int refresh_rate;
    int inverter_address;
    struct {
        double mux;
        int type;
        int points;
        int start;
        int offset;
    } batTemp, cycles, soh, soc;
} BatteryCommandFields;

static bool parse_channel(cJSON *node, double *mux, int *type, int *points, int *start, int *offset)
{
    if(!node || !cJSON_IsObject(node) || !mux || !type || !points || !start || !offset)
        return false;

    cJSON *muxNode = cJSON_GetObjectItem(node,"mux");
    cJSON *typeNode = cJSON_GetObjectItem(node,"type");
    cJSON *pointsNode = cJSON_GetObjectItem(node,"points");
    cJSON *startNode = cJSON_GetObjectItem(node,"start");
    cJSON *offsetNode = cJSON_GetObjectItem(node,"offset");

    if(!muxNode || !typeNode || !pointsNode || !startNode || !offsetNode)
        return false;

    if(!cJSON_IsNumber(muxNode) || !cJSON_IsNumber(typeNode) || !cJSON_IsNumber(pointsNode) ||
       !cJSON_IsNumber(startNode) || !cJSON_IsNumber(offsetNode))
        return false;

    *mux = muxNode->valuedouble;
    *type = typeNode->valueint;
    *points = pointsNode->valueint;
    *start = startNode->valueint;
    *offset = offsetNode->valueint;
    return true;
}

static bool validate_battery_command(cJSON *batteryCommand, BatteryCommandFields *out)
{
    if(!batteryCommand || !out)
    {
        ESP_LOGE(MESH_TAG,"Battery command argument invalid");
        return false;
    }

    if(!cJSON_IsObject(batteryCommand))
    {
        ESP_LOGE(MESH_TAG,"Battery command payload invalid");
        return false;
    }

    cJSON *general = cJSON_GetObjectItem(batteryCommand,"General");
    if(!general || !cJSON_IsObject(general))
    {
        ESP_LOGE(MESH_TAG,"Battery command missing General block");
        return false;
    }

    cJSON *refreshNode = cJSON_GetObjectItem(general,"refresh");
    cJSON *addrNode = cJSON_GetObjectItem(general,"InverterAddress");
    if(!refreshNode || !addrNode || !cJSON_IsNumber(refreshNode) || !cJSON_IsNumber(addrNode))
    {
        ESP_LOGE(MESH_TAG,"Battery General block incomplete");
        return false;
    }

    cJSON *batTemp = cJSON_GetObjectItem(batteryCommand,"batTemp");
    cJSON *cycles = cJSON_GetObjectItem(batteryCommand,"Cycles");
    cJSON *soh = cJSON_GetObjectItem(batteryCommand,"SOH");
    cJSON *soc = cJSON_GetObjectItem(batteryCommand,"SOC");

    if(!parse_channel(batTemp,&out->batTemp.mux,&out->batTemp.type,&out->batTemp.points,&out->batTemp.start,&out->batTemp.offset) ||
       !parse_channel(cycles,&out->cycles.mux,&out->cycles.type,&out->cycles.points,&out->cycles.start,&out->cycles.offset) ||
       !parse_channel(soh,&out->soh.mux,&out->soh.type,&out->soh.points,&out->soh.start,&out->soh.offset) ||
       !parse_channel(soc,&out->soc.mux,&out->soc.type,&out->soc.points,&out->soc.start,&out->soc.offset))
    {
        ESP_LOGE(MESH_TAG,"Battery command missing or invalid channel blocks");
        return false;
    }

    out->refresh_rate = refreshNode->valueint;
    out->inverter_address = addrNode->valueint;
    return true;
}

static void apply_battery_config(const BatteryCommandFields *fields)
{
    theConf.modbus_battery.refresh_rate = fields->refresh_rate;
    theConf.modbus_battery.batAddress = fields->inverter_address;

    theConf.modbus_battery.tempMux = fields->batTemp.mux;
    theConf.modbus_battery.tempType = fields->batTemp.type;
    theConf.modbus_battery.tempPoints = fields->batTemp.points;
    theConf.modbus_battery.tempStart = fields->batTemp.start;
    theConf.modbus_battery.tempOffset = fields->batTemp.offset;

    theConf.modbus_battery.cycleMux = fields->cycles.mux;
    theConf.modbus_battery.cycleType = fields->cycles.type;
    theConf.modbus_battery.cyclePoints = fields->cycles.points;
    theConf.modbus_battery.cycleStart = fields->cycles.start;
    theConf.modbus_battery.cycleOffset = fields->cycles.offset;

    theConf.modbus_battery.SOHMux = fields->soh.mux;
    theConf.modbus_battery.SOHType = fields->soh.type;
    theConf.modbus_battery.SOHPoints = fields->soh.points;
    theConf.modbus_battery.SOHStart = fields->soh.start;
    theConf.modbus_battery.SOHOffset = fields->soh.offset;

    theConf.modbus_battery.SOCMux = fields->soc.mux;
    theConf.modbus_battery.SOCType = fields->soc.type;
    theConf.modbus_battery.SOCPoints = fields->soc.points;
    theConf.modbus_battery.SOCStart = fields->soc.start;
    theConf.modbus_battery.SOCOffset = fields->soc.offset;

    write_to_flash();
}

static void log_battery_update(const BatteryCommandFields *fields)
{
    char *logBuffer = (char*)calloc(BATTERY_LOG_BUFFER_SIZE,1);
    if(logBuffer)
    {
        snprintf(logBuffer,BATTERY_LOG_BUFFER_SIZE,"Battery cfg addr %d refresh %d", fields->inverter_address, fields->refresh_rate);
        writeLog(logBuffer);
        free(logBuffer);
    }
}

static void dump_battery_config(void)
{
    ESP_LOGI(MESH_TAG,"Battery cfg -> refresh:%d addr:%d", theConf.modbus_battery.refresh_rate, theConf.modbus_battery.batAddress);
    ESP_LOGI(MESH_TAG,"batTemp mux:%.3f type:%d points:%d start:%d offset:%d", theConf.modbus_battery.tempMux, theConf.modbus_battery.tempType, theConf.modbus_battery.tempPoints, theConf.modbus_battery.tempStart, theConf.modbus_battery.tempOffset);
    ESP_LOGI(MESH_TAG,"Cycles mux:%.3f type:%d points:%d start:%d offset:%d", theConf.modbus_battery.cycleMux, theConf.modbus_battery.cycleType, theConf.modbus_battery.cyclePoints, theConf.modbus_battery.cycleStart, theConf.modbus_battery.cycleOffset);
    ESP_LOGI(MESH_TAG,"SOH mux:%.3f type:%d points:%d start:%d offset:%d", theConf.modbus_battery.SOHMux, theConf.modbus_battery.SOHType, theConf.modbus_battery.SOHPoints, theConf.modbus_battery.SOHStart, theConf.modbus_battery.SOHOffset);
    ESP_LOGI(MESH_TAG,"SOC mux:%.3f type:%d points:%d start:%d offset:%d", theConf.modbus_battery.SOCMux, theConf.modbus_battery.SOCType, theConf.modbus_battery.SOCPoints, theConf.modbus_battery.SOCStart, theConf.modbus_battery.SOCOffset);
}

int cmdBattery(void *argument)
{
    ESP_LOGI(MESH_TAG,"Battery Cmd");

    cJSON *batteryCommand = (cJSON *)argument;
    if(batteryCommand == NULL)
    {
        ESP_LOGE(MESH_TAG,"Battery command argument is NULL");
        return ESP_FAIL;
    }

    /*
     * Expected JSON format:

*/

    BatteryCommandFields fields = {0};
    if(!validate_battery_command(batteryCommand, &fields))
        return ESP_FAIL;

    apply_battery_config(&fields);
    ESP_LOGI(MESH_TAG,"Battery configuration updated");
    log_battery_update(&fields);

    if((theConf.debug_flags >> dXCMDS) & 1U)  
        dump_battery_config();

    return ESP_OK;
}