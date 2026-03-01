**Modbus Configuration general logic**

Configuration comes from the Mongoose Website schema
In it in the REST API (presentation and capture however we need) the data structure is FORCED as follows

each Device has an # Address and # Refresh interval

then all the specific Registers which have a specific order (sorry not a json so cannot find them by name)

## MUX -> which if 0 mean sWRITE to the holding register esle its a read and can be used to multiply the value received Its Float so it division also
### VType  ->  is the type of variable 
{
 case 0:
                descriptors->devices[sensor_count].param_type = PARAM_TYPE_U8;
                break;
            case 1:
                descriptors->devices[sensor_count].param_type = PARAM_TYPE_U16;
                break;
            case 2:
                descriptors->devices[sensor_count].param_type = PARAM_TYPE_U32;
                break;
            case 3:
                descriptors->devices[sensor_count].param_type = PARAM_TYPE_FLOAT;
                break;
            case 4:
                descriptors->devices[sensor_count].param_type = PARAM_TYPE_FLOAT_ABCD;
                break;
            case 5:
                descriptors->devices[sensor_count].param_type = PARAM_TYPE_FLOAT_CDAB;
                break;
            case 6:
                descriptors->devices[sensor_count].param_type = PARAM_TYPE_FLOAT_BADC;
                break;
            case 7:
                descriptors->devices[sensor_count].param_type = PARAM_TYPE_FLOAT_DCBA;
                break;
}
### Points -> which is how many UNITS from the modbus (1 Unit -> 2 bytes) should be consistent with vtype float=2 unit16=1 etc

### Start -> offset in the modbus memory map.  Float moves offset by 2 and unit16 (minimum modbus value) by 1 
### Offset -> is the value in our memory data structure (not in modbus) so its the strcutreu for the device in this memory

## so example

xRefresh int
xAddress int
--- for each register to access -----
devMux double (important)
devVtype int
devPoints int
devStart int
devOffset int
-----------
## THE ORDER SENT is in the REST API order in Mongosse Wizard

This configuration is saved/displayed to/from the website and stored int
theConf.xxxx where xxx is each Modbus device

-- at boot if theConf.modbuson enabled it will be processed as follows

- launch_sensors is called 
    - here we have all the modbus devices to manage
    - a generic configuration is made to pass to a generictask creator per device
    - has the Modbus configured characterist and other data
    - per device the Device configuration is read from the theConf.xxxx 
    - setModbusSensor is called with its name and characteristic are created for the GenericSensor task
    - genericsensor task is created and passed these characteristic form theConf. xxxx

- genericSensor TASK creates the modbus characteristics from this descripton into a memory structure 
- memory strucutre has the characteristic which have the FC for modbus and the memory area to recieve the read data
- infinite loop to every refresh X time to send to a queue that process the calls -> rs485 task

- RS485 task
    - waits for a queue to process in general terms the calling to the modbus
        - gets the characterristics passed to him from the caller
        - registers the Characteristics
        - start making the read requests(or write) to the Modbus
        - manages tiemout and errors
        - when done awakes the caller task (one of the genericSensor tasks)
- now the generictask wakes up and processes the data
    - print 
    - save to the FRAM



