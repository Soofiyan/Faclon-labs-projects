#include <Adafruit_MAX31865.h>

// Use software SPI: CS, DI, DO, CLK
Adafruit_MAX31865 max = Adafruit_MAX31865(10, 11, 12, 13);
// use hardware SPI, just pass in the CS pin
Adafruit_MAX31865 max1 = Adafruit_MAX31865(9, 11, 12, 13);

// The value of the Rref resistor. Use 430.0 for PT100 and 4300.0 for PT1000
// The 'nominal' 0-degrees-C resistance of the sensor
// 100.0 for PT100, 1000.0 for PT1000
#define RNOMINAL  100.0
float RREF = 430.0;
float RREF1 = 430.0;
uint8_t counter = 60;
void callibration();
void setup() {
  Serial.begin(115200);
  max.begin(MAX31865_3WIRE);  // set to 2WIRE or 4WIRE as necessary
  max1.begin(MAX31865_3WIRE);  // set to 2WIRE or 4WIRE as necessary
}


void loop() {
  if(counter >= 60)
  {
    uint16_t rtd = max.readRTD();
    uint16_t rtd1 = max1.readRTD();
    Serial.print("Long RTD value: "); Serial.println(rtd);
    Serial.print("Short RTD value: "); Serial.println(rtd1);
    float ratio = rtd;
    float ratio1 = rtd1;
    ratio1 /= 32768;
    ratio /= 32768;
    Serial.print("Ratio difference ");Serial.println(RREF*ratio - RREF1*ratio1);
    Serial.print("Long Ratio = "); Serial.println(ratio,8);
    Serial.print("Long Resistance = "); Serial.println(RREF*ratio,8);
    Serial.print("Long Temperature = "); Serial.println(max.temperature(RNOMINAL, RREF));
    
    Serial.print("Short Ratio = "); Serial.println(ratio1,8);
    Serial.print("Short Resistance = "); Serial.println(RREF1*ratio1,8);
    Serial.print("Short Temperature = "); Serial.println(max1.temperature(RNOMINAL, RREF1));
    // Check and print any faults
    Serial.println();
    delay(1000);
  }
  else
  {
    callibration();
  }
}
void callibration()
{
  uint16_t rtd = max.readRTD();
  uint16_t rtd1 = max1.readRTD();
  Serial.print("Long RTD value: "); Serial.println(rtd);
  Serial.print("Short RTD value: "); Serial.println(rtd1);
  float ratio = rtd;
  float ratio1 = rtd1;
  ratio1 /= 32768;
  ratio /= 32768;
  Serial.print("Ratio difference ");Serial.println(RREF*ratio - RREF1*ratio1);
  Serial.print("Long Ratio = "); Serial.println(ratio,8);
  Serial.print("Long Resistance = "); Serial.println(RREF*ratio,8);
  Serial.print("Long Temperature = "); Serial.println(max.temperature(RNOMINAL, RREF));
  
  Serial.print("Short Ratio = "); Serial.println(ratio1,8);
  Serial.print("Short Resistance = "); Serial.println(RREF1*ratio1,8);
  Serial.print("Short Temperature = "); Serial.println(max1.temperature(RNOMINAL, RREF1));
  // Check and print any faults
  Serial.println();
  delay(1000);
  float temp = RREF*ratio - RREF1*ratio1;
  if(temp >= 0.15 || temp <= -0.15)
  {
    RREF = RREF - floor(temp * 4);
  }
  Serial.println(RREF);
  Serial.println(RREF1);
  counter ++;
}
