#define GLOBAL
#include "includes.h"
#include "globals.h"

/*
 * Handles VFD monitor configuration pushed over mesh. Parses the incoming
 * JSON payload, validates required fields, updates persistent VFD monitor
 * settings, and stores them to flash.
 */

extern void write_to_flash();
extern void writeLog(const char* que);

#define VFDMON_LOG_BUFFER_SIZE 100

typedef struct
{
    int refresh;
    int address;
    double currmux;
    int currtype;
    int currpoints;
    int currstart;
    int curroff;
    double voltmux;
    int volttype;
    int voltpoints;
    int voltstart;
    int voltoff;
    double pwrmux;
    int pwrtype;
    int pwrpoints;
    int pwrstart;
    int pwroff;
    double rpmmux;
    int rpmtype;
    int rpmpoints;
    int rmpstart;
    int rpmoff;
} VFDMonCommandFields;

static bool validate_vfdmon_command(cJSON *vfdmonCommand, VFDMonCommandFields *out)
{
    if(!vfdmonCommand || !out)
    {
        MESP_LOGE(MESH_TAG,"VFDMon command argument invalid");
        return false;
    }

    if(!cJSON_IsObject(vfdmonCommand))
    {
        MESP_LOGE(MESH_TAG,"VFDMon command payload invalid");
        return false;
    }

    cJSON *refreshNode    = cJSON_GetObjectItem(vfdmonCommand, "refresh");
    cJSON *addressNode    = cJSON_GetObjectItem(vfdmonCommand, "address");
    cJSON *currmuxNode    = cJSON_GetObjectItem(vfdmonCommand, "currmux");
    cJSON *currtypeNode   = cJSON_GetObjectItem(vfdmonCommand, "currtype");
    cJSON *currpointsNode = cJSON_GetObjectItem(vfdmonCommand, "currpoints");
    cJSON *currstartNode  = cJSON_GetObjectItem(vfdmonCommand, "currstart");
    cJSON *curroffNode    = cJSON_GetObjectItem(vfdmonCommand, "curroff");
    cJSON *voltmuxNode    = cJSON_GetObjectItem(vfdmonCommand, "voltmux");
    cJSON *volttypeNode   = cJSON_GetObjectItem(vfdmonCommand, "volttype");
    cJSON *voltpointsNode = cJSON_GetObjectItem(vfdmonCommand, "voltpoints");
    cJSON *voltstartNode  = cJSON_GetObjectItem(vfdmonCommand, "voltstart");
    cJSON *voltoffNode    = cJSON_GetObjectItem(vfdmonCommand, "voltoff");
    cJSON *pwrmuxNode     = cJSON_GetObjectItem(vfdmonCommand, "pwrmux");
    cJSON *pwrtypeNode    = cJSON_GetObjectItem(vfdmonCommand, "pwrtype");
    cJSON *pwrpointsNode  = cJSON_GetObjectItem(vfdmonCommand, "pwrpoints");
    cJSON *pwrstartNode   = cJSON_GetObjectItem(vfdmonCommand, "pwrstart");
    cJSON *pwroffNode     = cJSON_GetObjectItem(vfdmonCommand, "pwroff");
    cJSON *rpmmuxNode     = cJSON_GetObjectItem(vfdmonCommand, "rpmmux");
    cJSON *rpmtypeNode    = cJSON_GetObjectItem(vfdmonCommand, "rpmtype");
    cJSON *rpmpointsNode  = cJSON_GetObjectItem(vfdmonCommand, "rpmpoints");
    cJSON *rmpstartNode   = cJSON_GetObjectItem(vfdmonCommand, "rmpstart");
    cJSON *rpmoffNode     = cJSON_GetObjectItem(vfdmonCommand, "rpmoff");

    if(!refreshNode || !addressNode || !currmuxNode || !currtypeNode || !currpointsNode || !currstartNode || !curroffNode ||
       !voltmuxNode || !volttypeNode || !voltpointsNode || !voltstartNode || !voltoffNode || !pwrmuxNode || !pwrtypeNode ||
       !pwrpointsNode || !pwrstartNode || !pwroffNode || !rpmmuxNode || !rpmtypeNode || !rpmpointsNode || !rmpstartNode || !rpmoffNode)
    {
        MESP_LOGE(MESH_TAG,"VFDMon command missing required fields");
        return false;
    }

    if(!cJSON_IsNumber(refreshNode) || !cJSON_IsNumber(addressNode) || !cJSON_IsNumber(currmuxNode) || !cJSON_IsNumber(currtypeNode) ||
       !cJSON_IsNumber(currpointsNode) || !cJSON_IsNumber(currstartNode) || !cJSON_IsNumber(curroffNode) || !cJSON_IsNumber(voltmuxNode) ||
       !cJSON_IsNumber(volttypeNode) || !cJSON_IsNumber(voltpointsNode) || !cJSON_IsNumber(voltstartNode) || !cJSON_IsNumber(voltoffNode) ||
       !cJSON_IsNumber(pwrmuxNode) || !cJSON_IsNumber(pwrtypeNode) || !cJSON_IsNumber(pwrpointsNode) || !cJSON_IsNumber(pwrstartNode) ||
       !cJSON_IsNumber(pwroffNode) || !cJSON_IsNumber(rpmmuxNode) || !cJSON_IsNumber(rpmtypeNode) || !cJSON_IsNumber(rpmpointsNode) ||
       !cJSON_IsNumber(rmpstartNode) || !cJSON_IsNumber(rpmoffNode))
    {
        MESP_LOGE(MESH_TAG,"VFDMon command has invalid field types");
        return false;
    }

    out->refresh    = refreshNode->valueint;
    out->address    = addressNode->valueint;
    out->currmux    = currmuxNode->valuedouble;
    out->currtype   = currtypeNode->valueint;
    out->currpoints = currpointsNode->valueint;
    out->currstart  = currstartNode->valueint;
    out->curroff    = curroffNode->valueint;
    out->voltmux    = voltmuxNode->valuedouble;
    out->volttype   = volttypeNode->valueint;
    out->voltpoints = voltpointsNode->valueint;
    out->voltstart  = voltstartNode->valueint;
    out->voltoff    = voltoffNode->valueint;
    out->pwrmux     = pwrmuxNode->valuedouble;
    out->pwrtype    = pwrtypeNode->valueint;
    out->pwrpoints  = pwrpointsNode->valueint;
    out->pwrstart   = pwrstartNode->valueint;
    out->pwroff     = pwroffNode->valueint;
    out->rpmmux     = rpmmuxNode->valuedouble;
    out->rpmtype    = rpmtypeNode->valueint;
    out->rpmpoints  = rpmpointsNode->valueint;
    out->rmpstart   = rmpstartNode->valueint;
    out->rpmoff     = rpmoffNode->valueint;
    return true;
}

static void apply_vfdmon_config(const VFDMonCommandFields *fields)
{
    theConf.modbus_vfd.refresh = fields->refresh;
    theConf.modbus_vfd.address = fields->address;
    theConf.modbus_vfd.currmux = fields->currmux;
    theConf.modbus_vfd.currtype = fields->currtype;
    theConf.modbus_vfd.currpoints = fields->currpoints;
    theConf.modbus_vfd.currstart = fields->currstart;
    theConf.modbus_vfd.curroff = fields->curroff;
    theConf.modbus_vfd.voltmux = fields->voltmux;
    theConf.modbus_vfd.volttype = fields->volttype;
    theConf.modbus_vfd.voltpoints = fields->voltpoints;
    theConf.modbus_vfd.voltstart = fields->voltstart;
    theConf.modbus_vfd.voltoff = fields->voltoff;
    theConf.modbus_vfd.pwrmux = fields->pwrmux;
    theConf.modbus_vfd.pwrtype = fields->pwrtype;
    theConf.modbus_vfd.pwrpoints = fields->pwrpoints;
    theConf.modbus_vfd.pwrstart = fields->pwrstart;
    theConf.modbus_vfd.pwroff = fields->pwroff;
    theConf.modbus_vfd.rpmmux = fields->rpmmux;
    theConf.modbus_vfd.rpmtype = fields->rpmtype;
    theConf.modbus_vfd.rpmpoints = fields->rpmpoints;
    theConf.modbus_vfd.rmpstart = fields->rmpstart;
    theConf.modbus_vfd.rpmoff = fields->rpmoff;

    write_to_flash();
}

static void log_vfdmon_update(const VFDMonCommandFields *fields)
{
    char *logBuffer = (char*)calloc(VFDMON_LOG_BUFFER_SIZE,1);
    if(logBuffer)
    {
        snprintf(logBuffer, VFDMON_LOG_BUFFER_SIZE, "VFDMon cfg addr %d refresh %d", fields->address, fields->refresh);
        writeLog(logBuffer);
        free(logBuffer);
    }
}

static void dump_vfdmon_config(void)
{
    MESP_LOGI(MESH_TAG, "VFDMon cfg -> refresh:%d addr:%d", theConf.modbus_vfd.refresh, theConf.modbus_vfd.address);
    MESP_LOGI(MESH_TAG, "Current mux:%.3f type:%d points:%d start:%d off:%d", theConf.modbus_vfd.currmux, theConf.modbus_vfd.currtype, theConf.modbus_vfd.currpoints, theConf.modbus_vfd.currstart, theConf.modbus_vfd.curroff);
    MESP_LOGI(MESH_TAG, "Volt mux:%.3f type:%d points:%d start:%d off:%d", theConf.modbus_vfd.voltmux, theConf.modbus_vfd.volttype, theConf.modbus_vfd.voltpoints, theConf.modbus_vfd.voltstart, theConf.modbus_vfd.voltoff);
    MESP_LOGI(MESH_TAG, "Power mux:%.3f type:%d points:%d start:%d off:%d", theConf.modbus_vfd.pwrmux, theConf.modbus_vfd.pwrtype, theConf.modbus_vfd.pwrpoints, theConf.modbus_vfd.pwrstart, theConf.modbus_vfd.pwroff);
    MESP_LOGI(MESH_TAG, "RPM mux:%.3f type:%d points:%d start:%d off:%d", theConf.modbus_vfd.rpmmux, theConf.modbus_vfd.rpmtype, theConf.modbus_vfd.rpmpoints, theConf.modbus_vfd.rmpstart, theConf.modbus_vfd.rpmoff);
}

int cmdVFDMon(void *argument)
{
    MESP_LOGI(MESH_TAG, "VFDMon Cmd");

    cJSON *vfdmonCommand = (cJSON *)argument;
    if(vfdmonCommand == NULL)
    {
        MESP_LOGE(MESH_TAG, "VFDMon command argument is NULL");
        return ESP_FAIL;
    }

    /*
     * Expected JSON format:
{"cmd":"VFDMon","refresh":10,"address":1,"currmux":1.0,"currtype":1,"currpoints":10,"currstart":0,"curroff":0,"voltmux":1.0,"volttype":1,"voltpoints":10,"voltstart":20,"voltoff":0,"pwrmux":1.0,"pwrtype":1,"pwrpoints":10,"pwrstart":40,"pwroff":0,"rpmmux":1.0,"rpmtype":1,"rpmpoints":10,"rmpstart":60,"rpmoff":0}
*/

    VFDMonCommandFields fields = {};
    if(!validate_vfdmon_command(vfdmonCommand, &fields))
        return ESP_FAIL;

    apply_vfdmon_config(&fields);
    MESP_LOGI(MESH_TAG, "VFDMon configuration updated");
    log_vfdmon_update(&fields);

    if((theConf.debug_flags >> dXCMDS) & 1U)
        dump_vfdmon_config();

    return ESP_OK;
}
