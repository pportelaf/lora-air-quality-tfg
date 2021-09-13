/***** BEGIN gas sensors configuration *****/
#define MULTICHANNEL_GAS_SENSOR_I2C_ADDR 0x04
// preheat 20 minutes
#define PREHEAT_TIME_IN_MILLISECONDS 1200000

#define MIN_VALUE_CO2 0
#define MAX_VALUE_C02 40000

#define MIN_VALUE_NH3 0
#define MAX_VALUE_NH3 500

// Maximum time to wait for sensor data (1 minute)
#define SDC30_DATA_AVAILABLE_TIMEOUT_IN_MILISECONDS 60000
/***** END gas sensors configuration *****/

/***** BEGIN app configuration *****/
// Interval between transmissions in seconds (15 min)
#define TRANSMISSION_INTERVAL_IN_SECONDS 900

// Timeout in seconds for retry transmission when an error occurs
#define TRANSMISSION_RETRY_TIMEOUT_IN_SECONDS 10

#define MAX_RETRY_MAX_TIMES 1
/***** END app configuration *****/

/***** BEGIN LMIC configuration *****/
// APP EUI in little endian format. Unused in ChirpStack
#define APP_EUI { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// DEV EUI in little endian format
#define DEV_EUI { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // FILLME

// APP KEY in big endian format
#define APP_KEY { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // FILLME

#define LORAWAN_PORT 1
#define LORAWAN_ERROR_PORT 2
/***** END LMIC configuration *****/
