/**
 * @file sendData.cpp
 * @brief Mesh data packet creation for solar blower system readings
 * 
 * This module is responsible for creating binary mesh data packets that contain
 * solar system readings and node status information. These packets are transmitted
 * through the ESP-MESH network to the root node.
 * 
 * Data Structure:
 * - Timestamp
 * - Node identification (pool ID, unit ID)
 * - Message sequence number
 * - Complete solar system readings
 * 
 * @note The returned mesh union structure must be freed by the caller
 * @warning This module includes simulation code (seed_blower) for testing
 */

#define GLOBAL
#include "includes.h" 
#include "globals.h"

// ===================================================================
// Constants
// ===================================================================

#define SENDDATA_TAG            "SENDDATA"

// Simulation control - set to 0 for production builds
#define ENABLE_SIMULATION       1

// ===================================================================
// External Dependencies
// ===================================================================

// External dependencies for solar blower system
extern void print_blower(char *title, solarSystem_t *solarData);
extern void delay(uint32_t milliseconds);
extern void seed_blower();

/**
 * @brief Creates a mesh data packet with current solar system readings
 * 
 * This function allocates and populates a mesh union structure with:
 * - Current timestamp
 * - Node identification (pool ID and unit ID)
 * - Message sequence number (incremented for non-root nodes)
 * - Complete solar system data snapshot
 * 
 * @param forced If true, forces data transmission regardless of timing
 *               (currently unused - reserved for future implementation)
 * 
 * @return Pointer to allocated meshunion_t structure containing sensor data,
 *         or NULL if memory allocation fails
 * 
 * @note Caller is responsible for freeing the returned structure
 * @note Increments message counter for non-root nodes only
 * @warning Contains seed_blower() call for simulation - remove in production
 */
meshunion_t *sendData(bool forced)
{
    // Note: 'forced' parameter reserved for future use
    (void)forced;  // Suppress unused parameter warning
    
    // Allocate mesh data structure
    meshunion_t *nodeDataPacket = (meshunion_t*)calloc(1, sizeof(meshunion_t));
    if(!nodeDataPacket)
    {
        ESP_LOGE(SENDDATA_TAG, "Failed to allocate memory for mesh data packet");
        return NULL;
    }

    // Get current timestamp
    time_t currentTimestamp = time(NULL);
    
    // Initialize packet header
    nodeDataPacket->cmd = MESH_INT_DATA_BIN;
    nodeDataPacket->nodedata.tstamp = currentTimestamp;
    
    // Update message counter for non-root nodes
    if(!esp_mesh_is_root())
    {
        theBlower.setStatsMsgOut();
    }
    
    // Set node identification
    nodeDataPacket->nodedata.msgnum = theBlower.getStatsMsgOut();
    nodeDataPacket->nodedata.nodeid = theConf.poolid;
    nodeDataPacket->nodedata.subnode = theConf.unitid;
    
#if ENABLE_SIMULATION
    // ===================================================================
    // SIMULATION: Seed test data
    // NOTE: Set ENABLE_SIMULATION to 0 for production builds
    // ===================================================================
    seed_blower();
#endif
    
    // ===================================================================
    // Retrieve and copy solar system data
    // ===================================================================
    solarSystem_t *solarSystemData = theBlower.getPtrSolarsystem();
    if(!solarSystemData)
    {
        ESP_LOGE(SENDDATA_TAG, "Failed to get solar system data pointer");
        free(nodeDataPacket);
        return NULL;
    }
    
    // Copy solar system readings into packet
    // Both structures must be same size - verified at compile time by sizeof()
    memcpy((void*)&nodeDataPacket->nodedata.solarData.solarSystem,
           solarSystemData,
           sizeof(solarSystem_t));
    
    ESP_LOGD(SENDDATA_TAG, "Data packet created: Pool=%d, Unit=%d, Msg=%d",
             nodeDataPacket->nodedata.nodeid,
             nodeDataPacket->nodedata.subnode,
             nodeDataPacket->nodedata.msgnum);
    
    return nodeDataPacket;  // Caller must free this allocation
}