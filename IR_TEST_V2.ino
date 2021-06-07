/*ADAlogger Code by Erik Bruenner*/

#define DECODE_NEC 1

//#include <IRremote.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_INA219.h>
#include <wire.h>
#include "IRLibAll.h"
#include <stdlib.h>

//specify IR pin
const int IR_RECEIVE_PIN = 6;
/*Pin to detect Battery Voltage*/
const int  PV_VOLT_PIN = A1;
/*THESE ARE PREDFINED FOR FEATHERM0*/
/*used for SD Library*/
const int cardSelect = 4;
const int cardDetect = 7;
//this is predefined for the feather M0
//const int IR_SEND_PIN = 12;
/*Calculated constant to convert to float voltage*/
const float ANALOG_CONST = 0.003226;
/*Global Variables to store Voltage and Current Values*/
float PV_VOLTAGE;
float PV_CUR;
Adafruit_INA219 ina219;

/*we initialize or IR state to IDLE*/
int IR_STATE = 0;

/*Declare our sender and Receiver objects*/
/*This is for IR library*/
IRsend mySender;
IRrecv myReceiver(IR_RECEIVE_PIN);
IRdecode myDecoder;

/*necessary setup for SD library*/
void INIT_SD()
{    
    if (!SD.begin(cardSelect)) 
    {
      Serial.println("initialization failed. Things to check:");
      Serial.println("1. is a card inserted?");
      Serial.println("2. is your wiring correct?");
      Serial.println("3. did you change the chipSelect pin to match your shield or module?");
      Serial.println("Note: press reset or reopen this serial monitor after fixing your issue!");
      while (true);
    }
}

/*Enable our IR receiver and make sure State is set to 0*/
void INIT_IR_RECV()
{
  /*enable the receiver*/
  myReceiver.enableIRIn();
  IR_STATE = 0;
}

/*init ouir INA219 library*/
void INIT_INA219()
{
    //INA219 SETUP
    if (! ina219.begin()) 
    {
        Serial.println("Failed to find INA219 chip");
        while (1) { delay(3); }
    }
}

/*Arduino Setup funciton, call all of the above setup functions*/
void setup()
{
    INIT_SD();
    INIT_IR_RECV();
    INIT_INA219();
}

/*Predefined constant types for Packet Sending*/
const uint32_t TIMEPACKET = 1;
const uint32_t VOLTAGEPACKET = 2;
const uint32_t CURRENTPACKET = 3;
const uint32_t EOFPACKET = 4;

void IR_SEND()
{
     File myFile;
     /*our buffer used to read lines from SD Card*/
     char buffer[35];
     /*Loop Index*/
     int i = 0;
     /*Buffers to store indiviual line tokens*/
     char voltage[11];
     char current[11];
     char timeval[11];
     /*32bit int to hold raw values*/
     uint32_t value;
     /*double precision floating point to temporaraly hold value*/
     double dubVal;
     /*open the sd card for reading*/
     myFile = SD.open("VD.txt");
     /*check to make sure file opened correctly*/
     if(myFile)
     {
        while(myFile.available())
        {
          /*read 34 byts from file into buffer*/
          myFile.read(buffer,34);
          /*null terminate buffer*/
          buffer[34] = '\0';
          /*we now have our line read*/

          /*copy our line tokens into our token buffers*/
          memcpy(voltage,&(buffer[0]),10);
          voltage[10] = '\0';
          memcpy(current, &(buffer[11]), 10);
          current[10] = '\0';
          memcpy(timeval, &(buffer[22]),10);
          timeval[10] = '\0';

          /*send the voltage*/
          /*store the voltage value as a double*/
          dubVal = atof(voltage);
          /*convert voltage to a 24 bit value between 0 and 5V*/
          dubVal = dubVal/5.0;
          dubVal = dubVal * 16777215.0;
          value = (uint32_t)dubVal;
          /*add in our packet header to the MSByte*/
          value = value | (VOLTAGEPACKET << 24);
          /*send final value*/
          mySender.send(DECODE_NEC, value, 32);

          /*send the current*/
          /*calcualte double from buffer token*/
          dubVal = atof(current);
          /*convert current value to 24 bit value between 0 and 1 amp*/
          dubVal = dubVal/1000.0;
          dubVal = dubVal * 16777215.0;
          value = (uint32_t)dubVal;
          /*add our packet header*/
          value = value | (CURRENTPACKET << 24);
          /*send our packet*/
          mySender.send(DECODE_NEC, value, 32);
          
          /*send the time*/
          /*calcuate an integer from the buffer value*/
          value = atoi(timeval);
          /*now we add in our packet data type (header)*/
          value = value | (TIMEPACKET << 24);
          /*send our value*/
          mySender.send(DECODE_NEC, value,32);
        }
        /*we have reached EOF*/
        /*Send EOF Packet 20 times to make sure receiver doesn't hang*/
        for(i = 0; i < 20; i++)
        {
          mySender.send(DECODE_NEC, EOFPACKET << 24, 32);
        }
     }
     /*print an error when we are unable to open file*/
     else 
     {
      Serial.println("unable to open VD.txt");
     }
     /*we now go back to Record Data State*/
     IR_STATE = 1;
     /*By Spec of the IR Library, we must reenable IR receiveing after we send*/
     myReceiver.enableIRIn();
     myFile.close();
}

/*This funtion updates the PV Voltage*/
void PV_GATHER()
{
    int PV_VAL = 0;
    /*Take the average of 10 voltage samples to avoid outliers*/
    for (int i = 0; i < 10; i ++)
    {
       PV_VAL += analogRead(PV_VOLT_PIN);
    }
    PV_VAL = PV_VAL/10;
    /*convert our voltage to a flaot value*/
    PV_VOLTAGE = float(PV_VAL)*ANALOG_CONST;
    /*measured vonstant with 1M resistor*/
    PV_VOLTAGE = PV_VOLTAGE*1.45;
}

/*This function updates the Current Value*/
void PV_CURRENT()
{
  float current_mA = 0;
  current_mA = ina219.getCurrent_mA();
  PV_CUR = current_mA;
}

/*This function clears the data stored on the SD Card*/
void CLEAR_DATA()
{
    SD.remove("VD.txt");
    IR_STATE = 1;
}

/*Receives IR Data from TV Remote*/
/*This is what changes our state machine*/
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
            IR_STATE = 2;
           }
           
           //button 1 on the remote. In this case we start collecting data
            if (myDecoder.value == 0x20DF8877) 
            {
                //in this state we are collecting data
                IR_STATE = 1;
            }
            
            //button 0 on remote. In this case we Idle
            else if (myDecoder.value == 0x20DF08F7) 
            {
                //in this state we idle
                IR_STATE = 0;
            }
      }
}

/*In this case we store our data on the sd card of the logger*/
void WRITE_TO_SD()
{
  int i = 0;
  float currentVal = 0;

  /*open the data file*/
  File voltageData = SD.open("VD.txt", FILE_WRITE);
  if(voltageData)
  {
    /*this printstring is what we will ultimetly write to the SD card*/
    String printstring = "";
    
    //add voltage value to print string
    currentVal = PV_VOLTAGE;
    //add leading zeros to standardize length when reading file
    for(i = 0;i < 10 - (String(currentVal)).length(); i++)
    {
      printstring += "0";
    }
    printstring += String(currentVal);
    printstring += "\t";
    
    //add current value to print string
    currentVal = PV_CUR;
    //add leading zeros
    for(i = 0;i < 10 - (String(currentVal)).length(); i++)
    {
      printstring += "0";
    }
    printstring += String(currentVal);
    printstring += "\t";

    //add time value to print string
    //add leading zeros
    for(i = 0; i < 10-String(millis()).length(); i ++)
    {
      printstring += "0";
    }
    printstring += String(millis());
    /*write value to sd card*/
    voltageData.println(printstring);
    voltageData.close();
  }
}

void loop() 
{
  /*this determines the sampling frequencey*/
  /*I found that 10seconds is a good interval*/
  int interval = 10000;
  static long startMillis = 0;
  long currentMillis = 0;

  //if our interval is over, we now take a reading
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


