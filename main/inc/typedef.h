#ifndef TYPEDEF_H
#define TYPEDEF_H
#include "includes.h"
#include "defines.h"
#include "mongoose_glue.h"  // For website definitions
#include "mbcontroller.h"    // For mb_parameter_descriptor_t

// Mongoose send double for floats so we will typedef it back to float



// ============================================================================
// MODBUS TYPES
// ============================================================================

typedef struct mod_st { 
    uint8_t  slave_address;
    uint8_t  function_code;
    uint16_t address;
    uint16_t points; 
    uint16_t timeout;
    uint8_t  tipo;
    void *   theAnswer;
    uint8_t  error;
} modbus_array_t;

typedef struct answer {
    char *theanswer;
    uint16_t len;
    time_t ts;
} answer_t;

// Modbus command line argument types
typedef struct modbus_general {
    struct arg_int *slave;        
    struct arg_int *address;        
    struct arg_int *points; 
    struct arg_str *saveid;       
    struct arg_end *end;
} modbus_general_t;

typedef struct modbus_generic{
    struct arg_int *slave;        
    struct arg_int *cmd;        
    struct arg_int *address;        
    struct arg_int *points;    
    struct arg_str *saveid;
    struct arg_end *end;
} modbus_generic_t;


typedef struct modbus_rec{
    uint8_t         slaveAddress;
    uint8_t         functionCode;
    uint16_t        startAdrress;
    uint16_t        countBytes;
    uint16_t crc;
} modbus_rec_t;

typedef struct rs485q_st {
    mb_parameter_descriptor_t*  descriptors;
    void *                      dataReceiver;      
    uint16_t                    numCids;
    TaskHandle_t                requester;
    int *                       errCode;
    int                         numerrs;
}
rs485queue_t;   

// ============================================================================
// ModBus Working Structures for dynamic Descriptors
// ============================================================================

//a "entry" representation of number of Columns in the field (OFFSET, START, POINTS, TYPE, MUX)
// in sensor case it also has the address of the sensor

typedef struct {
    double mux;
    int devices[5];
} five_t;
typedef struct {
    double mux;
    int devices[4];
} four_t;

typedef struct  {
    int regfresh;
    five_t specs[5];
} sensors_modbus_specs_t;

typedef struct  {
    int regfresh,addr;
    four_t specs[20];
} general_4modbus_specs_t;
typedef struct  {
    int regfresh,addr;
    five_t specs[20];
} general_5modbus_specs_t;

typedef struct {
    mb_parameter_descriptor_t devices[20];
} descriptor_array_t;

typedef void (*printcb)(void*, int *, char * color,int numerrs);

typedef struct {
    char *      modbus_sensor_name;
    uint8_t     modbus_sensor_spec_count;
    uint8_t     modbus_sensor_spec_columns;
    void *      modbus_sensor_specs;
    void *      modbus_sensor_data;
    uint16_t    modbus_sensor_data_size;
    printcb     modbus_print_function;
    char *      color;
} modbus_sensor_type_t;

// ============================================================================
// WEB STATE & DEBUG ENUMS
// ============================================================================

typedef enum {
    wNONE,
    wLOGIN,
    wMENU,
    wSETUP,
    wCHALL
} wstate_t;

enum {
    dSCH,
    dMESH,
    dBLE,
    dMQTT,
    dXCMDS,
    dBLOW,
    dLOGIC,
    dMODBUS,
    dLIMITS
} debug_flags_t;

typedef enum {
AHUM=0,
ATEMP,
WTEMP,
LIMITPH,
LIMITDO,
GENLCT,
BMDDKW,
BMCHKW,
LCONLI,
USEDEN,
GENEER,
BMDDKT,
BMCHKT,
BMDDAH,
BMCHAH,
BMTEMP,
BMCC,
BMSOH,
BMSOC,
PV1A,
PV1V
} limits_enum;

// ============================================================================
// PROFILE & SCHEDULE TYPES
// ============================================================================

typedef struct {
    uint8_t hourStart;
    uint32_t horarioLen;
    uint8_t pwmDuty;
} horario_t;

typedef struct {
    uint16_t day;
    uint32_t duration;
    uint8_t numHorarios;
    horario_t horarios[MAXHORARIOS];
} ciclo_t;

typedef struct {
    char name[20];
    char version[30];
    time_t issued;
    time_t expires;
    uint8_t numCycles;
    ciclo_t cycle[MAXCICLOS];
} profile_t;

typedef struct {
    uint8_t cycle;
    uint8_t day;
    uint8_t horario;
    uint8_t tostart;
    uint8_t horaslen;
    uint8_t pwm;
} start_timer_ctx_t;


// ============================================================================
// COMMAND LINE ARGUMENT TYPES
// ============================================================================

typedef struct fram {
    struct arg_str *format;  // Format WHOLE FRAM
    struct arg_str *midw;    // Read working meter ID
    struct arg_int *midr;    // Write working meter ID
    struct arg_int *seed;    // Write working meter kWh start
    struct arg_int *rstart;  // Read working meter kWh start
    struct arg_int *mtime;   // Write working meter datetime
    struct arg_int *mbeat;   // Write working meter beatstart aka beat
    struct arg_str *stats;   // Init meter internal values
    struct arg_end *end;
} framArgs_t;

typedef struct dbg {
    struct arg_str *schedule;  // Schedule timing etc
    struct arg_str *mesh;      // Mesh related
    struct arg_str *ble;       // BLE related
    struct arg_str *mqtt;      // MQTT related
    struct arg_str *xcmds;     // External commands related
    struct arg_str *blow;      // Blower data
    struct arg_str *logic;     // Logic data
    struct arg_str *modbus;    // modbus data
    struct arg_str *limits;    // limits tracking
    struct arg_str *all;       // All commands on/off
    struct arg_end *end;
} dbg_t;

typedef struct cfg {
    struct arg_lit *sch;        // All configuration x
    struct arg_lit *meshnet;     // Mesh related x
    struct arg_lit *mqtt;       // MQTT related x
    struct arg_lit *profile;    // Profile data x
    struct arg_lit *blow;       // Blower data x
    struct arg_lit *modbus;     // modbus data x
    struct arg_lit *limits;     // BLE related x
    struct arg_lit *prod  ;     // production data x
    struct arg_lit *stats  ;    // statistics data x
    struct arg_lit *system  ;    // system data x
    struct arg_lit *all;       // All commands on/off
    struct arg_end *end;
} cfg_t;

typedef struct logl{
    struct arg_str *level;        
    struct arg_end *end;
} loglevel_t;

typedef struct timerl {
    struct arg_int *first;
    struct arg_int *repeat;  // Init meter internal values
    struct arg_int *minute;  // Init meter internal values
    struct arg_end *end;
} timerl_t;

typedef struct prepaid {
    struct arg_int *unit;
    struct arg_int *balance;  // Init meter internal values
    struct arg_end *end;
} prepaid_t;

typedef struct aes_en_ec{
    struct arg_int *key;        
    struct arg_end *end;
} aes_en_dec_t;

typedef struct resetconf{
    struct arg_str *cflags;        
    struct arg_end *end;
} resetconf_t;

typedef struct securit{
    struct arg_str *password; 
    struct arg_str *newpass; 
    struct arg_str *nopass; 
    struct arg_end *end;
} securit_t;

typedef struct logop{
    struct arg_int *show;
    struct arg_int *erase;
    struct  arg_end *end;
} logargs_t;
typedef struct blowst {
    struct arg_str *seed;
    struct arg_str *init;
    struct arg_int *minute;
    struct arg_end *end;
} blow_t;

// ============================================================================
// MESH NETWORK TYPES
// ============================================================================

typedef struct medidores_mac {
    mesh_addr_t     big_table[MAXNODES];
    void            *thedata[MAXNODES];
} medidores_mac_t;

typedef struct master_Node {
    medidores_mac_t theTable;
    int existing_nodes;
} master_node_t;

typedef struct poolNodes_st{
    mesh_addr_t     address_table[MAXNODES];
    int             existing_nodes;
} poolNodes_t;

// ============================================================================
// FUNCTION POINTER TYPES
// ============================================================================

typedef int (*functrsn)(void *);

// ============================================================================
// MQTT TYPES
// ============================================================================

typedef struct mqttMsgInt {
    uint8_t *message;    // Memory of message. MUST be freed by the Submode Routine and allocated by caller
    uint16_t msgLen;
    char *queueName;     // Queue name to send
    uint32_t maxTime;    // Max ms to wait
} mqttMsg_t;

typedef struct mqttRecord {
    char *msg, *queue;
    uint16_t lenMsg;
    functrsn code;
    uint32_t *param;
    mesh_addr_t *addr;
} mqttSender_t;

// ============================================================================
// COMMAND & RECORD TYPES
// ============================================================================

typedef struct cmdRecord {
    char comando[20];
    char abr[6];
    functrsn code;
    uint32_t count;
} cmdRecord;

typedef struct medbkcup {
    char mid[13];
    uint16_t bpk;
    time_t backdate;
    uint32_t kwhstart;
} med_bck;

// ============================================================================
// CONFIGURATION TYPES
// ============================================================================

/**
 * @brief Main device configuration structure stored in flash/NVS
 * 
 * Contains all persistent configuration including network settings, MQTT credentials,
 * WiFi parameters, profiles, and Modbus device configurations.
 */
typedef struct config {
    time_t bornDate;
    uint32_t bootcount, lastResetCode, centinel;
    uint8_t minutes, masternode, unitid;
    uint32_t downtime;      // Downtime accumulator
    uint32_t mqttSlots;     // Slot number
    uint16_t loglevel;
    uint8_t meterconf, ptch;
    uint32_t lastRebootTime, meterconfdate, baset, cid, subnode, poolid;
    char mqttServer[100];
    char mqttUser[50];
    char mqttPass[50];
    char thessid[40], thepass[20];
    uint32_t totalnodes;  // Exact number of nodes in this pool
    uint16_t conns;       // Time to wait before sending metrics in ms
    uint16_t    repeat;
    char        kpass[20];
    time_t      lastKnownDate;
    int         mqttcertlen;
    char        mqttcert[2100];
    uint8_t     useSec;
    char        lastVersion[20];
    uint32_t    mqttDiscoRetry;
    profile_t profiles[MAXPROFILES];
    uint8_t activeProfile, dayCycle;
    time_t dateProfile, dateDayCycle;
    uint8_t blower_mode;           // 0=off, 1=activated for power loss cases
    uint32_t debug_flags, test_timer_div;
    uint8_t work_cycle, work_day, unit_num, delay_mesh;
    uint32_t loginwait;
    int limits[21][2];             // For 21 variables: [0]=min, [1]=max
    uint16_t    baud;
    uart_port_t port;
    struct limits milim;
    struct modbInverter modbus_inverter;
    struct modbSensors  modbus_sensors;
    struct modbBattery  modbus_battery;
    struct modbPanels modbus_panels;
} config_flash;

// ============================================================================
// METER TYPES
// ============================================================================

typedef struct meshp {
    mesh_addr_t *from;
    mesh_data_t *data;
} meshp_t;

/**
 * @brief Meter data structure - exact match to FRAM memory layout
 * 
 * This structure mirrors the physical layout in FRAM memory.
 * Memory offsets are documented for each field.
 * Total structure size must match FRAM allocation.
 */
typedef struct meterType2 {  // Exact match to FRAM struct
    uint16_t beat;       // Offset 2
    char mid[12];        // Offset 14
    uint8_t paymode, onoff;  // Offset 16
    int32_t prebal;      // Can be negative, Offset 20
    uint16_t maxamp, minamp;  // Offset 24
    uint32_t bpk, beatlife, kwhstart, lastupdate, lifekwh, lifedate, lastclock;  // Offset 52
    uint16_t months[12];      // Offset 76
    uint8_t monthHours[12][24];  // Offset 364
    uint32_t framWrites;
    uint32_t framReads;
    uint32_t pulseerrs;
} meterType;

// ============================================================================
// PACKED NETWORK MESSAGE TYPES
// ============================================================================

#pragma pack(push, 2)  // Align on word boundary for network transmission

/**
 * @brief Packed mesh network message format
 * 
 * This structure is transmitted over the mesh network.
 * Packed to 2-byte alignment for consistent network transmission.
 * Total message size: 62 bytes
 * Memory offsets are critical for protocol compatibility.
 */
typedef struct mesh_md_sg {
    char meter_id[12];     // Offset 0: Meter ID
    uint8_t paym;          // Offset 12: Payment method
    uint8_t poweroff;      // Offset 13: Locked or not
    uint16_t preval;       // Offset 14: Prepaid balance
    uint32_t kwhlife;      // Offset 16: kWh for life
    uint32_t beatlife;     // Offset 20: Beats for life
    uint16_t maxamp;       // Offset 24: Max amp registered
    uint16_t monthnum;     // Offset 26: Month number
    uint16_t monthkwh;     // Offset 28: Month kWh
    uint32_t intid;        // Offset 30: Unit's MAC
    uint32_t fwr;          // Offset 34: FRAM writes
    uint8_t horas[24];     // Offset 38: 24 hours kWh
} mesh_msg_t;  // Message size is 62 bytes


typedef struct thenode {
    uint32_t nodeid, subnode;
    uint32_t tstamp, msgnum;
    solarDef_t solarData;
} thenode_t;

/**
 * @brief Union wrapper for binary/JSON mesh data transmission
 * 
 * Allows same data to be sent as either:
 * - Binary format (nodedata)
 * - JSON string format (parajson)
 * Both formats share the same memory space.
 * Total size: 82 bytes (4 byte cmd + 78 byte payload)
 */
typedef struct thebigunion {  // Size: 4 + 78 = 82 bytes
    uint32_t cmd;
    union {
        thenode_t nodedata;                  // For binary data format
        char parajson[sizeof(thenode_t)];    // For JSON (same size as cmd)
    };
} meshunion_t;

#pragma pack(pop)

// ============================================================================
// UTILITY TYPES
// ============================================================================

typedef struct locker {
    char *data;
    int len;
} locker_t;

typedef struct lvg {
    char *msg;
    uint32_t wait;
    bool bigf;
} show_lvgl_t;

#pragma pack(push, 2)

/**
 * @brief Aggregated pool metrics message for MQTT transmission
 * 
 * Contains averaged solar system metrics for an entire pool of nodes.
 * Packed to 2-byte alignment for network efficiency.
 */
typedef struct shrimpMsg_st {
    uint32_t centinel;                  // offset 0
    uint16_t poolid;                     // offset 4    
    uint16_t countnodes;                 // offset 6
    time_t msgTime;                      // offset 8
    solarSystem_t poolAvgMetrics;        // offset 16 size 100
    uint16_t lim_errs;          // offset 116
} shrimpMsg_t;                  // totla size 118
#pragma pack(pop)

#endif // TYPEDEF_H
