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

        //Serial.println("int sens_value = "+String(sens_value));//DEBUG
        //Serial.println("float sens_voltage = "+String(sens_voltage));//DEBUG
        //Serial.println("float volts_per_percent = "+String(volts_per_percent));//DEBUG   
        //Serial.println("unit8_t battery_level = "+String(_battery_level));//DEBUG
        //Serial.println();
}

//attenuator calibration
String storage_att_factor(String su){
    _max_voltage=su.toFloat(); //string to float
    int sens_value = analogRead(0);  //read ADC0 (pin 0)
    float sens_voltage=sens_value * _adc_ref / 4096; // calculate (U_adc)
    _att_factor = _max_voltage/sens_voltage;
    Serial.println("new att_factor="+String(_att_factor));

    preferences.begin("Battery", false);
    preferences.putFloat("att_factor", _att_factor);
    preferences.putFloat("max_voltage", _max_voltage);
    preferences.end();

    return "new att_factor="+String(_att_factor)+"\r\n";
}

String storage_min_voltage(String su){
    _min_voltage=su.toFloat(); //string to float

    preferences.begin("Battery", false);
    preferences.putFloat("min_voltage", _min_voltage);
    preferences.end();

    return "new min_voltage="+String(_min_voltage)+"\r\n";
}


String parse_string(String in_str){
    in_str.trim();
    String ret_str = "???\r\n";

    // actions are performed depending on the command
    if (in_str=="at") {     //at test
        ret_str="OK\r\n";
    }
    else if (in_str=="at?") { //at? - help
        ret_str = String(shelp);
    }
    else if (in_str=="atv") { //atv - current voltage
        calculate_current_values();
        ret_str = String(_real_voltage,3)+"\r\n";
    }
    else if (in_str.substring(0,4)=="ath=") { //ath= - U_max, attenuator calibration
        ret_str = storage_att_factor(in_str.substring(4));
    }
    else if (in_str.substring(0,4)=="atl=") { //atl= - minimum voltage
        ret_str = storage_min_voltage(in_str.substring(4));
    }
    else if (in_str=="atz") { //atz - reset NVRAM
        reset_nvram();
        ret_str = "NVRAM Keys Reset..\r\n";
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
            
return ret_str;
}

void terminal_init(){
    pinMode(0, INPUT); // ADC0 pin input

    //read all NVRAM parameters
    preferences.begin("Battery", true); //NVRAM open (read only)
    _max_voltage = preferences.getFloat("max_voltage", 12.5);
    _min_voltage = preferences.getFloat("min_voltage", 9.3);
    _att_factor = preferences.getFloat("att_factor", 4);  //5.13
    preferences.end(); //NVRAM close
    calculate_current_values();
}

//read voltage of sensor(9-12V)
float read_voltage_sensor(){
    calculate_current_values();
    return _real_voltage;
}

//read % of sensor (0-100%)
uint8_t read_battery_level(){
    calculate_current_values();
    return _battery_level;
}