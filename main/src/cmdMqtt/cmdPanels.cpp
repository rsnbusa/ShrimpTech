#define GLOBAL
#include "includes.h"
#include "globals.h"

/*
 * Handles Modbus panels configuration pushed over mesh. Parses the incoming JSON payload,
 * validates required fields, updates persistent Modbus panel settings, and stores them
 * to flash so the running tasks pick up the new mapping on next read cycle.
 */

extern void write_to_flash();
extern void writeLog(const char* que);

#define PANELS_LOG_BUFFER_SIZE 100

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
    } pv2amps, pv2volts, pv1amps, pv1volts, charge;
} PanelsCommandFields;

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

static bool validate_panels_command(cJSON *panelsCommand, PanelsCommandFields *out)
{
    if(!panelsCommand || !out)
    {
        MESP_LOGE(MESH_TAG,"Panels command argument invalid");
        return false;
    }

    if(!cJSON_IsObject(panelsCommand))
    {
        MESP_LOGE(MESH_TAG,"Panels command payload invalid");
        return false;
    }

    cJSON *general = cJSON_GetObjectItem(panelsCommand,"General");
    if(!general || !cJSON_IsObject(general))
    {
        MESP_LOGE(MESH_TAG,"Panels command missing General block");
        return false;
    }

    cJSON *refreshNode = cJSON_GetObjectItem(general,"refresh");
    cJSON *addrNode = cJSON_GetObjectItem(general,"InverterAddress");
    if(!refreshNode || !addrNode || !cJSON_IsNumber(refreshNode) || !cJSON_IsNumber(addrNode))
    {
        MESP_LOGE(MESH_TAG,"Panels General block incomplete");
        return false;
    }

    cJSON *pv2amps = cJSON_GetObjectItem(panelsCommand,"pv2amps");
    cJSON *pv2volts = cJSON_GetObjectItem(panelsCommand,"pv2volts");
    cJSON *pv1amps = cJSON_GetObjectItem(panelsCommand,"pv1amps");
    cJSON *pv1volts = cJSON_GetObjectItem(panelsCommand,"pv1volts");
    cJSON *charge = cJSON_GetObjectItem(panelsCommand,"charge");

    if(!parse_channel(pv2amps,&out->pv2amps.mux,&out->pv2amps.type,&out->pv2amps.points,&out->pv2amps.start,&out->pv2amps.offset) ||
       !parse_channel(pv2volts,&out->pv2volts.mux,&out->pv2volts.type,&out->pv2volts.points,&out->pv2volts.start,&out->pv2volts.offset) ||
       !parse_channel(pv1amps,&out->pv1amps.mux,&out->pv1amps.type,&out->pv1amps.points,&out->pv1amps.start,&out->pv1amps.offset) ||
       !parse_channel(pv1volts,&out->pv1volts.mux,&out->pv1volts.type,&out->pv1volts.points,&out->pv1volts.start,&out->pv1volts.offset) ||
       !parse_channel(charge,&out->charge.mux,&out->charge.type,&out->charge.points,&out->charge.start,&out->charge.offset))
    {
        MESP_LOGE(MESH_TAG,"Panels command missing or invalid channel blocks");
        return false;
    }

    out->refresh_rate = refreshNode->valueint;
    out->inverter_address = addrNode->valueint;
    return true;
}

static void apply_panels_config(const PanelsCommandFields *fields)
{
    theConf.modbus_panels.refresh_rate = fields->refresh_rate;
    theConf.modbus_panels.PVAddress = fields->inverter_address;

    theConf.modbus_panels.PV2AmpsMux = fields->pv2amps.mux;
    theConf.modbus_panels.PV2AmpsType = fields->pv2amps.type;
    theConf.modbus_panels.PV2AmpsPoints = fields->pv2amps.points;
    theConf.modbus_panels.PV2AmpsStart = fields->pv2amps.start;
    theConf.modbus_panels.PV2AmpsOffset = fields->pv2amps.offset;

    theConf.modbus_panels.PV2VMux = fields->pv2volts.mux;
    theConf.modbus_panels.PV2VoltsType = fields->pv2volts.type;
    theConf.modbus_panels.PV2VPoints = fields->pv2volts.points;
    theConf.modbus_panels.PV2VStart = fields->pv2volts.start;
    theConf.modbus_panels.PV2VoltsOffset = fields->pv2volts.offset;

    theConf.modbus_panels.PV1AmpsMux = fields->pv1amps.mux;
    theConf.modbus_panels.PV1AmpsType = fields->pv1amps.type;
    theConf.modbus_panels.PV1AmpsPoints = fields->pv1amps.points;
    theConf.modbus_panels.PV1AmpsStart = fields->pv1amps.start;
    theConf.modbus_panels.PV1_AmpsOffset = fields->pv1amps.offset;

    theConf.modbus_panels.PV1VMux = fields->pv1volts.mux;
    theConf.modbus_panels.PV1VType = fields->pv1volts.type;
    theConf.modbus_panels.PV1VPoints = fields->pv1volts.points;
    theConf.modbus_panels.PV1VStart = fields->pv1volts.start;
    theConf.modbus_panels.PV1VoltsOffset = fields->pv1volts.offset;

    theConf.modbus_panels.ChargeMux = fields->charge.mux;
    theConf.modbus_panels.ChargeType = fields->charge.type;
    theConf.modbus_panels.ChargePoints = fields->charge.points;
    theConf.modbus_panels.ChargeStart = fields->charge.start;
    theConf.modbus_panels.Charge_StateOffset = fields->charge.offset;

    write_to_flash();
}

static void log_panels_update(const PanelsCommandFields *fields)
{
    char *logBuffer = (char*)calloc(PANELS_LOG_BUFFER_SIZE,1);
    if(logBuffer)
    {
        snprintf(logBuffer,PANELS_LOG_BUFFER_SIZE,"Panels cfg addr %d refresh %d", fields->inverter_address, fields->refresh_rate);
        writeLog(logBuffer);
        free(logBuffer);
    }
}

static void dump_panels_config(void)
{
    MESP_LOGI(MESH_TAG,"Panels cfg -> refresh:%d addr:%d", theConf.modbus_panels.refresh_rate, theConf.modbus_panels.PVAddress);
    MESP_LOGI(MESH_TAG,"PV2Amps mux:%.3f type:%d points:%d start:%d offset:%d", theConf.modbus_panels.PV2AmpsMux, theConf.modbus_panels.PV2AmpsType, theConf.modbus_panels.PV2AmpsPoints, theConf.modbus_panels.PV2AmpsStart, theConf.modbus_panels.PV2AmpsOffset);
    MESP_LOGI(MESH_TAG,"PV2Volts mux:%.3f type:%d points:%d start:%d offset:%d", theConf.modbus_panels.PV2VMux, theConf.modbus_panels.PV2VoltsType, theConf.modbus_panels.PV2VPoints, theConf.modbus_panels.PV2VStart, theConf.modbus_panels.PV2VoltsOffset);
    MESP_LOGI(MESH_TAG,"PV1Amps mux:%.3f type:%d points:%d start:%d offset:%d", theConf.modbus_panels.PV1AmpsMux, theConf.modbus_panels.PV1AmpsType, theConf.modbus_panels.PV1AmpsPoints, theConf.modbus_panels.PV1AmpsStart, theConf.modbus_panels.PV1_AmpsOffset);
    MESP_LOGI(MESH_TAG,"PV1Volts mux:%.3f type:%d points:%d start:%d offset:%d", theConf.modbus_panels.PV1VMux, theConf.modbus_panels.PV1VType, theConf.modbus_panels.PV1VPoints, theConf.modbus_panels.PV1VStart, theConf.modbus_panels.PV1VoltsOffset);
    MESP_LOGI(MESH_TAG,"Charge mux:%.3f type:%d points:%d start:%d offset:%d", theConf.modbus_panels.ChargeMux, theConf.modbus_panels.ChargeType, theConf.modbus_panels.ChargePoints, theConf.modbus_panels.ChargeStart, theConf.modbus_panels.Charge_StateOffset);
}

int cmdPanels(void *argument)
{
    MESP_LOGI(MESH_TAG,"Panels Cmd");

    cJSON *panelsCommand = (cJSON *)argument;
    if(panelsCommand == NULL)
    {
        MESP_LOGE(MESH_TAG,"Panels command argument is NULL");
        return ESP_FAIL;
    }

    /*
     * Expected JSON format:
{"cmd":"Panels","General":{"refresh":10,"InverterAddress":1},
"pv2amps":{"mux":1,"type":1,"points":10,"start":0,"offset":0},
"pv2volts":{"mux":1,"type":1,"points":10,"start":20,"offset":0},
"pv1amps":{"mux":1,"type":1,"points":10,"start":40,"offset":0},
"pv1volts":{"mux":1,"type":1,"points":10,"start":60,"offset":0},
"charge":{"mux":1,"type":1,"points":10,"start":80,"offset":0}}
*/

    PanelsCommandFields fields = {};
    if(!validate_panels_command(panelsCommand, &fields))
        return ESP_FAIL;

    apply_panels_config(&fields);
    MESP_LOGI(MESH_TAG,"Panels configuration updated");
    log_panels_update(&fields);

    if((theConf.debug_flags >> dXCMDS) & 1U)  
        dump_panels_config();

    return ESP_OK;
}