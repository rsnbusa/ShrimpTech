#define GLOBAL
#include "includes.h"
#include "globals.h"

extern void write_to_flash();
extern void writeLog(const char* que);

#define PID_LOG_BUFFER_SIZE 120

typedef struct
{
    int sampletime;
    bool nighonly;
    bool docontrol;
    double KD;
    double KI;
    double KP;
    double setpoint;
} PIDCommandFields;

static bool validate_pid_command(cJSON *pidCommand, PIDCommandFields *out)
{
    if(!pidCommand || !out)
    {
        MESP_LOGE(MESH_TAG, "PID command argument invalid");
        return false;
    }

    if(!cJSON_IsObject(pidCommand))
    {
        MESP_LOGE(MESH_TAG, "PID command payload invalid");
        return false;
    }

    cJSON *sampletimeNode = cJSON_GetObjectItem(pidCommand, "sampletime");
    cJSON *nighonlyNode = cJSON_GetObjectItem(pidCommand, "nighonly");
    cJSON *docontrolNode = cJSON_GetObjectItem(pidCommand, "docontrol");
    cJSON *kdNode = cJSON_GetObjectItem(pidCommand, "KD");
    cJSON *kiNode = cJSON_GetObjectItem(pidCommand, "KI");
    cJSON *kpNode = cJSON_GetObjectItem(pidCommand, "KP");
    cJSON *setpointNode = cJSON_GetObjectItem(pidCommand, "setpoint");

    if(!sampletimeNode || !nighonlyNode || !docontrolNode || !kdNode || !kiNode || !kpNode || !setpointNode)
    {
        MESP_LOGE(MESH_TAG, "PID command missing required fields");
        return false;
    }

    if(!cJSON_IsNumber(sampletimeNode) || !cJSON_IsBool(nighonlyNode) || !cJSON_IsBool(docontrolNode) ||
       !cJSON_IsNumber(kdNode) || !cJSON_IsNumber(kiNode) || !cJSON_IsNumber(kpNode) || !cJSON_IsNumber(setpointNode))
    {
        MESP_LOGE(MESH_TAG, "PID command has invalid field types");
        return false;
    }

    if(sampletimeNode->valueint <= 0)
    {
        MESP_LOGE(MESH_TAG, "PID sampletime must be > 0");
        return false;
    }

    out->sampletime = sampletimeNode->valueint;
    out->nighonly = cJSON_IsTrue(nighonlyNode);
    out->docontrol = cJSON_IsTrue(docontrolNode);
    out->KD = kdNode->valuedouble;
    out->KI = kiNode->valuedouble;
    out->KP = kpNode->valuedouble;
    out->setpoint = setpointNode->valuedouble;
    return true;
}

static void apply_pid_config(const PIDCommandFields *fields)
{
    theConf.doParms.sampletime = fields->sampletime;
    theConf.doParms.nighonly = fields->nighonly;
    theConf.doParms.docontrol = fields->docontrol;
    theConf.doParms.KD = fields->KD;
    theConf.doParms.KI = fields->KI;
    theConf.doParms.KP = fields->KP;
    theConf.doParms.setpoint = fields->setpoint;

    write_to_flash();
}

static void log_pid_update(const PIDCommandFields *fields)
{
    char *logBuffer = (char*)calloc(PID_LOG_BUFFER_SIZE, 1);
    if(logBuffer)
    {
        snprintf(logBuffer, PID_LOG_BUFFER_SIZE,
                 "PID cfg st:%d night:%d ctrl:%d Kp:%.3f Ki:%.3f Kd:%.3f sp:%.3f",
                 fields->sampletime,
                 fields->nighonly ? 1 : 0,
                 fields->docontrol ? 1 : 0,
                 fields->KP,
                 fields->KI,
                 fields->KD,
                 fields->setpoint);
        writeLog(logBuffer);
        free(logBuffer);
    }
}

int cmdPID(void *argument)
{
    MESP_LOGI(MESH_TAG, "PID Cmd");

    cJSON *pidCommand = (cJSON *)argument;
    if(pidCommand == NULL)
    {
        MESP_LOGE(MESH_TAG, "PID command argument is NULL");
        return ESP_FAIL;
    }

    /*
     * Expected JSON format:
     * {"cmd":"PID","sampletime":10,"nighonly":false,"docontrol":true,"KD":1.5,"KI":0.1,"KP":0.01,"setpoint":5.1,"unitid":1}
     */
    PIDCommandFields fields = {};
    if(!validate_pid_command(pidCommand, &fields))
        return ESP_FAIL;

    apply_pid_config(&fields);
    log_pid_update(&fields);

    MESP_LOGI(MESH_TAG,
              "PID updated sampletime:%d nighonly:%s docontrol:%s KP:%.3f KI:%.3f KD:%.3f setpoint:%.3f",
              theConf.doParms.sampletime,
              theConf.doParms.nighonly ? "true" : "false",
              theConf.doParms.docontrol ? "true" : "false",
              theConf.doParms.KP,
              theConf.doParms.KI,
              theConf.doParms.KD,
              theConf.doParms.setpoint);

    return ESP_OK;
}
