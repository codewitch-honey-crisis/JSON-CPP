// only use this project for Arduino
#ifdef ARDUINO
#include <Arduino.h>
#include <SD.h>
#include "main.hpp"
void setup();
void loop();

void loop() {}
void setup() {
    // wire an SD reader to your 
    // first SPI bus and the standard 
    // CS for your board
    //
    // put data.json on it but
    // name it "data.jso" due
    // to 8.3 filename limits
    // on some platforms
    Serial.begin(115200);
     
    if(!SD.begin()) {
      Serial.println("Unable to mount SD");
      while(true); // halt
    }
    benchmarks();
}
#endif
