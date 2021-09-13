#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <SparkFun_SCD30_Arduino_Library.h>
#include <MutichannelGasSensor.h>

// OLED pins
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_RST 16

#define OLED_SCREEN_WIDTH 128 // OLED display width, in pixels
#define OLED_SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_I2C_ADDR 0x3c

SCD30 airSensor;
Adafruit_SSD1306 display(OLED_SCREEN_WIDTH, OLED_SCREEN_HEIGHT, &Wire, OLED_RST);
MutichannelGasSensor multichannelGasSensor;
float ammonia;
uint16_t CO2;
bool autoCalibratonEnabled = false;

void setup() {
  Serial.begin(115200);
  Serial.println("Start SCD30");
  Wire.begin();
  setupOled();

  if (airSensor.begin(Wire, true) == false) {
    Serial.println("Air sensor not detected. Please check wiring. Freezing...");
    while (1);
  }

  airSensor.setMeasurementInterval(10);
  setupMultichannelGasSensor();
}

void setupOled() {
  //Reset OLED display via software
  pinMode(OLED_RST, OUTPUT);

  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  // Initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR, false, false)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  display.setTextColor(WHITE);
}

void setupMultichannelGasSensor() {
  multichannelGasSensor.begin(0x04);
  multichannelGasSensor.powerOn();
}

void loop() {
  if (airSensor.dataAvailable()) {
    CO2 = airSensor.getCO2();
    ammonia = multichannelGasSensor.measure_NH3();
    autoCalibratonEnabled = airSensor.getAutoSelfCalibration();

    Serial.print("co2(ppm):");
    Serial.print(CO2);

    Serial.print(" ammonia (%):");
    Serial.print(ammonia, 1);

    Serial.println();
    displayResults();
  } else {
    Serial.print(".");
  }

  delay(500);
}

void displayResults() {
  display.clearDisplay();
  
  display.setTextSize(2);

  display.setCursor(0,0);

  display.println("CO2: ");
  display.print(CO2);
  display.println(" ppm");

  display.println("NH3: ");
  display.print(ammonia);
  display.print(" ppm");

  display.display();
}
