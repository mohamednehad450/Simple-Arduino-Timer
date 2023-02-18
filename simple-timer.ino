#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <RTClib.h>

// Constants ----------------------------------------------------------------------------------------------------

#define TIMERS_COUNT 8 // max is 8 (due to menu restriction)
#define RELAYS_COUNT 8  // max is 8 (due to menu restriction and Timer datatype restriction)

// Relay DataType
typedef struct Relay {
  byte on_state;   // HIGH or LOW
  byte pin;        // pin number
};

// Change pin and on_state to correspond with your setup)
const Relay RELAY_1 = {on_state: LOW, pin: A0};
const Relay RELAY_2 = {on_state: LOW, pin: A1};
const Relay RELAY_3 = {on_state: LOW, pin: A2};
const Relay RELAY_4 = {on_state: LOW, pin: A3};
const Relay RELAY_5 = {on_state: LOW, pin: 13};
const Relay RELAY_6 = {on_state: LOW, pin: 12};
const Relay RELAY_7 = {on_state: LOW, pin: 11};
const Relay RELAY_8 = {on_state: LOW, pin: 10};


const Relay RELAYS[8] = {RELAY_1, RELAY_2, RELAY_3, RELAY_4, RELAY_5, RELAY_6, RELAY_7, RELAY_8};

// Buttons pins
const byte SELECT_PIN = 0;  // Pin number 2 on the Arduino Uno, INT0
const byte CURSER_PIN = 1;  // Pin number 2 on the Arduino Uno, INT2
const byte UP_PIN = 4;      // Pin number 4 on the Arduino Uno, INT2
const byte DOWN_PIN = 5;    // Pin number 5 on the Arduino Uno, INT2



const int INPUT_TICK = 300;
const int MENU_MODE_TICK = 150;
const int NORMAL_MODE_TICK = 1000;

const byte PAGE_PER_TIMER = 4; 

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


//------------------------------------------------------------------------------------------------------------------

// Varibles---------------------------------------------------------------------------------------------------------

// Relays
uint32_t RelaysCloseTime[RELAYS_COUNT];  // to keep track of ralays close time
boolean RelaysStates[RELAYS_COUNT];      // to keep track of running relays

// Menu
boolean main_menu_mode = false;
byte main_menu_index = 0;
boolean timers_menu_mode = false;
byte timers_menu_index = 0; 
byte timers_menu_subindex = 0; 
boolean refresh_LCD = false;


boolean time_set_mode = false;
DateTime temp_time;
boolean update_RTC_time = false;

long lastMillis = 0; // used for interrupt

byte cursor_index = 0;  // SubMenu Postion

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


void relay_on(Relay r) {
    digitalWrite(r.pin, r.on_state);
}
void relay_off(Relay r) {
  if (r.on_state == HIGH) {
    digitalWrite(r.pin, LOW);
  }
  else {
    digitalWrite(r.pin, HIGH);
  }
}

// gets the timers from the EEPROM
void getTimers() {
  for (int i = 0; i < TIMERS_COUNT; i++) {
    int eeAddress = sizeof(Timer) * i; //EEPROM address to start reading from
    EEPROM.get( eeAddress, Timers[i] );
  }
}

// update the current timers to EEPROM
void UpdateTimersEEPROM() {
  for (int i = 0; i < TIMERS_COUNT; i++) {
    int eeAddress = sizeof(Timer) * i; //EEPROM address to start reading from
    EEPROM.put(eeAddress, Timers[i]);
  }
}


void ResetTimer(byte timer_index) {
  
  (Timers[timer_index]).hour = 0;
  (Timers[timer_index]).minute = 0;
  (Timers[timer_index]).days = B00000000;
  (Timers[timer_index]).duration = 0;
  (Timers[timer_index]).dunit = 0;
  (Timers[timer_index]).relays = B00000000;
  
  int eeAddress = sizeof(Timer) * timer_index; //EEPROM address to start reading from
  EEPROM.put(eeAddress, Timers[timer_index]);
}

// interrupt handler
void Menu() {

  // adding delay between interrupts
  if ((millis() - lastMillis) > INPUT_TICK) {




    if (main_menu_mode) {

      // select timer page
      if (main_menu_index == 0) {
        main_menu_mode = false;
        timers_menu_index = cursor_index * 4;
      }
      // Set Date/Time page
      else if (main_menu_index == 1) {
        if (cursor_index == 1) {
          time_set_mode = true;
          main_menu_mode = false;
        }
        else {
          timers_menu_mode = false;
          main_menu_mode = false;
        }
      }
      // Rest Timer Page
      else if (main_menu_index == 2) {
        ResetTimer(cursor_index);
        timers_menu_mode = false;
        main_menu_mode = false;
      }
      main_menu_index = 0;

    }

    else if (time_set_mode) {
      time_set_mode = false;
      timers_menu_mode = false;
    }

    else if (timers_menu_mode) {
      timers_menu_index++;
      timers_menu_subindex++;
    }
    
    // In Normal mode
    else {
      timers_menu_mode = true;
      main_menu_mode = true;
    }

    cursor_index = 0;
    lastMillis = millis();
    refresh_LCD = true;

  }
}

void SubMenu() {

  // adding delay between interrupts
  if ((millis() - lastMillis) > INPUT_TICK) {
    cursor_index++;
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
void main_menu_screen() {
  if (refresh_LCD) {
    lcd.clear();
    refresh_LCD = false;
  }
  if (main_menu_index == 0) {
    lcd.setCursor(0, 0);
    lcd.print("Edit Timer:");

    lcd.setCursor(0, 1);

    for (int i = 0; i < TIMERS_COUNT; i++) {
      lcd.print(i + 1);
      lcd.print(" ");
    }

    if (cursor_index >= TIMERS_COUNT) {
      cursor_index = 0;
    }
    lcd.setCursor(cursor_index * 2, 1);
    lcd.cursor();
  }
  else if (main_menu_index == 1) {
    lcd.setCursor(0, 0);
    lcd.print("Set Date/Time? :");

    lcd.setCursor(4, 1);

    lcd.print("No  Yes");
    if (cursor_index >= 2) {
      cursor_index = 0;
    }
    lcd.setCursor(cursor_index * 4 + 5, 1);
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

    if (cursor_index >= TIMERS_COUNT) {
      cursor_index = 0;
    }
    lcd.setCursor(cursor_index * 2, 1);
    lcd.cursor();

  }

}

void change_time_screen() {
  if (refresh_LCD) {
    lcd.clear();
    refresh_LCD = false;

  }

  print_data_time(temp_time);

  if (cursor_index > 5) {
    cursor_index = 0;
  }

  if (cursor_index < 3) {
    lcd.setCursor((cursor_index % 3) * 3 + 9, 0);
  } else {
    lcd.setCursor((cursor_index % 3) * 3 + 7, 1);
  }
  lcd.cursor();
}


void change_time_screen_input() {
  //SubMenuUp
  if (digitalRead(UP_PIN) == HIGH) {
    refresh_LCD = true;
    update_RTC_time = true;
    if (cursor_index == 0) {
      temp_time = temp_time + TimeSpan(366, 0, 0, 0);
    }
    if (cursor_index == 1) {
      temp_time = temp_time + TimeSpan(31, 0, 0, 0);
    }
    if (cursor_index == 2) {
      temp_time = temp_time + TimeSpan(1, 0, 0, 0);
    }
    if (cursor_index == 3) {
      temp_time = temp_time + TimeSpan(0, 1, 0, 0);
    }
    if (cursor_index == 4) {
      temp_time = temp_time + TimeSpan(0, 0, 1, 0);
    }
    if (cursor_index == 5) {
      temp_time = temp_time + TimeSpan(0, 0, 0, 1);
    }

  }
  //SubMenuDown
  if (digitalRead(DOWN_PIN) == HIGH) {
    update_RTC_time = true;
    refresh_LCD = true;
    if (cursor_index == 0) {
      temp_time = temp_time - TimeSpan(366, 0, 0, 0);
    }
    if (cursor_index == 1) {
      temp_time = temp_time - TimeSpan(31, 0, 0, 0);
    }
    if (cursor_index == 2) {
      temp_time = temp_time - TimeSpan(1, 0, 0, 0);
    }
    if (cursor_index == 3) {
      temp_time = temp_time - TimeSpan(0, 1, 0, 0);
    }
    if (cursor_index == 4) {
      temp_time = temp_time - TimeSpan(0, 0, 1, 0);
    }
    if (cursor_index == 5) {
      temp_time = temp_time - TimeSpan(0, 0, 0, 1);
    }
  }

}


void main_menu_screen_input() {
  //SubMenuUp
  if (digitalRead(UP_PIN) == HIGH) {
    cursor_index = 0;
    refresh_LCD = true;
    main_menu_index = (main_menu_index + 1) % 3;
  }
  //SubMenuDown
  if (digitalRead(DOWN_PIN) == HIGH) {
    cursor_index = 0;
    refresh_LCD = true;
    if (main_menu_index == 0) {
      main_menu_index = 2;
    } else main_menu_index--;
  }

}


void timers_screen() {

  if (timers_menu_subindex >= PAGE_PER_TIMER) {
    timers_menu_mode = false;
    timers_menu_subindex = 0;
  }
  
  else {
    // to clear only once every new menu
    if (refresh_LCD) {
      lcd.clear();
      refresh_LCD = false;
    }

    // getting timer number from the menu pos
    byte timer_index = (timers_menu_index / PAGE_PER_TIMER);
    byte Display_Number =  (timers_menu_index / PAGE_PER_TIMER) + 1;

    lcd.cursor();

    // Timer time screen
    if ((timers_menu_index % PAGE_PER_TIMER) == 0) {
      lcd.setCursor(0, 0);
      lcd.print("Timer");
      lcd.print(Display_Number);
      lcd.print(" : Time");

      lcd.setCursor(0, 1);

      PrintTime((Timers[timer_index]).hour, (Timers[timer_index]).minute);

      if (cursor_index > 1) {
        cursor_index = 0;
      }
      lcd.setCursor((cursor_index * 3 + 1), 1);
    }

    // Timer days screen
    if ((timers_menu_index % PAGE_PER_TIMER) == 1) {
      lcd.setCursor(0, 0);

      lcd.print("Timer");
      lcd.print(Display_Number);
      lcd.print(" : Days");

      lcd.setCursor(0, 1);

      for (int i = 0; i < 7; i++) {
        lcd.print(weekdays_symbols[i]);
        if (weekdays[i] & (Timers[timer_index]).days) {
          lcd.print("*");
        }
        else {
          lcd.print(" ");
        }
      }
      if (cursor_index > 6) {
        cursor_index = 0;
      }
      lcd.setCursor(cursor_index * 2, 1);

    }

    // Timer duration screen
    if ((timers_menu_index % PAGE_PER_TIMER) == 2) {

      lcd.setCursor(0, 0);

      lcd.print("Timer");
      lcd.print(Display_Number);
      lcd.print(" : Duration");

      lcd.setCursor(0, 1);

      lcd.print((Timers[timer_index]).duration);
      lcd.print(" ");
      lcd.print(time_units[(Timers[timer_index]).dunit]);

      if (cursor_index > 1) {
        cursor_index = 0;
      }
      lcd.setCursor((cursor_index * 3), 1);

    }

    // Timer relays screen
    if ((timers_menu_index % PAGE_PER_TIMER) == 3) {
      lcd.setCursor(0, 0);
      lcd.print("Timer");
      lcd.print(Display_Number);
      lcd.print(" : Relays");

      lcd.setCursor(0, 1);

      for (int i = 0; i < RELAYS_COUNT; i++) {
        lcd.print(i + 1);
        if (Relays[i] & (Timers[timer_index]).relays) {
          lcd.print("*");
        } else {
          lcd.print(" ");
        }
      }
      if (cursor_index >= RELAYS_COUNT) {
        cursor_index = 0;
      }
      lcd.setCursor(cursor_index * 2, 1);
    }
  }
}


void timers_screen_input() {

  //SubMenuUp
  if (digitalRead(UP_PIN) == HIGH) {
    refresh_LCD = true;

    UpdateTimers = true;
    // getting timer number from the menu pos
    byte timer_index = (timers_menu_index / PAGE_PER_TIMER);

    // Timer Time screen
    if ((timers_menu_index % PAGE_PER_TIMER) == 0) {
      if (cursor_index == 0) {
        (Timers[timer_index]).hour = ((Timers[timer_index]).hour + 1) % 24;
      }
      else {
        (Timers[timer_index]).minute = ((Timers[timer_index]).minute + 1) % 60;
      }
    }

    // Timer days screen
    if ((timers_menu_index % PAGE_PER_TIMER) == 1) {

      (Timers[timer_index]).days = (Timers[timer_index]).days ^ weekdays[cursor_index];
    }

    // Timer duration screen
    if ((timers_menu_index % PAGE_PER_TIMER) == 2) {

      if (cursor_index == 0) {

        (Timers[timer_index]).duration = ((Timers[timer_index]).duration + 1) % 256;
      }
      else {

        (Timers[timer_index]).dunit = ((Timers[timer_index]).dunit + 1) % 3;
      }
    }
    // Timer relays screen
    if ((timers_menu_index % PAGE_PER_TIMER) == 3) {
      (Timers[timer_index]).relays = (Timers[timer_index]).relays ^ Relays[cursor_index];
    }
  }
  //SubMenuDown
  if (digitalRead(DOWN_PIN) == HIGH) {
    refresh_LCD = true;

    UpdateTimers = true;

    // getting timer number from the menu pos
    byte timer_index = (timers_menu_index / PAGE_PER_TIMER);

    // Timer Time screen
    if ((timers_menu_index % PAGE_PER_TIMER) == 0) {

      if (cursor_index == 0) {

        if ((Timers[timer_index]).hour > 0) {

          (Timers[timer_index]).hour = (Timers[timer_index]).hour - 1;
        }
        else {
          (Timers[timer_index]).hour = 23;
        }
      }
      else {
        if ((Timers[timer_index]).minute > 0) {
          (Timers[timer_index]).minute = (Timers[timer_index]).minute - 1;
        }
        else {
          (Timers[timer_index]).minute = 59;
        }
      }
    }

    // Timer days screen
    if ((timers_menu_index % PAGE_PER_TIMER) == 1) {

      (Timers[timer_index]).days = (Timers[timer_index]).days ^ weekdays[cursor_index];
    }

    // Timer duration screen
    if ((timers_menu_index % PAGE_PER_TIMER) == 2) {

      if (cursor_index == 0) {
        if ((Timers[timer_index]).duration > 0) {
          (Timers[timer_index]).duration--;
        }
      }
      else {
        if ((Timers[timer_index]).dunit > 0) {
          (Timers[timer_index]).dunit--;
        }
      }
    }
    // Timer relays screen
    if ((timers_menu_index % PAGE_PER_TIMER) == 3) {
      (Timers[timer_index]).relays = (Timers[timer_index]).relays ^ Relays[cursor_index];
    }
  }
}


void print_data_time(DateTime now) {

  lcd.noCursor();
  if (refresh_LCD) {
    lcd.clear();
    refresh_LCD = false;
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

            relay_on(RELAYS[j]);
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
        relay_off(RELAYS[i]);
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
  attachInterrupt(SELECT_PIN, Menu, RISING);

  //SubMenu buttons
  attachInterrupt(CURSER_PIN, SubMenu, RISING);
  pinMode(UP_PIN, INPUT);
  pinMode(DOWN_PIN, INPUT);

  // Relays initializing
  for (int i = 0; i < RELAYS_COUNT; i++) {
    pinMode(RELAYS[i].pin, OUTPUT);
    relay_off(RELAYS[i]);
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

  if (timers_menu_mode) {
    if (main_menu_mode) {
      main_menu_screen();
      main_menu_screen_input();
    
    } else if (time_set_mode) {
      change_time_screen();
      change_time_screen_input();
    }
    // Timer edit pages
    else {
      timers_screen();
      timers_screen_input();
    }
    delay(MENU_MODE_TICK);
  }
  else {

    // to apply changes to time if any
    if (update_RTC_time) {
      update_RTC_time = false;
      RTC.adjust(temp_time);
    }
    // to apply changes to timers if any
    if (UpdateTimers) {
      UpdateTimers = false;
      UpdateTimersEEPROM();
    }

    DateTime now = RTC.now();
    temp_time = RTC.now();

    print_data_time(now);

    // Checking all Timers that should run
    should_run(now);

    // Checking every Relay that should stop
    should_stop(now);

    delay(NORMAL_MODE_TICK);
  }
}
