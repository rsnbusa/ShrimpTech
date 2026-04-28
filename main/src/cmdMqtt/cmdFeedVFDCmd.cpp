#define GLOBAL
#include "includes.h"
#include "globals.h"

/*
 * Handles feeder VFD command configuration pushed over mesh. Parses the incoming
 * JSON payload, validates required fields, updates persistent feeder VFD command
 * settings, and stores them to flash so runtime tasks pick up the new mapping.
 */

extern void write_to_flash();
extern void writeLog(const char* que);

#define FEED_VFDCMD_LOG_BUFFER_SIZE 100

typedef struct
{
    int refresh;
    int address;
    double cmdmux;
    int cmdtype;
    int cmdpoints;
    int cmdstart;
    int cmdoff;
    double freqmux;
    int freqtype;
    int freqpoints;
    int freqstart;
    int freqoff;
} FeedVFDCmdCommandFields;

static bool validate_feed_vfdcmd_command(cJSON *feedVfdcmdCommand, FeedVFDCmdCommandFields *out)
{
    if(!feedVfdcmdCommand || !out)
    {
        MESP_LOGE(MESH_TAG,"FeedVFDCmd command argument invalid");
        return false;
    }

    if(!cJSON_IsObject(feedVfdcmdCommand))
    {
        MESP_LOGE(MESH_TAG,"FeedVFDCmd command payload invalid");
        return false;
    }

    cJSON *refreshNode   = cJSON_GetObjectItem(feedVfdcmdCommand, "refresh");
    cJSON *addressNode   = cJSON_GetObjectItem(feedVfdcmdCommand, "address");
    cJSON *cmdmuxNode    = cJSON_GetObjectItem(feedVfdcmdCommand, "cmdmux");
    cJSON *cmdtypeNode   = cJSON_GetObjectItem(feedVfdcmdCommand, "cmdtype");
    cJSON *cmdpointsNode = cJSON_GetObjectItem(feedVfdcmdCommand, "cmdpoints");
    cJSON *cmdstartNode  = cJSON_GetObjectItem(feedVfdcmdCommand, "cmdstart");
    cJSON *cmdoffNode    = cJSON_GetObjectItem(feedVfdcmdCommand, "cmdoff");
    cJSON *freqmuxNode   = cJSON_GetObjectItem(feedVfdcmdCommand, "freqmux");
    cJSON *freqtypeNode  = cJSON_GetObjectItem(feedVfdcmdCommand, "freqtype");
    cJSON *freqpointsNode= cJSON_GetObjectItem(feedVfdcmdCommand, "freqpoints");
    cJSON *freqstartNode = cJSON_GetObjectItem(feedVfdcmdCommand, "freqstart");
    cJSON *freqoffNode   = cJSON_GetObjectItem(feedVfdcmdCommand, "freqoff");

    if(!refreshNode || !addressNode || !cmdmuxNode || !cmdtypeNode || !cmdpointsNode || !cmdstartNode || !cmdoffNode ||
       !freqmuxNode || !freqtypeNode || !freqpointsNode || !freqstartNode || !freqoffNode)
    {
        MESP_LOGE(MESH_TAG,"FeedVFDCmd command missing required fields");
        return false;
    }

    if(!cJSON_IsNumber(refreshNode) || !cJSON_IsNumber(addressNode) || !cJSON_IsNumber(cmdmuxNode) || !cJSON_IsNumber(cmdtypeNode) ||
       !cJSON_IsNumber(cmdpointsNode) || !cJSON_IsNumber(cmdstartNode) || !cJSON_IsNumber(cmdoffNode) || !cJSON_IsNumber(freqmuxNode) ||
       !cJSON_IsNumber(freqtypeNode) || !cJSON_IsNumber(freqpointsNode) || !cJSON_IsNumber(freqstartNode) || !cJSON_IsNumber(freqoffNode))
    {
        MESP_LOGE(MESH_TAG,"FeedVFDCmd command has invalid field types");
        return false;
    }

    out->refresh    = refreshNode->valueint;
    out->address    = addressNode->valueint;
    out->cmdmux     = cmdmuxNode->valuedouble;
    out->cmdtype    = cmdtypeNode->valueint;
    out->cmdpoints  = cmdpointsNode->valueint;
    out->cmdstart   = cmdstartNode->valueint;
    out->cmdoff     = cmdoffNode->valueint;
    out->freqmux    = freqmuxNode->valuedouble;
    out->freqtype   = freqtypeNode->valueint;
    out->freqpoints = freqpointsNode->valueint;
    out->freqstart  = freqstartNode->valueint;
    out->freqoff    = freqoffNode->valueint;
    return true;
}

static void apply_feed_vfdcmd_config(const FeedVFDCmdCommandFields *fields)
{
    theConf.modbus_vfdcmdFeed.refresh = fields->refresh;
    theConf.modbus_vfdcmdFeed.address = fields->address;
    theConf.modbus_vfdcmdFeed.cmdmux = fields->cmdmux;
    theConf.modbus_vfdcmdFeed.cmdtype = fields->cmdtype;
    theConf.modbus_vfdcmdFeed.cmdpoints = fields->cmdpoints;
    theConf.modbus_vfdcmdFeed.cmdstart = fields->cmdstart;
    theConf.modbus_vfdcmdFeed.cmdoff = fields->cmdoff;
    theConf.modbus_vfdcmdFeed.freqmux = fields->freqmux;
    theConf.modbus_vfdcmdFeed.freqtype = fields->freqtype;
    theConf.modbus_vfdcmdFeed.freqpoints = fields->freqpoints;
    theConf.modbus_vfdcmdFeed.freqstart = fields->freqstart;
    theConf.modbus_vfdcmdFeed.freqoff = fields->freqoff;

    write_to_flash();
}

static void log_feed_vfdcmd_update(const FeedVFDCmdCommandFields *fields)
{
    char *logBuffer = (char*)calloc(FEED_VFDCMD_LOG_BUFFER_SIZE,1);
    if(logBuffer)
    {
        snprintf(logBuffer, FEED_VFDCMD_LOG_BUFFER_SIZE, "FeedVFDCmd cfg addr %d refresh %d", fields->address, fields->refresh);
        writeLog(logBuffer);
        free(logBuffer);
    }
}

static void dump_feed_vfdcmd_config(void)
{
    MESP_LOGI(MESH_TAG, "FeedVFDCmd cfg -> refresh:%d addr:%d", theConf.modbus_vfdcmdFeed.refresh, theConf.modbus_vfdcmdFeed.address);
    MESP_LOGI(MESH_TAG, "Cmd mux:%.3f type:%d points:%d start:%d off:%d", theConf.modbus_vfdcmdFeed.cmdmux, theConf.modbus_vfdcmdFeed.cmdtype, theConf.modbus_vfdcmdFeed.cmdpoints, theConf.modbus_vfdcmdFeed.cmdstart, theConf.modbus_vfdcmdFeed.cmdoff);
    MESP_LOGI(MESH_TAG, "Freq mux:%.3f type:%d points:%d start:%d off:%d", theConf.modbus_vfdcmdFeed.freqmux, theConf.modbus_vfdcmdFeed.freqtype, theConf.modbus_vfdcmdFeed.freqpoints, theConf.modbus_vfdcmdFeed.freqstart, theConf.modbus_vfdcmdFeed.freqoff);
}

int cmdFeedVFDCmd(void *argument)
{
    MESP_LOGI(MESH_TAG, "FeedVFDCmd Cmd");

    cJSON *feedVfdcmdCommand = (cJSON *)argument;
    if(feedVfdcmdCommand == NULL)
    {
        MESP_LOGE(MESH_TAG, "FeedVFDCmd command argument is NULL");
        return ESP_FAIL;
    }

    /*
     * Expected JSON format:
{"cmd":"FeedVFDCmd","refresh":10,"address":1,"cmdmux":1.0,"cmdtype":1,"cmdpoints":10,"cmdstart":0,"cmdoff":0,"freqmux":1.0,"freqtype":1,"freqpoints":10,"freqstart":20,"freqoff":0}
*/

    FeedVFDCmdCommandFields fields = {};
    if(!validate_feed_vfdcmd_command(feedVfdcmdCommand, &fields))
        return ESP_FAIL;

    apply_feed_vfdcmd_config(&fields);
    MESP_LOGI(MESH_TAG, "FeedVFDCmd configuration updated");
    log_feed_vfdcmd_update(&fields);

    if((theConf.debug_flags >> dXCMDS) & 1U)
        dump_feed_vfdcmd_config();

    return ESP_OK;
}