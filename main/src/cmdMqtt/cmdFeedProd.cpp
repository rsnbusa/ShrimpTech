#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "feederScheduler.h"

/*
 * Handles feeder production start/stop command received over MQTT.
 *
 * Expected JSON format:
 * {"cmd":"feedprod","order":"start"}
 * {"cmd":"feedprod","order":"stop"}
 */

extern void write_to_flash();
extern void writeLog(const char* que);

typedef enum {
    FEEDPROD_START = 0,
    FEEDPROD_STOP = 1,
    FEEDPROD_INVALID = -1
} feedprod_order_t;

static feedprod_order_t parse_order(cJSON *node)
{
    if (!cJSON_IsString(node) || !node->valuestring) {
        return FEEDPROD_INVALID;
    }

    if (strcmp(node->valuestring, "start") == 0 || strcmp(node->valuestring, "on") == 0) {
        return FEEDPROD_START;
    }

    if (strcmp(node->valuestring, "stop") == 0 || strcmp(node->valuestring, "off") == 0) {
        return FEEDPROD_STOP;
    }

    return FEEDPROD_INVALID;
}

static bool has_valid_feed_profile(void)
{
    for (int p = 0; p < MAXPROFILES; ++p) {
        const feedprofile_t *profile = &theConf.feedprofiles[p];
        if (profile->numCycles == 0 || profile->numCycles > MAXCICLOS) {
            continue;
        }

        for (int c = 0; c < profile->numCycles; ++c) {
            const ciclo_t *cycle = &profile->cycle[c];
            if (cycle->duration > 0 && cycle->numHorarios > 0) {
                return true;
            }
        }
    }

    return false;
}

int cmdFeedProd(void *argument)
{
    cJSON *feedProdCommand = (cJSON *)argument;

    if (feedProdCommand == NULL || !cJSON_IsObject(feedProdCommand)) {
        MESP_LOGE(MESH_TAG, "FeedProd command payload invalid");
        return ESP_FAIL;
    }

    cJSON *orderNode = cJSON_GetObjectItem(feedProdCommand, "order");
    feedprod_order_t order = parse_order(orderNode);

    if (order == FEEDPROD_INVALID) {
        MESP_LOGE(MESH_TAG, "FeedProd invalid order (expected start/stop)");
        return ESP_FAIL;
    }

    if (order == FEEDPROD_START) {
        if (!has_valid_feed_profile()) {
            theConf.feederConf = false;
            write_to_flash();
            MESP_LOGE(MESH_TAG, "FeedProd start rejected: no valid feed profile configured");
            writeLog("FeedProd start rejected: no valid feed profile configured");
            return ESP_FAIL;
        }

        theConf.feederConf = true;
        write_to_flash();
        feeder_scheduler_start_task();
        feeder_scheduler_kick();
        MESP_LOGI(MESH_TAG, "FeedProd start accepted: feeder production enabled");
        writeLog("FeedProd start: feeder production enabled");
        return ESP_OK;
    }

    theConf.feederConf = false;
    write_to_flash();
    MESP_LOGI(MESH_TAG, "FeedProd stop: feeder production disabled");
    writeLog("FeedProd stop: feeder production disabled");
    return ESP_OK;
}
