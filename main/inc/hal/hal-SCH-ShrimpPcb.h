#ifndef HAL_H
#define HAL_H
// for schematic Shrimp Basic V 1.2-2L
// EasyEDA project name is Shrimp-602-2L-1MB

#define GPS_RX                          (16)     //GPIO ofr GPS Rx

#define ONEWIRE_BUS_GPIO                (13)        // GPIO for DS18B20
// SPI for FRAM 
#define FMOSI							(01)    // GPIO for SPI MOSI
#define FMISO							(40)    // GPIO for SPI MISO
#define FCLK							(39)    // GPIO for SPI Clock
#define FCS								(38)    // GPIO for SPI Chip Select
#define FSDA                            (1)
#define FSCL                            (39)

#define WIFILED                         (41)

// RS485 GPIOs
#define RS485TX                         (2)         //GPIO for RS485 TX
#define RS485RX                         (42)        //GPIO for RS485 RX
#define RS485RTS                        (48)    // IF only a simple RTS is used, solder a 0 Resistance between RE and DE and use this define for both
// #define RS485RTS                        (19)    // IF only a simple RTS is used, solder a 0 Resistance between RE and DE and use this define for both

// Other GPIOS
// Door latch sensor
#define DOOR                            (14)        // GPIO for door sensor input
#define RPM                             (48)        // GPIO for Blower RPM input via HAL sensor

// Other GPIOS
#define HXDOUT                          (47) // GPIO for HX711 data output
#define HXSCK                           (21) // GPIO for HX711 clock
// valves GPIOs
#define VAL0OPEN                        (4) //GPIO for valve 0 open
#define VAL0CLOSE                       (5) //GPIO for valve 0 close
#define VAL1OPEN                        (6) //GPIO for valve 1 open
#define VAL1CLOSE                       (7) //GPIO for valve 1 close
#define VAL2OPEN                        (12) //GPIO for valve 2 open
#define VAL2CLOSE                       (9) //GPIO for valve 2 close
#define VAL3OPEN                        (10) //GPIO for valve 3 open
#define VAL3CLOSE                       (11) //GPIO for valve 3 close
#define FEEDEROPEN                      (15) //GPIO for feeder open
#define FEEDERCLOSE                     (3)  //GPIO for feeder close


#endif