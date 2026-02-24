/**
 * @file webserver.cpp
 * @brief Web server implementation for IoT device configuration and monitoring
 * 
 * This module provides a web interface using Mongoose for configuring and monitoring
 * the IoT device. It handles:
 * - Device initial configuration and authentication
 * - System settings (WiFi, MQTT, security)
 * - Profile management (schedules and cycles)
 * - Modbus device configuration (inverter, sensors, battery, panels)
 * - System status and statistics
 * - Device reboot control
 * 
 * @author rsn
 * @date Dec 29, 2019
 */
#ifndef TYPESweb_H_
#define TYPESweb_H_
#define GLOBAL

#include "includes.h"
#include "globals.h"
#include "mongoose_glue.h"

// Configuration state constants
#define CONF_STATE_UNCONFIGURED  0
#define CONF_STATE_CONFIGURED    1
#define CONF_STATE_PENDING       2
#define CONF_STATE_CONFIRMED     3
#define CONF_STATE_ACTIVE        4

// Timer constants (milliseconds)
#define RESTART_DELAY_MS        5000
#define WEB_TIMEOUT_MS          120000
#define REBOOT_ACTION_MS        1000

// Buffer sizes
#define KEY_BUFFER_SIZE         17
#define KEY_STRING_SIZE         33
#define AES_BUFFER_SIZE         100
#define CHALLENGE_KEY_SIZE      40
#define CHALLENGE_BYTES         4

// Status messages
#define MSG_READY               "Listo"
#define MSG_ENTER_KEY           "ENTER KEY"
#define MSG_REVIEWING           "REVIEWING"
#define MSG_RETRY               "Retry"

// Profile constants
#define MAX_PROFILES            2
#define MIN_PROFILES            1
#define MAX_PROFILES_ALLOWED    3

// Safe string copy helper
#define SAFE_STRCPY(dest, src, size) do { \
	if (src) { \
		strncpy(dest, src, (size) - 1); \
		dest[(size) - 1] = '\0'; \
	} \
} while(0)

// mongoose glue structures. 
//
// Delete the STATIC in the mongoose_glue.c or linker fails cannot find externals below
// AND AND double to float conversion in mongoose_glue.h
//
// below are the structures for the mongoose interface as well as Actions (independent from data)

extern struct battery s_battery;			// battery reading  
extern struct sensors s_sensors;			// sensors reading  
extern struct panels s_panels;				// panels reading  
extern struct energy s_energy;				// energy reading  
extern struct settings s_settings;			// main controller settings --> First Sidebar
extern struct system s_system;				// system setting like mqtt server etc. --> Second sidebar
extern struct profile s_profile;			// schedule profile cycles and working timers --> 5th sidebar
extern struct sysset s_sysset;				// system parameters --> third sidebar
extern struct DO s_DO;						// Dissolved Oxygen control parameters
extern uint64_t s_action_timeout_reboot;  	// Reboot button

#include "crypto_utils.h"
#include "time_utils.h"
extern void write_to_flash();
extern char levels[6][10];			// log levels external in cmdConfig.cpp
static bool restartf=false,restart_profile=false;;
extern 	int findLevel(char * cual);

/**
 * @brief Validates the authentication key against the expected challenge
 * 
 * Converts the provided key into a challenge value using AES encryption
 * and compares it against the expected challenge derived from the configuration ID.
 * 
 * @param key The 64-bit authentication key to validate
 * @return true if the key is valid, false otherwise
 */
bool check_key(uint64_t key)
{
	char cid_buffer[KEY_BUFFER_SIZE];
	char encryption_key[KEY_STRING_SIZE];
	int32_t challenge_from_key;
	
	// Extract challenge from the input key (reverse byte order)
	uint8_t *key_bytes = (uint8_t*)&key;
	uint8_t *challenge_bytes = (uint8_t*)&challenge_from_key;
	challenge_bytes += 3;  // Start from the end
	
	for (int i = 0; i < CHALLENGE_BYTES; i++)
	{
		*challenge_bytes = *key_bytes;
		challenge_bytes--;
		key_bytes++;
	}

	// Generate encryption key from configuration ID
	sprintf(cid_buffer, "%016d", theConf.cid);
	sprintf(encryption_key, "%s%s", cid_buffer, cid_buffer);  // Double the CID
	
	// Allocate buffer for encrypted challenge
	char *encrypted_challenge = (char*)calloc(AES_BUFFER_SIZE, 1);
	if (encrypted_challenge == NULL)
	{
		MESP_LOGE("KEY", "Failed to allocate memory for encryption");
		return false;
	}

	// Encrypt the secret and compare with the challenge
	int ret = shrimp_aes_encrypt(SUPERSECRET, sizeof(SUPERSECRET), encrypted_challenge, encryption_key);
	if (ret == ESP_FAIL)
	{
		MESP_LOGE("KEY", "AES encryption failed");
		free(encrypted_challenge);
		return false;
	}

	// Compare the encrypted result with the challenge from the key
	bool is_valid = (memcmp(encrypted_challenge, &challenge_from_key, CHALLENGE_BYTES) == 0);
	
	free(encrypted_challenge);
	return is_valid;
}

/**
 * @brief Helper function to check if settings should be disabled based on configuration state
 * @return 1 if disabled, 0 if enabled
 */
static inline int should_disable_settings(void)
{
	int que=0;

	if(webserverf)
		que= 11;
	else	
		que=(theConf.meterconf > CONF_STATE_PENDING) ? 1 : 0;
	return que;
}

/**
 * @brief Schedules a system restart after a delay
 * 
 * Creates and starts a timer that will save configuration and restart the device
 */
static void schedule_restart(void)
{
	TimerHandle_t restartTimer = xTimerCreate(
		"restart",
		pdMS_TO_TICKS(RESTART_DELAY_MS),
		pdFALSE,
		(void *)0,
		[](TimerHandle_t xTimer) {
			theConf.meterconf = CONF_STATE_CONFIGURED;
			write_to_flash();
			esp_restart();
		}
	);
	xTimerStart(restartTimer, 0);
}

/**
 * @brief Populates settings structure based on current configuration state
 */
static void populate_settings_data(void)
{
	if (theConf.meterconf == CONF_STATE_UNCONFIGURED)
	{
		strcpy(s_settings.msg_val, MSG_ENTER_KEY);
	}
	else
	{
		strcpy(s_settings.msg_val, MSG_REVIEWING);
		s_settings.delay_val = theConf.delay_mesh;
		s_settings.nodes_val = theConf.totalnodes;
		s_settings.unit_val = theConf.unitid;
		s_settings.master_val = theConf.masternode;
		s_settings.pool_val = theConf.poolid;
		s_settings.wifi_val = theConf.wifi_mode;
		s_settings.meshwifi_val = theConf.mesh_wifi;
	}
}

/**
 * @brief Applies validated settings to the configuration
 * @param data The settings structure containing new values
 */
static void apply_settings_to_config(struct settings *data)
{
	time_t now;
	time(&now);
	
	// Update configuration from settings
	theConf.masternode = data->master_val;
	theConf.poolid = data->pool_val;
	theConf.unitid = data->unit_val;
	theConf.totalnodes = data->nodes_val;
	theConf.delay_mesh = data->delay_val;
	theConf.wifi_mode = data->wifi_val;
	theConf.mesh_wifi = data->meshwifi_val;
	
	// Update configuration state
	theConf.meterconf = CONF_STATE_CONFIGURED;
	theConf.ptch = 1;  // Pop that Cherry :)
	theConf.meterconfdate = now;
	theConf.bornDate = theConf.meterconfdate;
	theConf.cid = 0;  // Reset Challenge ID, we are configured now
	
	write_to_flash();
}

/**
 * @brief Get current battery for the web interface
 * @param data Pointer to battery structure to populate
 */
void my_get_battery(struct battery *data) {

	solarSystem_t *solarData = theBlower.getPtrSolarsystem();
	s_battery.soc=solarData->battery.batSOC;
	s_battery.soh=solarData->battery.batSOH;	
	s_battery.temp=solarData->battery.batBmsTemp;
	s_battery.cycles=solarData->battery.batteryCycleCount;

	*data = s_battery;
}

/**
 * @brief Get current sensors for the web interface
 * @param data Pointer to sensors structure to populate
 */
void my_get_sensors(struct sensors *data) {

	solarSystem_t *solarData = theBlower.getPtrSolarsystem();
	s_sensors.airtemp = solarData->sensors.ATemp;
	s_sensors.humidity = solarData->sensors.AHum;
	s_sensors.wtemp = solarData->sensors.WTemp;
	s_sensors.ph = solarData->sensors.PH;
	s_sensors.doxy = solarData->sensors.DO;

	*data = s_sensors;
}

/**
 * @brief Get current panels for the web interface
 * @param data Pointer to panels structure to populate
 */
void my_get_panels(struct panels *data) {

	solarSystem_t *solarData = theBlower.getPtrSolarsystem();
	s_panels.pv1volts = solarData->pvPanel.pv1Volts;
	s_panels.pv2volts = solarData->pvPanel.pv2Volts;
	s_panels.pv1amps = solarData->pvPanel.pv1Amp;
	s_panels.pv2amps = solarData->pvPanel.pv2Amp;
	if(solarData->pvPanel.chargeCurr)
		strcpy(s_panels.chargingstate, "Charging");
	else
		strcpy(s_panels.chargingstate, "Discharging");
	*data = s_panels;
}

/**
 * @brief Get current energy for the web interface
 * @param data Pointer to energy structure to populate
 */
void my_get_energy(struct energy *data) {

	solarSystem_t *solarData = theBlower.getPtrSolarsystem();

	s_energy.bdisamphoy=solarData->energy.batDischgAHToday;
	s_energy.bcharamphoy=solarData->energy.batChgAHToday;
	s_energy. genkwhhoy=solarData->energy.generateEnergyToday;
	s_energy. bchkwhhoy=solarData->energy.batChgkWhToday;
	s_energy. loadkwhhoy=solarData->energy.genLoadConsumToday;


	*data = s_energy;
}

/**
 * @brief Get current settings for the web interface
 * @param data Pointer to settings structure to populate
 */
void my_get_settings(struct settings *data) {
	// Check if settings should be disabled based on configuration state
	s_settings.disable_val = should_disable_settings();
	// Set challenge initial value
	sprintf(s_settings.mac_val, "%d", theConf.cid);
	
	// Handle restart scenario
	if (restartf)
	{
		strcpy(s_settings.msg_val, MSG_READY);
		schedule_restart();
	}
	else
	{
		// Populate settings based on current state
		populate_settings_data();
	}
	
	*data = s_settings;
}

/**
 * @brief Apply new settings from the web interface after key validation
 * @param data Pointer to settings structure with new values
 */
void my_set_settings(struct settings *data) {

	// Allocate memory for challenge string
	// char *challenge_str = (char*)calloc(1, CHALLENGE_KEY_SIZE);
	// if (challenge_str == NULL)
	// {
	// 	MESP_LOGE("SETTINGS", "Failed to allocate memory for challenge");
	// 	strcpy(s_settings.msg_val, MSG_RETRY);
	// 	return;
	// }
	
	// Copy and validate the challenge key
	// strcpy(challenge_str, data->challenge_val);
	// char *endptr;
	// uint32_t key_value = strtoul(challenge_str, &endptr, 16);
	// free(challenge_str);
	
	// if (!check_key(key_value) && theConf.cid != 0)	// there is a challenge so it must be met else we editing certain fields
	// {
	// 	MESP_LOGW("SETTINGS", "Invalid authentication key (expected CID: %d)", theConf.cid);
	// 	s_settings = *data;
	// 	strcpy(s_settings.msg_val, MSG_RETRY);
	// 	return;
	// }
	
	// Challenge validated - apply settings
	MESP_LOGI("SETTINGS", "Authentication successful, applying configuration");
	s_settings = *data;
	apply_settings_to_config(data);
	restartf = true;
}

/**
 * @brief Apply system settings from web interface
 * @param data Pointer to system settings structure
 */
void my_set_system(struct system *data) {
	if (!data)
	{
		MESP_LOGE(TAG, "Invalid system data pointer");
		return;
	}
	
	s_system = *data;
	
	// Apply settings with safe string operations
	theConf.test_timer_div = s_system.repeat_val;
	theConf.collectimer = s_system.baset_val;
	
	SAFE_STRCPY(theConf.kpass, s_system.password_val, sizeof(theConf.kpass));
	
	int cual = findLevel(s_system.loglevel_val);
	if (cual >= 0)
	{
		theConf.loglevel = cual;
	}
	else
	{
		MESP_LOGW(TAG, "Invalid log level, keeping current: %d", theConf.loglevel);
	}
	
	theConf.loginwait = s_system.logtime_val;
	
	SAFE_STRCPY(theConf.mqttPass, s_system.mqttpass_val, sizeof(theConf.mqttPass));
	SAFE_STRCPY(theConf.mqttUser, s_system.mqttuser_val, sizeof(theConf.mqttUser));
	SAFE_STRCPY(theConf.mqttServer, s_system.mqttserver_val, sizeof(theConf.mqttServer));
	SAFE_STRCPY(theConf.mqttcert, s_system.mqttcert_val, sizeof(theConf.mqttcert));
	SAFE_STRCPY(theConf.thepass, s_system.ssidpass_val, sizeof(theConf.thepass));
	SAFE_STRCPY(theConf.thessid, s_system.ssid_val, sizeof(theConf.thessid));
	
	theConf.poolid = s_system.meshid_val;
	// theConf.totalnodes = s_system.nodes_val;
	// theConf.conns = s_system.conns_val;
	// theConf.mqttDiscoRetry = s_system.mqttreco_val;
	theConf.gpsSensor=s_system.gps;
	theConf.modbuson=s_system.modbussensor;
	theConf.temp_sensor=s_system.tempsensor;
	theConf.retain=s_system.retain;
	if (theConf.meterconf == CONF_STATE_CONFIRMED)
	{
		theConf.meterconf = CONF_STATE_PENDING;
	}
	
	write_to_flash();
	MESP_LOGI(TAG, "System settings applied successfully");
}

/**
 * @brief Get current system settings for web interface
 * @param data Pointer to system structure to populate
 */
void my_get_system(struct system *data) 
{
	if (!data)
	{
		MESP_LOGE(TAG, "Invalid system data pointer");
		return;
	}
	// if(webserverf)
	// 	s_settings.disable_val=11;	
	// else
	// 	s_system.disable_val = (theConf.meterconf > CONF_STATE_CONFIRMED) ? 1 : 0;
	s_system.meshid_val = theConf.poolid;
	s_system.gps=theConf.gpsSensor;
	s_system.tempsensor=theConf.temp_sensor; 
	s_system.modbussensor=theConf.modbuson;
	s_system.retain=theConf.retain;

	const esp_app_desc_t *mip = esp_app_get_description();
	if (mip && mip->version)
	{
		SAFE_STRCPY(s_system.version_val, mip->version, sizeof(s_system.version_val));
	}
	
	s_system.repeat_val = theConf.test_timer_div;
	s_system.baset_val = theConf.collectimer;
	
	SAFE_STRCPY(s_system.password_val, theConf.kpass, sizeof(s_system.password_val));
	SAFE_STRCPY(s_system.loglevel_val, levels[theConf.loglevel], sizeof(s_system.loglevel_val));
	SAFE_STRCPY(s_system.mqttpass_val, theConf.mqttPass, sizeof(s_system.mqttpass_val));
	SAFE_STRCPY(s_system.mqttuser_val, theConf.mqttUser, sizeof(s_system.mqttuser_val));
	SAFE_STRCPY(s_system.mqttserver_val, theConf.mqttServer, sizeof(s_system.mqttserver_val));
	SAFE_STRCPY(s_system.mqttcert_val, theConf.mqttcert, sizeof(s_system.mqttcert_val));
	SAFE_STRCPY(s_system.ssidpass_val, theConf.thepass, sizeof(s_system.ssidpass_val));
	SAFE_STRCPY(s_system.ssid_val, theConf.thessid, sizeof(s_system.ssid_val));
	
	*data = s_system;
}

/**
 * @brief Get current system settings for web interface
 * @param data Pointer to system structure to populate
 */
void my_get_DO(struct DO *data) 
{
	*data = theConf.doParms;
}


/**
 * @brief Apply system settings from web interface
 * @param data Pointer to system settings structure
 */
void my_set_DO(struct DO *data) {
	theConf.doParms = *data;
	write_to_flash();
}
/**
 * @brief Get system status and statistics for web interface
 * @param data Pointer to sysset structure to populate with system info
 * 
 * Retrieves comprehensive system information including:
 * - Firmware version and compile info
 * - Boot count and last reset reason
 * - Security and display settings
 * - FRAM statistics (reads/writes)
 * - Network information (MAC addresses, mesh status)
 * - Communication statistics (bytes, messages, connections)
 */
void my_get_sysset(struct sysset *data) 		// get data for general configuration and display
{
	// set s_sysset strucutre to stored values in Meter or theConf structures
	const esp_app_desc_t *mip=esp_app_get_description();
	strcpy(s_sysset.idf_val,mip->version);
	strcpy(s_sysset.compile_val,mip->idf_ver);
	s_sysset.boot_val=theConf.bootcount;
	s_sysset.lreason_val=theConf.lastResetCode;
	s_sysset.writes_val=theBlower.getFram_Writes();
	s_sysset.reads_val=theBlower.getFram_Reads();
	strcpy(s_sysset.nodetype_val,esp_mesh_is_root()?"ROOT":"NODE");		//not usefull since mesh is not active but...
	mesh_addr_t mmeshid;
	esp_mesh_get_id(&mmeshid);
	sprintf(s_sysset.mesh_val,MACSTR,MAC2STR(mmeshid.addr));			// mac ok
	unsigned char mac_base[6] = {0};
	esp_efuse_mac_get_default(mac_base);
	esp_read_mac(mac_base, ESP_MAC_WIFI_STA);
	sprintf(s_sysset.mac_val,MACSTR,MAC2STR(mac_base));
	s_sysset.bytesout_val=theBlower.getStatsBytesOut();
	s_sysset.bytesin_val=theBlower.getStatsBytesIn();
	s_sysset.msgin_val=theBlower.getStatsMsgIn();	
	s_sysset.msgout_val=theBlower.getStatsMsgOut();
	s_sysset.conns_val=theBlower.getStatsStaConns();
	s_sysset.disco_val=theBlower.getStatsStaDiscos();
    time_t  ahora=theBlower.getStatsLastCountTS();
	strcpy(s_sysset.ldate_val,ctime(&ahora));
	*data = s_sysset;
}

/**
 * @brief Set system settings (currently read-only)
 * @param data Pointer to sysset structure (unused)
 * @note This function is a placeholder as sysset is read-only
 */
void my_set_sysset(struct sysset *data) // there is no setting in this menu option
{
	s_sysset = *data;
	write_to_flash();
}

/**
 * @brief Display all configured profiles to console for debugging
 * 
 * Prints detailed information about all profiles including:
 * - Profile name and version
 * - Issued and expiry dates
 * - All cycles with start day and duration
 * - All horarios (schedules) with start time, duration, and PWM duty
 */
void show_profiles()
{
	
	for (int a=0;a<MAX_PROFILES;a++)
	{
		printf("======== Profile[%d] %s %s =========\n",a,theConf.profiles[a].name,theConf.profiles[a].version);
		printf("\tIssued %s",ctime(&theConf.profiles[a].issued));
		printf("\tExpires %s",ctime(&theConf.profiles[a].expires));
		printf("\tNumber of Cycles is %d\n",theConf.profiles[a].numCycles);
		for (int c=0;c<theConf.profiles[a].numCycles;c++)
		{
			printf(" \t\t---- Cycle[%d] Day %d Duration %d Hours %d\n",c,theConf.profiles[a].cycle[c].day,
					theConf.profiles[a].cycle[c].duration,theConf.profiles[a].cycle[c].numHorarios);
		
			for (int h=0;h<theConf.profiles[a].cycle[c].numHorarios;h++)
			{
				printf("\t\t\t• Horario[%d] Start %d Duration %d PWD %d\n",h,theConf.profiles[a].cycle[c].horarios[h].hourStart,
					theConf.profiles[a].cycle[c].horarios[h].horarioLen,theConf.profiles[a].cycle[c].horarios[h].pwmDuty);
			}
		printf("\n\n");
		}
	}	
}

/**
 * @brief Parse profile metadata (name, version, issued, expires)
 * @param pitem JSON object containing profile data
 * @param profile_index Index of the profile being parsed
 * @return ESP_OK on success, ESP_FAIL on error
 */
static err_t parse_profile_metadata(cJSON *pitem, int profile_index)
{
	struct tm tm = {0};
	cJSON *metad;
	
	if (!pitem)
	{
		MESP_LOGE(TAG, "Invalid profile item");
		return ESP_FAIL;
	}
	
	// Parse profile name with bounds checking
	metad = cJSON_GetObjectItem(pitem, "name");
	if (metad && metad->valuestring)
	{
		SAFE_STRCPY(theConf.profiles[profile_index].name, metad->valuestring, 
					sizeof(theConf.profiles[profile_index].name));
	}
	
	// Parse profile version with bounds checking
	metad = cJSON_GetObjectItem(pitem, "version");
	if (metad && metad->valuestring)
	{
		SAFE_STRCPY(theConf.profiles[profile_index].version, metad->valuestring,
					sizeof(theConf.profiles[profile_index].version));
	}
	
	// Parse issued date
	metad = cJSON_GetObjectItem(pitem, "issued");
	if (metad && metad->valuestring)
	{
		if (strptime(metad->valuestring, "%d-%m-%Y", &tm) != NULL)
		{
			theConf.profiles[profile_index].issued = mktime(&tm);
		}
		else
		{
			MESP_LOGW(TAG, "Profile %d: Invalid issued date format", profile_index);
		}
	}
		else
		{
			MESP_LOGW(TAG, "Profile %d: Invalid issued date format", profile_index);
		}
	
	
	// Parse expiry date
	metad = cJSON_GetObjectItem(pitem, "expires");
	if (metad && metad->valuestring)
	{
		if (strptime(metad->valuestring, "%d-%m-%Y", &tm) != NULL)
		{
			theConf.profiles[profile_index].expires = mktime(&tm);
		}
		else
		{
			MESP_LOGW(TAG, "Profile %d: Invalid expiry date format", profile_index);
		}
	}
	
	return ESP_OK;
}

/**
 * @brief Parse a single horario (schedule) item
 * @param hour_item JSON object containing horario data
 * @param profile_idx Profile index
 * @param cycle_idx Cycle index
 * @param hour_idx Horario index
 * @return ESP_OK on success, ESP_FAIL on error
 */
static err_t parse_horario(cJSON *hour_item, int profile_idx, int cycle_idx, int hour_idx)
{
	cJSON *hmeta;
	
	if (!hour_item)
	{
		MESP_LOGE(TAG, "Profile %d Cycle %d: Missing horario %d", profile_idx, cycle_idx, hour_idx);
		return ESP_FAIL;
	}
	
	// Parse start hour
	hmeta = cJSON_GetObjectItem(hour_item, "h_start_hour");
	if (!hmeta)
	{
		MESP_LOGE(TAG, "Profile %d Cycle %d Horario %d: Missing start hour", profile_idx, cycle_idx, hour_idx);
		return ESP_FAIL;
	}
	theConf.profiles[profile_idx].cycle[cycle_idx].horarios[hour_idx].hourStart = (float)hmeta->valueint;
	
	// Parse minutes start
	hmeta = cJSON_GetObjectItem(hour_item, "h_start_mins");
	if (!hmeta)
	{
		MESP_LOGE(TAG, "Profile %d Cycle %d Horario %d: Missing Minutes", profile_idx, cycle_idx, hour_idx);
		return ESP_FAIL;
	}
	theConf.profiles[profile_idx].cycle[cycle_idx].horarios[hour_idx].minutesStart = (float)hmeta->valueint;

	// Parse duration
	hmeta = cJSON_GetObjectItem(hour_item, "h_secs");
	if (!hmeta)
	{
		MESP_LOGE(TAG, "Profile %d Cycle %d Horario %d: Missing duration", profile_idx, cycle_idx, hour_idx);
		return ESP_FAIL;
	}
	theConf.profiles[profile_idx].cycle[cycle_idx].horarios[hour_idx].horarioLen = (float)hmeta->valueint;
	
	// Parse PWM duty
	hmeta = cJSON_GetObjectItem(hour_item, "pwm_duty");
	if (!hmeta)
	{
		MESP_LOGE(TAG, "Profile %d Cycle %d Horario %d: Missing PWM duty", profile_idx, cycle_idx, hour_idx);
		return ESP_FAIL;
	}
	theConf.profiles[profile_idx].cycle[cycle_idx].horarios[hour_idx].pwmDuty = hmeta->valueint;
	
	return ESP_OK;
}

/**
 * @brief Parse all horarios for a cycle
 * @param horas JSON array containing horarios
 * @param profile_idx Profile index
 * @param cycle_idx Cycle index
 * @return ESP_OK on success, ESP_FAIL on error
 */
static err_t parse_cycle_horarios(cJSON *horas, int profile_idx, int cycle_idx)
{
	if (!horas)
	{
		MESP_LOGE(TAG, "Profile %d Cycle %d: Missing horario array", profile_idx, cycle_idx);
		return ESP_FAIL;
	}
	
	int num_horarios = cJSON_GetArraySize(horas);
	if (num_horarios < 1 || num_horarios > MAXHORARIOS)
	{
		MESP_LOGE(TAG, "Profile %d Cycle %d: Invalid horarios count %d (max %d)", 
				 profile_idx, cycle_idx, num_horarios, MAXHORARIOS);
		return ESP_FAIL;
	}
	
	theConf.profiles[profile_idx].cycle[cycle_idx].numHorarios = num_horarios;
	
	// Parse each horario
	for (int h = 0; h < num_horarios; h++)
	{
		cJSON *hour_item = cJSON_GetArrayItem(horas, h);
		if (parse_horario(hour_item, profile_idx, cycle_idx, h) != ESP_OK)
		{
			return ESP_FAIL;
		}
	}
	
	return ESP_OK;
}

/**
 * @brief Parse a single cycle
 * @param cycle_item JSON object containing cycle data
 * @param profile_idx Profile index
 * @param cycle_idx Cycle index
 * @return ESP_OK on success, ESP_FAIL on error
 */
static err_t parse_cycle(cJSON *cycle_item, int profile_idx, int cycle_idx)
{
	cJSON *cycle_meta;
	
	if (!cycle_item)
	{
		MESP_LOGE(TAG, "Profile %d: Missing cycle %d", profile_idx, cycle_idx);
		return ESP_FAIL;
	}
	
	// Parse start day
	cycle_meta = cJSON_GetObjectItem(cycle_item, "startday");
	if (!cycle_meta)
	{
		MESP_LOGE(TAG, "Profile %d Cycle %d: Missing startday", profile_idx, cycle_idx);
		return ESP_FAIL;
	}
	theConf.profiles[profile_idx].cycle[cycle_idx].day = cycle_meta->valueint;
	
	// Parse duration (days)
	cycle_meta = cJSON_GetObjectItem(cycle_item, "dias");
	if (!cycle_meta)
	{
		MESP_LOGE(TAG, "Profile %d Cycle %d: Missing duration (dias)", profile_idx, cycle_idx);
		return ESP_FAIL;
	}
	theConf.profiles[profile_idx].cycle[cycle_idx].duration = cycle_meta->valueint;
	
	// Parse horarios
	cJSON *horas = cJSON_GetObjectItem(cycle_item, "horario");
	return parse_cycle_horarios(horas, profile_idx, cycle_idx);
}

/**
 * @brief Parse all cycles for a profile
 * @param ciclos JSON array containing cycles
 * @param profile_idx Profile index
 * @return ESP_OK on success, ESP_FAIL on error
 */
static err_t parse_profile_cycles(cJSON *ciclos, int profile_idx)
{
	if (!ciclos)
	{
		MESP_LOGE(TAG, "Profile %d: Missing cycles array", profile_idx);
		return ESP_FAIL;
	}
	
	int num_cycles = cJSON_GetArraySize(ciclos);
	if (num_cycles < 1 || num_cycles > MAXCICLOS)
	{
		MESP_LOGE(TAG, "Profile %d: Invalid cycles count %d (max %d)", 
				 profile_idx, num_cycles, MAXCICLOS);
		return ESP_FAIL;
	}
	
	theConf.profiles[profile_idx].numCycles = num_cycles;
	
	// Parse each cycle
	for (int c = 0; c < num_cycles; c++)
	{
		cJSON *cycle_item = cJSON_GetArrayItem(ciclos, c);
		if (parse_cycle(cycle_item, profile_idx, c) != ESP_OK)
		{
			return ESP_FAIL;
		}
	}
	
	return ESP_OK;
}

/**
 * @brief Parse and validate a JSON profile configuration
 * @param prof JSON string containing profile data
 * @return ESP_OK on success, ESP_FAIL on error
 */
err_t make_profile(char * prof)
{
	// Parse JSON
	cJSON *root = cJSON_Parse(prof);
	if (!root)
	{
		MESP_LOGE(TAG, "Failed to parse profile JSON");
		return ESP_FAIL;
	}
	
	// Get profiles array
	cJSON *profiles_array = cJSON_GetObjectItem(root, "profiles");
	if (!profiles_array)
	{
		MESP_LOGE(TAG, "Missing 'profiles' array in JSON");
		cJSON_Delete(root);
		return ESP_FAIL;
	}
	
	// Validate profile count
	int num_profiles = cJSON_GetArraySize(profiles_array);
	if (num_profiles < MIN_PROFILES || num_profiles >= MAX_PROFILES_ALLOWED)
	{
		MESP_LOGE(TAG, "Invalid profile count: %d (expected %d-%d)", 
				 num_profiles, MIN_PROFILES, MAX_PROFILES_ALLOWED - 1);
		cJSON_Delete(root);
		return ESP_FAIL;
	}
	
	// Parse each profile
	for (int a = 0; a < num_profiles; a++)
	{
		cJSON *pitem = cJSON_GetArrayItem(profiles_array, a);
		if (!pitem)
		{
			MESP_LOGE(TAG, "Failed to get profile %d", a);
			cJSON_Delete(root);
			return ESP_FAIL;
		}
		
		// Parse profile metadata
		if (parse_profile_metadata(pitem, a) != ESP_OK)
		{
			cJSON_Delete(root);
			return ESP_FAIL;
		}
		
		// Parse cycles
		cJSON *ciclos = cJSON_GetObjectItem(pitem, "ciclos");
		if (parse_profile_cycles(ciclos, a) != ESP_OK)
		{
			cJSON_Delete(root);     
			return ESP_FAIL;
		}
	}
	
	cJSON_Delete(root);
	MESP_LOGI(TAG, "Successfully parsed %d profile(s)", num_profiles);
	return ESP_OK;
}

/**
 * @brief Set and validate profile configuration from web interface
 * @param data Pointer to profile structure with new configuration
 */
int sanity_check_profile()
{
	// two main checks: cycle and horarios do not overlap
	// and that the profile is valid for current date
	for (int p = 0; p < MAX_PROFILES; ++p)
	{
		profile_t *prof = &theConf.profiles[p];
		if (prof->numCycles == 0)
		{
			continue;
		}
		
		// Check cycle overlaps (day ranges)
		for (int c1 = 0; c1 < prof->numCycles; ++c1)
		{
			uint32_t start1 = prof->cycle[c1].day;
			uint32_t end1 = start1 + prof->cycle[c1].duration; // end is exclusive
			
			for (int c2 = c1 + 1; c2 < prof->numCycles; ++c2)
			{
				uint32_t start2 = prof->cycle[c2].day;
				uint32_t end2 = start2 + prof->cycle[c2].duration; // end is exclusive
				
				if ((start1 < end2) && (start2 < end1))
				{
					MESP_LOGE(TAG, "Profile %d: Cycle overlap between %d (day %u len %u) and %d (day %u len %u)",
							 p, c1, (unsigned)start1, (unsigned)prof->cycle[c1].duration,
							 c2, (unsigned)start2, (unsigned)prof->cycle[c2].duration);
					return ESP_FAIL;		// abort process
				}
			}
			
			// Check horario overlaps within each cycle (hour ranges)
			uint8_t num_h = prof->cycle[c1].numHorarios;
			for (int h1 = 0; h1 < num_h; ++h1)
			{
				float h1_start = prof->cycle[c1].horarios[h1].hourStart
					+ prof->cycle[c1].horarios[h1].minutesStart/60;;
				float h1_end = h1_start + prof->cycle[c1].horarios[h1].horarioLen/3600;
				
				for (int h2 = h1 + 1; h2 < num_h; ++h2)
				{
					float h2_start = prof->cycle[c1].horarios[h2].hourStart+  
						prof->cycle[c1].horarios[h2].minutesStart/60;
					float h2_end = h2_start + prof->cycle[c1].horarios[h2].horarioLen/3600;
					
					if ((h1_start < h2_end) && (h2_start < h1_end))
					{
						MESP_LOGE(TAG, "Profile %d Cycle %d: Horario overlap between %d (%d-%d) and %d (%d-%d)",
								 p, c1, h1, (int)h1_start, (int)h1_end,
								 h2, (int)h2_start, (int)h2_end);
						return ESP_FAIL;		// abort process
					}
				}
			}
		}
	}
	return ESP_OK;
}

/**
 * @brief Set and validate profile configuration from web interface
 * @param data Pointer to profile structure with new configuration
 */
void my_set_profile(struct profile *data)
{
	if (!data)
	{
		MESP_LOGE(TAG, "Invalid profile data pointer");
		return;
	}
	
	s_profile = *data;
	
	// Validate JSON before processing
	cJSON *prof_root = cJSON_Parse(s_profile.schedule);
	if (!prof_root)
	{
		MESP_LOGE(TAG, "Invalid JSON Profile format");
		return;
	}
	
	// Try to save to file
	FILE* f = fopen(PROFILE_FILE, "w");
	if (f == NULL)
	{
		MESP_LOGE(TAG, "Failed to open profile file for writing: %s", PROFILE_FILE);
		cJSON_Delete(prof_root);
		return;
	}
	
	size_t written = fprintf(f, "%s", s_profile.schedule);
	fclose(f);
	
	if (written <= 0)
	{
		MESP_LOGE(TAG, "Failed to write profile to file");
		cJSON_Delete(prof_root);
		return;
	}
	
	cJSON_Delete(prof_root);
	restart_profile = true;
	// Parse and apply profile
	err_t err = make_profile(s_profile.schedule);
	if (err == ESP_OK)
	{
		if(sanity_check_profile()!=ESP_OK)
		{
			MESP_LOGE(TAG, "Profile sanity check failed, not applying profile");
			strcpy(s_profile.msg,"Conflicts");
			return;
		}
		strcpy(s_profile.msg,"OK");
		write_to_flash();
		show_profiles();
		MESP_LOGI(TAG, "Profile successfully applied");
	}
	else
	{
		MESP_LOGE(TAG, "Failed to parse and apply profile");
	}
}

/**
 * @brief Get current profile configuration for web interface
 * @param data Pointer to profile structure to populate
 */
void my_get_profile(struct profile *data)
{
	if (!data)
	{
		MESP_LOGE(TAG, "Invalid profile data pointer");
		return;
	}

	if(!restart_profile)
		strcpy(s_profile.msg,"");

	// s_settings.disable_val = should_disable_settings();
	
	FILE* f = fopen(PROFILE_FILE, "r");
	if (f == NULL)
	{
		MESP_LOGW(TAG, "Profile file not found, using empty profile");
		s_profile.schedule[0] = '\0';
		*data = s_profile;
		return;
	}
	
	// Read with size limit to prevent buffer overflow
	if (fgets(s_profile.schedule, sizeof(s_profile.schedule), f) == NULL)
	{
		MESP_LOGE(TAG, "Failed to read profile file");
		s_profile.schedule[0] = '\0';
	}
	
	fclose(f);
	*data = s_profile;
}

/**
 * @brief Set Modbus inverter configuration
 * @param data Pointer to modbInverter structure with new settings
 */
void my_set_modbInverter(struct modbInverter *data) // save limits from web to theblower
{
	theConf.modbus_inverter=*data;
	write_to_flash();
}

/**
 * @brief Get current Modbus inverter configuration
 * @param data Pointer to modbInverter structure to populate
 */
void my_get_modbInverter(struct modbInverter *data) // return limits saved in theblower
{
	// if( theConf.meterconf>CONF_STATE_PENDING)
   	// 	s_settings.disable_val=1;
	// else
   	// 	s_settings.disable_val=0;

	*data=theConf.modbus_inverter;
}

/**
 * @brief Set Modbus sensors configuration
 * @param data Pointer to modbSensors structure with new settings
 */
void my_set_modbSensors(struct modbSensors *data) // save limits from web to theblower
{
	theConf.modbus_sensors=*data;
	write_to_flash();
}

/**
 * @brief Get current Modbus sensors configuration
 * @param data Pointer to modbSensors structure to populate
 */
void my_get_modbSensors(struct modbSensors *data) // return limits saved in theblower
{
	*data=theConf.modbus_sensors;
}

/**
 * @brief Set Modbus battery configuration
 * @param data Pointer to modbBattery structure with new settings
 */
void my_set_modbBattery(struct modbBattery *data) // save limits from web to theblower
{
	theConf.modbus_battery=*data;
	write_to_flash();
}

/**
 * @brief Get current Modbus battery configuration
 * @param data Pointer to modbBattery structure to populate
 */
void my_get_modbBattery(struct modbBattery *data) // return limits saved in theblower
{
	*data=theConf.modbus_battery;
}

/**
 * @brief Get current Modbus solar panels configuration
 * @param data Pointer to modbPanels structure to populate
 */
void my_get_modbPanels(struct modbPanels *data) // return limits saved in theblower
{

	*data=theConf.modbus_panels;
}

/**
 * @brief Set Modbus solar panels configuration
 * @param data Pointer to modbPanels structure with new settings
 */
void my_set_modbPanels(struct modbPanels *data) // save limits from web to theblower
{
	theConf.modbus_panels=*data;
	write_to_flash();
}

/**
 * @brief Initialize SPIFFS filesystem for profile storage
 * 
 * Initializes the SPIFFS partition for storing profile configurations.
 * Performs the following:
 * - Mounts the SPIFFS partition labeled "profile"
 * - Runs filesystem integrity checks
 * - Reports partition size and usage
 * - Auto-formats if mount fails
 * 
 * The filesystem is mounted at /spiffs with a maximum of 2 files.
 */
void app_spiffs(void)
{
    MESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = "profile",
      .max_files = 2,
      .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            MESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            MESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            MESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }


    MESP_LOGI(TAG, "Performing SPIFFS_check().");
    ret = esp_spiffs_check(conf.partition_label);
    if (ret != ESP_OK) {
        MESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
        return;
    } else {
        MESP_LOGI(TAG, "SPIFFS_check() successful");
    }


    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        MESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
        esp_spiffs_format(conf.partition_label);
        return;
    } else {
        MESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    // Check consistency of reported partition size info.
    if (used > total) {
        MESP_LOGW(TAG, "Number of used bytes cannot be larger than total. Performing SPIFFS_check().");
        ret = esp_spiffs_check(conf.partition_label);
        // Could be also used to mend broken files, to clean unreferenced pages, etc.
        // More info at https://github.com/pellepl/spiffs/wiki/FAQ#powerlosses-contd-when-should-i-run-spiffs_check
        if (ret != ESP_OK) {
            MESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
            return;
        } else {
            MESP_LOGI(TAG, "SPIFFS_check() successful");
        }
    }

}

/**
 * @brief Check if system reboot is in progress
 * @return true if reboot action is active, false otherwise
 */
bool my_check_reboot(void) {
  return s_action_timeout_reboot > mg_now(); // Return true if Done is in progress
}

/**
 * @brief Initiate system reboot
 * @param params Mongoose parameters (unused)
 * 
 * Triggers a system restart after saving the configuration state.
 * Sets configuration state to CONFIGURED before rebooting.
 */
void my_start_reboot(struct mg_str params) {		//Done button pressed,
	s_action_timeout_reboot = mg_now() + REBOOT_ACTION_MS; // Start Done, finish after 1 second
	theConf.meterconf=CONF_STATE_CONFIGURED;
	esp_rom_printf("Webserver Restarting System\n");
	write_to_flash();
	esp_restart();
}

/**
 * @brief Start and run the web server
 * @param pArg Task parameter (unused)
 * 
 * Initializes the Mongoose web server and registers all HTTP handlers:
 * - settings: Initial device configuration with authentication
 * - system: System-wide settings (WiFi, MQTT, security)
 * - sysset: Read-only system status and statistics
 * - profile: Schedule profile management
 * - modbInverter: Modbus inverter settings
 * - modbSensors: Modbus sensors settings
 * - modbBattery: Modbus battery settings
 * - modbPanels: Modbus solar panels settings
 * - reboot: System reboot control
 * 
 * Creates a watchdog timer that restarts the device if configuration
 * is not completed within the timeout period (WEB_TIMEOUT_MS).
 * 
 * Runs indefinitely, polling the Mongoose event loop.
 */
void start_webserver(void *pArg)
{
	MESP_LOGW(MESH_TAG,"Starting webserver");
	mongoose_init();
	if(webserverf)
		s_settings.disable_val=11;		
  	mongoose_set_http_handlers("battery", my_get_battery, NULL);		
  	mongoose_set_http_handlers("sensors", my_get_sensors, NULL);		
  	mongoose_set_http_handlers("panels", my_get_panels, NULL);		
  	mongoose_set_http_handlers("energy", my_get_energy, NULL);		
  	mongoose_set_http_handlers("settings", my_get_settings, my_set_settings);		
  	mongoose_set_http_handlers("system", my_get_system ,my_set_system);				
  	mongoose_set_http_handlers("sysset", my_get_sysset ,my_set_sysset);				
  	mongoose_set_http_handlers("profile", my_get_profile ,my_set_profile);							
  	mongoose_set_http_handlers("modbInverter", my_get_modbInverter ,my_set_modbInverter);				
  	mongoose_set_http_handlers("modbSensors", my_get_modbSensors ,my_set_modbSensors);				
  	mongoose_set_http_handlers("modbBattery", my_get_modbBattery ,my_set_modbBattery);				
  	mongoose_set_http_handlers("modbPanels", my_get_modbPanels ,my_set_modbPanels);				
  	mongoose_set_http_handlers("DO", my_get_DO ,my_set_DO);				
  	mongoose_set_http_handlers("reboot", my_check_reboot ,my_start_reboot);		

	//web timeout if not done in 2 minutes restart
	webTimer=xTimerCreate("restart",pdMS_TO_TICKS(WEB_TIMEOUT_MS),pdFALSE,( void * ) 0, [] ( TimerHandle_t xTimer){	
			theConf.meterconf=CONF_STATE_PENDING;
			write_to_flash();
			esp_restart();});   

// start the meter now
	// xTimerStart(webTimer,0);

	for (;;) {
    mongoose_poll();
  	}

}



#endif