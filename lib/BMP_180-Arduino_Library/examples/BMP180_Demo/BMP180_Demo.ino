///////////////////////////////////////////////////////////////////////////////
/*                                                                           */
/*                                                                           */
/* (c) e-radionica.com 2015 - http://e-radionica.com/hr/faq/#privatnost      */
/* techsupport@e-radionica.com                                               */
/*                                                                           */
/*                                                                           */
/* Senzor tlaka zraka BMP180 - KKM tutorial                                 */
///////////////////////////////////////////////////////////////////////////////
 
#include "SFE_BMP180.h"
#include "Wire.h"
 
SFE_BMP180 tlak;
 
double tlakZraka, tempZraka; 
 
void setup() {
  // zapocinjem serijsku komunikaciju
  Serial.begin(9600);
   
  // provjera je li sve uredu
  if(tlak.begin())  Serial.println("BMP180 uspjesno povezan.");
  else
  {
    Serial.print("Upss.. provjeri kako je modul spojen na Croduino...");
    while(1);
  }
}
 
void loop() {
   
  // ocitavam i printam tlak zraka
  tlakZraka = ocitajTlak();
  Serial.println("Tlak zraka = " + String(tlakZraka) + "hPa");
   
  // ocitavam i zapisujem temp zraka
  tempZraka = ocitajTemperaturu();
  Serial.print("Temp zraka = " + String(tempZraka));
  Serial.println("C");
 
  // ispisuj podatke svakih 1000ms = 1sekundu
  delay(1000);
  Serial.println();
 
}
 
double ocitajTlak()
{
  char status;
 /*
  *  definiramo varijable:
  *  temp - temperatura zraka
  *  Tlak - tlak zraka
  *  tlak0 - tlak na povrsini mora
  *  nadVisina - nadmorska visina  
  */
  double temp, Tlak, tlak0, nadVisina;
   
  // zapocinjemo mjerenje temperature
  // ako je mjerenje uspješno funkcija vraca delay u ms
  // u suprotnom nam vraća 0 (nulu)
   
  status = tlak.startTemperature();
  if(status != 0)
  {
    delay(status);
    // vraća 1 za uspjesno, odnosno 0 za neuspjesno mjerenje
    status = tlak.getTemperature(temp);
    if(status != 0)
    {
      // zapocinjem mjerenje tlaka
      // 3 je oversampling_settings, pogledaj tutorial
      // ako je mjerenje uspješno funkcija vraca delay u ms
      // u suprotnom nam vraća 0 (nulu)
      status = tlak.startPressure(3);
      if(status != 0)
      {
        delay(status);
        // vraća 1 za uspjesno, odnosno 0 za neuspjesno mjerenje
        status = tlak.getPressure(Tlak,temp);
        if(status != 0)
        {
          return(Tlak);
        }
      }
    }
  }
}
 
double ocitajTemperaturu()
{
  char status;
  double temp;
   
  // zapocinjemo mjerenje temperature
  // ako je mjerenje uspješno funkcija vraca delay u ms
  // u suprotnom nam vraća 0 (nulu)
   
  status = tlak.startTemperature();
  if(status != 0)
  {
    delay(status);
    // vraća 1 za uspjesno, odnosno 0 za neuspjesno mjerenje
    status = tlak.getTemperature(temp);
    if(status != 0)
    {
      return(temp);
    }
  }
}
