


#include <M5Core2.h>
// #include <Arduino.h>
#include <string>
#include <TinyGPS++.h>
#include <RBD_Timer.h>
#include <I2C_BM8563.h> // RTC

RBD::Timer loggingtime{5000};  // stop logging after this delay.

// RTC BM8563 I2C port
// I2C pin definition for M5Stack Core2
#define BM8563_I2C_SDA 21
#define BM8563_I2C_SCL 22
I2C_BM8563 rtc(I2C_BM8563_DEFAULT_ADDRESS, Wire1);
I2C_BM8563_TimeTypeDef timeStruct;

// Globals! Living dangerously.
fs::File fd{}; // log file handle
char filename[16] = "";
// uint8_t buffer[1000] = {0};

constexpr uint32_t GPSBaud = 9600;
// The TinyGPSPlus object
TinyGPSPlus gps;
//void displayInfo(void);
bool log_it(uint8_t c);


void setup() {
  M5.begin();
  M5.Lcd.clear();
  M5.lcd.setRotation(3);
  M5.Lcd.setTextFont(1);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.setCursor(0, 10);

  // Init I2C
  Wire1.begin(BM8563_I2C_SDA, BM8563_I2C_SCL);
  // Init RTC
  rtc.begin();
  rtc.getTime(&timeStruct);
  M5.Lcd.printf("%02d:%02d:%02d\n",
                timeStruct.hours,
                timeStruct.minutes,
                timeStruct.seconds
               );

  // Open a new log file
  auto status = SD.begin(4);  // GPIO 4
  if(!status) {
    // Serial.println("Cannot open SD card.");
    M5.Lcd.println("Cannot open SD card.");
    while(1);  // stalled 
  }
  // make a new file name based on RTC
  snprintf(filename, 16, "/%02d-%02d-%02d.log", 
      timeStruct.hours,
      timeStruct.minutes,
      timeStruct.seconds
    );

  fd = SD.open(filename, "w");  // make an empty file
  if(!fd) {
    M5.Lcd.printf("%s\n", filename);
    M5.Lcd.println("No log file, stalling");
    while(1);
  }
  fd.printf("Start millis() = %d\n", millis());
  fd.close();
  //Serial.begin(115200); // to/from terminal
  Serial2.begin(GPSBaud); // to/from GPS

  //Serial.println("Hello GPS Test");
  M5.Lcd.println("Hello GPS Test");
  loggingtime.restart();
}

void loop() {
  static std::string sentence{""};
  static bool locationvalid = false;
  static bool altitudevalid = false;
  static bool timevalid = false;


  while (Serial2.available() > 0)
  {
    uint8_t c = Serial2.read();
    // M5.Lcd.printf("%c",c);
    sentence += c;
    // M5.Lcd.printf("%s\n", sentence.c_str());
    gps.encode(c);
    if(c == 0x0a) {
      fd = SD.open(filename, "a");  // open, append and close
      fd.printf("%d: ", millis());
      fd.printf("%s\n", sentence.c_str());
      fd.close();
      sentence.erase();
    }
  }

  if(gps.location.isValid() && !locationvalid) {
    fd = SD.open(filename, "a");  // open, append and close
    fd.printf("%d: Location is valid.\n", millis());
    fd.printf("lat, long: %f, %f\n",
          gps.location.lat(),
          gps.location.lng());
    fd.close();
    locationvalid = true;
  }

  if(gps.altitude.isValid() && !altitudevalid) {
    fd = SD.open(filename, "a");  // open, append and close
    fd.printf("%d: Altitude is valid.\n", millis());
    fd.printf("altitude: %f\n",
          gps.altitude.meters());
    fd.close();
    altitudevalid = true;
  }

  if(gps.time.isValid() && !timevalid) {
    fd = SD.open(filename, "a");  // open, append and close
    fd.printf("%d: Time is valid.\n", millis());
    fd.printf("%02d:%02d:%02d\n",
          gps.time.hour(),
          gps.time.minute(),
          gps.time.second());
    fd.close();
    timevalid = true;
  }

  if(locationvalid && altitudevalid && timevalid) {
    M5.Lcd.clear();
    M5.Lcd.setCursor(0, 10);
    M5.Lcd.println("Done!\n");
    while (1); // stall
  }
    
  // if (millis() > 5000 && gps.charsProcessed() < 10)
  // {
  //   Serial.println(F("No GPS detected: check wiring."));
  //   while(true);  // Stall
  // }
  // if(refresh.onRestart()) {
  //   Serial.println(millis());
  //   displayInfo();
  //   Serial.print(F("Satellites: ")); Serial.println(gps.satellites.value());
  //   M5.Lcd.print(F("Satellites: ")); M5.Lcd.println(gps.satellites.value());
  //   Serial.println();
  // }
}

bool log_it(uint8_t c) {
  static bool cr = false;
  static bool lf = false;
  if(!lf && (c == 0x0d))
    cr = true;
  if(cr && (c == 0x0a))
    lf = true;
  if(cr && lf) {
    cr = false;
    lf = false;
    return true;  // time to log the millis
  }
  else {
    return false; // not yet
  }
}

// void displayInfo()
// {
//   M5.Lcd.clear();
//   M5.Lcd.setCursor(0, 10);

//   Serial.print(F("Location: ")); 
//   M5.Lcd.print(F("Location: ")); 
//   if (gps.location.isValid())
//   {
//     Serial.print(gps.location.lat(), 6);
//     Serial.print(F(","));
//     Serial.println(gps.location.lng(), 6);
//     M5.Lcd.print(gps.location.lat(), 6);
//     M5.Lcd.print(F(","));
//     M5.Lcd.println(gps.location.lng(), 6);
//   }
//   else
//   {
//     Serial.println(F("INVALID"));
//     M5.Lcd.println(F("INVALID"));
//   }

//   Serial.print(F("Date/Time: "));
//   M5.Lcd.print(F("Date/Time: "));
//   if (gps.date.isValid())
//   {
//     Serial.print(gps.date.month());
//     Serial.print(F("/"));
//     Serial.print(gps.date.day());
//     Serial.print(F("/"));
//     Serial.println(gps.date.year());
//     M5.Lcd.print(gps.date.month());
//     M5.Lcd.print(F("/"));
//     M5.Lcd.print(gps.date.day());
//     M5.Lcd.print(F("/"));
//     M5.Lcd.println(gps.date.year());
//   }
//   else
//   {
//     Serial.print(F("INVALID"));
//     M5.Lcd.print(F("INVALID"));
//   }

//   Serial.print(F(" "));
//   M5.Lcd.print(F(" "));
//   if (gps.time.isValid())
//   {
//     if (gps.time.hour() < 10) Serial.print(F("0"));
//     Serial.print(gps.time.hour());
//     Serial.print(F(":"));
//     M5.Lcd.print(gps.time.hour());
//     M5.Lcd.print(F(":"));
//     if (gps.time.minute() < 10) Serial.print(F("0"));
//     Serial.print(gps.time.minute());
//     Serial.print(F(":"));
//     M5.Lcd.print(gps.time.minute());
//     M5.Lcd.print(F(":"));
//     if (gps.time.second() < 10) Serial.print(F("0"));
//     Serial.print(gps.time.second());
//     Serial.print(F("."));
//     M5.Lcd.print(gps.time.second());
//     M5.Lcd.print(F("."));
//     if (gps.time.centisecond() < 10) Serial.print(F("0"));
//     Serial.println(gps.time.centisecond());
//     M5.Lcd.println(gps.time.centisecond());
//   }
//   else
//   {
//     Serial.println(F("INVALID"));
//     M5.Lcd.println(F("INVALID"));
//   }

// }