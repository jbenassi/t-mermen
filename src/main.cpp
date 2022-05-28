#include <Arduino.h>
//#include "MIDIcontroller.h"
#include <MIDIUSB.h>
#include <MIDI.h>
#include <MIDI.hpp>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#define PIN        7
#define NUMPIXELS 1
byte MIDIchannel = 5;

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

unsigned char ok_flag;
unsigned char fail_flag;

unsigned short lenth_val = 0;
unsigned char i2c_rx_buf[16];
unsigned char dirsend_flag=0;
const int numReadings = 50;

int readings[numReadings];      // the readings from the TOF sensor
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average

void SensorRead(unsigned char addr,unsigned char* datbuf,unsigned char cnt)
{
    unsigned short result=0;
    // step 1: instruct sensor to read echoes
    Wire.beginTransmission(82); // transmit to device #82 (0x52)
    // the address specified in the datasheet is 164 (0xa4)
    // but i2c adressing uses the high 7 bits so it's 82
    Wire.write(byte(addr));      // sets distance data address (addr)
    Wire.endTransmission();      // stop transmitting
    // step 2: wait for readings to happen
    delay(1);                   // datasheet suggests at least 30uS
    // step 3: request reading from sensor
    Wire.requestFrom(82, cnt);    // request cnt bytes from slave device #82 (0x52)
    // step 5: receive reading from sensor
    if (cnt <= Wire.available()) { // if two bytes were received
        *datbuf++ = Wire.read();  // receive high byte (overwrites previous reading)
        *datbuf++ = Wire.read(); // receive low byte as lower 8 bits
    }
}

int ReadDistance(){
    SensorRead(0x00,i2c_rx_buf,2);
    lenth_val=i2c_rx_buf[0];
    lenth_val=lenth_val<<8;
    lenth_val|=i2c_rx_buf[1];
    //delay(300);
    return lenth_val;
}

void setup() {

#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
    clock_prescale_set(clock_div_1);
#endif

    for (int thisReading = 0; thisReading < numReadings; thisReading++) {
        readings[thisReading] = 0;
    }
    pixels.begin();
    Wire.begin();
    //Serial.begin(9600);
    //printf_begin();


}

void loop() {

    pixels.clear();
    int x=ReadDistance();

    if(x>=255)
        x = 255;

    //Serial.print(x);
    //Serial.println(" mm");
    int b = 255 - x;

    total = total - readings[readIndex];
    readings[readIndex] = b;
    total = total + readings[readIndex];
    readIndex = readIndex + 1;
    if (readIndex >= numReadings) {
        // ...wrap around to the beginning:
        readIndex = 0;
    }
    average = total / numReadings;
    //Serial.println("Value of b is: " + String(b));
    pixels.setPixelColor(0,1, average / (average / 2) ,average);
    pixels.setBrightness(average);

    //need to map a range for MIDI
    unsigned short midiVal = map(average, 0, 255, 0, 127);
    //Serial.println(midiVal);
    usbMIDI.sendControlChange(74, midiVal, 1);

    pixels.show();
    //delay(1);
    while (usbMIDI.read()) {
        // ignore incoming messages
    }
}

int serial_putc( char c, struct __file * )
{
    Serial.write( c );
    return c;
}




