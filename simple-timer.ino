#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include "math.h"
#include <EEPROM.h>
#include "RTClib.h"

#define no_timers 4
#define relay_no 4
#define MaxMenuPos no_timers*4-1

#define Relay1 A0
#define Relay2 A1
#define Relay3 A2
#define Relay4 A3
const byte RelaysPins[relay_no] = {Relay1, Relay2, Relay3, Relay4};

// RTC setup
RTC_DS1307 RTC;

// keypad setup
const byte rows = 4;
const byte cols = 4;
char keys[rows][cols] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte colpins[rows] = {8, 9, 10, 11};
byte rowpins[cols] = {4, 5, 6, 7};
Keypad keypad = Keypad(makeKeymap(keys), rowpins, colpins, rows , cols);

// LCD setup
LiquidCrystal_I2C lcd(0x3F, 4, 5);

// Pins

// constent

//  Week days in binary starting with sunday
const byte WeekDays[7] = {B00000001, B00000010, B00000100, B00001000, B00010000, B00100000, B01000000};
const char WeekSym[7] = {'S', 'M', 'T', 'W', 'T', 'F', 'S'};

const byte Relays[7] = {B00000001, B00000010, B00000100, B00001000, B00010000, B00100000, B01000000};

const String Dunits[3] = {"sec", "min", "hour"};



// Varibles
boolean RelaysStates[relay_no] = {false, false, false, false};
uint32_t RelaysCloseTime[relay_no];
int lastMin = 0;

// Menu
boolean MenuMode = false;
byte MenuPin = 0;  //interrupt pin
byte MenuPinD = 2; //digital pin
byte MenuPos = 0;  // menu postion
boolean NewMenu = false;

long lastMillis = 0; // used for interrupt




// Timer data
typedef struct Timer {
  byte hour;       // the hour and
  byte minute;     //   the minute the timer will run on
  byte days;       // days the timer will run in
  byte duration;   // duration of on time in dunit
  byte dunit;      // 0 for sec, 1 for min, 2 for hour
  byte relays;     // the relay the will be run by this timer
};

Timer* Timers = (Timer*) malloc(sizeof(Timer) * no_timers);

void initTimers() {

  for (int i = 0; i < no_timers; i++) {
    byte load;
    // hour
    load = 0;
    EEPROM.write(i * 6, load);

    // minute
    load = 0;
    EEPROM.write(i * 6 + 1, load);

    // days
    load = B00000000;
    EEPROM.write(i * 6 + 2, load);

    // duration
    load = 0;
    EEPROM.write(i * 6 + 3, load);

    // dunit
    load = 0;
    EEPROM.write(i * 6 + 4, load);

    // relays
    load = B00000000;
    EEPROM.write(i * 6 + 5, load);

  }

}


void getTimers() {

  //initTimers();   //THIS SHOULD RUN ONCE


  for (int i = 0; i < no_timers; i++) {
    // hour
    (Timers[i]).hour = EEPROM.read(i * 6);

    // minute
    (Timers[i]).minute = EEPROM.read(i * 6 + 1);

    // days
    (Timers[i]).days = EEPROM.read(i * 6 + 2);

    // duration
    (Timers[i]).duration = EEPROM.read(i * 6 + 3);

    // dunit
    (Timers[i]).dunit = EEPROM.read(i * 6 + 4);

    // relays
    (Timers[i]).relays = EEPROM.read(i * 6 + 5);

  }
}

void ShowMenu() {

  // to avoid clearing in every cycle
  if (NewMenu) {
    lcd.clear();
    NewMenu = false;
  }

  
  // getting timer number from the menu pos
  byte Timer_no = (MenuPos / 4);
  byte Display_Number =  (MenuPos / 4) + 1;

  // Timer time screen
  if ((MenuPos % 4) == 0) {
    lcd.setCursor(0, 0);
    lcd.print("Timer");
    lcd.print(Display_Number);
    lcd.print(" : Time");

    lcd.setCursor(0, 1);
    if ((Timers[Timer_no]).hour < 10) {
      lcd.print("0");
    }
    lcd.print((Timers[Timer_no]).hour);

    lcd.print(":");

    if ((Timers[Timer_no]).minute < 10) {
      lcd.print("0");
    }
    lcd.print((Timers[Timer_no]).minute);
  }

  // Timer days screen
  if ((MenuPos % 4) == 1) {
    lcd.setCursor(0, 0);

    lcd.print("Timer");
    lcd.print(Display_Number);
    lcd.print(" : Days");

    lcd.setCursor(0, 1);

    for (int i = 0; i < 7; i++) {
      lcd.print(WeekSym[i]);
      if (WeekDays[i] & (Timers[Timer_no]).days) {
        lcd.print("X");
      }
      else {
        lcd.print(" ");
      }
    }
  }


  // Timer duration screen
  if ((MenuPos % 4) == 2) {

    lcd.setCursor(0, 0);

    lcd.print("Timer");
    lcd.print(Display_Number);
    lcd.print(" : Duration");

    lcd.setCursor(0, 1);

    lcd.print((Timers[Timer_no]).duration);
    lcd.print(" ");
    lcd.print(Dunits[(Timers[Timer_no]).dunit]);
  }


  // Timer relays screen
  if ((MenuPos % 4) == 3) {
    lcd.setCursor(0, 0);
    lcd.print("Timer");
    lcd.print(Display_Number);
    lcd.print(" : Relays");

    lcd.setCursor(0, 1);

    for (int i = 0; i < relay_no; i++) {
      lcd.print(i + 1);
      if (Relays[i] & (Timers[Timer_no]).relays) {
        lcd.print("X");
      } else {
        lcd.print(" ");
      }
    }
  }

}

void ShowDateTime(DateTime now) {

  // Showing Date
  lcd.setCursor(0, 0);
  lcd.print("Date: ");
  lcd.print(now.year());
  lcd.print("/");
  if (now.month() < 10) {
    lcd.print("0");
  }
  lcd.print(now.month());
  lcd.print("/");
  if (now.day() < 10) {
    lcd.print("0");
  }
  lcd.print(now.day());

  // Showing Time
  lcd.setCursor(0, 1);
  lcd.print("Time: ");
  if (now.hour() < 10) {
    lcd.print("0");
  }
  lcd.print(now.hour());
  lcd.print(":");
  if (now.minute() < 10) {
    lcd.print(0);
  }
  lcd.print(now.minute());
  lcd.print(":");
  if (now.second() < 10) {
    lcd.print(0);
  }
  lcd.print(now.second());

}

void Menu() {

  NewMenu = true;

  // adding 150ms delay between interrupts
  if ((millis() - lastMillis) > 150) {


    if (MenuMode == false) {
      MenuMode = true;
    }
    else if (MenuPos == MaxMenuPos) {
      MenuMode = false;
      MenuPos = 0;
    }
    else {
      MenuPos++;
    }
    lastMillis = millis();
  }
}




void should_run(DateTime now) {

  // to check  Timers only once a minute
  if (lastMin != now.minute()) {
    lastMin = now.minute();

    // getting the number of day in the week
    byte today = now.dayOfTheWeek();

    // going through every timer
    for (int i = 0; i < no_timers; i++) {

      // if today is one of the Timer days
      if ((Timers[i]).days & WeekDays[today]) {


        // if the time of the Timer has come
        if (((Timers[i]).hour == now.hour()) && ((Timers[i]).minute == now.minute())) {


          int dur = (Timers[i]).duration;

          // converting duration into seconds
          for (int k = (Timers[i]).dunit; k > 0; k--) {
            dur = dur * 60;
          }

          // see which relay should be turned on
          for (int j = 0; j < relay_no; j++) {
            if ((Relays[j] & (Timers[i]).relays) && !(RelaysStates[j])) {

              RelaysStates[j] = true;

              RelaysCloseTime[j] = (now.unixtime() + dur);

              digitalWrite(RelaysPins[j], HIGH);

            }
          }
        }
      }
    }

  }
}

void should_stop(DateTime now) {

  for (int i = 0; i < relay_no; i++) {
    if (RelaysStates[i]) {
      if (now.unixtime() >= RelaysCloseTime[i]) {
        digitalWrite(RelaysPins[i], LOW);
        RelaysStates[i] = false;
      }
    }
  }
}

void HandleInput(){
  
}


void setup() {

  Wire.begin();
  RTC.begin();
  Wire.begin();
  lcd.init();
  lcd.backlight();

  // interrupt
  pinMode(MenuPinD, INPUT);
  digitalWrite(MenuPinD, HIGH);
  attachInterrupt(MenuPin, Menu, LOW);

  // Relays pins
  for (int i = 0; i < relay_no; i++) {
    pinMode(RelaysPins[i], OUTPUT);
    digitalWrite(RelaysPins[i], LOW);
  }

  // getting timers from EEPROM
  getTimers();


}

void loop() {

  if (MenuMode) {

    ShowMenu();

    HandleInput();


    delay(150);
  }
  else {

    DateTime now = RTC.now();

    ShowDateTime(now);

    // Checking all Timers that should run
    should_run(now);

    // Checking every Relay that should stop
    should_stop(now);




    delay(1000);
  }

}
