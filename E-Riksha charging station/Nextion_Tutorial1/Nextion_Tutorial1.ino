/* 
The following sketch is an example of how to send data to the Nextion display without any library.

The data can be text, numbers, or any other atribute of an object.

You can also send System Variables to change settings on the display like: brightness, serial baud rate, etc.

Connection with Arduino Uno/Nano:
* +5V = 5V
* TX  = none
* RX  = pin 1 (TX)
* GND = GND


This sketch was made for my 1st video tutorial shown here: https://www.youtube.com/watch?v=wIWxSLVAAQE

Made by InterlinkKnight
Last update: 02/04/2018
*/





int variable1 = 0;  // This is a simple variable to keep increasing a number to have something dynamic to show on the display.





void setup() {  // Put your setup code here, to run once:
  
  Serial.begin(9600);  // Start serial comunication at baud=9600. For Arduino mega you would have to add a
                       // number (example: "Serial1.begin(9600);").
                       // If you use an Arduino mega, you have to also edit everything on this sketch that
                       // says "Serial" and replace it with "Serial1" (or whatever number you are using).




  /*
  // I am going to change the Serial baud to a faster rate (optional):
  delay(500);  // This dalay is just in case the nextion display didn't start yet, to be sure it will receive the following command.
  Serial.print("baud=38400");  // Set new baud rate of nextion to 38400, but is temporal. Next time nextion is
                               // power on, it will retore to default baud of 9600.
                               // To take effect, make sure to reboot the arduino (reseting arduino is not enough).
                               // If you want to change the default baud, send the command as "bauds=38400", instead of "baud=38400".
                               // If you change default, everytime the nextion is power on is going to have that baud rate.
  Serial.write(0xff);  // We always have to send this three lines after each command sent to nextion.
  Serial.write(0xff);
  Serial.write(0xff);

  Serial.end();  // End the serial comunication of baud=9600.

  Serial.begin(38400);  // Start serial comunication at baud=38400.
  */

  



}  // End of setup





void loop() {  // Put your main code here, to run repeatedly:

  variable1++;  // Increase the value of the variable by 1.
  if(variable1 == 201){  // If the variable reach 201...
    variable1 = 0;  // Set the variable to 0 so it starts over again.
  }





  // We are going to send the variable value to the object called n0:
  // After the name of the object you need to put the dot val because val is the atribute we want to change on that object.
  Serial.print("n0.val=");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
  Serial.print(variable1);  // This is the value you want to send to that object and atribute mention before.
  Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
  Serial.write(0xff);
  Serial.write(0xff);





  // We are going to update the progress bar to show the value of the variable.
  // The progress bar range goes from 0 to 100, so when the variable is greater than 100, the progress bar will keep showing full (100%).
  Serial.print("j0.val=");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
  Serial.print(variable1);  // This is the value you want to send to that object and atribute mention before.
  Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
  Serial.write(0xff);
  Serial.write(0xff);





  // We are going to update the gauge to show the value of the variable.
  // But instead of updating the gauge directly, we are going to send the value to a variable called va0 on the nextion display.
  // On the nextion display we have a code on the timer (tm0) that compares the variable value and the gauge value, and updates the gauge only when they are different.
  // The gauge range goes from 0 (pointing to the left) to 359.
  Serial.print("va0.val=");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
  Serial.print(variable1);  // This is the value you want to send to that object and atribute mention before.
  Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
  Serial.write(0xff);
  Serial.write(0xff);





  // We are going to update the text to show "Hot!" when the variable is greater than 99, and show "Normal" when variable is below 100.
    if(variable1 > 99){  // If the variable is greater than 99...
    Serial.print("t0.txt=");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.print("\"");  // Since we are sending text we need to send double quotes before and after the actual text.
    Serial.print("Hot!");  // This is the text we want to send to that object and atribute mention before.
    Serial.print("\"");  // Since we are sending text we need to send double quotes before and after the actual text.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);
  }else{  // Since condition was false, do the following:
    Serial.print("t0.txt=");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.print("\"");  // Since we are sending text we need to send double quotes before and after the actual text.
    Serial.print("Normal");  // This is the text we want to send to that object and atribute mention before.
    Serial.print("\"");  // Since we are sending text we need to send double quotes before and after the actual text.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);
  }





  // We are going to hide the progress bar if the variable is greater than 119:
  if(variable1 > 119){  // If the variable is greater than 119...
    Serial.print("vis j0,0");  // This is to hide or show an object. The name of the object comes before the comma, and after the comma goes a 0 to hide or 1 to show.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);
  }else{  // Since condition was false, do the following:
    Serial.print("vis j0,1");  // This is to hide or show an object. The name of the object comes before the comma, and after the comma goes a 0 to hide or 1 to show.    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);
  }





  // We are going to change the color of the progress bar to red if the variable is greater than 49, and change to green if is below 50:
  if(variable1 > 49){  // If the variable is greater than 49...
    Serial.print("j0.pco=63488");  // Change color of the progress bar to red.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);
  }else{  // Since condition was false, do the following:
    Serial.print("j0.pco=1024");  // Change color of the progress bar to green.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);
  }





}   // End of loop


