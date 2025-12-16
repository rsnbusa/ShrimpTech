Program Flow

 sending telemetry from node(also root) to Root (Host) 

A timer is started collectTimer that is the heartbeat of the logic, timer can be changed to lesser or faster beat.

The routine called when time expires is root_collect_meter_data which will get routing table to get #. of active nodes (in the mesh) that will be used to count receivng message to the "sendmetrics" cmd sent in a broadcast message to the mesh. When a node recieves this message it will send it Metrics. This routine call a get_routing_table() function to get the list of nodes in the mesh. The number of nodes is saved as masterNode.num_nodes. 

The routine, in root, root_check_incoming_meters will receive the raw data and save it in a table (masterNode.theTable.big_table[a]) until all nodes have sent its dat or a timeout (launched by the sendmetrics cmd) happens. It verifies the receiving mac addr message for sanity checking. If the message is valid then it saves the data into a temp buffer (masterNode.theTable.temp_buffer). 
Then it groups them, not in shrimp case, and send a MQtt message to host. When sending it it will free the buffer so it can be reused again.

This is the main loop logic.

