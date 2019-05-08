//*******************************************************************************
//* SIM808 GPS/DFRobot_SIM808/GSM Shield Library
//* @author [Jason](jason.ling@dfrobot.com)
//* @version  V1.0
//* @date  2016-09-23
//* 
//* The MIT License (MIT)
//* Permission is hereby granted, free of charge, to any person obtaining a copy
//* of this software and associated documentation files (the "Software"), to deal
//* in the Software without restriction, including without limitation the rights
//* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//* copies of the Software, and to permit persons to whom the Software is
//* furnished to do so, subject to the following conditions:
//*
//* The above copyright notice and this permission notice shall be included in
//* all copies or substantial portions of the Software.
//*
//* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//* THE SOFTWARE.
//*********************************************************************************

//Chris Griffiths
//ELEC495
//Prof Dixon
//GPS/GSM Dog Collar

#include <SoftwareSerial.h>
#include <DFRobot_sim808.h>
#include <stdlib.h>
#include <stdio.h>

//Variables sued for SMS parser
#define MESSAGE_LENGTH 160
char message[MESSAGE_LENGTH]; //incoming message
int messageIndex = 0;
char phone[16]; //Incoming phone number
char datetime[24];  //when message is received

//Variable used to keep track of last geolocationn
long lastMillis = 0;

//For GEOFENCE
boolean GEOFENCE = false; //is GEOFENCE on?
float geoLat; //Current lat
float geoLon; //Current lon

//Serial communication
#define PIN_TX 7
#define PIN_RX 8
SoftwareSerial mySerial(PIN_TX,PIN_RX);
DFRobot_SIM808 sim808(&mySerial);

void setup() {
  //Begin serial communication
  mySerial.begin(19200);
  Serial.begin(19200);
  
  //Wait for serial communication to be initialized
  while(!sim808.init()) {
      Serial.print("Sim808 init error\r\n");
      delay(1000);
  }
  delay(3000);  
  Serial.println("Init Success");


  //Turn on GPS
  if( sim808.attachGPS())
      Serial.println("Open the GPS power success");
  else 
      Serial.println("Open the GPS power failure");
      
  Serial.println();
  
  //Wait for initial location
  while (!getLOCATION()){}
}

void loop(){
  //Update location every 90 seconds
  if ((millis() - lastMillis) > 90000){
    while (!getLOCATION()){}
    lastMillis = millis();
  }

//Wait for incoming SMS
readSMS();

  //Check if device is still within GEOFENCE area if geofence is on
  if (GEOFENCE){
    if (sim808.GPSdata.lat > (geoLat + 0.0013)) //0.0013 lat is about 500 ft
      brokenGEOFENCE();
    else if (sim808.GPSdata.lat < (geoLat - 0.0013))
      brokenGEOFENCE();
    else if (sim808.GPSdata.lon > (geoLon + 0.002)) //0.002 lon is about 500 ft
      brokenGEOFENCE();
    else if (sim808.GPSdata.lon < (geoLon - 0.002))
      brokenGEOFENCE();
  }
}

//Called when geofence has been broken
void brokenGEOFENCE(){
  
  Serial.println("WARNING! Device left geofence area.");
  Serial.println();
  sim808.sendSMS(phone,"WARNING! Device left geofence area.");
  sendLOCATION();
}

//Checks for SMS
void readSMS(){
  //Detect any unread SMS
  messageIndex = sim808.isSMSunread();

  //Message is available
  if (messageIndex > 0) { 
    Serial.print("messageIndex: ");
    Serial.println(messageIndex);

    //Retrieve message information
    sim808.readSMS(messageIndex, message, MESSAGE_LENGTH, phone, datetime);     

    //Delete the message from memory (SIM card can only hold about 20 SMS)
    sim808.deleteSMS(messageIndex);

    //Present SMS information to serial window
    Serial.print("From number: ");
    Serial.println(phone);  
    Serial.print("Datetime: ");
    Serial.println(datetime);        
    Serial.print("Recieved Message: ");
    Serial.println(strupr(message));    
    Serial.println();
  }

    //Parse for commands
    if (!strcmp(strupr(message),"LOCATION")){
      message[0] = '\0';  //message variable will be set to null terminator to ensure command isnt looped
      Serial.println(message);
      sendLOCATION();
    }
    else if (!strcmp(strupr(message),"HELP")){
      message[0] = '\0';
      Serial.println(message);
      sendHELP();
    }  
    else if (!strcmp(strupr(message),"GEOFENCE")){
      message[0] = '\0';
      Serial.println(message);  
      sendGEOFENCE();
  }
}

//Send list of available commands
void sendHELP(){
  String outgoing;
  
  Serial.println("Help requested...");
  
  outgoing += F("Commands:\n");
  outgoing += F("HELP -- This menu\n");
  outgoing += F("LOCATION -- Request device location\n");
  outgoing += F("GEOFENCE -- Create geofence area\n");
  Serial.println(outgoing);
  Serial.println();

  //convert string variable to char array for use with library
  char outgoing2[160];
  strcpy(outgoing2, outgoing.c_str());

  //library function to send SMS
  sim808.sendSMS(phone,outgoing2);
}

//"$GPGGA,171501.000,4931.1455,N,00742.7887,E,1,9,0.90,260.2,M,47.";
bool getLOCATION(){

   //Check if location is available, if it is save info to variables and display on serial window
   if (sim808.getGPS()) {
    Serial.print("Lock time :");
    Serial.print(sim808.GPSdata.hour);
    Serial.print(":");
    Serial.print(sim808.GPSdata.minute);
    Serial.print(":");
    Serial.println(sim808.GPSdata.second);
    Serial.print("latitude :");
    Serial.println(sim808.GPSdata.lat,10);
    Serial.print("longitude :");
    Serial.println(sim808.GPSdata.lon,10);
    Serial.println();
    return true;
  } else {
    //Serial.println("Waiting for location...");
    return false;
  }
}

//Sends a constructed google maps URL via SMS
void sendLOCATION(){

  Serial.println("Location requested...");
  //Update location before sending message
  while (!getLOCATION()){}

  String URL;

  URL += F("http://maps.google.com/maps?q=");
  URL += String(sim808.GPSdata.lat,8);
  URL += F(",");
  URL += String(sim808.GPSdata.lon,8);
  Serial.println(URL);
  Serial.println();
  
  char URL2[160];
  strcpy(URL2, URL.c_str());

  sim808.sendSMS(phone,URL2);
}

//Toggle GEOFENCE on or off and send status via SMS
void sendGEOFENCE(){
  //Toggle GEOFENCE
  GEOFENCE = !GEOFENCE;

  //When GEOFENCE is turned on
  if (GEOFENCE){
    while (!getLOCATION()){}
    Serial.println("Geofence is on");
    Serial.println();
    sim808.sendSMS(phone,"Geofence is on");
    //Store current lat and long to be check against later
    geoLat = sim808.GPSdata.lat;
    geoLon = sim808.GPSdata.lon;
  }
  //When GEOFENCE is turned off
  else{
    Serial.println("Geofence is off");
    Serial.println();
    sim808.sendSMS(phone,"Geofence is off");
  }
}
