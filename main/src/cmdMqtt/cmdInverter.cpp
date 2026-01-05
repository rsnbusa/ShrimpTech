#define GLOBAL
#include "includes.h"
#include "globals.h"

/*
 * Handles Modbus inverter configuration pushed over mesh. Parses the incoming JSON payload,
 * validates required fields, updates persistent Modbus inverter settings, and stores them
 * to flash so the running tasks pick up the new mapping on next read cycle.
 */

extern void write_to_flash();
extern void writeLog(char* que);

#define INVERTER_LOG_BUFFER_SIZE 100

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
    } loadUsedHoy, batDscHoy, batChdHoy, loadUsedTotal, usedkwhHoy, genkWhHoy, batDscTotal, batChgTotal, batChHoy;
} InverterCommandFields;

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

static bool validate_inverter_command(cJSON *inverterCommand, InverterCommandFields *out)
{
    if(!inverterCommand || !out)
    {
        ESP_LOGE(MESH_TAG,"Inverter command argument invalid");
        return false;
    }

    if(!cJSON_IsObject(inverterCommand))
    {
        ESP_LOGE(MESH_TAG,"Inverter command payload invalid");
        return false;
    }

    cJSON *general = cJSON_GetObjectItem(inverterCommand,"General");
    if(!general || !cJSON_IsObject(general))
    {
        ESP_LOGE(MESH_TAG,"Inverter command missing General block");
        return false;
    }

    cJSON *refreshNode = cJSON_GetObjectItem(general,"refresh");
    cJSON *addrNode = cJSON_GetObjectItem(general,"InverterAddress");
    if(!refreshNode || !addrNode || !cJSON_IsNumber(refreshNode) || !cJSON_IsNumber(addrNode))
    {
        ESP_LOGE(MESH_TAG,"Inverter General block incomplete");
        return false;
    }

    cJSON *loadUsedHoy = cJSON_GetObjectItem(inverterCommand,"LoadUsedHoy"); //
    cJSON *batDscHoy = cJSON_GetObjectItem(inverterCommand,"BatDscHoy");
    cJSON *batChdHoy = cJSON_GetObjectItem(inverterCommand,"BatChdHoy");
    cJSON *loadUsedTotal = cJSON_GetObjectItem(inverterCommand,"LoadUsedTotal"); //
    cJSON *usedkwhHoy = cJSON_GetObjectItem(inverterCommand,"UsedkwhHoy");
    cJSON *genkWhHoy = cJSON_GetObjectItem(inverterCommand,"GenkWhHoy");
    cJSON *batDscTotal = cJSON_GetObjectItem(inverterCommand,"BatDscTotal");
    cJSON *batChgTotal = cJSON_GetObjectItem(inverterCommand,"BatChgTotal");
    cJSON *batChHoy = cJSON_GetObjectItem(inverterCommand,"BatChHoy");

    if(!parse_channel(loadUsedHoy,&out->loadUsedHoy.mux,&out->loadUsedHoy.type,&out->loadUsedHoy.points,&out->loadUsedHoy.start,&out->loadUsedHoy.offset) ||
       !parse_channel(batDscHoy,&out->batDscHoy.mux,&out->batDscHoy.type,&out->batDscHoy.points,&out->batDscHoy.start,&out->batDscHoy.offset) ||
       !parse_channel(batChdHoy,&out->batChdHoy.mux,&out->batChdHoy.type,&out->batChdHoy.points,&out->batChdHoy.start,&out->batChdHoy.offset) ||
       !parse_channel(loadUsedTotal,&out->loadUsedTotal.mux,&out->loadUsedTotal.type,&out->loadUsedTotal.points,&out->loadUsedTotal.start,&out->loadUsedTotal.offset) ||
       !parse_channel(usedkwhHoy,&out->usedkwhHoy.mux,&out->usedkwhHoy.type,&out->usedkwhHoy.points,&out->usedkwhHoy.start,&out->usedkwhHoy.offset) ||
       !parse_channel(genkWhHoy,&out->genkWhHoy.mux,&out->genkWhHoy.type,&out->genkWhHoy.points,&out->genkWhHoy.start,&out->genkWhHoy.offset) ||
       !parse_channel(batDscTotal,&out->batDscTotal.mux,&out->batDscTotal.type,&out->batDscTotal.points,&out->batDscTotal.start,&out->batDscTotal.offset) ||
       !parse_channel(batChgTotal,&out->batChgTotal.mux,&out->batChgTotal.type,&out->batChgTotal.points,&out->batChgTotal.start,&out->batChgTotal.offset) ||
       !parse_channel(batChHoy,&out->batChHoy.mux,&out->batChHoy.type,&out->batChHoy.points,&out->batChHoy.start,&out->batChHoy.offset))
    {
        ESP_LOGE(MESH_TAG,"Inverter command missing or invalid channel blocks");
        return false;
    }

    out->refresh_rate = refreshNode->valueint;
    out->inverter_address = addrNode->valueint;
    return true;
}

static void apply_inverter_config(const InverterCommandFields *fields)
{
    theConf.modbus_inverter.refresh_rate = fields->refresh_rate;
    theConf.modbus_inverter.InverterAddress = fields->inverter_address;

    theConf.modbus_inverter.I10_LoadUsedHoyMux = fields->loadUsedHoy.mux;
    theConf.modbus_inverter.I10_LoadUsedHoyType = fields->loadUsedHoy.type;
    theConf.modbus_inverter.I10_LoadUsedHoyPoints = fields->loadUsedHoy.points;
    theConf.modbus_inverter.I10_LoadUsedHoyStart = fields->loadUsedHoy.start;
    theConf.modbus_inverter.I10_LoadUsedHoyOff = fields->loadUsedHoy.offset;

    theConf.modbus_inverter.I9_BatDscHoyMux = fields->batDscHoy.mux;
    theConf.modbus_inverter.I9_BatDscHoyType = fields->batDscHoy.type;
    theConf.modbus_inverter.I9_BatDscHoyPoints = fields->batDscHoy.points;
    theConf.modbus_inverter.I9_BatDscHoyStart = fields->batDscHoy.start;
    theConf.modbus_inverter.I9_BatDscHoyOff = fields->batDscHoy.offset;

    theConf.modbus_inverter.I8_BatChdHoyMux = fields->batChdHoy.mux;
    theConf.modbus_inverter.I8_BatChdHoyType = fields->batChdHoy.type;
    theConf.modbus_inverter.I8_BatChdHoyPoints = fields->batChdHoy.points;
    theConf.modbus_inverter.I8_BatChdHoyStart = fields->batChdHoy.start;
    theConf.modbus_inverter.I8_BatChdHoyOff = fields->batChdHoy.offset;

    theConf.modbus_inverter.I7_LoadUsedTotalMux = fields->loadUsedTotal.mux;
    theConf.modbus_inverter.I7_LoadUsedTotalType = fields->loadUsedTotal.type;
    theConf.modbus_inverter.I7_LoadUsedTotalPoints = fields->loadUsedTotal.points;
    theConf.modbus_inverter.I7_LoadUsedTotalStart = fields->loadUsedTotal.start;
    theConf.modbus_inverter.I7_LoadUsedTotalOff = fields->loadUsedTotal.offset;

    theConf.modbus_inverter.I6_UsedkwhHoyMux = fields->usedkwhHoy.mux;
    theConf.modbus_inverter.I6_UsedkwhHoyType = fields->usedkwhHoy.type;
    theConf.modbus_inverter.I6_UsedkwhHoyPoints = fields->usedkwhHoy.points;
    theConf.modbus_inverter.I6_UsedkwhHoyStart = fields->usedkwhHoy.start;
    theConf.modbus_inverter.I6_UsedkwhHoyOff = fields->usedkwhHoy.offset;

    theConf.modbus_inverter.I5_GenkWhHoyMux = fields->genkWhHoy.mux;
    theConf.modbus_inverter.I5_GenkWhHoyType = fields->genkWhHoy.type;
    theConf.modbus_inverter.I5_GenkWhHoyPoints = fields->genkWhHoy.points;
    theConf.modbus_inverter.I5_GenkWhHoyStart = fields->genkWhHoy.start;
    theConf.modbus_inverter.I5_GenkWhHoyOff = fields->genkWhHoy.offset;

    theConf.modbus_inverter.I4_BatDscTotalMux = fields->batDscTotal.mux;
    theConf.modbus_inverter.I4_BatDscTotalType = fields->batDscTotal.type;
    theConf.modbus_inverter.I4_BatDscTotalPoints = fields->batDscTotal.points;
    theConf.modbus_inverter.I4_BatDscTotalStart = fields->batDscTotal.start;
    theConf.modbus_inverter.I4_BatDscTotalOff = fields->batDscTotal.offset;

    theConf.modbus_inverter.I3_BatChgTotalMux = fields->batChgTotal.mux;
    theConf.modbus_inverter.I3_BatChgTotalType = fields->batChgTotal.type;
    theConf.modbus_inverter.I3_BatChgTotalPoints = fields->batChgTotal.points;
    theConf.modbus_inverter.I3_BatChgTotalStart = fields->batChgTotal.start;
    theConf.modbus_inverter.I3_BatChgTotalOff = fields->batChgTotal.offset;

    theConf.modbus_inverter.I2_BatDscHoyMux = fields->batDscHoy.mux;
    theConf.modbus_inverter.I2_BatDscHoyType = fields->batDscHoy.type;
    theConf.modbus_inverter.I2_BatDscHoyPoints = fields->batDscHoy.points;
    theConf.modbus_inverter.I2_BatDscHoyStart = fields->batDscHoy.start;
    theConf.modbus_inverter.I2_BatDscHoyOff = fields->batDscHoy.offset;

    theConf.modbus_inverter.I1_BatChHoyMux = fields->batChHoy.mux;
    theConf.modbus_inverter.I1_BatChHoyType = fields->batChHoy.type;
    theConf.modbus_inverter.I1_BatChHoyPoints = fields->batChHoy.points;
    theConf.modbus_inverter.I1_BatChHoyStart = fields->batChHoy.start;
    theConf.modbus_inverter.I1_BatChHoyOff = fields->batChHoy.offset;

    write_to_flash();
}

static void log_inverter_update(const InverterCommandFields *fields)
{
    char *logBuffer = (char*)calloc(INVERTER_LOG_BUFFER_SIZE,1);
    if(logBuffer)
    {
        snprintf(logBuffer,INVERTER_LOG_BUFFER_SIZE,"Inverter cfg addr %d refresh %d", fields->inverter_address, fields->refresh_rate);
        writeLog(logBuffer);
        free(logBuffer);
    }
}

static void dump_inverter_config(void)
{
    ESP_LOGI(MESH_TAG,"Inverter cfg -> refresh:%d addr:%d", theConf.modbus_inverter.refresh_rate, theConf.modbus_inverter.InverterAddress);
    ESP_LOGI(MESH_TAG,"LoadUsedHoy mux:%.3f type:%d points:%d start:%d offset:%d", theConf.modbus_inverter.I10_LoadUsedHoyMux, theConf.modbus_inverter.I10_LoadUsedHoyType, theConf.modbus_inverter.I10_LoadUsedHoyPoints, theConf.modbus_inverter.I10_LoadUsedHoyStart, theConf.modbus_inverter.I10_LoadUsedHoyOff);
    ESP_LOGI(MESH_TAG,"BatDscHoy mux:%.3f type:%d points:%d start:%d offset:%d", theConf.modbus_inverter.I9_BatDscHoyMux, theConf.modbus_inverter.I9_BatDscHoyType, theConf.modbus_inverter.I9_BatDscHoyPoints, theConf.modbus_inverter.I9_BatDscHoyStart, theConf.modbus_inverter.I9_BatDscHoyOff);
    ESP_LOGI(MESH_TAG,"BatChdHoy mux:%.3f type:%d points:%d start:%d offset:%d", theConf.modbus_inverter.I8_BatChdHoyMux, theConf.modbus_inverter.I8_BatChdHoyType, theConf.modbus_inverter.I8_BatChdHoyPoints, theConf.modbus_inverter.I8_BatChdHoyStart, theConf.modbus_inverter.I8_BatChdHoyOff);
    ESP_LOGI(MESH_TAG,"LoadUsedTotal mux:%.3f type:%d points:%d start:%d offset:%d", theConf.modbus_inverter.I7_LoadUsedTotalMux, theConf.modbus_inverter.I7_LoadUsedTotalType, theConf.modbus_inverter.I7_LoadUsedTotalPoints, theConf.modbus_inverter.I7_LoadUsedTotalStart, theConf.modbus_inverter.I7_LoadUsedTotalOff);
    ESP_LOGI(MESH_TAG,"UsedkwhHoy mux:%.3f type:%d points:%d start:%d offset:%d", theConf.modbus_inverter.I6_UsedkwhHoyMux, theConf.modbus_inverter.I6_UsedkwhHoyType, theConf.modbus_inverter.I6_UsedkwhHoyPoints, theConf.modbus_inverter.I6_UsedkwhHoyStart, theConf.modbus_inverter.I6_UsedkwhHoyOff);
    ESP_LOGI(MESH_TAG,"GenkWhHoy mux:%.3f type:%d points:%d start:%d offset:%d", theConf.modbus_inverter.I5_GenkWhHoyMux, theConf.modbus_inverter.I5_GenkWhHoyType, theConf.modbus_inverter.I5_GenkWhHoyPoints, theConf.modbus_inverter.I5_GenkWhHoyStart, theConf.modbus_inverter.I5_GenkWhHoyOff);
    ESP_LOGI(MESH_TAG,"BatDscTotal mux:%.3f type:%d points:%d start:%d offset:%d", theConf.modbus_inverter.I4_BatDscTotalMux, theConf.modbus_inverter.I4_BatDscTotalType, theConf.modbus_inverter.I4_BatDscTotalPoints, theConf.modbus_inverter.I4_BatDscTotalStart, theConf.modbus_inverter.I4_BatDscTotalOff);
    ESP_LOGI(MESH_TAG,"BatChgTotal mux:%.3f type:%d points:%d start:%d offset:%d", theConf.modbus_inverter.I3_BatChgTotalMux, theConf.modbus_inverter.I3_BatChgTotalType, theConf.modbus_inverter.I3_BatChgTotalPoints, theConf.modbus_inverter.I3_BatChgTotalStart, theConf.modbus_inverter.I3_BatChgTotalOff);
    ESP_LOGI(MESH_TAG,"BatDscHoy2 mux:%.3f type:%d points:%d start:%d offset:%d", theConf.modbus_inverter.I2_BatDscHoyMux, theConf.modbus_inverter.I2_BatDscHoyType, theConf.modbus_inverter.I2_BatDscHoyPoints, theConf.modbus_inverter.I2_BatDscHoyStart, theConf.modbus_inverter.I2_BatDscHoyOff);
    ESP_LOGI(MESH_TAG,"BatChHoy mux:%.3f type:%d points:%d start:%d offset:%d", theConf.modbus_inverter.I1_BatChHoyMux, theConf.modbus_inverter.I1_BatChHoyType, theConf.modbus_inverter.I1_BatChHoyPoints, theConf.modbus_inverter.I1_BatChHoyStart, theConf.modbus_inverter.I1_BatChHoyOff);
}

int cmdInverter(void *argument)
{
    ESP_LOGI(MESH_TAG,"Inverter Cmd");

    cJSON *inverterCommand = (cJSON *)argument;
    if(inverterCommand == NULL)
    {
        ESP_LOGE(MESH_TAG,"Inverter command argument is NULL");
        return ESP_FAIL;
    }

    /*
     * Expected JSON format:
{"cmd":"Inverter","General":{"refresh":10,"InverterAddress":1},"LoadUsedHoy":{"mux":1,"type":1,"points":10,"start":0,"offset":0},"BatDscHoy":{"mux":1,"type":1,"points":10,"start":80,"offset":0},"BatChdHoy":{"mux":1,"type":1,"points":10,"start":40,"offset":0},"LoadUsedTotal":{"mux":1,"type":1,"points":10,"start":60,"offset":0},"UsedkwhHoy":{"mux":1,"type":1,"points":10,"start":80,"offset":0},"GenkWhHoy":{"mux":1,"type":1,"points":10,"start":80,"offset":0},"BatDscTotal":{"mux":1,"type":1,"points":10,"start":80,"offset":0},"BatChgTotal":{"mux":1,"type":1,"points":10,"start":80,"offset":0},"BatChHoy":{"mux":1,"type":1,"points":10,"start":80,"offset":0}}

*/

    InverterCommandFields fields = {0};
    if(!validate_inverter_command(inverterCommand, &fields))
        return ESP_FAIL;

    apply_inverter_config(&fields);
    ESP_LOGI(MESH_TAG,"Inverter configuration updated");
    log_inverter_update(&fields);
    
    if((theConf.debug_flags >> dXCMDS) & 1U)  
        dump_inverter_config();

    return ESP_OK;
}