#include <Arduino.h>
//#include <nvs_flash.h> //for all NVRAM clean

extern void ble_ups_init();
extern void update_battery_level(uint8_t blevel);
extern void wdt_handle();
extern void terminal_init();
extern uint8_t read_battery_level();

void setup() {
  delay(7000);  //10 sec for Platformio start terminal...

  Serial.begin(115200);

//for all NVRAM erase (#include <nvs_flash.h>)
    //nvs_flash_erase();      // erase the NVS partition and...
    //nvs_flash_init();       // initialize the NVS partition.
    //Serial.print("NVRAM cleared, STOP..");
    //while (true); //STOP..

  //LED (internal)
  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH); //led = OFF 

  Serial.println();
  Serial.println("BLE Battery Level Indicator");
  Serial.println("----------------Start Info----------------");
  Serial.printf("Total heap:\t%d \r\n", ESP.getHeapSize());
  Serial.printf("Free heap:\t%d \r\n", ESP.getFreeHeap());
  //Serial.println("ADC_PIN= "+String(sens_pin));
  Serial.println("------------------------------------------");

  terminal_init();
  ble_ups_init();
  
  Serial.println("OK!-START..");
}

//uint8_t battery_level = 0;

void loop() {
  update_battery_level(read_battery_level());  //change Battery Service value

  wdt_handle();
  delay(5000); //5 sec.
  //battery_level++;
  //if (int(battery_level) == 100)
  //  battery_level = 0;
}
