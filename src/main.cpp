#include <Arduino.h>
//#include <nvs_flash.h> //for all NVRAM clean

extern void ble_ups_init();
extern void update_battery_level(uint8_t blevel);
extern void wdt_handle();
extern void terminal_init();
extern uint8_t read_battery_level();
extern void adc_filter_handle();

long time_last_wdt=0;
long time_last_adc=0;

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
  Serial.println("------------------------------------------");

  terminal_init();
  ble_ups_init();
  
  Serial.println("OK!-START..");
}

void loop() {
  long time_now=millis();
  //Task T=1sec.
  if( abs(time_now - time_last_adc) >= 1000 or time_last_adc > time_now){
    adc_filter_handle();
    time_last_adc=time_now;
  }
  //Task T=5sec.
  if( abs(time_now - time_last_wdt) >= 5000 or time_last_wdt > time_now){
    update_battery_level(read_battery_level());  //change Battery Service value
    wdt_handle();
    time_last_wdt=time_now;
  }

  // Позволяем FreeRTOS усыпить процессор на 1 секунду до следующей проверки
  vTaskDelay(pdMS_TO_TICKS(500)); //1000
}
