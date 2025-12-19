#ifndef TYPES_H_
#define TYPES_H_
#include "includes.h"
#include "defines.h"

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
    dLOGIC
};

typedef enum {
AHUM,
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


//kbd definitions 
typedef struct fram{
    struct arg_str *format;         //format WHOLE fram
    struct arg_str *midw;            // read working meter id
    struct arg_int *midr;            // write working meter id
    struct arg_int *seed;       // write working meter kwh start
    struct arg_int *rstart;         // read working meter kwh start
    struct arg_int *mtime;          // write working meter datetime
    struct arg_int *mbeat;          // write working meter beatstart aka beat
    struct arg_str *stats;          // init meter internal values
    struct arg_end *end;
} framArgs_t;

typedef struct dbg{
    struct arg_str *schedule;         // schedule timing etc
    struct arg_str *mesh;            // mesh related
    struct arg_str *ble;            // ble related
    struct arg_str *mqtt;            // mqtt related
    struct arg_str *xcmds;            // external cmds related
    struct arg_str *blow;            // print. blower data
    struct arg_str *logic;            // print. blower data
    struct arg_str *all;            // all cmds on/off
    struct arg_end *end;
} dbg_t;

typedef struct logl{
    struct arg_str *level;        
    struct arg_end *end;
} loglevel_t;

typedef struct timerl{
    
    struct arg_int *first; 
    struct arg_int *repeat;          // init meter internal values
    struct arg_end *end;
} timerl_t;

typedef struct prepaid{
    
    struct arg_int *unit; 
    struct arg_int *balance;          // init meter internal values
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

//end kbd defs
typedef struct medidores_mac{
    mesh_addr_t     big_table[MAXNODES];
    bool            received[MAXNODES];
    void            *thedata[MAXNODES];
    char            meterName[MAXNODES][20];
    uint32_t        lastkwh[MAXNODES];
    uint8_t         skipcounter[MAXNODES];
    bool            sendit[MAXNODES];
    uint8_t         onoff[MAXNODES];
} medidores_mac_t;

typedef struct master_Node{
    medidores_mac_t     theTable;
    int                 existing_nodes;
}master_node_t;

typedef struct mqttMsgInt{
	uint8_t 	*message;	// memory of message. MUST be freed by the Submode Routine and allocated by caller
	uint16_t	msgLen;
	char		*queueName;	// queue name to send
	uint32_t	maxTime;	//max ms to wait
}mqttMsg_t;


typedef int (*functrsn)(void *);

typedef struct cmdRecord{
    char 		comando[20];
    char        abr[6];
    functrsn 	code;
    uint32_t	count;
}cmdRecord;

typedef struct mqttRecord{
    char        * msg,*queue;
    uint16_t    lenMsg;
    functrsn 	code;
    uint32_t    *param;
    mesh_addr_t *addr;
}mqttSender_t;

typedef struct medbkcup{
    char        mid[13];
    uint16_t    bpk;
    time_t      backdate;
    uint32_t    kwhstart;
} med_bck;

typedef struct config {
    time_t 		bornDate;
    uint32_t 	bootcount,lastResetCode,centinel;
    uint8_t		bleboot,masternode,unitid;
    uint32_t    downtime;               //downtime accumulator
    uint32_t    mqttSlots;          //slot number
    uint16_t    loglevel;
    uint8_t     meterconf,ptch;
    uint32_t    lastRebootTime,meterconfdate,baset,cid,subnode,poolid;
    char        mqttServer[100];
    char        mqttUser[50];
    char        mqttPass[50];
    char        thessid[40], thepass[20];
    uint32_t    totalnodes;     // this gives the EXACT number of nodes in this pool
    uint16_t    conns;          // time to wiat before sending metrics in ms
    uint16_t    repeat;
    char        kpass[20];
    time_t      lastKnownDate;
    int         mqttcertlen;
    char        mqttcert[2100];
    uint8_t     useSec;
    char        lastVersion[20];
    uint32_t    mqttDiscoRetry;
    profile_t   profiles[MAXPROFILES];
    uint8_t     activeProfile,dayCycle;
    time_t      dateProfile,dateDayCycle;
    uint8_t     blower_mode;        //0 off 1 activated for power loss cases
    uint32_t    debug_flags,test_timer_div;
    uint8_t     work_cycle,work_day,unit_num,delay_mesh;
    uint32_t    loginwait;
    int         limits[21][2];     //for 21 variables , min 0 max 1

} config_flash;

typedef struct meshp{
    mesh_addr_t *from;
    mesh_data_t *data;
} meshp_t;
typedef struct meterType2{          //exact match to fram struct
    uint16_t beat;              //2
    char        mid[12];               // 14
    uint8_t     paymode,onoff;          //16
    int32_t     prebal;  //can be negative  //20
    uint16_t    maxamp,minamp; //24
    uint32_t    bpk,beatlife,kwhstart,lastupdate,lifekwh,lifedate,lastclock; //52
    uint16_t    months[12];                        // 76
    uint8_t     monthHours[12][24];                 // 288+76 =364
    uint32_t    framWrites;
    uint32_t    framReads;
    uint32_t    pulseerrs;
} meterType;

#pragma pack(push, 2)       //align on word boundary this struct and enxt one then normal c++ compiler reqiuirements
typedef struct mesh_md_sg {
char        meter_id[12];           // 0 meter id
uint8_t     paym;                   //12 payment method
uint8_t     poweroff;               //13 locked or not
uint16_t    preval;                 //14 prepaid balance
uint32_t    kwhlife;                //16 kwh for life
uint32_t    beatlife;               //20 betas for life
uint16_t    maxamp;                 //24 max amp registered
uint16_t    monthnum;               //26 month number
uint16_t    monthkwh;               //28 monthkwh
uint32_t    intid;                  //30 units mac
uint32_t    fwr;                    //34 fram writes
uint8_t     horas[24];              //38 24 hours kwh
} mesh_msg_t;           //message size is 62


typedef struct thenode {
uint32_t    nodeid,subnode;
uint32_t    tstamp,msgnum;
solarDef_t  solarData;      // want a solarSystem+_t  
} thenode_t;                

typedef struct thebigunion{     //4 + 78 = 82
        uint32_t cmd;                      
        union  {
            thenode_t nodedata;                         // for binary data format
            char    parajson[sizeof(thenode_t)];        // for json which should have the same cmd
        };
    } meshunion_t;

#pragma pack(pop)

typedef struct locker{
    char * data;
    int     len;
}locker_t;

typedef struct lvg {
    char *msg;
    uint32_t wait;
    bool bigf;
} show_lvgl_t;

#pragma pack(push, 2)

typedef struct shrimpMsg_st{
    uint32_t centinel;
    uint16_t poolid;
    uint16_t countnodes;
    time_t msgTime;
    solarSystem_t poolAvgMetrics;
} shrimpMsg_t;
#pragma pack(pop)

#endif
