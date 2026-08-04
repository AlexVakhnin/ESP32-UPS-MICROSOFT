#include <Arduino.h>
#include <Preferences.h>

Preferences preferences; //for NVRAM

float _real_voltage =0; //current measuring voltage ADC0 with attenuator
uint8_t _battery_level = 0; //current battery level %

float _max_voltage =0;  //maximum possible voltage (EEPROM) 12.5
float _min_voltage =0;  //minimum possible voltage (EEPROM) 9.3
float _att_factor = 0;  //attenuator factor (EEPROM) 5.13

const float _adc_ref =2.99; //reference voltage of the ADC

const char shelp[] = "ati - parameter list\r\n"
                     "atv - current voltage\r\n"                
                     "atz - reset NVRAM\r\n"                
                     "atr - reboot device\r\n"                
                     "ath=[U_in] - max voltage(100%)->calibr\r\n"                
                     "atl=[U_min] - min voltage(0%)\r\n"                
;

void reset_nvram(){
  preferences.begin("Battery", false); //open namespace (read-write)
  preferences.clear(); //delete all keys in the namespace
  preferences.end();  //close namespace
}

//read ADC and calculate values
void calculate_current_values(){
    int sens_value = analogRead(0);  //read ADC0 (pin 0)
    float sens_voltage=sens_value * _adc_ref / 4096; // calculate (U_adc)
    _real_voltage = sens_voltage * _att_factor; //real voltage with attenuatir (U_inp)
    float volts_per_percent = abs((_max_voltage-_min_voltage)/100);
    _battery_level = abs((_real_voltage - _min_voltage) / volts_per_percent);  //level %

        Serial.println("int sens_value = "+String(sens_value));//DEBUG
        Serial.println("float sens_voltage = "+String(sens_voltage));//DEBUG
        Serial.println("float volts_per_percent = "+String(volts_per_percent));//DEBUG   
}


String parse_string(String in_str){
    in_str.trim();
    String ret_str = "???\r\n";

    // actions are performed depending on the command
    if (in_str=="at") {     //at
        ret_str="OK\r\n"; //sensor number
    }
    else if (in_str=="at?") { //at? - help
        ret_str = String(shelp);
    }
    else if (in_str=="atz") { //atz - reset NVRAM
        reset_nvram();
        ret_str = "NVRAM Keys Reset..";
    }
    else if (in_str=="atr") { //atr - reboot device
        ESP.restart();
    }
    else if (in_str=="ati") { //ati - information
        calculate_current_values();
        ret_str ="real_voltage="+String(_real_voltage)
            +"\r\nbattery_level(%)="+String(_battery_level)
            +"\r\nmax_voltage="+String(_max_voltage)
            +"\r\nmin_voltage="+String(_min_voltage)
            +"\r\natt_factor="+String(_att_factor) 
            +"\r\nadc_ref_voltage="+String(_adc_ref)+"\r\n";
    }
            
/*        
            else if (pstr=="ati"||pstr=="ati\r\n") { //ati - information
              String zone="";String ac220="";
              if(zone_flag==1) {zone ="HIGH";} else if(zone_flag==2) {zone ="LOW";} else {zone ="MIDDLE";}
              if(ac220v_flag) {ac220="ON";} else {ac220="OFF";}  
              String s ="name="+dev_name
                  +"\r\natv_counter="+String(ble_period)+"<-"+String(ble_pcounter)
                  +"\r\nstatus="+dispstatus
                  +"\r\nzone="+zone
                  +"\r\nac220v="+ac220
                  +"\r\nrelay="+String(digitalRead(orange_pin))
                  +"\r\nalarm_h="+String(alarm_h)
                  +"\r\nalarm_l="+String(alarm_l) 
                  +"\r\nreal_voltage="+String(real_voltage);
                ble_handle_tx(s); //information for debug
            }
            else if (pstr=="atv"||pstr=="atv\r\n") { //atv - result voltage

                //измеряем время между опросами напряжения от orange pi
                ble_period=ble_pcounter; //время между BLE опросами
                ble_pcounter=0; //обнуляем счетчик (T=2000)

                String rv = String(real_voltage,3);
                ble_handle_tx(rv); //ответ c учетом калибровки
            }
            else if (pstr.substring(0,4)=="atu=") {  //atu= - attenuator 0 calibration
                storage_factor_u(pstr.substring(4));
            }
            else if (pstr.substring(0,5)=="atu1=") {  //atu1= - attenuator 1 calibration
                storage_factor1_u(pstr.substring(5)); //с позиции 5 и до конца..
            }
            else if (pstr.substring(0,4)=="ata=") { //ata= - ADC calibration
                storage_adc_u(pstr.substring(4));
            }
            else if (pstr.substring(0,4)=="ath=") { //ath= - alarm_h save NVRAM
                storage_alarm_h(pstr.substring(4)); //alarm_h
            }
            else if (pstr.substring(0,4)=="atl=") { //atl= - alarm_l save NVRAM
                storage_alarm_l(pstr.substring(4)); //alarm_l
            }
            else if (pstr.substring(0,4)=="atn=") { //atn= - dev_name save NVRAM
                storage_dev_name(pstr.substring(4)); //dev_name
            }
*/

return ret_str;
}

void terminal_init(){
    pinMode(0, INPUT); // ADC0 pin input

    //read all NVRAM parameters
    preferences.begin("Battery", true); //NVRAM open (read only)
    _max_voltage = preferences.getFloat("max_voltage", 12.5);
    _min_voltage = preferences.getFloat("min_voltage", 9.3);
    _att_factor = preferences.getFloat("att_factor", 5.13);  //5.13
    preferences.end(); //NVRAM close
}

//read voltage of sensor(9-12V)
float read_voltage_sensor(){


    float value = 12.05;
    return value;
}

//read % of sensor (0-100%)
uint8_t read_battery_level(){


    uint8_t value = 50;
    return value;
}