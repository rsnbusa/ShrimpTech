#ifndef FORWARDS_H
#define FORWARDS_H

/**
 * @file forwards.h
 * @brief Forward declarations for all public functions in the ShrimpIoT project
 * 
 * This file contains function prototypes organized by functional area to allow
 * cross-module function calls without circular dependencies.
 */

// ============================================================================
// SYSTEM & UTILITY FUNCTIONS
// ============================================================================
// Note: delay() and xmillis() moved to time_utils.h
// Note: AES functions moved to crypto_utils.h

// ============================================================================
// TIMER FUNCTIONS
// ============================================================================

void blower_end(TimerHandle_t xTimer);
void blower_start(TimerHandle_t xTimer);
void root_set_senddata_timer();
void start_schedule_timers(void *pArg);

// ============================================================================
// DISPLAY FUNCTIONS
// ============================================================================

void displayManager(void *pArg);
esp_err_t init_lcd();
void oled_disp(lv_disp_t *disp);
void showData(void *pArg);

// ============================================================================
// MESH NETWORK FUNCTIONS
// ============================================================================

void check_dups(cJSON *elArr, int nodeid);
char *convertBinaryToJson(meshunion_t *elNodo);
void delete_dup_buffers();
void print_blower(char *title, solarSystem_t *msolar, bool dumphex);
err_t root_mesh_broadcast_msg(char *msg);
meshunion_t *sendData(bool forced);
void send_duplicate_msg();
void send_login_msg(char *title);

// ============================================================================
// MQTT FUNCTIONS
// ============================================================================

void mqttMgr(void *pArg);
void root_mqtt_sender(void *pArg);
esp_err_t root_send_confirmation_central(char *msg, uint16_t size, char *cualQ);
static void root_mqtt_app_start();
static esp_mqtt_client_handle_t root_setupMqtt();

// ============================================================================
// TASK FUNCTIONS
// ============================================================================

void emergencyTask(void *pArg);
void kbd(void *pArg);
void main_app(void *parg);
void nimble_test(void *parg);
void rs485_task(void *pArg);
void rs485_task_manager(void *pArg);
void sensor_task(void *pArg);
void battery_task(void *pArg);
void panels_task(void *pArg);
void energy_task(void *pArg);
void start_webserver(void *pArg);
void generic_modbus_task(void *pArg);
void print_energy_data(void* energy,  int *errors);
void print_battery_data(void* batteryData, int *errors);
void print_panel_data(void* pvPanel, int *errors);
// ============================================================================
// CONFIGURATION & STORAGE FUNCTIONS
// ============================================================================

void erase_config();
static void pcnt_init(int *unit);
void save_inst_msg(char *mid, int bpk, int kwhstart, char *who);
void send_911_call(char *theEmergency, char *section);
void write_to_flash();

// ============================================================================
// LOGGING FUNCTIONS
// ============================================================================

void logFileInit();
err_t read_log(int nlines);
void writeLog(char *que);

// ============================================================================
// CONSOLE COMMAND FUNCTIONS (Pointer-based)
// ============================================================================

int cmdDisplay(void *argument);
int cmdEraseMetrics(void *argument);
int cmdFormat(void *argument);
int cmdLock(void *argument);
int cmdLogs(void *argument);
int cmdMQTT(void *argument);
int cmdNetw(void *argument);
int cmdProd(void *argument);
int cmdReset(void *argument);
int cmdSendMetrics(void *argument);
int cmdUpdate(void *argument);

// ============================================================================
// CONSOLE COMMAND FUNCTIONS (argc/argv-based)
// ============================================================================

int cmdAdc(int argc, char **argv);
int cmdApp(int argc, char **argv);
int cmdBasetimer(int argc, char **argv);
int cmdBlow(int argc, char **argv);
int cmdConfig(int argc, char **argv);
int cmdDebug(int argc, char **argv);
int cmdEnDecrypt(int argc, char **argv);
int cmdErase(int argc, char **argv);
int cmdFindUnit(int argc, char **argv);
int cmdFram(int argc, char **argv);
int cmdLog(int argc, char **argv);
int cmdLogLevel(int argc, char **argv);
int cmdMeter(int argc, char **argv);
int cmdMetersreset(int argc, char **argv);
int cmdMID(int argc, char **argv);
int cmdNode(int argc, char **argv);
int cmdPCNT(int argc, char **argv);
int cmdPrepaid(int argc, char **argv);
int cmdResetConf(int argc, char **argv);
int cmdSecurity(int argc, char **argv);
int cmdSkip(int argc, char **argv);
int cmdZeroMeter(int argc, char **argv);

#endif // FORWARDS_H
