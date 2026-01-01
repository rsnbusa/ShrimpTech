#ifndef TYPESnim_H_
#define TYPESnim_H_
#define GLOBAL

#include "globals.h"
#include "includes.h"
#include "forwards.h"

int device_profile(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
int device_day(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
int device_start(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

static const ble_uuid16_t serial_service_uuid = BLE_UUID16_INIT( 0x180);
static const ble_uuid16_t profile_uuid = BLE_UUID16_INIT( 0xfef4);
static const ble_uuid16_t day_uuid = BLE_UUID16_INIT( 0xdead);
static const ble_uuid16_t start_uuid = BLE_UUID16_INIT( 0xdeaf);

static ble_uuid_t *uuid_ss = (ble_uuid_t *)&serial_service_uuid;
static ble_uuid_t *uuid_profile = (ble_uuid_t *)&profile_uuid;
static ble_uuid_t *uuid_day = (ble_uuid_t *)&day_uuid;
static ble_uuid_t *uuid_start = (ble_uuid_t *)&start_uuid;

 uint8_t ble_addr_type;

struct ble_gap_adv_params adv_params;
bool status = false;
void ble_app_advertise(void);


// Static characteristic definitions to ensure lifetime
static struct ble_gatt_chr_def serial_service_characteristics[] = {
  {
    .uuid = uuid_profile,
    .access_cb = device_profile,
    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
  },
  {
    .uuid = uuid_day,
    .access_cb = device_day,
    .flags =  BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
  },
  {
    .uuid = uuid_start,
    .access_cb = device_start,
    .flags = BLE_GATT_CHR_F_WRITE,
  },
  { 0 } // End of characteristics list
};

// Define static BLE GATT service array
 struct ble_gatt_svc_def serial_service[] = {
  {
    .type = BLE_GATT_SVC_TYPE_PRIMARY,
    .uuid = uuid_ss,
    .characteristics =serial_service_characteristics // Use the static characteristics array
  },
  { 0 } // End of service list
};


void ble_app_advertise(void);

// Write data to ESP32 defined as server
 int device_profile(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char *buf,*data;
    buf=(char*)calloc(100,1);
    // switch
    // printf("Data from the client: %.*s\n", ctxt->om->om_len, ctxt->om->om_data);

        switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
        case BLE_GATT_ACCESS_OP_READ_DSC:
            // This is a READ operation
            sprintf(buf,"Active Profile %d from %s",theConf.activeProfile,ctime(&theConf.dateProfile));
            // sprintf(buf,"Active Profile %d from %s",theConf.activeProfile,ctime(&theConf.dateProfile));
            printf("Input Profile %s\n",buf);
            os_mbuf_append(ctxt->om, buf, strlen(buf));
            break;
            
        case BLE_GATT_ACCESS_OP_WRITE_CHR:
        case BLE_GATT_ACCESS_OP_WRITE_DSC:
            // This is a WRITE operation
            // handle_write_request(ctxt);
            data = (char *)ctxt->om->om_data;
            data[ctxt->om->om_len]=0;
            theConf.activeProfile=atoi(data);
            printf("Save Profile # %d\n",theConf.activeProfile);
            time(&theConf.dateProfile);
            write_to_flash();
            break;
        default:
            return BLE_ATT_ERR_UNLIKELY;
    }
    free(buf);
    memset(data, 0, strlen(data));

    return 0;
}

// Read data from ESP32 defined as server
 int device_day(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char *buf,*data;
    buf=(char*)calloc(100,1);
    // switch
    // printf("Data from the client: %.*s\n", ctxt->om->om_len, ctxt->om->om_data);

        switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
        case BLE_GATT_ACCESS_OP_READ_DSC:
            // This is a READ operation
            sprintf(buf,"Active Day %d from %s",theConf.dayCycle,ctime(&theConf.dateDayCycle));
            // sprintf(buf,"Active Profile %d from %s",theConf.activeProfile,ctime(&theConf.dateProfile));
            printf("BLE Day %s\n",buf);
            os_mbuf_append(ctxt->om, buf, strlen(buf));
            break;
            
        case BLE_GATT_ACCESS_OP_WRITE_CHR:
        case BLE_GATT_ACCESS_OP_WRITE_DSC:
            // This is a WRITE operation
            // handle_write_request(ctxt);
            data = (char *)ctxt->om->om_data;
            data[ctxt->om->om_len]=0;
            theConf.dayCycle=atoi(data);
            printf("BLE Save Day Start # %d\n",theConf.dayCycle);
            time(&theConf.dateDayCycle);
            write_to_flash();
            break;
        default:
            return BLE_ATT_ERR_UNLIKELY;
    }
    free(buf);
    memset(data, 0, strlen(data));

    return 0;
}

 int device_start(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
char *data;

        switch (ctxt->op) {           
        case BLE_GATT_ACCESS_OP_WRITE_CHR:
        case BLE_GATT_ACCESS_OP_WRITE_DSC:
            // This is a WRITE operation
            // handle_write_request(ctxt);
            data = (char *)ctxt->om->om_data;
            data[ctxt->om->om_len]=0;
            theConf.test_timer_div=atoi(data);
            printf("Start Cycle Profile # %d Day Start %d Divisors %d\n",theConf.activeProfile,theConf.dayCycle,theConf.test_timer_div);
            theConf.blower_mode=1;
            // theConf.bleboot=MESH_MODE;      // mesh mode
            write_to_flash();
            esp_restart();
            break;
        default:
            return BLE_ATT_ERR_UNLIKELY;
    }

    memset(data, 0, strlen(data));

    return 0;
}

// BLE event handling
 int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    // Advertise if connected
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI("GAP", "BLE GAP EVENT CONNECT %s", event->connect.status == 0 ? "OK!" : "FAILED!");
        if (event->connect.status != 0)
        {
            ble_app_advertise();
        }
        break;
    // Advertise again after completion of the event
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI("GAP", "BLE GAP EVENT DISCONNECTED");
        if (event->connect.status != 0)
        {
            ble_app_advertise();
        }
        break;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI("GAP", "BLE GAP EVENT");
        ble_app_advertise();
        break;
    default:
        break;
    }
    return 0;
}

// Define the BLE connection
void ble_app_advertise(void)
{
    // GAP - device name definition
    struct ble_hs_adv_fields fields;
    const char *device_name;
    memset(&fields, 0, sizeof(fields));
    device_name = ble_svc_gap_device_name(); // Read the BLE device name
    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;
    ble_gap_adv_set_fields(&fields);

    // GAP - device connectivity definition
    // struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    // adv_params.conn_mode = BLE_GAP_CONN_MODE_UND; // connectable or non-connectable
    // adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN; // discoverable or non-discoverable
    // ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
}

// The application
void ble_app_on_sync(void)
{
    ble_hs_id_infer_auto(0, &ble_addr_type); // Determines the best address type automatically
    ble_app_advertise();                     // Define the BLE connection
}

// The infinite task
void host_task(void *param)
{
    nimble_port_run(); // This function will return only when nimble_port_stop() is executedprintf("Nimble stop\n");
}

void connect_ble(void)
{
    nimble_port_init();                        // 3 - Initialize the host stack
    char *algo=(char*)malloc(20);               
    sprintf(algo,"SHRIMP-%d-%d",theConf.poolid,theConf.unitid);
    ble_svc_gap_device_name_set(algo);          // 4 - Initialize NimBLE configuration - server name
    ble_svc_gap_init();                        // 4 - Initialize NimBLE configuration - gap service
    ble_svc_gatt_init();                       // 4 - Initialize NimBLE configuration - gatt service

    int rc = ble_gatts_count_cfg(serial_service);
    if (rc != 0) {
        ESP_LOGE("BLE", "Failed to count BLE services: %d", rc);
    }
    ble_gatts_add_svcs(serial_service);             // 4 - Initialize NimBLE configuration - queues gatt services.
    ble_hs_cfg.sync_cb = ble_app_on_sync;      // 5 - Initialize application
    nimble_port_freertos_init(host_task);      // 6 - Run the thread
    free(algo);
}

void nimble_test(void *parg)
{
    connect_ble();
    while(true)
    {
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND; // connectable or non-connectable
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN; // discoverable or non-discoverable
    ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
    vTaskDelay(250);
    }
}
#endif