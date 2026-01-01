/**
 * @file cmdMetersreset.cpp
 * @brief Console command to broadcast reinstall message to mesh network
 * 
 * This file implements a command that sends a "reinstall" message to all
 * nodes in the mesh network. Only functional when executed on the root node.
 * The broadcast triggers all receiving nodes to perform a reinstallation sequence.
 * 
 * @note Part of the ShrimpIoT console command system
 * @note Only works on root node, silently does nothing on non-root nodes
 */

#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

/**
 * @brief Console command to reset meters via mesh broadcast
 * 
 * Sends a JSON "reinstall" command to all nodes in the mesh network.
 * The command only executes if the current node is the mesh root.
 * 
 * @param argc Argument count (unused)
 * @param argv Argument vector (unused)
 * @return 0 on success
 * 
 * @note Usage: metersreset
 * @note Only functional on root node
 * @note Broadcasts: {"cmd":"reinstall"} to all mesh nodes
 */
int cmdMetersreset(int argc, char **argv)
{
    mesh_data_t data;
    esp_err_t err;

    if (esp_mesh_is_root())
    {
        char *broadcast = (char*)calloc(50, 1);
        if (!broadcast)
        {
            ESP_LOGE(MESH_TAG, "Could not malloc broadcast message - FATAL");
            return 0;
        }

        strcpy(broadcast, "{\"cmd\":\"reinstall\"}");
        data.data = (uint8_t*)broadcast;
        data.size = strlen(broadcast);
        data.proto = MESH_PROTO_BIN;
        data.tos = MESH_TOS_P2P;
        
        err = esp_mesh_send(&GroupID, &data, MESH_DATA_P2P, NULL, MESH_OPT_SEND_GROUP);
        if (err)
            ESP_LOGE(MESH_TAG, "Broadcast failed with error: 0x%x", err);
        else
            printf("Reinstall broadcast sent to all mesh nodes\n");
        
        free(broadcast);
    }
    else
    {
        printf("Error: This command only works on root node\n");
    }
    
    return 0;
}