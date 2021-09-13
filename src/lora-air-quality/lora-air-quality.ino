#include <TTGO_LoRaWAN.h>
#include <Wire.h>
#include <SparkFun_SCD30_Arduino_Library.h>
#include <MutichannelGasSensor.h>
#include "config.h"

#define SDC30_MEASUREMENT_INTERVAL_IN_SECONDS 10

enum {
    MEASSUREMENT_STATUS_OK = 0,
    MEASSUREMENT_STATUS_NH3_BELLOW_MIN_LIMIT = -1,
    MEASSUREMENT_STATUS_NH3_ABOVE_MAX_LIMIT = -2,
    MEASSUREMENT_STATUS_CO2_ABOVE_MAX_LIMIT = -3,
    MEASSUREMENT_STATUS_CO2_TIMEOUT = -4
};

typedef int MEASUREMENT_STATUS;

TTGO_LoRaWAN ttgoLoRaWAN;

SCD30 airSensor;
MutichannelGasSensor multichannelGasSensor;
MEASUREMENT_STATUS measurementStatus;

static osjob_t transmitDataJob;
static osjob_t transmitErrorJob;

float ammonia;
uint16_t CO2;
int retryTransmissionsCount = 0;

static const u1_t PROGMEM APPEUI[8] = APP_EUI;
void os_getArtEui (u1_t* buf) {
  memcpy_P(buf, APPEUI, 8);
}

static const u1_t PROGMEM DEVEUI[8] = DEV_EUI;
void os_getDevEui (u1_t* buf) {
  memcpy_P(buf, DEVEUI, 8);
}

static const u1_t PROGMEM APPKEY[16] = APP_KEY;
void os_getDevKey (u1_t* buf) {
  memcpy_P(buf, APPKEY, 16);
}

void setup() {
  #if SERIAL_DEBUG_ENABLED
    Serial.begin(115200);
  #endif

  ttgoLoRaWAN.begin();
  Wire.begin();

  airSensor.begin();
  airSensor.setMeasurementInterval(SDC30_MEASUREMENT_INTERVAL_IN_SECONDS);
  setupMultichannelGasSensor();

  delay(PREHEAT_TIME_IN_MILLISECONDS);
  os_setCallback(&transmitDataJob, transmitData);
}

void setupMultichannelGasSensor() {
  multichannelGasSensor.begin(MULTICHANNEL_GAS_SENSOR_I2C_ADDR);
  multichannelGasSensor.powerOn();
}

void loop() {
  os_runloop();
}

void transmitData(osjob_t* job) {
  /**
   * CO2: 16 bits (0- 40000)
   * NH3: 15 bits (0.0 - 2000.0)
   */
  byte dataBytes[4];
  int ammoniaFormatted;

  updateGasValues();

  if (measurementStatus != MEASSUREMENT_STATUS_OK) {
    #if SERIAL_DEBUG_ENABLED
      DEBUG_PRINT("measurementStatus status is %i, transmit error", measurementStatus)
    #endif

    os_setCallback(&transmitErrorJob, transmitError);

    return;
  }

  ammoniaFormatted = (int)(ammonia * 10);

  dataBytes[0] = CO2 >> 8;
  dataBytes[1] = CO2 &0xFF;
  dataBytes[2] = ammoniaFormatted >> 7;
  dataBytes[3] = (ammoniaFormatted << 1);

  #if SERIAL_DEBUG_ENABLED
    DEBUG_PRINT("TX data \"%02X%02X%02X%02X\" (%i bytes)", dataBytes[0], dataBytes[1], dataBytes[2], dataBytes[3], sizeof(dataBytes));
  #endif

  ttgoLoRaWAN.sendData(LORAWAN_PORT, dataBytes, sizeof(dataBytes), 0, transmitDataCallback);
}

void transmitError(osjob_t* job) {
  /**
   * error key: 2 bits. Error goes from -1 to -4, applying (error * -1 - 1), goes from 0 to 3.
   */
  byte dataBytes[1];

  dataBytes[0] = (measurementStatus * -1 - 1) << 6;

  #if SERIAL_DEBUG_ENABLED
    DEBUG_PRINT("TX error data \"%02X\" (%i bytes)", dataBytes[0], sizeof(dataBytes));
  #endif

  ttgoLoRaWAN.sendData(LORAWAN_ERROR_PORT, dataBytes, sizeof(dataBytes), 0, transmitErrorCallback);
}

void updateGasValues() {
  unsigned int startTime = millis();
  bool maxTimeToWaitForDataReached = false;

  #if SERIAL_DEBUG_ENABLED
    DEBUG_PRINT("Updating gas values");
  #endif

  while (!airSensor.dataAvailable() && !maxTimeToWaitForDataReached) {
    maxTimeToWaitForDataReached = (millis() - startTime) > SDC30_DATA_AVAILABLE_TIMEOUT_IN_MILISECONDS;
    os_runloop_once();
  }

  #if SERIAL_DEBUG_ENABLED
    DEBUG_PRINT("maxTimeToWaitForDataReached %i", maxTimeToWaitForDataReached);
  #endif

  if (maxTimeToWaitForDataReached) {
    measurementStatus = MEASSUREMENT_STATUS_CO2_TIMEOUT;
    return;
  }

  CO2 = airSensor.getCO2();
  ammonia = multichannelGasSensor.measure_NH3();

  #if SERIAL_DEBUG_ENABLED
    DEBUG_PRINT("CO2: %i, NH3: %f", CO2, ammonia);
  #endif

  checktMeasurementErrors();

  #if SERIAL_DEBUG_ENABLED
    DEBUG_PRINT("checktMeasurementErrors with status %i", measurementStatus);
  #endif
}

void checktMeasurementErrors() {
    if (ammonia < MIN_VALUE_NH3) {
    measurementStatus = MEASSUREMENT_STATUS_NH3_BELLOW_MIN_LIMIT;
    return;
  }

  if (ammonia > MAX_VALUE_NH3) {
    measurementStatus = MEASSUREMENT_STATUS_NH3_ABOVE_MAX_LIMIT;
    return;
  }

  if (CO2 > MAX_VALUE_C02) {
    measurementStatus = MEASSUREMENT_STATUS_CO2_ABOVE_MAX_LIMIT;
    return;
  }

  measurementStatus = MEASSUREMENT_STATUS_OK;
}

void transmitDataCallback(bool success) {
  if (!success && canRetryTransmission()) {
    #if SERIAL_DEBUG_ENABLED
      DEBUG_PRINT("Data transmission error, retry transmission in %ld seconds", TRANSMISSION_RETRY_TIMEOUT_IN_SECONDS);
    #endif
    os_setTimedCallback(&transmitDataJob, os_getTime() + sec2osticks(TRANSMISSION_RETRY_TIMEOUT_IN_SECONDS), transmitData);
    retryTransmissionsCount++;
  } else {
    #if SERIAL_DEBUG_ENABLED
      if (!success) {
        DEBUG_PRINT("Max retry transmissions reached (%i), retry transmissions count %i.", MAX_RETRY_MAX_TIMES, retryTransmissionsCount);
      }
    #endif

    retryTransmissionsCount = 0;
    os_setTimedCallback(&transmitDataJob, os_getTime() + sec2osticks(TRANSMISSION_INTERVAL_IN_SECONDS), transmitData);
  }
}


void transmitErrorCallback(bool success) {
  if (!success && canRetryTransmission()) {
    #if SERIAL_DEBUG_ENABLED
      DEBUG_PRINT("Error transmission error, retry transmission in %ld seconds", TRANSMISSION_RETRY_TIMEOUT_IN_SECONDS);
    #endif
    os_setTimedCallback(&transmitErrorJob, os_getTime() + sec2osticks(TRANSMISSION_RETRY_TIMEOUT_IN_SECONDS), transmitError);
    retryTransmissionsCount++;
  } else {
    #if SERIAL_DEBUG_ENABLED
      if (!success) {
        DEBUG_PRINT("Max retry transmissions reached (%i), retry transmissions count %i.", MAX_RETRY_MAX_TIMES, retryTransmissionsCount);
      }
    #endif

    retryTransmissionsCount = 0;
    os_setTimedCallback(&transmitDataJob, os_getTime() + sec2osticks(TRANSMISSION_INTERVAL_IN_SECONDS), transmitData);
  }
}

bool canRetryTransmission() {
  return retryTransmissionsCount < MAX_RETRY_MAX_TIMES;
}
