/**
 * @file cmdTasks.cpp
 * @brief Console command to display FreeRTOS task list
 * 
 * This file implements a command that displays all running FreeRTOS tasks
 * with their stack information, state, and other relevant details.
 * 
 * @note Part of the ShrimpIoT console command system
 * @note Requires configUSE_TRACE_FACILITY=1 in FreeRTOSConfig.h
 */

#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

/**
 * @brief Console command to display FreeRTOS task information
 * 
 * Displays basic FreeRTOS task information:
 * - Number of running tasks
 * - Heap memory statistics
 * 
 * @param argc Argument count (unused)
 * @param argv Argument vector (unused)
 * @return 0 on success
 * 
 * @note Usage: tasks
 * @note To see detailed task list, enable configUSE_TRACE_FACILITY in FreeRTOS config
 */
int cmdTasks(int argc, char **argv)
{
    printf("\n%s====== FreeRTOS Task Information ======%s\n", CYAN, RESETC);
    
    // Display task count (always available)
    UBaseType_t num_tasks = uxTaskGetNumberOfTasks();
    printf("Total tasks running: %d\n", num_tasks);
    
    // Display heap information (always available)
    printf("\n%sHeap Memory:%s\n", CYAN, RESETC);
    printf("  Free:    %lu bytes\n", esp_get_free_heap_size());
    printf("  Minimum: %lu bytes\n", esp_get_minimum_free_heap_size());
    
    // vTaskList is only built when both trace facility and stats formatting are enabled.
#if ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS == 1 )
    printf("\n%sDetailed Task List:%s\n", CYAN, RESETC);
    char *task_list_buffer = (char*)malloc(4096);
    if (task_list_buffer)
    {
        vTaskList(task_list_buffer);
        printf("%sName          State  Priority  Stack  Num%s\n", CYAN, RESETC);
        printf("%s============================================================%s\n", CYAN, RESETC);
        printf("%s\n", task_list_buffer);
        printf("%s============================================================%s\n", CYAN, RESETC);
        free(task_list_buffer);
    }
    else
    {
        printf("Could not allocate buffer for detailed task list\n");
    }
#else
    printf("\n%sNote:%s Detailed task list requires:\n", YELLOW, RESETC);
    printf("      - configUSE_TRACE_FACILITY=1\n");
    printf("      - configUSE_STATS_FORMATTING_FUNCTIONS=1\n");
#endif
    
    // Try to display runtime stats if available
#if ( configGENERATE_RUN_TIME_STATS == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS == 1 )
    printf("\n%sRuntime Statistics:%s\n", CYAN, RESETC);
    char *runtime_stats_buffer = (char*)malloc(4096);
    if (runtime_stats_buffer)
    {
        vTaskGetRunTimeStats(runtime_stats_buffer);
        printf("%sTask              Abs Time  %% Time%s\n", CYAN, RESETC);
        printf("%s============================================================%s\n", CYAN, RESETC);
        printf("%s\n", runtime_stats_buffer);
        printf("%s============================================================%s\n", CYAN, RESETC);
        free(runtime_stats_buffer);
    }
#endif
    
    printf("%s%s\n", RESETC, RESETC);
    return 0;
}
