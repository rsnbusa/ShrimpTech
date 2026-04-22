#define GLOBAL
#include "includes.h"
#include "globals.h"

extern void write_to_flash();
extern void writeLog(const char* que);

#define INVERT_STATUS_LOG_BUFFER_SIZE 100

typedef struct
{
    int refresh;
    int address;
    double mux;
    int type;
    int points;
    int start;
    int offset;
    int unitid;
} InvertStatusCommandFields;

static bool validate_invert_status_command(cJSON *invertStatusCommand, InvertStatusCommandFields *out)
{
    if(!invertStatusCommand || !out)
    {
        MESP_LOGE(MESH_TAG, "InvertStatus command argument invalid");
        return false;
    }

    if(!cJSON_IsObject(invertStatusCommand))
    {
        MESP_LOGE(MESH_TAG, "InvertStatus command payload invalid");
        return false;
    }

    cJSON *refreshNode = cJSON_GetObjectItem(invertStatusCommand, "refresh");
    cJSON *addressNode = cJSON_GetObjectItem(invertStatusCommand, "address");
    cJSON *muxNode = cJSON_GetObjectItem(invertStatusCommand, "invstatusmux");
    cJSON *typeNode = cJSON_GetObjectItem(invertStatusCommand, "invstatustype");
    cJSON *pointsNode = cJSON_GetObjectItem(invertStatusCommand, "invstatuspoints");
    cJSON *startNode = cJSON_GetObjectItem(invertStatusCommand, "invstatusstart");
    cJSON *offsetNode = cJSON_GetObjectItem(invertStatusCommand, "invstatusoff");
    cJSON *unitidNode = cJSON_GetObjectItem(invertStatusCommand, "unitid");

    if(!refreshNode || !addressNode || !muxNode || !typeNode || !pointsNode || !startNode || !offsetNode || !unitidNode)
    {
        MESP_LOGE(MESH_TAG, "InvertStatus command missing required fields");
        return false;
    }

    if(!cJSON_IsNumber(refreshNode) || !cJSON_IsNumber(addressNode) || !cJSON_IsNumber(muxNode) ||
       !cJSON_IsNumber(typeNode) || !cJSON_IsNumber(pointsNode) || !cJSON_IsNumber(startNode) ||
       !cJSON_IsNumber(offsetNode) || !cJSON_IsNumber(unitidNode))
    {
        MESP_LOGE(MESH_TAG, "InvertStatus command has invalid field types");
        return false;
    }

    out->refresh = refreshNode->valueint;
    out->address = addressNode->valueint;
    out->mux = muxNode->valuedouble;
    out->type = typeNode->valueint;
    out->points = pointsNode->valueint;
    out->start = startNode->valueint;
    out->offset = offsetNode->valueint;
    out->unitid = unitidNode->valueint;

    return true;
}

static void apply_invert_status_config(const InvertStatusCommandFields *fields)
{
    theConf.inverter.refresh = fields->refresh;
    theConf.inverter.address = fields->address;
    theConf.inverter.mux = fields->mux;
    theConf.inverter.type = fields->type;
    theConf.inverter.points = fields->points;
    theConf.inverter.start = fields->start;
    theConf.inverter.offset = fields->offset;

    write_to_flash();
}

static void log_invert_status_update(const InvertStatusCommandFields *fields)
{
    char *logBuffer = (char*)calloc(INVERT_STATUS_LOG_BUFFER_SIZE, 1);
    if(logBuffer)
    {
        snprintf(logBuffer, INVERT_STATUS_LOG_BUFFER_SIZE,
                 "InvertStatus cfg addr %d refresh %d", fields->address, fields->refresh);
        writeLog(logBuffer);
        free(logBuffer);
    }
}

static void dump_invert_status_config(void)
{
    MESP_LOGI(MESH_TAG, "InvertStatus cfg -> refresh:%d addr:%d mux:%.3f type:%d points:%d start:%d off:%d",
              theConf.inverter.refresh,
              theConf.inverter.address,
              theConf.inverter.mux,
              theConf.inverter.type,
              theConf.inverter.points,
              theConf.inverter.start,
              theConf.inverter.offset);
}

int cmdInvertStatus(void *argument)
{
    cJSON *invertStatusCommand = (cJSON *)argument;
    if(invertStatusCommand == NULL)
    {
        MESP_LOGE(MESH_TAG, "InvertStatus command argument is NULL");
        return ESP_FAIL;
    }

    /*
     * Expected JSON format:
{"cmd":"InvertStatus","refresh":10,"address":1,"invstatusmux":1,"invstatustype":1,"invstatuspoints":10,"invstatusstart":0,"invstatusoff":0,"unitid":255}
     */

    InvertStatusCommandFields fields = {};
    if(!validate_invert_status_command(invertStatusCommand, &fields))
        return ESP_FAIL;

    apply_invert_status_config(&fields);
    MESP_LOGI(MESH_TAG, "InvertStatus configuration updated");
    log_invert_status_update(&fields);

    if((theConf.debug_flags >> dXCMDS) & 1U)
        dump_invert_status_config();

    return ESP_OK;
}
