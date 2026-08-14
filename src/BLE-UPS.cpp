#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
//#include "esp_bt.h" // Нужен для настройки мощности TX
#include <esp_wifi.h> //нужен для полного отключения передатчика WIFI


//Nordic UART Service (NUS)
#define Service_Term_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" //offset: 0x0001
#define Characteristic_Rx_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" //offset: 0x0002 (Write)
#define Characteristic_Tx_UUID "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" //offset: 0x0003 (Notify)

//Battery Service
#define Service_Batt_UUID "0000180F-0000-1000-8000-00805f9b34fb"  //offset: 0x180F
#define Characteristic_Batt_UUID "00002a19-0000-1000-8000-00805f9b34fb"  //offset: 0x2A19 (Read, Notify)

extern String parse_string(String in_str);  //TERM.cpp
bool _BLEConnected = false;
uint16_t m_connect_id = -1; //when connecting, remember the ID
long wdt_counter = 0; //to determine if the BLE client is frozen

BLEServer* pServer = NULL;
//Create Characteristics with their UUID and Properties
BLECharacteristic pTxCharacteristic(Characteristic_Tx_UUID, BLECharacteristic::PROPERTY_NOTIFY);
BLECharacteristic pRxCharacteristic(Characteristic_Rx_UUID, BLECharacteristic::PROPERTY_WRITE);
BLECharacteristic pBattCharacteristic(Characteristic_Batt_UUID,
                        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

//Connect, Disconnect handling
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) {
        Serial.println("Event-Connect..");
        _BLEConnected = true;
        m_connect_id = param->connect.conn_id; //we remember..
        wdt_counter = 0; //wdt reset
        digitalWrite(8, LOW); //led = ON
    };
    void onDisconnect(BLEServer* pServer) {
        _BLEConnected = false;
        m_connect_id = -1; //reset..
        digitalWrite(8, HIGH); //led = OFF
        Serial.println("Event-Disconnect..");
        delay(300); // give the bluetooth stack the chance to get things ready
        BLEDevice::startAdvertising();  // restart advertising
    }
};

class MyRxCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            wdt_counter=0; //received a string (client is alive) - reset the watchdog counter
            //analyze input string, generate output string             
            String tx_str = parse_string(String(value.c_str()));
            pTxCharacteristic.setValue(tx_str.c_str());
            pTxCharacteristic.notify();
        }
    }
};



void ble_ups_init(){
    // Completely halt the Wi-Fi modem to isolate BLE
    esp_wifi_stop(); 

    BLEDevice::init("UPS-PC-WIN11"); //init BLE stack..
    
    //---------------
    // 2. Снижение мощности TX (для ESP32-C3 доступно от -24 до +21 dBm)
    // ESP_PWR_LVL_N9 (-9 dBm) или ESP_PWR_LVL_N12 (-12 dBm) сильно экономят ток
    //esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_N9);
    //esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_N9);
    //---------------
    
    pServer = BLEDevice::createServer();  //Srever create
    pServer->setCallbacks(new MyServerCallbacks()); //set callback for Server
    
    BLEService *pTerm = pServer->createService(Service_Term_UUID);  //Service Terminal create
    pTerm->addCharacteristic(&pTxCharacteristic);  //Characteristic Tx (Notify)
    pTxCharacteristic.addDescriptor(new BLE2902());  //notifications control from client side
    pTerm->addCharacteristic(&pRxCharacteristic);  //Characteristic Rx (Write)
    pRxCharacteristic.setCallbacks(new MyRxCallbacks()); //set callback for terminal Rx
    
    BLEService *pBatt = pServer->createService(Service_Batt_UUID);  //Service Battery create
    pBatt->addCharacteristic(&pBattCharacteristic);  //Characteristic Batt (Read, Notify)
    pBattCharacteristic.addDescriptor(new BLE2902());  //notifications control from client side

    pTerm->start(); //Service Terminal start
    pBatt->start(); //Service Battery start
    pServer->getAdvertising()->addServiceUUID(Service_Term_UUID);  //Advertising init
    pServer->getAdvertising()->start(); //Advertising start

    //--------------
    // 4. Настройка интервалов рекламы (Advertising)
    //BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    //pAdvertising->addServiceUUID(pBatt->getUUID());
    //pAdvertising->setScanResponse(true);
  
    // Увеличиваем интервал рекламы до 1.5 секунд (1500 мс / 0.625 = 2400)
    //pAdvertising->setMinInterval(2400); 
    //pAdvertising->setMaxInterval(2400);
  
    // 5. Запрос на редкие интервалы соединения после подключения смартфона
    // Мин интервал: 200мс (160 * 1.25), Макс интервал: 400мс (320 * 1.25)
    // Slave Latency: 4 (пропуск 4 тактов связи), Timeout: 6 секунд (600)
    //pAdvertising->setMinPreferred(160);
    //pAdvertising->setMaxPreferred(320); 

    //BLEDevice::startAdvertising(); //Advertising start
    //--------------

}

//change Battery service value
void update_battery_level(uint8_t blevel){ 
    if (blevel > 100) {blevel=100;}
    if (blevel < 0) {blevel=0;}
    pBattCharacteristic.setValue(&blevel, 1);
    pBattCharacteristic.notify();
}
//watchdog timer - BLE client hang handling
void wdt_handle(){
  if(_BLEConnected and m_connect_id != -1){
    wdt_counter++;
    if(wdt_counter>30){ //30 * 5 sec
      wdt_counter=0;
      pServer->disconnect(m_connect_id) ;//force disconnect client..
    }

  }
}