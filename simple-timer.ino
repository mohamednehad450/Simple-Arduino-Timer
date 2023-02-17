#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <RTClib.h>

// constant ----------------------------------------------------------------------------------------------------
#define no_timers 4
#define relay_no 4 // max is 8
#define MaxMenuPos no_timers*4-1

// Relays pins (change them to correspond with your setup)
#define Relay1 A0
#define Relay2 A1
#define Relay3 A2
#define Relay4 A3
#define Relay5 6
#define Relay6 7
#define Relay7 8
#define Relay8 9
const byte RelaysPins[8] = {Relay1, Relay2, Relay3, Relay4, Relay5, Relay6, Relay7, Relay8};

const int InterruptDelay = 300;
const int MenuModeDelay = 150;
const int NormalModeDelay = 1000;

// RTC setup
RTC_DS1307 RTC;

// LCD setup
LiquidCrystal_I2C lcd(0x3F, 4, 5);

//  Week days in binary starting with sunday
const byte WeekDays[7] = {B00000001, B00000010, B00000100, B00001000, B00010000, B00100000, B01000000};

// Week days symbols for display
const char WeekSym[7] = {'S', 'M', 'T', 'W', 'T', 'F', 'S'};

// Duration units for display
const String Dunits[3] = {"sec", "min", "hour"};

// Relays in binary
const byte Relays[8] = {B00000001, B00000010, B00000100, B00001000, B00010000, B00100000, B01000000, B10000000};

// SubMenu
const byte SubMenuPin = 1;       // interrupt pin

const byte SubMenuUpPin = 4;     //   SubMenu Buttons Pins
const byte SubMenuDownPin = 5;   //

// Menu
const byte MenuPin = 0;  //interrupt pin


//------------------------------------------------------------------------------------------------------------------

// Varibles---------------------------------------------------------------------------------------------------------

// Relays
uint32_t RelaysCloseTime[relay_no];  // to keep track of ralays close time
boolean RelaysStates[relay_no];      // to keep track of running relays

// Menu
boolean MenuMode = false;

byte MenuPos = 0;  // menu postion
boolean NewMenu = false;  // used for lcd clearing

long lastMillis = 0; // used for interrupt


// SubMenu
byte SubMenuPos = 0;  // SubMenu Postion

// Timers
boolean UpdateTimers = false;      // tells weather or not the timers were modified
boolean ActiveTimers[no_timers];   // keeps track of active timer in a given minute (not to check them again in the same minute)
byte lastMin;   // to keep track of the last minute to reset ActiveTimers


// Timer DataType
typedef struct Timer {
  byte hour;       // the hour and
  byte minute;     //             the minute the timer will run on
  byte days;       // days the timer will run in
  byte duration;   // duration of the timer in dunit
  byte dunit;      // Duration unit (0 for sec, 1 for min, 2 for hour)
  byte relays;     // the relays of the timer
};


// Timers array
Timer* Timers = (Timer*) malloc(sizeof(Timer) * no_timers);

//-----------------------------------------------------------------------------------------------------------


// gets the timers from the EEPROM
void getTimers() {

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

// update the current timers to EEPROM
void UpdateTimersEEPROM() {

  for (int i = 0; i < no_timers; i++) {
    // hour
    EEPROM.write(i * 6, (Timers[i]).hour);

    // minute
    EEPROM.write(i * 6 + 1, (Timers[i]).minute);

    // days
    EEPROM.write(i * 6 + 2, (Timers[i]).days);

    // duration
    EEPROM.write(i * 6 + 3, (Timers[i]).duration);

    // dunit
    EEPROM.write(i * 6 + 4, (Timers[i]).dunit);

    // relays
    EEPROM.write(i * 6 + 5, (Timers[i]).relays);

  }
}

// interrupt handler
void Menu() {

  // adding delay between interrupts
  if ((millis() - lastMillis) > InterruptDelay) {

    NewMenu = true;

    SubMenuPos = 0;

    
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

void SubMenu() {

  // adding delay between interrupts
  if ((millis() - lastMillis) > InterruptDelay) {
    SubMenuPos++;
    lastMillis = millis();
  }

}

void ShowMenu() {

  // to clear only once every new menu
  if (NewMenu) {
    lcd.clear();
    NewMenu = false;
  }

  // getting timer number from the menu pos
  byte Timer_no = (MenuPos / 4);
  byte Display_Number =  (MenuPos / 4) + 1;

  lcd.cursor();

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

    if (SubMenuPos > 1) {
      SubMenuPos = 0;
    }
    lcd.setCursor((SubMenuPos * 3 + 1), 1);
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
    if (SubMenuPos > 6) {
      SubMenuPos = 0;
    }
    lcd.setCursor(SubMenuPos * 2, 1);

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

    if (SubMenuPos > 1) {
      SubMenuPos = 0;
    }
    lcd.setCursor((SubMenuPos * 3), 1);

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
    if (SubMenuPos > relay_no - 1) {
      SubMenuPos = 0;
    }
    lcd.setCursor(SubMenuPos * 2, 1);
  }

}


void HandleInput() {

  //SubMenuUp
  if (digitalRead(SubMenuUpPin) == HIGH) {

    UpdateTimers = true;
    // getting timer number from the menu pos
    byte Timer_no = (MenuPos / 4);

    // Timer Time screen
    if ((MenuPos % 4) == 0) {
      if (SubMenuPos == 0) {
        (Timers[Timer_no]).hour = ((Timers[Timer_no]).hour + 1) % 24;
      }
      else {
        (Timers[Timer_no]).minute = ((Timers[Timer_no]).minute + 1) % 60;
      }
    }

    // Timer days screen
    if ((MenuPos % 4) == 1) {

      (Timers[Timer_no]).days = (Timers[Timer_no]).days ^ WeekDays[SubMenuPos];
    }

    // Timer duration screen
    if ((MenuPos % 4) == 2) {

      if (SubMenuPos == 0) {

        (Timers[Timer_no]).duration = ((Timers[Timer_no]).duration + 1) % 256;
      }
      else {

        (Timers[Timer_no]).dunit = ((Timers[Timer_no]).dunit + 1) % 3;
      }
    }
    // Timer relays screen
    if ((MenuPos % 4) == 3) {
      (Timers[Timer_no]).relays = (Timers[Timer_no]).relays ^ Relays[SubMenuPos];
    }
  }
  //SubMenuDown
  if (digitalRead(SubMenuDownPin) == HIGH) {

    UpdateTimers = true;

    // getting timer number from the menu pos
    byte Timer_no = (MenuPos / 4);

    // Timer Time screen
    if ((MenuPos % 4) == 0) {

      if (SubMenuPos == 0) {

        if ((Timers[Timer_no]).hour > 0) {

          (Timers[Timer_no]).hour = (Timers[Timer_no]).hour - 1;
        }
        else {
          (Timers[Timer_no]).hour = 23;
        }
      }
      else {
        if ((Timers[Timer_no]).minute > 0) {
          (Timers[Timer_no]).minute = (Timers[Timer_no]).minute - 1;
        }
        else {
          (Timers[Timer_no]).minute = 59;
        }
      }
    }

    // Timer days screen
    if ((MenuPos % 4) == 1) {

      (Timers[Timer_no]).days = (Timers[Timer_no]).days ^ WeekDays[SubMenuPos];
    }

    // Timer duration screen
    if ((MenuPos % 4) == 2) {

      if (SubMenuPos == 0) {
        if ((Timers[Timer_no]).duration > 0) {
          (Timers[Timer_no]).duration--;
        }
      }
      else {
        if ((Timers[Timer_no]).dunit > 0) {
          (Timers[Timer_no]).dunit--;
        }
      }
    }
    // Timer relays screen
    if ((MenuPos % 4) == 3) {
      (Timers[Timer_no]).relays = (Timers[Timer_no]).relays ^ Relays[SubMenuPos];
    }
  }
}


void ShowDateTime(DateTime now) {

  lcd.noCursor();
  
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


void should_run(DateTime now) {

  // Rest ActiveTimers
  if (lastMin != now.minute()) {
    lastMin = now.minute();
    for (int i = 0; i < no_timers; i++) {
      ActiveTimers[i] = false;
    }
  }

  // getting the number of day in the week
  byte today = now.dayOfTheWeek();

  // going through every timer
  for (int i = 0; i < no_timers; i++) {

    // if today is one of the Timer days and skip Timers that already worked is the current minute
    if ((Timers[i]).days & WeekDays[today] && !(ActiveTimers[i])) {

      // if the time of the Timer has come
      if (((Timers[i]).hour == now.hour()) && ((Timers[i]).minute == now.minute())) {

        // set as ActiveTimer
        ActiveTimers[i] = true;

        int dur = (Timers[i]).duration;

        // converting duration into seconds
        for (int k = (Timers[i]).dunit; k > 0; k--) {
          dur = dur * 60;
        }

        // see which relay should be turned on
        for (int j = 0; j < relay_no; j++) {
          if ((Relays[j] & (Timers[i]).relays)) {

            RelaysStates[j] = true;

            if ((now.unixtime() + dur) > RelaysCloseTime[j]) {
              RelaysCloseTime[j] = (now.unixtime() + dur);
            }

            digitalWrite(RelaysPins[j], LOW);
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
        digitalWrite(RelaysPins[i], HIGH);
        RelaysStates[i] = false;
      }
    }
  }
}


void setup() {
  // RTC and lcd setup
  Wire.begin();
  RTC.begin();
  lcd.init();
  lcd.backlight();

  // Menu button
  attachInterrupt(MenuPin, Menu, RISING);

  //SubMenu buttons
  attachInterrupt(SubMenuPin, SubMenu, RISING);
  pinMode(SubMenuUpPin, INPUT);
  pinMode(SubMenuDownPin, INPUT);

  // Relays initializing
  for (int i = 0; i < relay_no; i++) {
    pinMode(RelaysPins[i], OUTPUT);
    digitalWrite(RelaysPins[i], HIGH);
  }
  for (int i = 0; i < relay_no; i++) {
    RelaysStates[i] = false;
  }
  for (int i = 0; i < relay_no; i++) {
    RelaysCloseTime[i] = 0;
  }

  // Timers initializing
  getTimers();
  for (int i = 0; i < no_timers; i++) {
    ActiveTimers[i] = false;
  }
}

void loop() {

  if (MenuMode) {

    ShowMenu();

    HandleInput();

    delay(MenuModeDelay);
  }
  else {

    DateTime now = RTC.now();

    ShowDateTime(now);

    // Checking all Timers that should run
    should_run(now);

    // Checking every Relay that should stop
    should_stop(now);

    if (UpdateTimers) {
      UpdateTimers = false;
      UpdateTimersEEPROM();
    }

    delay(NormalModeDelay);
  }
}
