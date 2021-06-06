/*ADAlogger Code by Erik Bruenner*/

#define DECODE_NEC 1
#define DEBUG 1

//#include <IRremote.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_INA219.h>
#include <wire.h>
#include "IRLibAll.h"
#include <stdlib.h>



//specify IR pin
const int IR_RECEIVE_PIN = 6;
const int  PV_VOLT_PIN = A1;
const int cardSelect = 4;
const int cardDetect = 7;
//this is predefined for the feather M0
//const int IR_SEND_PIN = 12;
const float ANALOG_CONST = 0.003226;
const int CURRENT_LED = 11;
float PV_VOLTAGE;
float PV_CUR;
Adafruit_INA219 ina219;

int IR_STATE = 0;

IRsend mySender;
IRrecv myReceiver(IR_RECEIVE_PIN);
IRdecode myDecoder;
void INIT_SD()
{    
    if(DEBUG)Serial.print("Initializing SD card...");
  
    if (!SD.begin(cardSelect)) 
    {
      Serial.println("initialization failed. Things to check:");
  
      Serial.println("1. is a card inserted?");
  
      Serial.println("2. is your wiring correct?");
  
      Serial.println("3. did you change the chipSelect pin to match your shield or module?");
  
      Serial.println("Note: press reset or reopen this serial monitor after fixing your issue!");
  
      while (true);
    }

//    File voltageData = SD.open("VD.txt", FILE_WRITE);
//    if(voltageData)
//    {
//      voltageData.close();
//    }
//    else
//    {
//      while(true)
//      {
//        Serial.println("Text file init failed");
//      }
//    }
}

void INIT_IR_RECV()
{
  /*enable the receiver*/
  myReceiver.enableIRIn();
  IR_STATE = 0;
}

void INIT_INA219()
{
    //INA219 SETUP
    if (! ina219.begin()) 
    {
        if(DEBUG)Serial.println("Failed to find INA219 chip");
        while (1) { delay(3); }
    }
}

void INIT_CURRENT_INDICATOR()
{
  pinMode(CURRENT_LED, OUTPUT);
}

void setup()
{
  /*init serial with 9600 baud rate*/
    if(DEBUG)Serial.begin(115200);
//    Serial2.begin(115200);
    INIT_SD();
    INIT_IR_RECV();
    INIT_INA219();
    INIT_CURRENT_INDICATOR();
}

const uint32_t TIMEPACKET = 1;
const uint32_t VOLTAGEPACKET = 2;
const uint32_t CURRENTPACKET = 3;
const uint32_t EOFPACKET = 4;

void IR_SEND()
{
     uint32_t irda;
     File myFile;
     String line = "";
     char buffer[35];
     int i = 0;
     char voltage[11];
     char current[11];
     char timeval[11];
     uint32_t value;
     double dubVal;
     /*open the sd card for reading*/
     myFile = SD.open("VD.txt");
     
     if(myFile)
     {
        if(DEBUG)Serial.println("file successfully opened for reading");
        while(myFile.available())
        {
          if(DEBUG)Serial.println("Reading Line");
//          for(i = 0; i < 34; i ++)
//          {
//             buffer[i] = myFile.read();
//          }
          myFile.read(buffer,34);
          buffer[34] = '\0';
          /*we now have our line read*/
          memcpy(voltage,&(buffer[0]),10);
          voltage[10] = '\0';
          memcpy(current, &(buffer[11]), 10);
          current[10] = '\0';
          memcpy(timeval, &(buffer[22]),10);
          timeval[10] = '\0';

          /*send the voltage*/
          dubVal = atof(voltage);
          /*convert voltage to a 24 bit value between 0 and 5V*/
          dubVal = dubVal/5.0;
          dubVal = dubVal * 16777215.0;
          value = (uint32_t)dubVal;
          /*clear upper 8 bits*/
          value = value | (VOLTAGEPACKET << 24);
          mySender.send(DECODE_NEC, value, 32);

          /*send the current*/
          dubVal = atof(current);
          /*convert current value to 24 bit value between 0 and 1 amp*/
          dubVal = dubVal/1000.0;
          dubVal = dubVal * 16777215.0;
          value = (uint32_t)dubVal;
          value = value | (CURRENTPACKET << 24);
          mySender.send(DECODE_NEC, value, 32);
          
          /*send the time*/
          value = atoi(timeval);
          /*now we add in our packet data type*/
          value = value | (TIMEPACKET << 24);
          /*send our value*/
          mySender.send(DECODE_NEC, value,32);
        }
        /*we have reached EOF*/
        for(i = 0; i < 20; i++)
        {
          mySender.send(DECODE_NEC, EOFPACKET << 24, 32);
        }
     }
     else 
     {
      if(DEBUG)Serial.println("unable to open VD.txt");
     }
     IR_STATE = 1;
     if(DEBUG)Serial.println("Leaving IR_SEND state");
     myReceiver.enableIRIn();
     myFile.close();
}

void PV_GATHER()
{
  int PV_VAL = 0;
    for (int i = 0; i < 10; i ++)
    {
       PV_VAL += analogRead(PV_VOLT_PIN);
    }
    PV_VAL = PV_VAL/10;
    PV_VOLTAGE = float(PV_VAL)*ANALOG_CONST;
}

void PV_CURRENT()
{
  float current_mA = 0;;
  current_mA = ina219.getCurrent_mA();
  PV_CUR = current_mA;
  if(PV_CUR > 1)
  {
    digitalWrite(CURRENT_LED, HIGH);
  }
  else
  {
    digitalWrite(CURRENT_LED, LOW);
  }
}
void CLEAR_DATA()
{
    if(DEBUG)Serial.println("deleting data");
    SD.remove("VD.txt");
    IR_STATE = 1;
}
void IR_RECEIVE()
{
    if (myReceiver.getResults()) 
    {
          myDecoder.decode();
          //dump ir results for testing purposes
//          myDecoder.dumpResults(true);
          myReceiver.enableIRIn();

         //replay button on remote
         //this will wipe the data file
         if(myDecoder.value == 0x20DF58A7)
         {
          IR_STATE = 3;
         }
         //button 2 on the remote. In stis case we send data from SD card
         if(myDecoder.value == 0x20DF48B7)
         {
          if(DEBUG)Serial.println("sending data");
          IR_STATE = 2;
         }
         //button 1 on the remote. In this case we start collecting data
        if (myDecoder.value == 0x20DF8877) 
        {
            //in this state we are collecting data
            IR_STATE = 1;
        }
        //button 0 on remote. In this case we 
        else if (myDecoder.value == 0x20DF08F7) 
        {
            //in this state we idle
            IR_STATE = 0;
        }
    }
}

void WRITE_TO_SD()
{
  int i = 0;
  float currentVal = 0;
  File voltageData = SD.open("VD.txt", FILE_WRITE);
  if(voltageData)
  {
    String printstring = "";
    //add voltage value to print string
    currentVal = PV_VOLTAGE;
    if(currentVal < 0)
    {
      currentVal = 0.0;
    }
    //add leading zeros to standardize length when reading file
    for(i = 0;i < 10 - (String(currentVal)).length(); i++)
    {
      printstring += "0";
    }
    printstring += String(currentVal);
    printstring += "\t";

    //add current value to print string
    currentVal = PV_CUR;
    if(currentVal < 0)
    {
      currentVal = 0.0;
    }

    for(i = 0;i < 10 - (String(currentVal)).length(); i++)
    {
      printstring += "0";
    }
    printstring += String(currentVal);
    printstring += "\t";

    //add time value to print string
    /*print leading zeros for formatting purposes*/
    for(i = 0; i < 10-String(millis()).length(); i ++)
    {
      printstring += "0";
    }
    printstring += String(millis());
    Serial.print("Writing to SD Card: ");
    Serial.println(printstring);
    voltageData.println(printstring);
    voltageData.close();
  }
}
void loop() 
{
  /*set interval to 20 seconds*/
  int interval = 5000;
  static long startMillis = 0;
  long currentMillis = 0;

  //if our half second delay is over, we now take a reading

  if ( (IR_STATE == 1) &&((currentMillis = millis())-startMillis) > interval)
  {
    startMillis = currentMillis;
    PV_GATHER();
    PV_CURRENT();
    WRITE_TO_SD();
  }

  if(IR_STATE == 2)
  {
    IR_SEND();
  }
  if(IR_STATE == 3)
  {
    CLEAR_DATA();
  }
    IR_RECEIVE();
}


