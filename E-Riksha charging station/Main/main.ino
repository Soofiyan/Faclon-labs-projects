#include "Keypad.h"
#include <SPI.h>
#include "mcp_can.h"

//keypad
const byte Rows= 5;
const byte Cols= 4;
char keymap[Rows][Cols]=
{
{'F1', 'F2', 'left', 'right'},
{'1', '2', '3', 'clear'},
{'4', '5', '6', 'menu'},
{'7', '8', '9', 'enter'},
{'*', '0', '#', 'stop'}
};
byte rPins[Rows]= {A4,A5,6,7,8};
byte cPins[Cols]= {A0,A1,A2,A3};

//can
long unsigned int rxId;
unsigned long rcvTime;
unsigned char len = 0;
unsigned char buf[8];
const int SPI_CS_PIN = 9;

//spi between battery and controller
uint8_t slave_sel = 10;
uint8_t MISO_pin = 11;
uint8_t clk_pin = 13;
byte data = 0; 
volatile byte indx;
volatile boolean process;

Keypad kpd= Keypad(makeKeymap(keymap), rPins, cPins, Rows, Cols);
MCP_CAN CAN(SPI_CS_PIN);
SoftwareSerial mySerial(0, 1);

void setup()
{
     SPI.begin();
     SPI.setBitOrder(LSBFIRST);
     pinMode(MISO_pin, OUTPUT);
     indx = 0; // buffer empty
     SPCR |= _BV(SPE);
     process = false;
     SPI.attachInterrupt(); // turn on interrupt
     Serial.begin(115200);
     mySerial.begin(9600);
     while (CAN_OK != CAN.begin(CAN_250KBPS))              // init can bus : baudrate = 500k
     {
        Serial.println("CAN BUS Module Failed to Initialized");
        Serial.println("Retrying....");
        delay(200);
     }
}

ISR (SPI_STC_vect) // SPI interrupt routine 
{ 
   byte c = SPDR; // read byte from SPI Data Register
   if (indx < sizeof buff) {
      buff [indx++] = c; // save data in the next index in the array buff
      if (c == '\r') //check for the end of the word
      process = true;
   }
}

void loop()
{
     //keypad block
     char keypressed = kpd.getKey();
     if (keypressed != NO_KEY)    //NO_KEY is '\0'
     { 
          Serial.println(keypressed);
     }
     
     //can block
     if(CAN_MSGAVAIL == CAN.checkReceive()) 
     {
        //rcvTime = millis();
        CAN.readMsgBuf(&len, buf);    //recieving 8 bytes of data from the charger.
        rxId= CAN.getCanId();
        //checking here if the correct rxID has been received
        //Serial.print(rcvTime);
        Serial.print(rxId);

        for(int i = 0; i<len; i++)
        {
          Serial.print(buf[i]);        
        }
      }
      
      //spi block
      if (process) 
      {
          process = false; //reset the process
          Serial.println (buff); //print the array on serial monitor
          indx= 0; //reset button to zero
      }
}
