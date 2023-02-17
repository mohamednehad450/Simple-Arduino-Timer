#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <RTClib.h>

// constant ----------------------------------------------------------------------------------------------------
#define TIMERS_COUNT 8 // max is 8 (due to menu restriction)
#define RELAYS_COUNT 8  // max is 8 (due to menu restriction and Timer datatype restriction)

// Relays pins (change them to correspond with your setup)
#define RELAY_1 A0
#define RELAY_2 A1
#define RELAY_3 A2
#define RELAY_4 A3
#define RELAY_5 A4
#define RELAY_6 A5
#define RELAY_7 10
#define RELAY_8 11

const byte RelaysPins[8] = {RELAY_1, RELAY_2, RELAY_3, RELAY_4, RELAY_5, RELAY_6, RELAY_7, RELAY_8};

const int RelayDelayTime = 0;

const int INPUT_TICK = 300;
const int MENU_MODE_TICK = 150;
const int NORMAL_MODE_TICK = 1000;

// RTC setup
RTC_DS1307 RTC;

// LCD setup
LiquidCrystal_I2C lcd(0x3F, 16, 2);

//  Week days in binary starting with sunday
const byte weekdays[7] = {B00000001, B00000010, B00000100, B00001000, B00010000, B00100000, B01000000};

// Week days symbols for display
const char weekdays_symbols[7] = {'S', 'M', 'T', 'W', 'T', 'F', 'S'};

// Duration units for display
const String time_units[3] = {"sec", "min", "hour"};

// Relays in binary
const byte Relays[8] = {B00000001, B00000010, B00000100, B00001000, B00010000, B00100000, B01000000, B10000000};

// SubMenu
const byte SubMenuPin = 1;       // interrupt pin

const byte SubMenuUpPin = 4;     //   SubMenu Buttons Pins
const byte SubMenuDownPin = 5;   //

// Menu
const byte MenuPin = 0;   //interrupt pin
const byte MaxNoMenu = 4; // max No. of pages per timer

//------------------------------------------------------------------------------------------------------------------

// Varibles---------------------------------------------------------------------------------------------------------

// Relays
uint32_t RelaysCloseTime[RELAYS_COUNT];  // to keep track of ralays close time
boolean RelaysStates[RELAYS_COUNT];      // to keep track of running relays

// Menu
boolean MenuMode = false;
boolean MainMenuMode = false;
byte MainMenuPos = 0;
byte MenuPos = 0;  // menu postion
boolean NewMenu = false;  // used for lcd clearing
byte NoMenu = 0; 


boolean TimeSetMode = false;
DateTime TempTime;
boolean SetTime = false;

long lastMillis = 0; // used for interrupt


// SubMenu
byte SubMenuPos = 0;  // SubMenu Postion

// Timers
boolean UpdateTimers = false;      // tells weather or not the timers were modified
boolean ActiveTimers[TIMERS_COUNT];   // keeps track of active timer in a given minute (not to check them again in the same minute)
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
Timer* Timers = (Timer*) malloc(sizeof(Timer) * TIMERS_COUNT);

//-----------------------------------------------------------------------------------------------------------


// gets the timers from the EEPROM
void getTimers() {

  for (int i = 0; i < TIMERS_COUNT; i++) {
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

  for (int i = 0; i < TIMERS_COUNT; i++) {
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


void ResetTimer(byte Timer_no) {
  
  (Timers[Timer_no]).hour = 0;
  (Timers[Timer_no]).minute = 0;
  (Timers[Timer_no]).days = B00000000;
  (Timers[Timer_no]).duration = 0;
  (Timers[Timer_no]).dunit = 0;
  (Timers[Timer_no]).relays = B00000000;

  // hour
  EEPROM.write(Timer_no * 6, (Timers[Timer_no]).hour);

  // minute
  EEPROM.write(Timer_no * 6 + 1, (Timers[Timer_no]).minute);

  // days
  EEPROM.write(Timer_no * 6 + 2, (Timers[Timer_no]).days);

  // duration
  EEPROM.write(Timer_no * 6 + 3, (Timers[Timer_no]).duration);

  // dunit
  EEPROM.write(Timer_no * 6 + 4, (Timers[Timer_no]).dunit);

  // relays
  EEPROM.write(Timer_no * 6 + 5, (Timers[Timer_no]).relays);

}

// interrupt handler
void Menu() {

  // adding delay between interrupts
  if ((millis() - lastMillis) > INPUT_TICK) {


    // if in Normal mode
    if (MenuMode == false) {
      MenuMode = true;
      MainMenuMode = true;
    }
    // if in MainMenu
    else if (MainMenuMode) {

      // select timer page
      if (MainMenuPos == 0) {
        MainMenuMode = false;
        MenuPos = SubMenuPos * 4;
      }
      // Set Date/Time page
      else if (MainMenuPos == 1) {
        if (SubMenuPos == 1) {
          TimeSetMode = true;
          MainMenuMode = false;
        }
        else {
          MenuMode = false;
          MainMenuMode = false;
        }
      }
      // Rest Timer Page
      else if (MainMenuPos == 2) {
        ResetTimer(SubMenuPos);
        MenuMode = false;
        MainMenuMode = false;
      }
      MainMenuPos = 0;

    }
    // if in TimeSetMode
    else if (TimeSetMode) {
      TimeSetMode = false;
      MenuMode = false;
    }

    else {
      MenuPos++;
      NoMenu++;
    }

    SubMenuPos = 0;
    lastMillis = millis();
    NewMenu = true;

  }
}

void SubMenu() {

  // adding delay between interrupts
  if ((millis() - lastMillis) > INPUT_TICK) {
    SubMenuPos++;
    lastMillis = millis();
  }

}
void PrintTime(int hour, int min, int sec = -1) {
  if (hour < 10) {
    lcd.print("0");
  }
  lcd.print(hour);
  lcd.print(":");
  if (min < 10) {
    lcd.print("0");
  }
  lcd.print(min);
  if (sec >= 0) {
    lcd.print(":");
    if (sec < 10) {
      lcd.print("0");
    }
    lcd.print(sec);
  }
}
void ShowMainMenu() {
  if (NewMenu) {
    lcd.clear();
    NewMenu = false;
  }
  if (MainMenuPos == 0) {
    lcd.setCursor(0, 0);
    lcd.print("Edit Timer:");

    lcd.setCursor(0, 1);

    for (int i = 0; i < TIMERS_COUNT; i++) {
      lcd.print(i + 1);
      lcd.print(" ");
    }

    if (SubMenuPos >= TIMERS_COUNT) {
      SubMenuPos = 0;
    }
    lcd.setCursor(SubMenuPos * 2, 1);
    lcd.cursor();
  }
  else if (MainMenuPos == 1) {
    lcd.setCursor(0, 0);
    lcd.print("Set Date/Time? :");

    lcd.setCursor(4, 1);

    lcd.print("No  Yes");
    if (SubMenuPos >= 2) {
      SubMenuPos = 0;
    }
    lcd.setCursor(SubMenuPos * 4 + 5, 1);
    lcd.cursor();


  }
  else {
    lcd.setCursor(0, 0);
    lcd.print("Rest Timer:");

    lcd.setCursor(0, 1);

    for (int i = 0; i < TIMERS_COUNT; i++) {
      lcd.print(i + 1);
      lcd.print(" ");
    }

    if (SubMenuPos >= TIMERS_COUNT) {
      SubMenuPos = 0;
    }
    lcd.setCursor(SubMenuPos * 2, 1);
    lcd.cursor();

  }

}

void ShowTimeSet() {
  if (NewMenu) {
    lcd.clear();
    NewMenu = false;

  }

  ShowDateTime(TempTime);

  if (SubMenuPos > 5) {
    SubMenuPos = 0;
  }

  if (SubMenuPos < 3) {
    lcd.setCursor((SubMenuPos % 3) * 3 + 9, 0);
  } else {
    lcd.setCursor((SubMenuPos % 3) * 3 + 7, 1);
  }
  lcd.cursor();
}


void HandleTimeSetInput() {
  //SubMenuUp
  if (digitalRead(SubMenuUpPin) == HIGH) {
    NewMenu = true;
    SetTime = true;
    if (SubMenuPos == 0) {
      TempTime = TempTime + TimeSpan(366, 0, 0, 0);
    }
    if (SubMenuPos == 1) {
      TempTime = TempTime + TimeSpan(31, 0, 0, 0);
    }
    if (SubMenuPos == 2) {
      TempTime = TempTime + TimeSpan(1, 0, 0, 0);
    }
    if (SubMenuPos == 3) {
      TempTime = TempTime + TimeSpan(0, 1, 0, 0);
    }
    if (SubMenuPos == 4) {
      TempTime = TempTime + TimeSpan(0, 0, 1, 0);
    }
    if (SubMenuPos == 5) {
      TempTime = TempTime + TimeSpan(0, 0, 0, 1);
    }

  }
  //SubMenuDown
  if (digitalRead(SubMenuDownPin) == HIGH) {
    SetTime = true;
    NewMenu = true;
    if (SubMenuPos == 0) {
      TempTime = TempTime - TimeSpan(366, 0, 0, 0);
    }
    if (SubMenuPos == 1) {
      TempTime = TempTime - TimeSpan(31, 0, 0, 0);
    }
    if (SubMenuPos == 2) {
      TempTime = TempTime - TimeSpan(1, 0, 0, 0);
    }
    if (SubMenuPos == 3) {
      TempTime = TempTime - TimeSpan(0, 1, 0, 0);
    }
    if (SubMenuPos == 4) {
      TempTime = TempTime - TimeSpan(0, 0, 1, 0);
    }
    if (SubMenuPos == 5) {
      TempTime = TempTime - TimeSpan(0, 0, 0, 1);
    }
  }

}


void HandleMenuInput() {
  //SubMenuUp
  if (digitalRead(SubMenuUpPin) == HIGH) {
    SubMenuPos = 0;
    NewMenu = true;
    MainMenuPos = (MainMenuPos + 1) % 3;
  }
  //SubMenuDown
  if (digitalRead(SubMenuDownPin) == HIGH) {
    SubMenuPos = 0;
    NewMenu = true;
    if (MainMenuPos == 0) {
      MainMenuPos = 2;
    } else MainMenuPos--;
  }

}


void ShowMenu() {

  if (NoMenu >= MaxNoMenu) {
    MenuMode = false;
    NoMenu = 0;

  }
  else {
    // to clear only once every new menu
    if (NewMenu) {
      lcd.clear();
      NewMenu = false;
    }

    // getting timer number from the menu pos
    byte Timer_no = (MenuPos / MaxNoMenu);
    byte Display_Number =  (MenuPos / MaxNoMenu) + 1;

    lcd.cursor();

    // Timer time screen
    if ((MenuPos % MaxNoMenu) == 0) {
      lcd.setCursor(0, 0);
      lcd.print("Timer");
      lcd.print(Display_Number);
      lcd.print(" : Time");

      lcd.setCursor(0, 1);

      PrintTime((Timers[Timer_no]).hour, (Timers[Timer_no]).minute);

      if (SubMenuPos > 1) {
        SubMenuPos = 0;
      }
      lcd.setCursor((SubMenuPos * 3 + 1), 1);
    }

    // Timer days screen
    if ((MenuPos % MaxNoMenu) == 1) {
      lcd.setCursor(0, 0);

      lcd.print("Timer");
      lcd.print(Display_Number);
      lcd.print(" : Days");

      lcd.setCursor(0, 1);

      for (int i = 0; i < 7; i++) {
        lcd.print(weekdays_symbols[i]);
        if (weekdays[i] & (Timers[Timer_no]).days) {
          lcd.print("*");
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
    if ((MenuPos % MaxNoMenu) == 2) {

      lcd.setCursor(0, 0);

      lcd.print("Timer");
      lcd.print(Display_Number);
      lcd.print(" : Duration");

      lcd.setCursor(0, 1);

      lcd.print((Timers[Timer_no]).duration);
      lcd.print(" ");
      lcd.print(time_units[(Timers[Timer_no]).dunit]);

      if (SubMenuPos > 1) {
        SubMenuPos = 0;
      }
      lcd.setCursor((SubMenuPos * 3), 1);

    }

    // Timer relays screen
    if ((MenuPos % MaxNoMenu) == 3) {
      lcd.setCursor(0, 0);
      lcd.print("Timer");
      lcd.print(Display_Number);
      lcd.print(" : Relays");

      lcd.setCursor(0, 1);

      for (int i = 0; i < RELAYS_COUNT; i++) {
        lcd.print(i + 1);
        if (Relays[i] & (Timers[Timer_no]).relays) {
          lcd.print("*");
        } else {
          lcd.print(" ");
        }
      }
      if (SubMenuPos >= RELAYS_COUNT) {
        SubMenuPos = 0;
      }
      lcd.setCursor(SubMenuPos * 2, 1);
    }
  }
}


void HandleTimerInput() {

  //SubMenuUp
  if (digitalRead(SubMenuUpPin) == HIGH) {
    NewMenu = true;

    UpdateTimers = true;
    // getting timer number from the menu pos
    byte Timer_no = (MenuPos / MaxNoMenu);

    // Timer Time screen
    if ((MenuPos % MaxNoMenu) == 0) {
      if (SubMenuPos == 0) {
        (Timers[Timer_no]).hour = ((Timers[Timer_no]).hour + 1) % 24;
      }
      else {
        (Timers[Timer_no]).minute = ((Timers[Timer_no]).minute + 1) % 60;
      }
    }

    // Timer days screen
    if ((MenuPos % MaxNoMenu) == 1) {

      (Timers[Timer_no]).days = (Timers[Timer_no]).days ^ weekdays[SubMenuPos];
    }

    // Timer duration screen
    if ((MenuPos % MaxNoMenu) == 2) {

      if (SubMenuPos == 0) {

        (Timers[Timer_no]).duration = ((Timers[Timer_no]).duration + 1) % 256;
      }
      else {

        (Timers[Timer_no]).dunit = ((Timers[Timer_no]).dunit + 1) % 3;
      }
    }
    // Timer relays screen
    if ((MenuPos % MaxNoMenu) == 3) {
      (Timers[Timer_no]).relays = (Timers[Timer_no]).relays ^ Relays[SubMenuPos];
    }
  }
  //SubMenuDown
  if (digitalRead(SubMenuDownPin) == HIGH) {
    NewMenu = true;

    UpdateTimers = true;

    // getting timer number from the menu pos
    byte Timer_no = (MenuPos / MaxNoMenu);

    // Timer Time screen
    if ((MenuPos % MaxNoMenu) == 0) {

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
    if ((MenuPos % MaxNoMenu) == 1) {

      (Timers[Timer_no]).days = (Timers[Timer_no]).days ^ weekdays[SubMenuPos];
    }

    // Timer duration screen
    if ((MenuPos % MaxNoMenu) == 2) {

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
    if ((MenuPos % MaxNoMenu) == 3) {
      (Timers[Timer_no]).relays = (Timers[Timer_no]).relays ^ Relays[SubMenuPos];
    }
  }
}


void ShowDateTime(DateTime now) {

  lcd.noCursor();
  if (NewMenu) {
    lcd.clear();
    NewMenu = false;
  }

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

  PrintTime(now.hour(), now.minute(), now.second());
}


void should_run(DateTime now) {

  // Rest ActiveTimers
  if (lastMin != now.minute()) {
    lastMin = now.minute();
    for (int i = 0; i < TIMERS_COUNT; i++) {
      ActiveTimers[i] = false;
    }
  }

  // getting the number of day in the week
  byte today = now.dayOfTheWeek();

  // going through every timer
  for (int i = 0; i < TIMERS_COUNT; i++) {

    // if today is one of the Timer days and skip Timers that already worked is the current minute
    if ((Timers[i]).days & weekdays[today] && !(ActiveTimers[i])) {

      // if the time of the Timer has come
      if (((Timers[i]).hour == now.hour()) && ((Timers[i]).minute == now.minute())) {

        // set as ActiveTimer
        ActiveTimers[i] = true;

        uint32_t dur = (Timers[i]).duration;

        // converting duration into seconds
        for (int k = (Timers[i]).dunit; k > 0; k--) {
          dur = dur * 60;
        }

        // see which relay that should be turned on
        for (int j = 0; j < RELAYS_COUNT; j++) {
          if ((Relays[j] & (Timers[i]).relays)) {

            RelaysStates[j] = true;

            if ((now.unixtime() + dur) > RelaysCloseTime[j]) {
              RelaysCloseTime[j] = (now.unixtime() + dur);
            }

            digitalWrite(RelaysPins[j], LOW);
            delay(RelayDelayTime);
          }
        }
      }
    }
  }
}


void should_stop(DateTime now) {

  for (int i = 0; i < RELAYS_COUNT; i++) {
    if (RelaysStates[i]) {
      if (now.unixtime() >= RelaysCloseTime[i]) {
        digitalWrite(RelaysPins[i], HIGH);
        RelaysStates[i] = false;
        delay(RelayDelayTime);
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
  for (int i = 0; i < RELAYS_COUNT; i++) {
    pinMode(RelaysPins[i], OUTPUT);
    digitalWrite(RelaysPins[i], HIGH);
  }
  for (int i = 0; i < RELAYS_COUNT; i++) {
    RelaysStates[i] = false;
  }
  for (int i = 0; i < RELAYS_COUNT; i++) {
    RelaysCloseTime[i] = 0;
  }

  // Timers initializing
  getTimers();  // loading timers for EEPROM

  for (int i = 0; i < TIMERS_COUNT; i++) {
    ActiveTimers[i] = false;
  }
}

void loop() {

  if (MenuMode) {

    if (MainMenuMode) {
      ShowMainMenu();
      HandleMenuInput();
      
    } else if (TimeSetMode) {

      ShowTimeSet();
      HandleTimeSetInput();

    }
    // Timer edit pages
    else {
      ShowMenu();
      HandleTimerInput();
    }
    delay(MENU_MODE_TICK);
  }
  else {

    // to apply changes to time if any
    if (SetTime) {
      SetTime = false;
      RTC.adjust(TempTime);
    }
    // to apply changes to timers if any
    if (UpdateTimers) {
      UpdateTimers = false;
      UpdateTimersEEPROM();
    }

    DateTime now = RTC.now();
    TempTime = RTC.now();

    ShowDateTime(now);

    // Checking all Timers that should run
    should_run(now);

    // Checking every Relay that should stop
    should_stop(now);

    delay(NORMAL_MODE_TICK);
  }
}
