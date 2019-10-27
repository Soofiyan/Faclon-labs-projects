/*****************Unitech Pump Controlling ang enery monitoring******************
 * This program takes data from energy meter of Unitech and flow meter. 
 * It control pumps which is 50hp.
 * Tag for pump is "MT" it controlled by main, star and delta contactors which connected on  pins 7, 11 and 12.
 * 
 */
#include <Modbus.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
SoftwareSerial Serial1(8, 9); // RX, TX  to communicate with MC60
#define READ_WRITE_EN_GPIO_PIN       2/*this GPIO pin of arduino board which is connected to the Read write enable of the TTL-RS485 converter*/
Modbus Mb;
char rgucTemp[100]="\0";
char MbData[100]="\0";
void(*resetFunc) (void) = 0; //declare reset function @ address 0
/********************************************************************************************************************************/
//#define serialInterrupt 6
//#define serialReady 5
bool MC60ready = 0;
unsigned long MC60millis = 0;
/***************************************************************************************************************************/
char dname[]="{\"device\":\"AUTOTEST\",\"data\":[{ \"tag\":\"";                                     
//char dnameF[]="{\"device\":\"NMPG_C1\",\"fault\":true,\"data\":[{ \"tag\":\"";
unsigned long currentMillis = 0;
const byte serialInterruptPin = 6;
const byte serialReadyPin = 5;
//---------------------------------------------------------------------------------------------------------------------------
/*Modbus Master command to read different data from the modbus device*/
unsigned long j1millis =0;
unsigned long j2millis =0;
unsigned char SelecMeter[5][8]={/* Dev_ID, FC,  Add_H Add_L Num_H Num_L CRC_L  CRC_H*/
                                     {0x01, 0x03, 0x00, 0x69, 0x00, 0x02 },//40106 Avg Volts -01 03 00 69 00 02 14 17 
                                     {0x01, 0x03, 0x00, 0x77, 0x00, 0x02 },//40120 Avg Current- 01 03 00 77 00 02 74 11 
                                     {0x01, 0x03, 0x00, 0x93, 0x00, 0x02 },//40148 Active power -01 03 00 93 00 02 34 26  
                                     {0x01, 0x03, 0x00, 0x8B, 0x00, 0x02 },//40140 Power factor - 01 03 00 8B 00 02 B4 21
                                     {0x01, 0x03, 0x00, 0xDF, 0x00, 0x02 },//40224 Fwd energy - 01 03 00 DF 00 02 F5 F1 
};
//-----------------------------------------------------------------------------------------------------------------------------
                             
String dname1="FCEL_T";
String json;
String json1;
float data[26];
int Mflag = 0;
int MT = 0;
int MTpin = A0;
int Pflag = 0;
float FRT1 = 0;
float FRT2 = 0;
int AMM = 0;
int AMMPin = 3;
#define Motor 11;
void setup()
{
  Serial1.begin(9600);
  //Serial1.begin(9600);
  //Serial2.begin(9600);
  Mb.MBbegin(&Serial, 9600, READ_WRITE_EN_GPIO_PIN); /*intilize Serial2 for Modbus communication */ 
   /*this pin used for the Read(Low)/Write(High) switching of ttl to RS485 converter*/  
   pinMode(serialReadyPin,OUTPUT);
  pinMode(serialInterruptPin, INPUT);
  pinMode(MTpin, OUTPUT);
  digitalWrite(MTpin, LOW);
  
/********************************************************************************************************************************/
 delay(5000);
// pinMode(serialReady,OUTPUT);
// pinMode(serialInterrupt,INPUT);
// 
 delay(30000);
  j1millis = millis();  
  delay(30000);
  String json = "${\"device\":\""+dname1+"\",\"data\":[{\"tag\":\"Status\",\"value\":1}";
  Serial1.print(json);
  delay(10000); 
  
}
void loop()
{
  String response = "";
      
   response = mc60Status();
  
   if ( response.startsWith("#{\"token\":") == 1 )
   {
    String token = "";  
    int i=0;
    while(response[i+10] != ',')
    {
      token += response[i+10];
      i++;
    }
    //Serial1.println(token);
    //Serial1.println(i);
    
    i = i + 2;
    String sensor = "";
    while(response[i+10] != '\"')
    {
      sensor += response[i+10];
      i++;
    }
    
    //Serial1.println(sensor);
    if(sensor.startsWith("MT") == 1)
    {
      if(response[i+10+2] == '1')
      {  
        digitalWrite(MTpin,HIGH);
        MT = 1;
        String json = "#{\"device\":\""+dname1+"\",\"token\":\"" + token + "\",\"data\":[{\"tag\":\"MT\",\"value\":1}";
        Serial1.print(json); 
        delay(4000);
      }
      else if(response[i+10+2] == '0')
      {
        digitalWrite(MTpin,LOW);
        MT = 0;
        String json = "#{\"device\":\""+dname1+"\",\"token\":\"" + token + "\",\"data\":[{\"tag\":\"MT\",\"value\":0}";
        Serial1.print(json);     
        delay(2000);
      }    
    }
    
   }
   else if ( response.startsWith("${\"token\":") == 1 )
   {
    String token = "";  
    int i=0;
    while(response[i+10] != ',')
    {
      token += response[i+10];
      i++;
    }
//    Serial1.println(token);
//    Serial1.println(i);
    
    i = i + 2;
    String sensor = "";
    while(response[i+10] != '\"')
    {
      sensor += response[i+10];
      i++;
    }
    
    //Serial1.println(sensor);
    if(sensor.startsWith("MT") == 1)
    {
      if(response[i+10+2] == '1')
      {
        digitalWrite(MTpin,HIGH);
        MT = 1;
        delay(3000); 
        String json = "${\"device\":\""+dname1+"\",\"token\":" + token + ",\"data\":[{\"tag\":\"MT\",\"value\":1}";
        Serial1.print(json); 
        delay(4000);
      }
      else if(response[i+10+2] == '0')
      {
        digitalWrite(MTpin,LOW);
        MT = 0;
        delay(3000); 
        String json = "${\"device\":\""+dname1+"\",\"token\":" + token + ",\"data\":[{\"tag\":\"MT\",\"value\":0}";
        Serial1.print(json);
        delay(4000);     
      }    
    }
    
   }
   else if ( response.startsWith("{\"MT\":1}") == 1 )
   {
      digitalWrite(MTpin,HIGH);
      MT = 1;
      delay(3000); 
      String json = "{\"device\":\""+dname1+"\",\"data\":[{\"tag\":\"MT\",\"value\":1}";
      Serial1.print(json);
      delay(4000); 
   }
   else if ( response.startsWith("{\"MT\":0}") == 1 )
   {
      digitalWrite(MTpin,LOW);
      MT = 0;
      delay(3000); 
      String json = "{\"device\":\""+dname1+"\",\"data\":[{\"tag\":\"MT\",\"value\":0}";
      Serial1.print(json);
      delay(4000); 
   }
    else if ( response.startsWith("{\"RS\":1}") == 1 )
   {
      resetFunc();  //call reset
   }
   
 //---------------------------------------------------------------------------------------------------------------
          
  if(millis() - j1millis >= 25000)
  {
    
  float Avg_Volts=Avgvolt_Read(0);
  
  float Avg_Current=Avgvolt_Read(1);
  
  float Active_Power=Avgvolt_Read(2);
  
  float Power_Factor=Avgvolt_Read(3);
  
  float Fwd_Energy=Avgvolt_Read(4);
    if (Avg_Volts != -1)
    {
      String json1 = "{\"device\":\""+dname1+"\", \"data\":[{ \"tag\":\"AVT\", \"value\": "+String(Avg_Volts,4)+"},{ \"tag\":\"ACR\", \"value\":"+String(Avg_Current,4)+"},{ \"tag\":\"MT\", \"value\":"+String(MT)+"}";
      Serial1.print(json1);
      json1 = ",{ \"tag\":\"APR\", \"value\":"+String(Active_Power,4)+"},{ \"tag\":\"PWF\", \"value\":"+String(Power_Factor,6)+"},{ \"tag\":\"FWE\", \"value\":"+String(Fwd_Energy,2)+"}";
      Serial1.print(json1);
    }
    else
    {
      String json1 = "{\"device\":\""+dname1+"\", \"data\":[{ \"tag\":\"Status\", \"value\": "+String(0)+"},{ \"tag\":\"MT\", \"value\":"+String(MT)+"}";
      Serial1.print(json1);
    }
    
    j1millis = millis();
  }
 
}
//--------------------------------------------------------------------------------------------------------------------------------------------
float Avgvolt_Read(int i){  
  unsigned char ucsize = 0;
  memset(rgucTemp,0,50);
  float fReadData = -1;
  ucsize = Mb.Mbwrite((char *)SelecMeter[i], 6);
      delay(500);
  //Serial1.print("Modbus");
  if(1 == Mb.bMbFrameReady)
  {
    char rgucTemp1[20];
    ucsize = Mb.Mbread(rgucTemp);
    
    *((char *)&fReadData + 3) = rgucTemp[5];
    *((char *)&fReadData + 2) = rgucTemp[6];
    *((char *)&fReadData + 1) = rgucTemp[3];
    *((char *)&fReadData + 0) = rgucTemp[4];
    //Serial1.println("connected");
  }
  return fReadData;  //Returns Flow for this year
}
/********************************************************************************************************************************/
String mc60Status()
{ 
  String response = "";
  char incomingByte;
  for(int i=0;i<600;i++)
  {
    char a = Serial1.read();
  }
  if ( digitalRead(serialInterruptPin) == 0 )
  {
    //Serial.println("loop");
    digitalWrite(serialReadyPin,HIGH);
    delay(800);
    while (Serial1.available() > 0) {
                  // read the incoming byte:
                  incomingByte = Serial1.read();
                  response += incomingByte;
                  // say what you got:
                  //Serial.print("I received: ");
                  //Serial.print(incomingByte);
          }
  
    digitalWrite(serialReadyPin,LOW);      
   // Serial1.println(response);
  }
  
  
  return response;
}
/********************************************************************************************************************************/
