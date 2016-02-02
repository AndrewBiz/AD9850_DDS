/* AD9850 DDS based wave generator
   inspired by project of Richard Visokey AD7C - www.ad7c.com
   Andrew Bizyaev (ANB) github.com/andrewbiz
*/

#include <LiquidCrystal.h>
// LCD keypad ARDUINO pins mapping:
#define D4     4 //LCD data
#define D5     5 //LCD data
#define D6     6 //LCD data
#define D7     7 //LCD data
#define RS     8 //LCD RS
#define ENABLE 9 //LCD ENABLE
#define D10   10 // LCD Backlight control

LiquidCrystal lcd(RS, ENABLE, D4, D5, D6, D7);

// LCD keypad button pins mapping
#define btnNONE  0 // originally btnNONE
#define btnMEMO1 1 // originally btnSELECT
#define btnMEMO2 2 // originally btnLEFT
#define btnUP    3 // originally btnUP
#define btnDOWN  4 // originally btnDOWN
#define btnONOFF 5 // originally btnRIGHT
#define btnERROR 99 //

// AD9850 module pins
#define DATA  A1 // connect to serial data load pin (DATA)
#define W_CLK A2 // connect to word load clock pin (CLK)
#define FQ_UD A3 // connect to freq update pin (FQ)
#define RESET A4 // connect to reset pin (RST)

#define pulseHigh(pin) {digitalWrite(pin, HIGH); digitalWrite(pin, LOW); }

double frequency; //frequency of VFO
int backlight_state;

void setup() {
  // Serial
  Serial.begin(9600);
  delay(1000);

  // setup AD9850
  pinMode(DATA,  OUTPUT);
  pinMode(W_CLK, OUTPUT);
  pinMode(FQ_UD, OUTPUT);
  pinMode(RESET, OUTPUT);
  pulseHigh(RESET);
  pulseHigh(W_CLK);
  pulseHigh(FQ_UD);  // this pulse enables serial mode on the AD9850 - Datasheet page 12.
  // init the frequency
  frequency = 20000000.0;
  Serial.println(frequency);
  set_frequency(frequency);

  // LCD setup
  lcd.begin(16, 2);
  pinMode(D10, OUTPUT);   // backlight pin
  backlight_state = HIGH; // backlight is ON when reset
  digitalWrite(D10, backlight_state); 
  LCD_show_frequency();
}

void loop() {
  //set_frequency(frequency);
  //Serial.println(frequency);
  delay(100);
  LCD_show_frequency();
  
  lcd.setCursor(0,1);  // move to the begining of the second line
  switch(read_LCD_buttons()){
    case btnMEMO1:{
      lcd.print("MEMO1 SHORT");
      break;
    }
    case btnMEMO2:{
      lcd.print("MEMO2 SHORT");
      break;
    }
    case btnUP:{
      lcd.print("delta +");
      break;
    }
    case btnDOWN:{
      lcd.print("delta -");
      break;
    }
    case btnONOFF:{
      backlight_state = (backlight_state == LOW) ? HIGH : LOW;
      digitalWrite(D10, backlight_state); 
      lcd.print("BACKLIGHT=");
      lcd.print(backlight_state);
      break;
    }
    case btnERROR:{
      lcd.print("BTN ERROR!!!");
      break;
    }
  }
}

// set frequency in AD9850
void set_frequency(double frequency) {
  // datasheet page 8: frequency = <sys clock> * <frequency tuning word>/2^32
  // AD9850 allows an output frequency resolution of 0.0291 Hz with a 125 MHz reference clock applied
  //double freq_tuning_word_d = frequency * 4294967295.0/125000000.0;  // note 125 MHz clock on 9850. You can make 'slight' tuning variations here by adjusting the clock frequency.

  int32_t freq_tuning_word = frequency * 4294967295.0/125000000.0;  // note 125 MHz clock on 9850. You can make 'slight' tuning variations here by adjusting the clock frequency.
  //Serial.println(freq_tuning_word_d);
  Serial.println(freq_tuning_word);

  for (int b=0; b<4; b++, freq_tuning_word>>=8) {
    transfer_byte(freq_tuning_word & 0xFF);
  }
  transfer_byte(0x00); // Final control byte, all 0 for 9850 chip
  pulseHigh(FQ_UD);  // Done!  Should see output
}

// transfers a byte, a bit at a time, LSB first to the 9850 via serial DATA line
void transfer_byte(byte data) {
  for (int i=0; i<8; i++, data>>=1) {
    digitalWrite(DATA, data & 0x01);
    delay(20);
    pulseHigh(W_CLK); //after each bit sent, CLK is pulsed high
  }
}

// Show frequency
void LCD_show_frequency(){
  lcd.setCursor(0,0);
  lcd.print(frequency);
  lcd.print("Hz");
}

byte read_LCD_buttons(){      // read the buttons
  int key = analogRead(A0);   // read the value from the sensor 
  // buttons when read are centered at these valies: 0, 144, 329, 504, 741
  if (key > 1000) return btnNONE;  
  if (key < 50)   return btnONOFF;  
  if (key < 250)  return btnUP; 
  if (key < 450)  return btnDOWN; 
  if (key < 650)  return btnMEMO2; 
  if (key < 850)  return btnMEMO1;  
  return btnERROR;             // when all others fail, return this.
}

