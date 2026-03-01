#define GLOBAL
#include "includes.h"
#include "globals.h"

/*
 * Handles VFD monitor command configuration pushed over mesh. Parses the incoming
 * JSON payload, validates required fields, updates persistent VFD command settings,
 * and stores them to flash so runtime tasks pick up the new mapping.
 */

extern void write_to_flash();
extern void writeLog(char* que);

#define VFDCMD_LOG_BUFFER_SIZE 100

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
} VFDCmdCommandFields;

static bool validate_vfdcmd_command(cJSON *vfdcmdCommand, VFDCmdCommandFields *out)
{
    if(!vfdcmdCommand || !out)
    {
        MESP_LOGE(MESH_TAG,"VFDCmd command argument invalid");
        return false;
    }

    if(!cJSON_IsObject(vfdcmdCommand))
    {
        MESP_LOGE(MESH_TAG,"VFDCmd command payload invalid");
        return false;
    }

    cJSON *refreshNode   = cJSON_GetObjectItem(vfdcmdCommand, "refresh");
    cJSON *addressNode   = cJSON_GetObjectItem(vfdcmdCommand, "address");
    cJSON *cmdmuxNode    = cJSON_GetObjectItem(vfdcmdCommand, "cmdmux");
    cJSON *cmdtypeNode   = cJSON_GetObjectItem(vfdcmdCommand, "cmdtype");
    cJSON *cmdpointsNode = cJSON_GetObjectItem(vfdcmdCommand, "cmdpoints");
    cJSON *cmdstartNode  = cJSON_GetObjectItem(vfdcmdCommand, "cmdstart");
    cJSON *cmdoffNode    = cJSON_GetObjectItem(vfdcmdCommand, "cmdoff");
    cJSON *freqmuxNode   = cJSON_GetObjectItem(vfdcmdCommand, "freqmux");
    cJSON *freqtypeNode  = cJSON_GetObjectItem(vfdcmdCommand, "freqtype");
    cJSON *freqpointsNode= cJSON_GetObjectItem(vfdcmdCommand, "freqpoints");
    cJSON *freqstartNode = cJSON_GetObjectItem(vfdcmdCommand, "freqstart");
    cJSON *freqoffNode   = cJSON_GetObjectItem(vfdcmdCommand, "freqoff");

    if(!refreshNode || !addressNode || !cmdmuxNode || !cmdtypeNode || !cmdpointsNode || !cmdstartNode || !cmdoffNode ||
       !freqmuxNode || !freqtypeNode || !freqpointsNode || !freqstartNode || !freqoffNode)
    {
        MESP_LOGE(MESH_TAG,"VFDCmd command missing required fields");
        return false;
    }

    if(!cJSON_IsNumber(refreshNode) || !cJSON_IsNumber(addressNode) || !cJSON_IsNumber(cmdmuxNode) || !cJSON_IsNumber(cmdtypeNode) ||
       !cJSON_IsNumber(cmdpointsNode) || !cJSON_IsNumber(cmdstartNode) || !cJSON_IsNumber(cmdoffNode) || !cJSON_IsNumber(freqmuxNode) ||
       !cJSON_IsNumber(freqtypeNode) || !cJSON_IsNumber(freqpointsNode) || !cJSON_IsNumber(freqstartNode) || !cJSON_IsNumber(freqoffNode))
    {
        MESP_LOGE(MESH_TAG,"VFDCmd command has invalid field types");
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

static void apply_vfdcmd_config(const VFDCmdCommandFields *fields)
{
    theConf.modbus_vfdcmd.refresh = fields->refresh;
    theConf.modbus_vfdcmd.address = fields->address;
    theConf.modbus_vfdcmd.cmdmux = fields->cmdmux;
    theConf.modbus_vfdcmd.cmdtype = fields->cmdtype;
    theConf.modbus_vfdcmd.cmdpoints = fields->cmdpoints;
    theConf.modbus_vfdcmd.cmdstart = fields->cmdstart;
    theConf.modbus_vfdcmd.cmdoff = fields->cmdoff;
    theConf.modbus_vfdcmd.freqmux = fields->freqmux;
    theConf.modbus_vfdcmd.freqtype = fields->freqtype;
    theConf.modbus_vfdcmd.freqpoints = fields->freqpoints;
    theConf.modbus_vfdcmd.freqstart = fields->freqstart;
    theConf.modbus_vfdcmd.freqoff = fields->freqoff;

    write_to_flash();
}

static void log_vfdcmd_update(const VFDCmdCommandFields *fields)
{
    char *logBuffer = (char*)calloc(VFDCMD_LOG_BUFFER_SIZE,1);
    if(logBuffer)
    {
        snprintf(logBuffer, VFDCMD_LOG_BUFFER_SIZE, "VFDCmd cfg addr %d refresh %d", fields->address, fields->refresh);
        writeLog(logBuffer);
        free(logBuffer);
    }
}

static void dump_vfdcmd_config(void)
{
    MESP_LOGI(MESH_TAG, "VFDCmd cfg -> refresh:%d addr:%d", theConf.modbus_vfdcmd.refresh, theConf.modbus_vfdcmd.address);
    MESP_LOGI(MESH_TAG, "Cmd mux:%.3f type:%d points:%d start:%d off:%d", theConf.modbus_vfdcmd.cmdmux, theConf.modbus_vfdcmd.cmdtype, theConf.modbus_vfdcmd.cmdpoints, theConf.modbus_vfdcmd.cmdstart, theConf.modbus_vfdcmd.cmdoff);
    MESP_LOGI(MESH_TAG, "Freq mux:%.3f type:%d points:%d start:%d off:%d", theConf.modbus_vfdcmd.freqmux, theConf.modbus_vfdcmd.freqtype, theConf.modbus_vfdcmd.freqpoints, theConf.modbus_vfdcmd.freqstart, theConf.modbus_vfdcmd.freqoff);
}

int cmdVFDCmd(void *argument)
{
    MESP_LOGI(MESH_TAG, "VFDCmd Cmd");

    cJSON *vfdcmdCommand = (cJSON *)argument;
    if(vfdcmdCommand == NULL)
    {
        MESP_LOGE(MESH_TAG, "VFDCmd command argument is NULL");
        return ESP_FAIL;
    }

    /*
     * Expected JSON format:
{"cmd":"VFDCmd","refresh":10,"address":1,"cmdmux":1.0,"cmdtype":1,"cmdpoints":10,"cmdstart":0,"cmdoff":0,"freqmux":1.0,"freqtype":1,"freqpoints":10,"freqstart":20,"freqoff":0}
*/

    VFDCmdCommandFields fields = {0};
    if(!validate_vfdcmd_command(vfdcmdCommand, &fields))
        return ESP_FAIL;

    apply_vfdcmd_config(&fields);
    MESP_LOGI(MESH_TAG, "VFDCmd configuration updated");
    log_vfdcmd_update(&fields);

    if((theConf.debug_flags >> dXCMDS) & 1U)
        dump_vfdcmd_config();

    return ESP_OK;
}
