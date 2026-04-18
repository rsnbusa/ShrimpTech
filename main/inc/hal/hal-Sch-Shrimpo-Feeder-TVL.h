#ifndef HAL_H
#define HAL_H
// Hardware Abstraction Layer (HAL) for ShrimpIoT
// schematic is for Shrimp-Feeder-TVL (EasyEDA project name)

#define GPS_RX                          (16)     //GPIO ofr GPS Rx
#define ONEWIRE_BUS_GPIO                (13)        // GPIO for DS18B20


// SPI for FRAM 
#define FMOSI							(01)    // GPIO for SPI MOSI
#define FMISO							(40)    // GPIO for SPI MISO
#define FCLK							(39)    // GPIO for SPI Clock
#define FCS								(38)    // GPIO for SPI Chip Select
#define FSDA                            FMOSI      
#define FSCL                            FCLK

#define WIFILED                         (41)

// RS485 GPIOs
#define RS485TX                         (8)         //GPIO for RS485 TX
#define RS485RX                         (18)        //GPIO for RS485 RX
#define RS485DE                         (19)        //GPIO for RS485 MBDE
#define RS485RE                         (20)        //GPIO for RS485 NMBRE
#define RS485RTS                        RS485RE    // IF only a simple RTS is used, solder a 0 Resistance between RE and DE and use this define for both

// Other GPIOS
// Door latch sensor
#define DOOR                            (02)        // GPIO for door sensor input
#define RPM                             (42)        // GPIO for Blower RPM input via HAL sensor
#define HXDOUT                          (48)        // GPIO for HX711 data output
#define HXCLK                           (14)        // GPIO for HX711 clock
#define HXSCK                           HXCLK       // HX711 clock alias

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