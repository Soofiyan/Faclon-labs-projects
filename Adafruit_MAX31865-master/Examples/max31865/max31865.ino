#include <Adafruit_MAX31865.h>

// Use software SPI: CS, DI, DO, CLK
Adafruit_MAX31865 max = Adafruit_MAX31865(10, 11, 12, 13);
//Adafruit_MAX31865 max1 = Adafruit_MAX31865(9, 11, 12, 13); Incase of using second pt100 in the same microcontroller just
//short the spi pins except cs pin for both max31865 and just change cs to any digital pin.

// The value of the Rref resistor. Use 430.0 for PT100 and 4300.0 for PT1000
// The 'nominal' 0-degrees-C resistance of the sensor
// 100.0 for PT100, 1000.0 for PT1000
#define RNOMINAL  100.0
float RREF = 430.0;

void setup() {
  Serial.begin(115200);
  max.begin(MAX31865_3WIRE); // set to 3WIRE
  max1.begin(MAX31865_3WIRE);
}


void loop() {
    uint16_t rtd = max.readRTD();//Reading rtd values from max31865 breakout board
    Serial.print("RTD value: "); Serial.println(rtd);
    float ratio = rtd; 
    ratio /= 32768; // Getting the ratio by dividing the rtd value by 32768
    Serial.print("Ratio = "); Serial.println(ratio,8);
    Serial.print("Resistance = "); Serial.println(RREF*ratio,8); //Resistance of the pt100 sensor.
    Serial.print("Temperature = "); Serial.println(max.temperature(RNOMINAL, RREF)); //getting the temperature values from the max31865 breakout board.
    Serial.println();
    delay(500);
}
