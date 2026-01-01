/**
 * @file cmdNode.cpp
 * @brief Console command to change node pool ID
 * 
 * This file implements a password-protected command to change the node's
 * pool ID. Requires password authentication before allowing the change.
 * The new pool ID is persisted to flash memory.
 * 
 * @note Part of the ShrimpIoT console command system
 * @note Changes require system restart to take full effect
 */

#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

/**
 * @brief Console command to change node pool ID
 * 
 * Password-protected command that allows changing the node's pool ID.
 * The pool ID identifies which pool/group this node belongs to in the
 * mesh network. Changes are saved to flash immediately.
 * 
 * @param argc Argument count from argtable3 parser
 * @param argv Argument vector from argtable3 parser
 * @return 0 on success
 * 
 * @note Usage: node -p <password> -n <new_id>
 *       -p, --password: Authentication password
 *       -n, --newpass: New node/pool ID (numeric)
 * 
 * @note System restart may be required for changes to take full effect
 */
int cmdNode(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **)&appNode);
    if (nerrors != 0) {
        arg_print_errors(stderr, appNode.end, argv[0]);
        return 0;
    }
    
    if (appNode.password->count) 
    {
        if (strcmp(appNode.password->sval[0], theConf.kpass) == 0)
        {
            if (appNode.newpass->count)
            {
                int nn = atoi(appNode.newpass->sval[0]);
                printf("New Node ID [%d]\n", nn);
                theConf.poolid = nn;
                write_to_flash();
                printf("Node ID updated and saved to flash\n");
            }
            else
            {
                printf("Error: New node ID not specified\n");
            }
        }
        else
        {
            printf("Wrong password\n");
        }
    }
    else
    {
        printf("Password required. Use -p <password> -n <new_id>\n");
    }

    return 0;
}