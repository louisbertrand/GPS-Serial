/** @brief Reset the realtime clock on M5Stack Core2
 * @author Louis Bertrand <louis@bertrandtech.ca>
 * 
 * Based on PCF8563_Alarms example in RTC by Michael Miller
*/


#include <M5Core2.h>
#include <I2C_BM8563.h>
#include <RBD_Timer.h>
#include <TinyGPS++.h>

// RTC BM8563 I2C port
// I2C pin definition for M5Stack Core2
#define BM8563_I2C_SDA 21
#define BM8563_I2C_SCL 22
I2C_BM8563 rtc(I2C_BM8563_DEFAULT_ADDRESS, Wire1);
I2C_BM8563_DateTypeDef dateStruct;
I2C_BM8563_TimeTypeDef timeStruct;

constexpr uint32_t GPSBaud = 9600;
// The TinyGPSPlus object
TinyGPSPlus gps;

// How often to update the RTC display once we're set.
RBD::Timer update_timer{1000};

bool rtc_voltage_good = false;
bool have_gps = false;

void rtc_display(void);  // Display date and time from RTC
void gps_display(void);  // Display date and time from GPS
void rtc_program(void);  // Set the RTC based on GPS time

void setup() {
  Serial.begin(115200);
  /// Hardware configurations
  M5.begin();

  // Init I2C
  Wire1.begin(BM8563_I2C_SDA, BM8563_I2C_SCL);
  // Init RTC
  rtc.begin();

  Serial2.begin(GPSBaud); // to/from GPS

  M5.Lcd.clear();
  M5.lcd.setRotation(3);
  M5.Lcd.setTextFont(2);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.setCursor(0, 10);
  M5.Lcd.println("RTC Reset");
  Serial.println("RTC Reset");

  // Check RTC Voltage Low flag
  if(rtc.getVoltLow()) {
    Serial.println("RTC low voltage.");
    M5.Lcd.println("RTC low voltage.");
  }
  else {
    Serial.println("RTC voltage good.");
    M5.Lcd.println("RTC voltage good.");
    rtc_voltage_good = true;

  // Get RTC
  rtc.getDate(&dateStruct);
  rtc.getTime(&timeStruct);

  // Print RTC
  Serial.printf("%04d/%02d/%02d %02d:%02d:%02d\n",
                dateStruct.year,
                dateStruct.month,
                dateStruct.date,
                timeStruct.hours,
                timeStruct.minutes,
                timeStruct.seconds
               );

  rtc_display();
  // M5.Lcd.setCursor(0, 100);
  // M5.Lcd.printf("%04d/%02d/%02d %02d:%02d:%02d\n",
  //               dateStruct.year,
  //               dateStruct.month,
  //               dateStruct.date,
  //               timeStruct.hours,
  //               timeStruct.minutes,
  //               timeStruct.seconds
  //              );

  M5.Lcd.setCursor(0, 100);
  Serial.println("Wait for GPS...");
  M5.Lcd.println("Wait for GPS...");

  }
} // end setup()

void loop() {
  while (Serial2.available() > 0)
  {
    uint8_t c = Serial2.read();
    // M5.Lcd.printf("%c",c);
    // sentence += c;
    // M5.Lcd.printf("%s\n", sentence.c_str());
    gps.encode(c);
    // if(c == 0x0a) {
    //   fd = SD.open(filename, "a");  // open, append and close
    //   fd.printf("%d: ", millis());
    //   fd.printf("%s\n", sentence.c_str());
    //   fd.close();
    //   sentence.erase();
    // }
  }

  if(!have_gps) {
    if(gps.time.isValid() && gps.time.isUpdated()) {
      have_gps = true;
      gps_display();
      rtc_program();
      update_timer.restart();
    }
  }
  else {
    if(update_timer.onRestart()) {
      // Get RTC and show it
      rtc.getDate(&dateStruct);
      rtc.getTime(&timeStruct);
      rtc_display();
    }
  }
}

void gps_display(void) {
  // M5.Lcd.clear();
  M5.Lcd.setCursor(0, 100);
  M5.lcd.println("                ");
  M5.lcd.println("                ");
  M5.Lcd.setCursor(0, 100);
  M5.Lcd.printf("%04d-%02d-%02d\n",
    gps.date.year(), // Year (2000+) (u16)
    gps.date.month(), // Month (1-12) (u8)
    gps.date.day());
  M5.Lcd.printf("%02d:%02d:%02d\n",
    gps.time.hour(),
    gps.time.minute(),
    gps.time.second());
}

void rtc_display(void) {
  // M5.Lcd.clear();
  M5.Lcd.setCursor(0, 100);
  M5.lcd.println("                ");
  M5.lcd.println("                ");
  M5.Lcd.setCursor(0, 100);
  M5.Lcd.printf("%04d-%02d-%02d\n",
                dateStruct.year,
                dateStruct.month,
                dateStruct.date);
  M5.Lcd.printf("%02d:%02d:%02d\n",
                timeStruct.hours,
                timeStruct.minutes,
                timeStruct.seconds
               );
  Serial.print(".");
}

void rtc_program(void) {
  // Set RTC time
  timeStruct.hours   = gps.time.hour();
  timeStruct.minutes = gps.time.minute();
  timeStruct.seconds = gps.time.second();
  rtc.setTime(&timeStruct);
  // Set RTC Date
  dateStruct.weekDay = 0;  // not sure what to do about this
  dateStruct.month   = gps.date.month();
  dateStruct.date    = gps.date.day();
  dateStruct.year    = gps.date.year();
  rtc.setDate(&dateStruct);
}
