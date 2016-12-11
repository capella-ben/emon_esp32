#include "nvs.h"
#include "nvs_flash.h"

esp_err_t err;
nvs_handle nvs;


void writePair(nvs_handle handle, char* key, char* value) {
  err = nvs_set_str(handle, key, value);
  if (err != 0) {
    Serial.print("Error writing key: ");
    Serial.print(key);
    Serial.print(" ERROR CODE: ");
    Serial.println(err, HEX);
  }
  err = nvs_commit(handle);
  if (err != 0) {
    Serial.print("Error committing: ");
    Serial.print(key);
    Serial.print(" ERROR CODE: ");
    Serial.println(err, HEX);
  }

  // read the value back
  size_t required_size;
  nvs_get_str(handle, key, NULL, &required_size);    //find the size of the value
  char* valueOut = (char*)malloc(required_size);       //set the size of the value
  nvs_get_str(handle, key, valueOut, &required_size);   //get the value

  Serial.print(key);
  Serial.print(":\t");
  Serial.println(valueOut);


}



void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println("Writing Config...");
  Serial.println();

  //initialise NVS
  err = nvs_flash_init();

  // open NVS and get the handle
  err = nvs_open("homeSys", NVS_READWRITE, &nvs);
  if (err != 0) {
    Serial.print("Error opening NVS. ERROR CODE: ");
    Serial.println(err, HEX);
  }

  // Erase all keys in a namespace
  err = nvs_erase_all(nvs);
  if (err != 0) {
    Serial.print("Error erasing namespace. ERROR CODE: ");
    Serial.println(err, HEX);
  }

  // write a value to NVS
  writePair(nvs, "ip", "192.168.0.100");
  writePair(nvs, "netmask", "255.255.255.0");
  writePair(nvs, "gateway", "192.168.0.1");
  writePair(nvs, "wifiSsid", "***");
  writePair(nvs, "wifiPwd", "***");
  


}

void loop() {


}
