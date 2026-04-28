#define GLOBAL
#include "includes.h"
#include "globals.h"

/*
 * Handles feeder VFD monitor configuration pushed over mesh. Parses the incoming
 * JSON payload, validates required fields, updates persistent feeder VFD monitor
 * settings, and stores them to flash.
 */

extern void write_to_flash();
extern void writeLog(const char* que);

#define FEED_VFDMON_LOG_BUFFER_SIZE 100

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
} FeedVFDMonCommandFields;

static bool validate_feed_vfdmon_command(cJSON *feedVfdmonCommand, FeedVFDMonCommandFields *out)
{
    if(!feedVfdmonCommand || !out)
    {
        MESP_LOGE(MESH_TAG,"FeedVFDMon command argument invalid");
        return false;
    }

    if(!cJSON_IsObject(feedVfdmonCommand))
    {
        MESP_LOGE(MESH_TAG,"FeedVFDMon command payload invalid");
        return false;
    }

    cJSON *refreshNode    = cJSON_GetObjectItem(feedVfdmonCommand, "refresh");
    cJSON *addressNode    = cJSON_GetObjectItem(feedVfdmonCommand, "address");
    cJSON *currmuxNode    = cJSON_GetObjectItem(feedVfdmonCommand, "currmux");
    cJSON *currtypeNode   = cJSON_GetObjectItem(feedVfdmonCommand, "currtype");
    cJSON *currpointsNode = cJSON_GetObjectItem(feedVfdmonCommand, "currpoints");
    cJSON *currstartNode  = cJSON_GetObjectItem(feedVfdmonCommand, "currstart");
    cJSON *curroffNode    = cJSON_GetObjectItem(feedVfdmonCommand, "curroff");
    cJSON *voltmuxNode    = cJSON_GetObjectItem(feedVfdmonCommand, "voltmux");
    cJSON *volttypeNode   = cJSON_GetObjectItem(feedVfdmonCommand, "volttype");
    cJSON *voltpointsNode = cJSON_GetObjectItem(feedVfdmonCommand, "voltpoints");
    cJSON *voltstartNode  = cJSON_GetObjectItem(feedVfdmonCommand, "voltstart");
    cJSON *voltoffNode    = cJSON_GetObjectItem(feedVfdmonCommand, "voltoff");
    cJSON *pwrmuxNode     = cJSON_GetObjectItem(feedVfdmonCommand, "pwrmux");
    cJSON *pwrtypeNode    = cJSON_GetObjectItem(feedVfdmonCommand, "pwrtype");
    cJSON *pwrpointsNode  = cJSON_GetObjectItem(feedVfdmonCommand, "pwrpoints");
    cJSON *pwrstartNode   = cJSON_GetObjectItem(feedVfdmonCommand, "pwrstart");
    cJSON *pwroffNode     = cJSON_GetObjectItem(feedVfdmonCommand, "pwroff");
    cJSON *rpmmuxNode     = cJSON_GetObjectItem(feedVfdmonCommand, "rpmmux");
    cJSON *rpmtypeNode    = cJSON_GetObjectItem(feedVfdmonCommand, "rpmtype");
    cJSON *rpmpointsNode  = cJSON_GetObjectItem(feedVfdmonCommand, "rpmpoints");
    cJSON *rmpstartNode   = cJSON_GetObjectItem(feedVfdmonCommand, "rmpstart");
    cJSON *rpmoffNode     = cJSON_GetObjectItem(feedVfdmonCommand, "rpmoff");

    if(!refreshNode || !addressNode || !currmuxNode || !currtypeNode || !currpointsNode || !currstartNode || !curroffNode ||
       !voltmuxNode || !volttypeNode || !voltpointsNode || !voltstartNode || !voltoffNode || !pwrmuxNode || !pwrtypeNode ||
       !pwrpointsNode || !pwrstartNode || !pwroffNode || !rpmmuxNode || !rpmtypeNode || !rpmpointsNode || !rmpstartNode || !rpmoffNode)
    {
        MESP_LOGE(MESH_TAG,"FeedVFDMon command missing required fields");
        return false;
    }

    if(!cJSON_IsNumber(refreshNode) || !cJSON_IsNumber(addressNode) || !cJSON_IsNumber(currmuxNode) || !cJSON_IsNumber(currtypeNode) ||
       !cJSON_IsNumber(currpointsNode) || !cJSON_IsNumber(currstartNode) || !cJSON_IsNumber(curroffNode) || !cJSON_IsNumber(voltmuxNode) ||
       !cJSON_IsNumber(volttypeNode) || !cJSON_IsNumber(voltpointsNode) || !cJSON_IsNumber(voltstartNode) || !cJSON_IsNumber(voltoffNode) ||
       !cJSON_IsNumber(pwrmuxNode) || !cJSON_IsNumber(pwrtypeNode) || !cJSON_IsNumber(pwrpointsNode) || !cJSON_IsNumber(pwrstartNode) ||
       !cJSON_IsNumber(pwroffNode) || !cJSON_IsNumber(rpmmuxNode) || !cJSON_IsNumber(rpmtypeNode) || !cJSON_IsNumber(rpmpointsNode) ||
       !cJSON_IsNumber(rmpstartNode) || !cJSON_IsNumber(rpmoffNode))
    {
        MESP_LOGE(MESH_TAG,"FeedVFDMon command has invalid field types");
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

static void apply_feed_vfdmon_config(const FeedVFDMonCommandFields *fields)
{
    theConf.modbus_vfdFeed.refresh = fields->refresh;
    theConf.modbus_vfdFeed.address = fields->address;
    theConf.modbus_vfdFeed.currmux = fields->currmux;
    theConf.modbus_vfdFeed.currtype = fields->currtype;
    theConf.modbus_vfdFeed.currpoints = fields->currpoints;
    theConf.modbus_vfdFeed.currstart = fields->currstart;
    theConf.modbus_vfdFeed.curroff = fields->curroff;
    theConf.modbus_vfdFeed.voltmux = fields->voltmux;
    theConf.modbus_vfdFeed.volttype = fields->volttype;
    theConf.modbus_vfdFeed.voltpoints = fields->voltpoints;
    theConf.modbus_vfdFeed.voltstart = fields->voltstart;
    theConf.modbus_vfdFeed.voltoff = fields->voltoff;
    theConf.modbus_vfdFeed.pwrmux = fields->pwrmux;
    theConf.modbus_vfdFeed.pwrtype = fields->pwrtype;
    theConf.modbus_vfdFeed.pwrpoints = fields->pwrpoints;
    theConf.modbus_vfdFeed.pwrstart = fields->pwrstart;
    theConf.modbus_vfdFeed.pwroff = fields->pwroff;
    theConf.modbus_vfdFeed.rpmmux = fields->rpmmux;
    theConf.modbus_vfdFeed.rpmtype = fields->rpmtype;
    theConf.modbus_vfdFeed.rpmpoints = fields->rpmpoints;
    theConf.modbus_vfdFeed.rmpstart = fields->rmpstart;
    theConf.modbus_vfdFeed.rpmoff = fields->rpmoff;

    write_to_flash();
}

static void log_feed_vfdmon_update(const FeedVFDMonCommandFields *fields)
{
    char *logBuffer = (char*)calloc(FEED_VFDMON_LOG_BUFFER_SIZE,1);
    if(logBuffer)
    {
        snprintf(logBuffer, FEED_VFDMON_LOG_BUFFER_SIZE, "FeedVFDMon cfg addr %d refresh %d", fields->address, fields->refresh);
        writeLog(logBuffer);
        free(logBuffer);
    }
}

static void dump_feed_vfdmon_config(void)
{
    MESP_LOGI(MESH_TAG, "FeedVFDMon cfg -> refresh:%d addr:%d", theConf.modbus_vfdFeed.refresh, theConf.modbus_vfdFeed.address);
    MESP_LOGI(MESH_TAG, "Current mux:%.3f type:%d points:%d start:%d off:%d", theConf.modbus_vfdFeed.currmux, theConf.modbus_vfdFeed.currtype, theConf.modbus_vfdFeed.currpoints, theConf.modbus_vfdFeed.currstart, theConf.modbus_vfdFeed.curroff);
    MESP_LOGI(MESH_TAG, "Volt mux:%.3f type:%d points:%d start:%d off:%d", theConf.modbus_vfdFeed.voltmux, theConf.modbus_vfdFeed.volttype, theConf.modbus_vfdFeed.voltpoints, theConf.modbus_vfdFeed.voltstart, theConf.modbus_vfdFeed.voltoff);
    MESP_LOGI(MESH_TAG, "Power mux:%.3f type:%d points:%d start:%d off:%d", theConf.modbus_vfdFeed.pwrmux, theConf.modbus_vfdFeed.pwrtype, theConf.modbus_vfdFeed.pwrpoints, theConf.modbus_vfdFeed.pwrstart, theConf.modbus_vfdFeed.pwroff);
    MESP_LOGI(MESH_TAG, "RPM mux:%.3f type:%d points:%d start:%d off:%d", theConf.modbus_vfdFeed.rpmmux, theConf.modbus_vfdFeed.rpmtype, theConf.modbus_vfdFeed.rpmpoints, theConf.modbus_vfdFeed.rmpstart, theConf.modbus_vfdFeed.rpmoff);
}

int cmdFeedVFDMon(void *argument)
{
    MESP_LOGI(MESH_TAG, "FeedVFDMon Cmd");

    cJSON *feedVfdmonCommand = (cJSON *)argument;
    if(feedVfdmonCommand == NULL)
    {
        MESP_LOGE(MESH_TAG, "FeedVFDMon command argument is NULL");
        return ESP_FAIL;
    }

    /*
     * Expected JSON format:
{"cmd":"FeedVFDMon","refresh":10,"address":1,"currmux":1.0,"currtype":1,"currpoints":10,"currstart":0,"curroff":0,"voltmux":1.0,"volttype":1,"voltpoints":10,"voltstart":20,"voltoff":0,"pwrmux":1.0,"pwrtype":1,"pwrpoints":10,"pwrstart":40,"pwroff":0,"rpmmux":1.0,"rpmtype":1,"rpmpoints":10,"rmpstart":60,"rpmoff":0}
*/

    FeedVFDMonCommandFields fields = {};
    if(!validate_feed_vfdmon_command(feedVfdmonCommand, &fields))
        return ESP_FAIL;

    apply_feed_vfdmon_config(&fields);
    MESP_LOGI(MESH_TAG, "FeedVFDMon configuration updated");
    log_feed_vfdmon_update(&fields);

    if((theConf.debug_flags >> dXCMDS) & 1U)
        dump_feed_vfdmon_config();

    return ESP_OK;
}