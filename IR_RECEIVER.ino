/*
 * SimpleReceiver.cpp
 *
 * Demonstrates receiving NEC IR codes with IRrecv
 *
 *  Copyright (C) 2020-2021  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of Arduino-IRremote https://github.com/z3t0/Arduino-IRremote.
 *
 *  MIT License
 */

/*
 * Specify which protocol(s) should be used for decoding.
 * If no protocol is defined, all protocols are active.
 */
#define DECODE_NEC 1

//#include <IRremote.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_INA219.h>
#include <wire.h>
#include "IRLibAll.h"



//specify IR pin
const int IR_RECEIVE_PIN = 6;

IRrecv myReceiver(IR_RECEIVE_PIN);
IRdecode myDecoder;

void INIT_IR_RECV()
{
  /*enable the receiver*/
  myReceiver.enableIRIn();
}

void setup()
{
  /*init serial with 9600 baud rate*/
    Serial.begin(115200);
//    INIT_SD();
    INIT_IR_RECV();
}

void IR_RECEIVE()
{
  uint32_t value;
    if (myReceiver.getResults()) {
         myDecoder.decode();
         value = myDecoder.value;
          Serial.write(value>>24);
          Serial.write(value>>16);
          Serial.write(value>>8);
          Serial.write(value>>0);
          //dump ir results for testing purposes
//          myDecoder.dumpResults(true);
          myReceiver.enableIRIn();
    }
}

void loop() 
{
    IR_RECEIVE();
}

