#define GLOBAL
#include "includes.h"
#include "globals.h"

/*
 * Handles system-wide limits configuration pushed over MQTT. Parses the incoming JSON payload,
 * validates required fields, updates persistent limits settings, and stores them
 * to flash so the system can enforce proper operational boundaries.
 */

extern void write_to_flash();
extern void writeLog(char* que);

#define LIMITS_LOG_BUFFER_SIZE 500

typedef struct
{
    // PV Panels
    int pvVoltsMin;
    int pvVoltsMax;
    int pvAmpsMin;
    int pvAmpsMax;

    // Battery
    int batterySOCMin;
    int batterySOCMax;
    int batterySOHMin;
    int batterySOHMax;
    int batteryCyclesMin;
    int batteryCyclesMax;
    int batteryTempMin;
    int batteryTempMax;

    // Amps
    int chargeTodayMin;
    int chargeTodayMax;
    int dischargeTodayMin;
    int dischargeTodayMax;
    int chargeTotalMin;
    int chargeTotalMax;
    int dischargeTotalMin;
    int dischargeTotalMax;

    // Energy (kWh)
    int generatedTodayMin;
    int generatedTodayMax;
    int consumedTodayMin;
    int consumedTodayMax;
    int loadConsumedTodayMin;
    int loadConsumedTodayMax;
    int loadConsumedTotalMin;
    int loadConsumedTotalMax;
    int batteryChargeTodayMin;
    int batteryChargeTodayMax;
    int batteryDischargeTodayMin;
    int batteryDischargeTodayMax;

    // Sensors
    int dissolvedOxygenMin;
    int dissolvedOxygenMax;
    int phMin;
    int phMax;
    int waterTempMin;
    int waterTempMax;
    int ambientTempMin;
    int ambientTempMax;
    int humidityMin;
    int humidityMax;
} LimitsCommandFields;

static bool parse_limit_pair(cJSON *node, const char *minKey, const char *maxKey, int *minVal, int *maxVal)
{
    if(!node || !cJSON_IsObject(node) || !minVal || !maxVal)
        return false;

    cJSON *minNode = cJSON_GetObjectItem(node, minKey);
    cJSON *maxNode = cJSON_GetObjectItem(node, maxKey);

    if(!minNode || !maxNode)
        return false;

    if(!cJSON_IsNumber(minNode) || !cJSON_IsNumber(maxNode))
        return false;

    *minVal = minNode->valueint;
    *maxVal = maxNode->valueint;
    return true;
}

static bool validate_limits_command(cJSON *limitsCommand, LimitsCommandFields *out)
{
    if(!limitsCommand || !out)
    {
        ESP_LOGE(MESH_TAG,"Limits command argument invalid");
        return false;
    }

    if(!cJSON_IsObject(limitsCommand))
    {
        ESP_LOGE(MESH_TAG,"Limits command payload invalid");
        return false;
    }

    // Parse PV Panels section
    cJSON *pvPanels = cJSON_GetObjectItem(limitsCommand, "PVPanels");
    if(!pvPanels || !cJSON_IsObject(pvPanels))
    {
        ESP_LOGE(MESH_TAG,"Limits command missing PVPanels section");
        return false;
    }

    if(!parse_limit_pair(pvPanels, "VoltsMin", "VoltsMax", &out->pvVoltsMin, &out->pvVoltsMax))
    {
        ESP_LOGE(MESH_TAG,"PVPanels Volts limits invalid");
        return false;
    }

    if(!parse_limit_pair(pvPanels, "AmpsMin", "AmpsMax", &out->pvAmpsMin, &out->pvAmpsMax))
    {
        ESP_LOGE(MESH_TAG,"PVPanels Amps limits invalid");
        return false;
    }

    // Parse Battery section
    cJSON *battery = cJSON_GetObjectItem(limitsCommand, "Battery");
    if(!battery || !cJSON_IsObject(battery))
    {
        ESP_LOGE(MESH_TAG,"Limits command missing Battery section");
        return false;
    }

    if(!parse_limit_pair(battery, "SOCMin", "SOCMax", &out->batterySOCMin, &out->batterySOCMax) ||
       !parse_limit_pair(battery, "SOHMin", "SOHMax", &out->batterySOHMin, &out->batterySOHMax) ||
       !parse_limit_pair(battery, "CyclesMin", "CyclesMax", &out->batteryCyclesMin, &out->batteryCyclesMax) ||
       !parse_limit_pair(battery, "TempMin", "TempMax", &out->batteryTempMin, &out->batteryTempMax))
    {
        ESP_LOGE(MESH_TAG,"Battery limits invalid");
        return false;
    }

    // Parse Amps section
    cJSON *amps = cJSON_GetObjectItem(limitsCommand, "Amps");
    if(!amps || !cJSON_IsObject(amps))
    {
        ESP_LOGE(MESH_TAG,"Limits command missing Amps section");
        return false;
    }

    if(!parse_limit_pair(amps, "ChargeTodayMin", "ChargeTodayMax", &out->chargeTodayMin, &out->chargeTodayMax) ||
       !parse_limit_pair(amps, "DischargeTodayMin", "DischargeTodayMax", &out->dischargeTodayMin, &out->dischargeTodayMax) ||
       !parse_limit_pair(amps, "ChargeTotalMin", "ChargeTotalMax", &out->chargeTotalMin, &out->chargeTotalMax) ||
       !parse_limit_pair(amps, "DischargeTotalMin", "DischargeTotalMax", &out->dischargeTotalMin, &out->dischargeTotalMax))
    {
        ESP_LOGE(MESH_TAG,"Amps limits invalid");
        return false;
    }

    // Parse Energy section
    cJSON *energy = cJSON_GetObjectItem(limitsCommand, "Energy");
    if(!energy || !cJSON_IsObject(energy))
    {
        ESP_LOGE(MESH_TAG,"Limits command missing Energy section");
        return false;
    }

    if(!parse_limit_pair(energy, "GeneratedTodayMin", "GeneratedTodayMax", &out->generatedTodayMin, &out->generatedTodayMax) ||
       !parse_limit_pair(energy, "ConsumedTodayMin", "ConsumedTodayMax", &out->consumedTodayMin, &out->consumedTodayMax) ||
       !parse_limit_pair(energy, "LoadConsumedTodayMin", "LoadConsumedTodayMax", &out->loadConsumedTodayMin, &out->loadConsumedTodayMax) ||
    //    !parse_limit_pair(energy, "LoadConsumedTotalMin", "LoadConsumedTotalMax", &out->loadConsumedTotalMin, &out->loadConsumedTotalMax) ||
       !parse_limit_pair(energy, "BatteryChargeTodayMin", "BatteryChargeTodayMax", &out->batteryChargeTodayMin, &out->batteryChargeTodayMax) ||
       !parse_limit_pair(energy, "BatteryDischargeTodayMin", "BatteryDischargeTodayMax", &out->batteryDischargeTodayMin, &out->batteryDischargeTodayMax))
    {
        ESP_LOGE(MESH_TAG,"Energy limits invalid");
        return false;
    }

    // Parse Sensors section
    cJSON *sensors = cJSON_GetObjectItem(limitsCommand, "Sensors");
    if(!sensors || !cJSON_IsObject(sensors))
    {
        ESP_LOGE(MESH_TAG,"Limits command missing Sensors section");
        return false;
    }

    if(!parse_limit_pair(sensors, "DissolvedOxygenMin", "DissolvedOxygenMax", &out->dissolvedOxygenMin, &out->dissolvedOxygenMax) ||
       !parse_limit_pair(sensors, "PHMin", "PHMax", &out->phMin, &out->phMax) ||
       !parse_limit_pair(sensors, "WaterTempMin", "WaterTempMax", &out->waterTempMin, &out->waterTempMax) ||
       !parse_limit_pair(sensors, "AmbientTempMin", "AmbientTempMax", &out->ambientTempMin, &out->ambientTempMax) ||
       !parse_limit_pair(sensors, "HumidityMin", "HumidityMax", &out->humidityMin, &out->humidityMax))
    {
        ESP_LOGE(MESH_TAG,"Sensors limits invalid");
        return false;
    }

    return true;
}

static void apply_limits_config(const LimitsCommandFields *fields)
{
    // PV Panels
    theConf.milim.vmin = fields->pvVoltsMin;
    theConf.milim.vmax = fields->pvVoltsMax;
    theConf.milim.amin = fields->pvAmpsMin;
    theConf.milim.amax = fields->pvAmpsMax;

    // Battery
    theConf.milim.bSOCmin = fields->batterySOCMin;
    theConf.milim.bSOCmax = fields->batterySOCMax;
    theConf.milim.bSOHmin = fields->batterySOHMin;
    theConf.milim.bSOHmax = fields->batterySOHMax;
    theConf.milim.bcyclemin = fields->batteryCyclesMin;
    theConf.milim.bcyclemax = fields->batteryCyclesMax;
    theConf.milim.btempmin = fields->batteryTempMin;
    theConf.milim.btempmax = fields->batteryTempMax;

    de
    // Amps
    theConf.milim.bcAhoymin = fields->chargeTodayMin;
    theConf.milim.bcAhoymax = fields->chargeTodayMax;
    theConf.milim.bdAhoymin = fields->dischargeTodayMin;
    theConf.milim.bdAhoymax = fields->dischargeTodayMax;
    theConf.milim.bcATotmin = fields->chargeTotalMin;
    theConf.milim.bcATotmax = fields->chargeTotalMax;
    theConf.milim.bdATotmin = fields->dischargeTotalMin;
    theConf.milim.bdATotmax = fields->dischargeTotalMax;

    // Energy (kWh)
    theConf.milim.kwgtodaymin = fields->generatedTodayMin;
    theConf.milim.kwgtodaymax = fields->generatedTodayMax;
    theConf.milim.kwctodaymin = fields->consumedTodayMin;
    theConf.milim.kwctodaymax = fields->consumedTodayMax;
    theConf.milim.kwloadhoymin = fields->loadConsumedTodayMin;
    theConf.milim.kwloadhoymax = fields->loadConsumedTodayMax;
    // Note: Total load consumed doesn't have a specific field in current limits structure
    theConf.milim.kwbatchoymin = fields->batteryChargeTodayMin;
    theConf.milim.kwbatchoymax = fields->batteryChargeTodayMax;
    theConf.milim.kwbatdhoymin = fields->batteryDischargeTodayMin;
    theConf.milim.kwbatdhoymax = fields->batteryDischargeTodayMax;

    // Sensors
    theConf.milim.domin = fields->dissolvedOxygenMin;
    theConf.milim.domax = fields->dissolvedOxygenMax;
    theConf.milim.phmin = fields->phMin;
    theConf.milim.phmax = fields->phMax;
    theConf.milim.wtempmin = fields->waterTempMin;
    theConf.milim.wtempmax = fields->waterTempMax;
    theConf.milim.atempmin = fields->ambientTempMin;
    theConf.milim.atempmax = fields->ambientTempMax;
    theConf.milim.hummin = fields->humidityMin;
    theConf.milim.hummax = fields->humidityMax;

    memcpy(&theConf.limits, &theConf.milim, sizeof(theConf.milim));
    write_to_flash();
}

static void log_limits_update(const LimitsCommandFields *fields)
{
    char *logBuffer = (char*)calloc(LIMITS_LOG_BUFFER_SIZE,1);
    if(logBuffer)
    {
    // char logBuffer[LIMITS_LOG_BUFFER_SIZE];
    snprintf(logBuffer, LIMITS_LOG_BUFFER_SIZE, "Limits cfg PV:%d-%dV Bat:%d-%d%% DO:%d-%dppm", 
             fields->pvVoltsMin, fields->pvVoltsMax,
             fields->batterySOCMin, fields->batterySOCMax,
             fields->dissolvedOxygenMin, fields->dissolvedOxygenMax);
    writeLog(logBuffer);
    vTaskDelay(pdMS_TO_TICKS(100));
    free(logBuffer);
    }
}

static void dump_limits_config(void)
{
    ESP_LOGI(MESH_TAG,"=== LIMITS CONFIGURATION ===");
    ESP_LOGI(MESH_TAG,"PV Panels -> Volts:%d-%d Amps:%d-%d", 
             theConf.milim.vmin, theConf.milim.vmax,
             theConf.milim.amin, theConf.milim.amax);
    ESP_LOGI(MESH_TAG,"Battery -> SOC:%d-%d%% SOH:%d-%d%% Cycles:%d-%d Temp:%d-%d°C", 
             theConf.milim.bSOCmin, theConf.milim.bSOCmax,
             theConf.milim.bSOHmin, theConf.milim.bSOHmax,
             theConf.milim.bcyclemin, theConf.milim.bcyclemax,
             theConf.milim.btempmin, theConf.milim.btempmax);
    ESP_LOGI(MESH_TAG,"Amps -> ChargeToday:%d-%d DischargeToday:%d-%d ChargeTotal:%d-%d DischargeTotal:%d-%d",
             theConf.milim.bcAhoymin, theConf.milim.bcAhoymax,
             theConf.milim.bdAhoymin, theConf.milim.bdAhoymax,
             theConf.milim.bcATotmin, theConf.milim.bcATotmax,
             theConf.milim.bdATotmin, theConf.milim.bdATotmax);
    ESP_LOGI(MESH_TAG,"Energy -> GenToday:%d-%dkWh ConsumedToday:%d-%dkWh LoadToday:%d-%dkWh",
             theConf.milim.kwgtodaymin, theConf.milim.kwgtodaymax,
             theConf.milim.kwctodaymin, theConf.milim.kwctodaymax,
             theConf.milim.kwloadhoymin, theConf.milim.kwloadhoymax);
    ESP_LOGI(MESH_TAG,"Energy -> BatChargeToday:%d-%dkWh BatDischargeToday:%d-%dkWh",
             theConf.milim.kwbatchoymin, theConf.milim.kwbatchoymax,
             theConf.milim.kwbatdhoymin, theConf.milim.kwbatdhoymax);
    ESP_LOGI(MESH_TAG,"Sensors -> DO:%d-%dppm PH:%d-%d WaterTemp:%d-%d°C AmbientTemp:%d-%d°C Humidity:%d-%d%%",
             theConf.milim.domin, theConf.milim.domax,
             theConf.milim.phmin, theConf.milim.phmax,
             theConf.milim.wtempmin, theConf.milim.wtempmax,
             theConf.milim.atempmin, theConf.milim.atempmax,
             theConf.milim.hummin, theConf.milim.hummax);
}

int cmdLimits(void *argument)
{
    ESP_LOGI(MESH_TAG,"Limits Cmd");

    cJSON *limitsCommand = (cJSON *)argument;
    if(limitsCommand == NULL)
    {
        ESP_LOGE(MESH_TAG,"Limits command argument is NULL");
        return ESP_FAIL;
    }

    /*
     * Expected JSON format:
     * {"cmd":"limits",
     *  "PVPanels":{"VoltsMin":340,"VoltsMax":390,"AmpsMin":14,"AmpsMax":15},
     *  "Battery":{"SOCMin":20,"SOCMax":80,"SOHMin":20,"SOHMax":100,"CyclesMin":0,"CyclesMax":5000,"TempMin":10,"TempMax":50},
     *  "Amps":{"ChargeTodayMin":780,"ChargeTodayMax":820,"DischargeTodayMin":720,"DischargeTodayMax":820,
     *          "ChargeTotalMin":720,"ChargeTotalMax":850,"DischargeTotalMin":720,"DischargeTotalMax":820},
     *  "Energy":{"GeneratedTodayMin":40,"GeneratedTodayMax":42,"ConsumedTodayMin":40,"ConsumedTodayMax":42,
     *            "LoadConsumedTodayMin":40,"LoadConsumedTodayMax":42,"LoadConsumedTotalMin":40,"LoadConsumedTotalMax":42,
     *            "BatteryChargeTodayMin":40,"BatteryChargeTodayMax":42,"BatteryDischargeTodayMin":40,"BatteryDischargeTodayMax":42},
     *  "Sensors":{"DissolvedOxygenMin":40,"DissolvedOxygenMax":70,"PHMin":50,"PHMax":70,
     *             "WaterTempMin":19,"WaterTempMax":31,"AmbientTempMin":19,"AmbientTempMax":32,
     *             "HumidityMin":50,"HumidityMax":90}}
     */

    LimitsCommandFields fields = {0};
    if(!validate_limits_command(limitsCommand, &fields))
        return ESP_FAIL;

    apply_limits_config(&fields);
    ESP_LOGI(MESH_TAG,"Limits configuration updated");
    log_limits_update(&fields);

    if((theConf.debug_flags >> dLIMITS) & 1U)  
        dump_limits_config();

    return ESP_OK;
}
