SECURITY STRATEGY

When new, it is not provisioned to any AP so bluetooth provisioning is started (this is parked state, waiting for provisioning)
Once provisioned, system continues and generates a NODE id, This node id is to used as input to an aes crypto cypher  with a SUPERSECRET seed
This will return a 32 bytes were we take first 4 bytes HEX (8 chars) that should be passed to the Web configuration page as a Password.
Inside the MCU there si a cmd AES that takes this Node Id and returns the 8 chars for the password.

In real life, the Node Id should be sent to the OP Command Center (sms, whataspp, phone whatever) where they will input the Node Id to the algorith and give back the 8 chars password. Presummably the OCC will authenticate the "caller" as authorized personnel.
