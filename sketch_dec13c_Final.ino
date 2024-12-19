/***************************************************
  Adafruit VS1053 Codec + RTC-based Motor Control Combined Example
  Fixes:
  - Removed conflicting pin assignments for VS1053 and motors
  - Ensures MP3 plays at the trigger time
****************************************************/

#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"

RTC_DS1307 rtc;

char daysOfTheWeek[7][12] = {
  "Sunday", "Monday", "Tuesday", "Wednesday", 
  "Thursday", "Friday", "Saturday"
};

// -------------------- VS1053 DEFINITIONS --------------------
// Hardware SPI pins (on Arduino Uno/Nano: 13 = SCK, 12 = MISO, 11 = MOSI)
// VS1053 control pins:
#define VS1053_RESET   -1   // VS1053 reset pin (unused if tied to Arduino reset)
#define VS1053_CS       7   // VS1053 chip select pin
#define VS1053_DCS      6   // VS1053 Data/command select pin
#define VS1053_DREQ     3   // VS1053 Data request pin (must be interrupt-capable)

// -------------------- SD CARD PIN --------------------
#define SD_CS           4   // SD card chip select

// Create the VS1053 FilePlayer object
Adafruit_VS1053_FilePlayer musicPlayer(
  VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, SD_CS
);

// -------------------- MOTOR DEFINITIONS --------------------
// Use pins that do NOT conflict with 3 or 4
// For example, use 8, 9, 10, 11 for motors:
#define LEFT_A   8
#define LEFT_B   9
#define RIGHT_A  10
#define RIGHT_B  11

// -------------------- FUNCTION PROTOTYPES --------------------
void forward();
void stopMotors();
void printDirectory(File dir, int numTabs = 0);

void setup() {
  
  stopMotors(); // Ensure motors off at startup
  Serial.begin(57600);
  
#ifndef ESP8266
  while (!Serial); // Wait for native USB boards
#endif

  
  // ------------------ Initialize RTC ------------------
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1) delay(10);
  }
  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running, setting time to compile time.");
    // Uncomment if you need to reset RTC to the sketch compile time:
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // ------------------ Initialize Motor Pins ------------------
  pinMode(LEFT_A, OUTPUT);
  pinMode(LEFT_B, OUTPUT);
  pinMode(RIGHT_A, OUTPUT);
  pinMode(RIGHT_B, OUTPUT);
  stopMotors();

  // ------------------ Initialize VS1053 ------------------
  if (!musicPlayer.begin()) {
    Serial.println(F("VS1053 not found. Check wiring!"));
    while (1);
  }
  Serial.println(F("VS1053 found."));

  // ------------------ Initialize SD Card ------------------
  if (!SD.begin(SD_CS)) {
    Serial.println(F("SD Card initialization failed."));
    while (1);
  }

  // Optional: Print files on SD card
  printDirectory(SD.open("/"), 0);

  // Set volume (lower = louder). For instance: 1,1 is very loud; 20,20 is quieter
  musicPlayer.setVolume(1, 1);

  // Use DREQ interrupts for asynchronous playback
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);

  Serial.println("Setup complete.");
  stopMotors(); // Ensure motors off at startup
}

void loop() {
  DateTime now = rtc.now();

  // Print current time (optional)
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  Serial.println();
  
  // Check time every ~1 second
  // delay(1000);

  // If it's 4:18, run motors & play MP3; otherwise stop.
  // For testing, I've used 19:58. Change to (now.hour() == 4 && now.minute() == 18) for 4:18 AM.
  if (now.hour() == 10 && now.minute() == 45) {
    forward();
    // Start playing MP3 if not already playing
    if (musicPlayer.stopped()) {
      Serial.println(F("Starting/Looping track001.mp3"));
      musicPlayer.startPlayingFile("/track001.mp3");
    }
  } else {
    stopMotors();
    // Optionally stop music if it’s outside the trigger time
    if (!musicPlayer.stopped()) {
      Serial.println(F("Stopping music (outside trigger time)"));
      musicPlayer.stopPlaying();
    }
  }
}

// -------------------- MOTOR CONTROL FUNCTIONS --------------------
void stopMotors() {
  digitalWrite(LEFT_A, LOW);
  digitalWrite(LEFT_B, LOW);
  digitalWrite(RIGHT_A, LOW);
  digitalWrite(RIGHT_B, LOW);
}

void forward() {
  digitalWrite(LEFT_A, HIGH);
  digitalWrite(LEFT_B, LOW);
  digitalWrite(RIGHT_A, HIGH);
  digitalWrite(RIGHT_B, LOW);
}

// -------------------- FILE LISTING HELPER (OPTIONAL) --------------------
void printDirectory(File dir, int numTabs) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}