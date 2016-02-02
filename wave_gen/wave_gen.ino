/* AD9850 DDS based wave generator
   inspired by project of Richard Visokey AD7C - www.ad7c.com
   Andrew Bizyaev (ANB) github.com/andrewbiz
*/

#include <LiquidCrystal.h>
#include <EEPROM.h>

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
#define btnDELTA 5 // originally btnRIGHT
#define btnERROR 99 //

// AD9850 module pins
#define DATA  A1 // connect to serial data load pin (DATA)
#define W_CLK A2 // connect to word load clock pin (CLK)
#define FQ_UD A3 // connect to freq update pin (FQ)
#define RESET A4 // connect to reset pin (RST)

#define pulseHigh(pin) {digitalWrite(pin, HIGH); digitalWrite(pin, LOW); }

#define MIN_FREQUENCY 0.01
#define MAX_FREQUENCY 20000000.0
#define DEF_FREQUENCY 1000.0 //default freq
#define MAX_FREQUENCY_INDEX 8
#define DEF_FREQUENCY_INDEX 5 //default freq index
#define SAVE_TO_M0_INTERVAL 5000 //after 5 sec current frequency will be saved to EEPROM 

struct MemoryRecord{
  float frequency;
  byte frequency_delta_index;
};
float frequency = DEF_FREQUENCY; //frequency of VFO
byte frequency_delta_index = DEF_FREQUENCY_INDEX;
const float frequency_delta[] = {0.01, 0.1, 1, 10, 100, 1000, 10000, 100000, 1000000};
const byte EEPROM_address[] = {0, 5, 10};
unsigned long time_last_fchange = 0;
int backlight_state;
unsigned long last_key_pressed_time = 0;
boolean need_save_to_m0 = false;
int sf_count = 0;

void setup() {
  // Serial
  Serial.begin(9600);

  // LCD setup
  lcd.begin(16, 2);
  pinMode(D10, OUTPUT);   // backlight pin
  backlight_state = HIGH; // backlight is ON when reset
  digitalWrite(D10, backlight_state); 

  init_memory();
  
  //read from default memory slot
  read_from_memory(0);

  // setup AD9850
  pinMode(DATA,  OUTPUT);
  pinMode(W_CLK, OUTPUT);
  pinMode(FQ_UD, OUTPUT);
  pinMode(RESET, OUTPUT);
  pulseHigh(RESET);
  delay(10);
  pulseHigh(FQ_UD);  // this pulse enables serial mode on the AD9850 - Datasheet page 12.
  
  // init the frequency
  set_frequency();

  LCD_show_frequency();
  LCD_show_frequency_delta(" ");
}

void loop() {
  delay(100);
  //test
  ++sf_count;
  Serial.print(millis()); Serial.print(" : "); Serial.println(sf_count);
  if(sf_count >= 10) {
    set_frequency();
    sf_count = 0;
    Serial.println(millis());
  }  
  if(((millis() - last_key_pressed_time) > SAVE_TO_M0_INTERVAL) and need_save_to_m0){
    save_to_memory(0);
    need_save_to_m0 = false;
  }  
  switch(read_LCD_buttons()){
    case btnMEMO1:{
      Serial.println("Key btnMEMO1 pressed");
      read_from_memory(1);
      set_frequency();
      need_save_to_m0 = true;
      LCD_show_frequency();
      LCD_show_frequency_delta(" ");
      break;
    }
    case btnMEMO2:{
      Serial.println("Key btnMEMO2 pressed");
      read_from_memory(2);
      set_frequency();
      need_save_to_m0 = true;
      LCD_show_frequency();
      LCD_show_frequency_delta(" ");
      break;
    }
    case btnUP:{
      Serial.println("Key btnUP pressed");      
      LCD_show_frequency_delta("+");
      frequency_inc();
      LCD_show_frequency();
      break;
    }
    case btnDOWN:{
      Serial.println("Key btnDOWN pressed");
      LCD_show_frequency_delta("-");
      frequency_dec();
      LCD_show_frequency();
      break;
    }
    case btnDELTA:{
      Serial.println("Key btnDELTA pressed");
      frequency_delta_index++;      
      if (frequency_delta_index > MAX_FREQUENCY_INDEX) {
        frequency_delta_index = 0;
      } 
      LCD_show_frequency_delta(" "); 
      break;
    }
    case btnERROR:{
      Serial.println("Key error");
      lcd.print("BTN ERROR!!!");
      break;
    }
  }
}

// set frequency in AD9850
void set_frequency() {
  // datasheet page 8: frequency = <sys clock> * <frequency tuning word>/2^32
  // AD9850 allows an output frequency resolution of 0.0291 Hz with a 125 MHz reference clock applied
  //double freq_tuning_word_d = frequency * 4294967295.0/125000000.0;  // note 125 MHz clock on 9850. You can make 'slight' tuning variations here by adjusting the clock frequency.
  int32_t freq_tuning_word = frequency * 4294967295.0/125000000.0;  // note 125 MHz clock on 9850. You can make 'slight' tuning variations here by adjusting the clock frequency.
  Serial.print("Setting frequency to AD9850: "); Serial.println(frequency);
  Serial.print(freq_tuning_word); Serial.print(" = "); Serial.println(freq_tuning_word, BIN);

  for (int b=0; b<4; b++, freq_tuning_word>>=8) {
    transfer_byte(freq_tuning_word & 0xFF);
    //Serial.print(" ");
  }
  transfer_byte(0x00); // Final control byte, all 0 for 9850 chip
  
  pulseHigh(FQ_UD);  // Done!  Should see output
  time_last_fchange = millis();
  Serial.println();
}

// transfers a byte, a bit at a time, LSB first to the 9850 via serial DATA line
void transfer_byte(byte data) {
  for (int i=0; i<8; i++, data>>=1) {
    digitalWrite(DATA, data & 0x01);
    //Serial.print(data & 0x01);
    //delay(5);
    pulseHigh(W_CLK); //after each bit sent, CLK is pulsed high
  }
}

// Show frequency
void LCD_show_frequency(){
  lcd.setCursor(0,0);
  lcd.print("                ");
  lcd.setCursor(0,0);
  lcd.print(frequency);
  lcd.print(" Hz");
}

void LCD_show_frequency_delta(String prefix){
  String lcd_info = prefix + String(frequency_delta[frequency_delta_index]) + " Hz";
  LCD_show_line2(lcd_info);
}

// Show other info
void LCD_show_line2(String info){
  lcd.setCursor(0,1);  // move to the begining of the second line
  lcd.print("                ");
  lcd.setCursor(0,1);  // move to the begining of the second line
  lcd.print(info);
}

byte read_LCD_buttons(){      // read the buttons
  int key = analogRead(A0);   // read the value from the sensor 
  // buttons when read are centered at these valies: 0, 144, 329, 504, 741
  if (key < 850)  last_key_pressed_time = millis(); //any button pressed
  if (key > 1000) return btnNONE;  
  if (key < 50)   return btnDELTA;  
  if (key < 250)  return btnUP; 
  if (key < 450)  return btnDOWN; 
  if (key < 650)  return btnMEMO2; 
  if (key < 850)  return btnMEMO1;  
  return btnERROR;             // when all others fail, return this.
}

//init memory
void init_memory() {
  for (byte i=0; i<3; i++) {
    Serial.print("Initializing M"); Serial.println(i);
    MemoryRecord m = { -1.0f, 99};
    EEPROM.get(EEPROM_address[i], m);
    float f = float(m.frequency);
    if(isnan(f)) f = 0.0;
    if(isinf(f)) f = 0.0;
    byte fdi = byte(m.frequency_delta_index);
    if(isnan(fdi)) fdi = 0;
    if(isinf(fdi)) fdi = 0;    
    Serial.print("Read stored values: "); Serial.print(f);
    Serial.print(", delta = "); Serial.println(frequency_delta[fdi]);
    if((f <= MIN_FREQUENCY) or (f >= MAX_FREQUENCY)){
      m.frequency = DEF_FREQUENCY;
      Serial.print("Set new frequency: "); Serial.println(m.frequency); 
    }
    else {
      m.frequency = f;
    }
    if((fdi < 0) or (fdi > MAX_FREQUENCY_INDEX)){
      m.frequency_delta_index = DEF_FREQUENCY_INDEX;
      Serial.print("Set new delta: "); Serial.println(frequency_delta[m.frequency_delta_index]); 
    }
    else {
      m.frequency_delta_index = fdi;
    }  
    EEPROM.put(EEPROM_address[i], m);
  }
}

// working with memory
void read_from_memory(byte memory_slot){
  MemoryRecord m;
  EEPROM.get(EEPROM_address[memory_slot], m);
  frequency = float(m.frequency);
  frequency_delta_index = byte(m.frequency_delta_index);
  Serial.print("Read from EEPROM"); ; Serial.print(memory_slot); Serial.print(": ");
  Serial.print(frequency); Serial.print(", delta ");
  Serial.println(frequency_delta[frequency_delta_index]);
  lcd.setCursor(14,0);
  lcd.print("M"); lcd.print(memory_slot);
  delay(800);
  lcd.setCursor(14,0);
  lcd.print("  ");
 }

void save_to_memory(byte memory_slot){
  MemoryRecord m = {
    frequency,
    frequency_delta_index
  };
  EEPROM.put(EEPROM_address[memory_slot], m);
  Serial.print("Saved to EEPROM"); Serial.print(memory_slot); Serial.print(": ");
  Serial.print(frequency); Serial.print(", delta ");
  Serial.println(frequency_delta[frequency_delta_index]);
  for (byte i=0; i<3; i++) { 
    lcd.setCursor(14,0);
    lcd.print("M"); lcd.print(memory_slot);
    delay(400);
    lcd.setCursor(14,0);
    lcd.print("  ");
    delay(400);
  }  
}  

void frequency_inc() {
  if((frequency + frequency_delta[frequency_delta_index]) <= MAX_FREQUENCY) {  
    frequency = frequency + frequency_delta[frequency_delta_index];
    set_frequency();
    need_save_to_m0 = true;
  }  
}

void frequency_dec() {
  if((frequency - frequency_delta[frequency_delta_index]) >= MIN_FREQUENCY) {  
    frequency = frequency - frequency_delta[frequency_delta_index];
    set_frequency();
    need_save_to_m0 = true;
  }  
}

