#include "Keypad.h"
char keymap[Rows][Cols]=
{
{'1', '2', '3', 'clear'},
{'4', '5', '6', 'menu'},
{'7', '8', '9', 'enter'},
{'*', '0', '#', 'stop'}
};

byte rPins[4]= {4,5,6,7};
byte cPins[4]= {0,1,2,3};

Keypad kpd= Keypad(makeKeymap(keymap), rPins, cPins, 4, 4);

void setup()
{
     Serial.begin(9600);
}

void loop()
{
     char keypressed = kpd.getKey();
}
