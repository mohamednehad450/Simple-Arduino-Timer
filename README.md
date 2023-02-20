# Simple-Arduino-Timer

## Table of content
- [Demo](#demo)
- [Features](#features)
- [Hardware](#hardware)
    + [Components](#components)
    + [Circuit Diagram](#circuit-diagram)
- [Software](#software)
- [API docs](#api-docs)

## Demo
![MainMenu](images/main-menu.gif "Main Menu")
![MainMenu](images/edit-timer.gif "Edit Timer")
![MainMenu](images/reset-timer.gif "Reset Timer")
![MainMenu](images/change-date.gif "Change Date")

## Features

- Supports up to 8 relays
- 8 individual weekly timers
- Each timer can run multiples relays
- Each timer can run for a duration of up to 24 hours
- Utilizes RTC to keep track of the current time and date

## Hardware

### Components
- 1 x Arduino Uno
- 1 x 16x2 I2C LCD
- 1 x DS1307 RTC
- 1 x 8 channel relay board
- 4 x Push buttons
- 4 x 10k ohm resistors
- 1 x 5v power supply

### Circuit diagram
![Circuit](circuit.png "Circuit")

## Software

### Dependencies

1. [RTClib by Adafruit](https://github.com/adafruit/RTClib): install from the Arduino Library Manager
2. [Adafruit_BusIO](https://github.com/adafruit/Adafruit_BusIO) - (will be installed automatically with RTClib)
3. [LiquidCrystal_I2C by Frank de Brabander](https://github.com/johnrickman/LiquidCrystal_I2C): install from the Arduino Library Manager
