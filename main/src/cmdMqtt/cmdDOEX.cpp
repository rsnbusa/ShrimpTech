#define GLOBAL
#include "includes.h"
#include "globals.h"

/*
 * Handles DOEX command received over MQTT. Parses the remote DO sensor payload,
 * validates all required fields, and injects the sensor data into the blower
 * via setSensors().
 *
 * Expected JSON format:
 * {"cmd":"DOEX","DOLevel":6.50,"DOCount":10,"DOretry":0,"PHLevel":7.20,
 *  "IRLevel":850.00,"SALevel":35.00,"WaterTemp":28.00,"Interval":60}
 */

extern void writeLog(char* que);

#define DOEX_LOG_BUFFER_SIZE 140

typedef struct
{
    double DOLevel;
    int    DOCount;
    int    DOretry;
    double PHLevel;
    double IRLevel;
    double SALevel;
    double WaterTemp;
    int    Interval;
} DOEXFields;

static bool validate_doex_command(cJSON *cmd, DOEXFields *out)
{
    if (!cmd || !out)
    {
        MESP_LOGE(MESH_TAG, "DOEX: null argument");
        return false;
    }

    if (!cJSON_IsObject(cmd))
    {
        MESP_LOGE(MESH_TAG, "DOEX: payload is not a JSON object");
        return false;
    }

    cJSON *doLevelNode  = cJSON_GetObjectItem(cmd, "DOLevel");
    cJSON *doCountNode  = cJSON_GetObjectItem(cmd, "DOCount");
    cJSON *doRetryNode  = cJSON_GetObjectItem(cmd, "DOretry");
    cJSON *phLevelNode  = cJSON_GetObjectItem(cmd, "PHLevel");
    cJSON *irLevelNode  = cJSON_GetObjectItem(cmd, "IRLevel");
    cJSON *saLevelNode  = cJSON_GetObjectItem(cmd, "SALevel");
    cJSON *wTempNode    = cJSON_GetObjectItem(cmd, "WaterTemp");
    cJSON *intervalNode = cJSON_GetObjectItem(cmd, "Interval");

    if (!doLevelNode || !doCountNode || !doRetryNode || !phLevelNode ||
        !irLevelNode || !saLevelNode || !wTempNode   || !intervalNode)
    {
        MESP_LOGE(MESH_TAG, "DOEX: missing required fields");
        return false;
    }

    if (!cJSON_IsNumber(doLevelNode) || !cJSON_IsNumber(doCountNode) ||
        !cJSON_IsNumber(doRetryNode) || !cJSON_IsNumber(phLevelNode) ||
        !cJSON_IsNumber(irLevelNode) || !cJSON_IsNumber(saLevelNode) ||
        !cJSON_IsNumber(wTempNode)   || !cJSON_IsNumber(intervalNode))
    {
        MESP_LOGE(MESH_TAG, "DOEX: invalid field types");
        return false;
    }

    out->DOLevel  = doLevelNode->valuedouble;
    out->DOCount  = doCountNode->valueint;
    out->DOretry  = doRetryNode->valueint;
    out->PHLevel  = phLevelNode->valuedouble;
    out->IRLevel  = irLevelNode->valuedouble;
    out->SALevel  = saLevelNode->valuedouble;
    out->WaterTemp = wTempNode->valuedouble;
    out->Interval  = intervalNode->valueint;
    return true;
}

static void apply_doex_data(const DOEXFields *f)
{
    theBlower.setSensors((float)f->DOLevel, (float)f->PHLevel,
                         (float)f->WaterTemp, temperature, 0);
}

static void log_doex_update(const DOEXFields *f)
{
    char *buf = (char *)calloc(DOEX_LOG_BUFFER_SIZE, 1);
    if (buf)
    {
        snprintf(buf, DOEX_LOG_BUFFER_SIZE,
                 "DOEX DO:%.2f cnt:%d ret:%d PH:%.2f IR:%.2f SA:%.2f Wt:%.2f iv:%d",
                 f->DOLevel, f->DOCount, f->DOretry,
                 f->PHLevel, f->IRLevel, f->SALevel,
                 f->WaterTemp, f->Interval);
        writeLog(buf);
        free(buf);
    }
}

int cmdDOEX(void *argument)
{
    MESP_LOGI(MESH_TAG, "DOEX Cmd");

    cJSON *doexCommand = (cJSON *)argument;
    if (doexCommand == NULL)
    {
        MESP_LOGE(MESH_TAG, "DOEX: argument is NULL");
        return ESP_FAIL;
    }

    DOEXFields fields = {0};
    if (!validate_doex_command(doexCommand, &fields))
        return ESP_FAIL;

    apply_doex_data(&fields);
    log_doex_update(&fields);

    MESP_LOGI(MESH_TAG,
              "DOEX DO:%.2f cnt:%d ret:%d PH:%.2f IR:%.2f SA:%.2f Wt:%.2f Interval:%d",
              fields.DOLevel, fields.DOCount, fields.DOretry,
              fields.PHLevel, fields.IRLevel, fields.SALevel,
              fields.WaterTemp, fields.Interval);

    return ESP_OK;
}
