#include <Arduino.h>

String parse_string(String in_str){
    in_str.trim();
    String ret_str = "!"+in_str+"\r\n";

return ret_str;
}