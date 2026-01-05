#define GLOBAL
#include "includes.h"
#include "globals.h"

/*
 * Handles Modbus sensors configuration pushed over mesh. Parses the incoming JSON payload,
 * validates required fields, updates persistent Modbus sensor settings, and stores them
 * to flash so the running tasks pick up the new mapping on next read cycle.
 */

extern void write_to_flash();
extern void writeLog(char* que);

#define SENSORS_LOG_BUFFER_SIZE 100

typedef struct
{
    int refresh_rate;
    struct {
        int address;
        double mux;
        int type;
        int points;
        int start;
        int offset;
    } doCh, phCh, waterTempCh, ambientCh, humidityCh;
} SensorsCommandFields;

static bool parse_channel_with_address(cJSON *node, int *address, double *mux, int *type, int *points, int *start, int *offset)
{
    if(!node || !cJSON_IsObject(node) || !address || !mux || !type || !points || !start || !offset)
        return false;

    cJSON *addrNode = cJSON_GetObjectItem(node,"address");
    cJSON *muxNode = cJSON_GetObjectItem(node,"mux");
    cJSON *typeNode = cJSON_GetObjectItem(node,"type");
    cJSON *pointsNode = cJSON_GetObjectItem(node,"points");
    cJSON *startNode = cJSON_GetObjectItem(node,"start");
    cJSON *offsetNode = cJSON_GetObjectItem(node,"offset");

    if(!addrNode || !muxNode || !typeNode || !pointsNode || !startNode || !offsetNode)
        return false;

    if(!cJSON_IsNumber(addrNode) || !cJSON_IsNumber(muxNode) || !cJSON_IsNumber(typeNode) || !cJSON_IsNumber(pointsNode) ||
       !cJSON_IsNumber(startNode) || !cJSON_IsNumber(offsetNode))
        return false;

    *address = addrNode->valueint;
    *mux = muxNode->valuedouble;
    *type = typeNode->valueint;
    *points = pointsNode->valueint;
    *start = startNode->valueint;
    *offset = offsetNode->valueint;
    return true;
}

static bool validate_sensors_command(cJSON *sensorsCommand, SensorsCommandFields *out)
{
    if(!sensorsCommand || !out)
    {
        ESP_LOGE(MESH_TAG,"Sensors command argument invalid");
        return false;
    }

    if(!cJSON_IsObject(sensorsCommand))
    {
        ESP_LOGE(MESH_TAG,"Sensors command payload invalid");
        return false;
    }

    cJSON *general = cJSON_GetObjectItem(sensorsCommand,"General");
    if(!general || !cJSON_IsObject(general))
    {
        ESP_LOGE(MESH_TAG,"Sensors command missing General block");
        return false;
    }

    cJSON *refreshNode = cJSON_GetObjectItem(general,"refresh");
    if(!refreshNode || !cJSON_IsNumber(refreshNode))
    {
        ESP_LOGE(MESH_TAG,"Sensors General block incomplete");
        return false;
    }

    cJSON *doCh = cJSON_GetObjectItem(sensorsCommand,"DO");
    cJSON *phCh = cJSON_GetObjectItem(sensorsCommand,"PH");
    cJSON *waterTempCh = cJSON_GetObjectItem(sensorsCommand,"WaterTemp");
    cJSON *ambientCh = cJSON_GetObjectItem(sensorsCommand,"Ambient");
    cJSON *humidityCh = cJSON_GetObjectItem(sensorsCommand,"Humidity");

    if(!parse_channel_with_address(doCh,&out->doCh.address,&out->doCh.mux,&out->doCh.type,&out->doCh.points,&out->doCh.start,&out->doCh.offset) ||
       !parse_channel_with_address(phCh,&out->phCh.address,&out->phCh.mux,&out->phCh.type,&out->phCh.points,&out->phCh.start,&out->phCh.offset) ||
       !parse_channel_with_address(waterTempCh,&out->waterTempCh.address,&out->waterTempCh.mux,&out->waterTempCh.type,&out->waterTempCh.points,&out->waterTempCh.start,&out->waterTempCh.offset) ||
       !parse_channel_with_address(ambientCh,&out->ambientCh.address,&out->ambientCh.mux,&out->ambientCh.type,&out->ambientCh.points,&out->ambientCh.start,&out->ambientCh.offset) ||
       !parse_channel_with_address(humidityCh,&out->humidityCh.address,&out->humidityCh.mux,&out->humidityCh.type,&out->humidityCh.points,&out->humidityCh.start,&out->humidityCh.offset))
    {
        ESP_LOGE(MESH_TAG,"Sensors command missing or invalid channel blocks");
        return false;
    }

    out->refresh_rate = refreshNode->valueint;
    return true;
}

static void apply_sensors_config(const SensorsCommandFields *fields)
{
    theConf.modbus_sensors.refresh_rate = fields->refresh_rate;

    theConf.modbus_sensors.DOMux = fields->doCh.mux;
    theConf.modbus_sensors.DOType = fields->doCh.type;
    theConf.modbus_sensors.DOPoints = fields->doCh.points;
    theConf.modbus_sensors.DOStart = fields->doCh.start;
    theConf.modbus_sensors.DOOffset = fields->doCh.offset;
    theConf.modbus_sensors.DOAddress = fields->doCh.address;

    theConf.modbus_sensors.PHMux = fields->phCh.mux;
    theConf.modbus_sensors.PHType = fields->phCh.type;
    theConf.modbus_sensors.PHPoints = fields->phCh.points;
    theConf.modbus_sensors.PHStart = fields->phCh.start;
    theConf.modbus_sensors.PHOffset = fields->phCh.offset;
    theConf.modbus_sensors.PHAddress = fields->phCh.address;

    theConf.modbus_sensors.WMux = fields->waterTempCh.mux;
    theConf.modbus_sensors.WType = fields->waterTempCh.type;
    theConf.modbus_sensors.WPoints = fields->waterTempCh.points;
    theConf.modbus_sensors.WStart = fields->waterTempCh.start;
    theConf.modbus_sensors.WOffset = fields->waterTempCh.offset;
    theConf.modbus_sensors.WAddress = fields->waterTempCh.address;

    theConf.modbus_sensors.AMux = fields->ambientCh.mux;
    theConf.modbus_sensors.AType = fields->ambientCh.type;
    theConf.modbus_sensors.APoints = fields->ambientCh.points;
    theConf.modbus_sensors.AStart = fields->ambientCh.start;
    theConf.modbus_sensors.AOffset = fields->ambientCh.offset;
    theConf.modbus_sensors.AAddress = fields->ambientCh.address;

    theConf.modbus_sensors.humMux = fields->humidityCh.mux;
    theConf.modbus_sensors.humType = fields->humidityCh.type;
    theConf.modbus_sensors.humPoints = fields->humidityCh.points;
    theConf.modbus_sensors.humStart = fields->humidityCh.start;
    theConf.modbus_sensors.HumOffset = fields->humidityCh.offset;
    theConf.modbus_sensors.HAddress = fields->humidityCh.address;

    // write_to_flash();
}

static void log_sensors_update(const SensorsCommandFields *fields)
{
    char *logBuffer = (char*)calloc(SENSORS_LOG_BUFFER_SIZE,1);
    if(logBuffer)
    {
        snprintf(logBuffer,SENSORS_LOG_BUFFER_SIZE,"Sensors cfg refresh %d", fields->refresh_rate);
        writeLog(logBuffer);
        free(logBuffer);
    }
}

static void dump_sensors_config(void)
{
    ESP_LOGI(MESH_TAG,"Sensors cfg -> refresh:%d", theConf.modbus_sensors.refresh_rate);
    ESP_LOGI(MESH_TAG,"DO addr:%d mux:%.3f type:%d points:%d start:%d offset:%d", theConf.modbus_sensors.DOAddress, theConf.modbus_sensors.DOMux, theConf.modbus_sensors.DOType, theConf.modbus_sensors.DOPoints, theConf.modbus_sensors.DOStart, theConf.modbus_sensors.DOOffset);
    ESP_LOGI(MESH_TAG,"PH addr:%d mux:%.3f type:%d points:%d start:%d offset:%d", theConf.modbus_sensors.PHAddress, theConf.modbus_sensors.PHMux, theConf.modbus_sensors.PHType, theConf.modbus_sensors.PHPoints, theConf.modbus_sensors.PHStart, theConf.modbus_sensors.PHOffset);
    ESP_LOGI(MESH_TAG,"WaterTemp addr:%d mux:%.3f type:%d points:%d start:%d offset:%d", theConf.modbus_sensors.WAddress, theConf.modbus_sensors.WMux, theConf.modbus_sensors.WType, theConf.modbus_sensors.WPoints, theConf.modbus_sensors.WStart, theConf.modbus_sensors.WOffset);
    ESP_LOGI(MESH_TAG,"Ambient addr:%d mux:%.3f type:%d points:%d start:%d offset:%d", theConf.modbus_sensors.AAddress, theConf.modbus_sensors.AMux, theConf.modbus_sensors.AType, theConf.modbus_sensors.APoints, theConf.modbus_sensors.AStart, theConf.modbus_sensors.AOffset);
    ESP_LOGI(MESH_TAG,"Humidity addr:%d mux:%.3f type:%d points:%d start:%d offset:%d", theConf.modbus_sensors.HAddress, theConf.modbus_sensors.humMux, theConf.modbus_sensors.humType, theConf.modbus_sensors.humPoints, theConf.modbus_sensors.humStart, theConf.modbus_sensors.HumOffset);
}

int cmdSensors(void *argument)
{
    ESP_LOGI(MESH_TAG,"Sensors Cmd");

    cJSON *sensorsCommand = (cJSON *)argument;
    if(sensorsCommand == NULL)
    {
        ESP_LOGE(MESH_TAG,"Sensors command argument is NULL");
        return ESP_FAIL;
    }

    /*
     * Expected JSON format:
{"cmdarr":[{"cmd":"Sensors","General":{"refresh":10},"DO":{"address":16,"mux":1,"type":1,"points":10,"start":0,"offset":0},"PH":{"address":17,"mux":1,"type":1,"points":10,"start":20,"offset":0},
"WaterTemp":{"address":18,"mux":1,"type":1,"points":10,"start":40,"offset":0},"Ambient":{"address":19,"mux":1,"type":1,"points":10,"start":60,"offset":0},
"Humidity":{"address":20,"mux":1,"type":1,"points":10,"start":80,"offset":0}}]}
*/

    SensorsCommandFields fields = {0};
    if(!validate_sensors_command(sensorsCommand, &fields))
        return ESP_FAIL;

    apply_sensors_config(&fields);
    ESP_LOGI(MESH_TAG,"Sensors configuration updated");
    log_sensors_update(&fields);

    if((theConf.debug_flags >> dXCMDS) & 1U)  
        dump_sensors_config();

    return ESP_OK;
}