//
// Copyright (c) 2020 Fw-Box (https://fw-box.com)
// Author: CHEN GUANG WU
//
// Description :
//   None
//
// Connections :
//
// Required Library :
// arduino-cli lib install PubSubClient
// arduino-cli lib install "Adafruit BMP085 Library"
// arduino-cli lib install BH1750
// arduino-cli lib install Sodaq_SHT2x
//

#include "FwBox.h"
#include <Sodaq_SHT2x.h>
#include <Adafruit_BMP085.h>
#include <BH1750.h> // Light Sensor (BH1750)

#define DEVICE_TYPE 29
#define FIRMWARE_VERSION "1.0.2"

#define ANALOG_VALUE_DEBOUNCING 8

//
// Debug definitions
//
#define FW_BOX_DEBUG 0

#if FW_BOX_DEBUG == 1
  #define DBG_PRINT(VAL) Serial.print(VAL)
  #define DBG_PRINTLN(VAL) Serial.println(VAL)
  #define DBG_PRINTF(FORMAT, ARG) Serial.printf(FORMAT, ARG)
  #define DBG_PRINTF2(FORMAT, ARG1, ARG2) Serial.printf(FORMAT, ARG1, ARG2)
#else
  #define DBG_PRINT(VAL)
  #define DBG_PRINTLN(VAL)
  #define DBG_PRINTF(FORMAT, ARG)
  #define DBG_PRINTF2(FORMAT, ARG1, ARG2)
#endif

//
// Light Sensor (BH1750FVI)
//
BH1750 SensorLight;

//
// Sensor - BMP180
//
Adafruit_BMP085 SensorBmp;

bool SensorShtReady = false;
bool SensorLightReady = false;
bool SensorBmpReady = false;

//
// The sensor's values
//
float HumidityValue = 0.0;
float TemperatureValue = 0.0;
float LightValue = 0.0;
int PressureValue = 0;

String ValUnit[MAX_VALUE_COUNT];

unsigned long ReadingTime = 0;

void setup()
{
  Wire.begin();  // Join IIC bus for Light Sensor (BH1750).
  Serial.begin(9600);

  fbEarlyBegin(DEVICE_TYPE, FIRMWARE_VERSION);

  pinMode(LED_BUILTIN, OUTPUT);

  //
  // Set the unit of the values before "display".
  //
  ValUnit[0] = "°C";
  ValUnit[1] = "%";
  ValUnit[2] = "Lux";
  ValUnit[3] = "Pa";
  
  //
  // Initialize the fw-box core
  //
  fbBegin(DEVICE_TYPE, FIRMWARE_VERSION);

  //
  // Scan SHT2X
  //
  Wire.beginTransmission(0x40);
  if (Wire.endTransmission() == 0) { // Exist
      SensorShtReady = true;
  }

  //
  // Initialize the Light Sensor
  //
  if (SensorLight.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    SensorLightReady = true;
#if DEBUG == 1
    DBG_PRINTLN("BH1750 Advanced begin");
#endif // #if DEBUG == 1
  }
  else {
#if DEBUG == 1
    DBG_PRINTLN("Error initialising BH1750");
#endif // #if DEBUG == 1
  }
  delay(1000);

  //
  // Initialize the BMP Sensor
  //
  if (SensorBmp.begin()) {
    SensorBmpReady = true;
  }
  else {
#if DEBUG == 1
    DBG_PRINTLN("Could not find a valid BMP180 sensor, check wiring!");
#endif // #if DEBUG == 1
  }
  delay(1000);

  //
  // Get the and unit of the values from fw-box server after "fbBegin".
  //
  for (int vi = 0; vi < 4; vi++) {
    if (FwBoxIns.getValUnit(vi).length() > 0)
      ValUnit[vi] = FwBoxIns.getValUnit(vi);
  }

  //WiFi.disconnect();

} // void setup()

void loop()
{
  if ((ReadingTime == 0) || ((millis() - ReadingTime) > 2000)) {
    //
    // Read sensors
    //
    readSensor();

    //
    // Check if any reads failed.
    //
    if (isnan(HumidityValue) || isnan(TemperatureValue)) {
    }
    else {
      //
      // Filter the wrong values.
      //
      if( (TemperatureValue > 1) &&
          (TemperatureValue < 70) && 
          (HumidityValue > 10) &&
          (HumidityValue < 95) ) {
  
        FwBoxIns.setValue(0, TemperatureValue);
        FwBoxIns.setValue(1, HumidityValue);
      }
    }

    if (LightValue > 0) {
      FwBoxIns.setValue(2, LightValue);
    }

    if (PressureValue > (101325 / 3)) { // 101325 Pa = 1 atm
      FwBoxIns.setValue(3, PressureValue);
    }

    ReadingTime = millis();
  } // END OF "if((ReadingTime == 0) || ((millis() - ReadingTime) > 2000))"

  //
  // Run the handle
  //
  fbHandle();

} // END OF "void loop()"

void readSensor()
{
  if (SensorShtReady == true) {
    //
    // Read the temperature as Celsius (the default)
    // Calculate the average temperature of sensors - SHT and BMP.
    //
    TemperatureValue = (SHT2x.GetTemperature() + SensorBmp.readTemperature()) / 2;
    DBG_PRINTF2("Temperature : %f %s\n", TemperatureValue, ValUnit[0].c_str());

    //
    // Read the humidity(Unit:%)
    //
    HumidityValue = SHT2x.GetHumidity();
    DBG_PRINTF2("Humidity : %f %s\n", HumidityValue, ValUnit[1].c_str());
  }

  //
  // Read the Light level (Unit:Lux)
  //
  if (SensorLightReady == true) {
    LightValue = SensorLight.readLightLevel();
    if (LightValue > 0) {
      DBG_PRINTF2("Light : %f %s\n", LightValue, ValUnit[2].c_str());
    }
  }

  //
  // Read the Pressure (Unit:Pa)
  //  
  if (SensorBmpReady == true) {
    PressureValue = SensorBmp.readPressure();
    DBG_PRINTF2("Pressure : %d %s\n", PressureValue, ValUnit[3].c_str());
  }
}
