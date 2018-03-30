#include <SoftwareSerial.h>
#define pwmPin 10
#define pin_SW_SDA A4
#define pin_SW_SCL A5
#include <Wire.h>
#include <iarduino_OLED_txt.h>
iarduino_OLED_txt myOLED(0x3C);
extern uint8_t SmallFont[];
extern uint8_t MediumFont[];
 
SoftwareSerial swSerial(A0, A1); // RX, TX

void setup() {
  Serial.begin(9600);
  swSerial.begin(9600);
  pinMode(pwmPin, INPUT);
  
  myOLED.begin();
  myOLED.setFont(SmallFont);
  myOLED.setCoding(TXT_UTF8);

  /*
  Specs https://revspace.nl/MHZ19
  5000 ppm range: 0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x13, 0x88, 0xCB
  */

  // Write command in the 6th and 7th byte
  //           bytes:                         3     4           6     7
  byte setrangeA_cmd[9] = {0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x13, 0x88, 0xCB}; // set the range 0 - 5000ppm
  unsigned char setrangeA_response[9]; 
  swSerial.write(setrangeA_cmd,9);
  swSerial.readBytes(setrangeA_response, 9);
  int setrangeA_i;
  byte setrangeA_crc = 0;
  for (setrangeA_i = 1; setrangeA_i < 8; setrangeA_i++) setrangeA_crc+=setrangeA_response[setrangeA_i];
  setrangeA_crc = 255 - setrangeA_crc;
  setrangeA_crc += 1;
  if ( !(setrangeA_response[0] == 0xFF && setrangeA_response[1] == 0x99 && setrangeA_response[8] == setrangeA_crc) ) {

    Serial.println("Range CRC error: " + String(setrangeA_crc) + " / "+ String(setrangeA_response[8]) + " (bytes 6 and 7)");

    myOLED.clrScr();
    myOLED.print( "Range CRC error: ", 0, 0);
    myOLED.print( String(setrangeA_crc), 0, 1);
    myOLED.print( " / ", OLED_C, 1);
    myOLED.print( String(setrangeA_response[8]), OLED_R, 1);
    myOLED.print( "(bytes 6 and 7)", 0, 2);

  } else {
   
    Serial.println("350 - 450 ppm: Normal level in the open air.");
    Serial.println("< 600 ppm: Acceptable level. Recommended for bedrooms, kindergartens and schools.");
    Serial.println("600 - 1000 ppm: Complaints about stale air, possible decrease in concentration of attention.");
    Serial.println("1000 ppm: Maximum level of standards of ASHRAE (American Society of Heating, Refrigerating and Air-Conditioning Engineers) and OSHA (Occupational Safety & Health Administration).");
    Serial.println("1000 - 2500 ppm: General lethargy, reduced concentration of attention, headache is possible");
    Serial.println("2500 - 5000 ppm: Undesirable impact on health is possible.");

    myOLED.clrScr();
    myOLED.print( "---------------------", 0, 0);
    myOLED.print( "350-450ppm: Open air", 0, 1);
    myOLED.print( "<600: Ok inside", 0, 2);
    myOLED.print( "600-1k: Concentr.loss", 0, 3);
    myOLED.print( "1k: Max allowed OSHA", 0, 4);
    myOLED.print( "1k-2.5k: Headache", 0, 5);
    myOLED.print( "2.5-5k: Unhealthy", 0, 6);
    myOLED.print( "---------------------", 0, 7);
    
  }
  Serial.println("------------");
  delay(3000);
}

void loop() {

  byte measure_cmd[9] = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79};
  unsigned char measure_response[9]; 
  unsigned long th, tl, ppm5000 = 0;

  // get CO2 concentration via UART 
  swSerial.write(measure_cmd,9);
  swSerial.readBytes(measure_response, 9);
  int i;
  byte crc = 0;
  for (i = 1; i < 8; i++) crc+=measure_response[i];
  crc = 255 - crc;
  crc += 1;
  if ( !(measure_response[0] == 0xFF && measure_response[1] == 0x86 && measure_response[8] == crc) ) {
    
    Serial.println("CRC error: " + String(crc) + " / "+ String(measure_response[8]));
    
    myOLED.clrScr();
    myOLED.print( "CRC error: ", 0, 0);
    myOLED.print( String(crc), 0, 1);
    myOLED.print( String(measure_response[8]), 0, 2);
  } 
  unsigned int responseHigh = (unsigned int) measure_response[2];
  unsigned int responseLow = (unsigned int) measure_response[3];
  unsigned int ppm = (256*responseHigh) + responseLow;

  // get CO2 concentration via PWM 
  do {
    th = pulseIn(pwmPin, HIGH, 1004000) / 1000;
    tl = 1004 - th;
    ppm5000 =  5000 * (th-2)/(th+tl-4); // calculation for the range 0 - 5000ppm 
  } while (th == 0);

  Serial.print(ppm);
  Serial.println(" ppm (UART)");
  Serial.print(ppm5000);
  Serial.println(" ppm (PWM)");
  Serial.println("------------");
  
  myOLED.clrScr();
  myOLED.setFont(MediumFont);
  myOLED.print( ppm, 0, 2);
  myOLED.print( ppm5000, 0, 4);
  myOLED.setFont(SmallFont);
  myOLED.print( "ppm (UART)", OLED_R, 2);
  myOLED.print( "ppm (PWM) ", OLED_R, 4);
  
  delay(6000);
}
