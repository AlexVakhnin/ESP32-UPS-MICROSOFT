#include <Arduino.h>

extern void ble_ups_init(); //BLE-UPS.cpp
extern void update_battery_level(uint8_t blevel); //BLE-UPS.cpp

void setup() {
  Serial.begin(115200);

  //LED (internal)
  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH); //led = OFF
  
  delay(7000);  //10 sec for Platformio start terminal...

  Serial.println();
  Serial.println("BLE Battery Level Indicator");
  Serial.println("----------------Start Info-----------------");
  Serial.printf("Total heap:\t%d \r\n", ESP.getHeapSize());
  Serial.printf("Free heap:\t%d \r\n", ESP.getFreeHeap());
  //Serial.println("ADC_PIN= "+String(sens_pin));
  Serial.println("-----------------------------------------");

  ble_ups_init();
  Serial.println("OK!-START..");
}

uint8_t battery_level = 0;

void loop() {
  update_battery_level(battery_level);  //change Battery Service value
  //Serial.println(int(battery_level));

  delay(5000);
  battery_level++;
  if (int(battery_level) == 100)
    battery_level = 0;
}
