/**
 * Experiment with ublox commands on BN-880 GNSS receiver.
 * This test sends the UBX-MON-VER command to read out the firmware and protocol versions.
*/

#include <M5Core2.h>
// #include <Arduino.h>
#include <string>
#include <TinyGPS++.h>
#include <RBD_Timer.h>
#include <I2C_BM8563.h> // RTC

RBD::Timer loggingtime{2000};  // stop logging after this delay.

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


typedef struct {
  uint8_t cka;
  uint8_t ckb;
} Checksum;

/** 
 * @brief 8-bit Fletcher checksum calculation
 * @ref RFC 1145 https://www.ietf.org/rfc/rfc1145.txt
 * @ref U-Blox manual section 32.4 UBX Checksum
 * @param cksum struct of CK_A and CK_B (as defined in U-Blox manual)
 * @param payload pointer to first byte in checksummable payload (excludes sync chars)
 * @param len length of checksummable payload (n bytes)
 * @return void
*/
void fletcher8(Checksum& cksum, const uint8_t* payload, size_t len);


/* UBX Commands 
 * last two bytes are 00 to be filled in with the checksum
*/
uint8_t MON_VER[] = {0xB5, 0x62, 0x0A, 0x04, 0x00, 0x00, 0x00, 0x00}; // Ref 32.16.13 UBX-MON-VER (0x0A 0x04)
const PROGMEM  uint8_t ClearConfig[] = {0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x01, 0x19, 0x98};



void setup() {
  M5.begin();
  M5.Lcd.clear();
  M5.lcd.setRotation(3);
  M5.Lcd.setTextFont(2);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.setCursor(0, 10);

/*   // Init I2C
  Wire1.begin(BM8563_I2C_SDA, BM8563_I2C_SCL);
  // Init RTC
  rtc.begin();
  rtc.getTime(&timeStruct);
  M5.Lcd.printf("%02d:%02d:%02d\n",
                timeStruct.hours,
                timeStruct.minutes,
                timeStruct.seconds
               );
 */
/* 
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
 */
/* 
  fd = SD.open(filename, "w");  // make an empty file
  if(!fd) {
    M5.Lcd.printf("%s\n", filename);
    M5.Lcd.println("No log file, stalling");
    while(1);
  }
  fd.printf("Start millis() = %d\n", millis());
  fd.close();
 */

  Serial.begin(115200); // to/from terminal
  Serial2.begin(GPSBaud); // to/from GPS

  //Serial.println("Hello GPS Test");
  M5.Lcd.println("Hello GPS Test");
  loggingtime.restart();

  // ClearConfig[2]
  Checksum sum{};
  fletcher8(sum, &ClearConfig[2], sizeof(ClearConfig)-4);
  M5.Lcd.printf("checksum %02x, %02x\n", sum.cka, sum.ckb);

  // Compose UBX-MON-VER message
  M5.Lcd.println("Sending MON-VER command");
  fletcher8(sum, &MON_VER[2], sizeof(MON_VER)-4);
  int index = sizeof(MON_VER) - 2;
  MON_VER[index] = sum.cka;
  MON_VER[index+1] = sum.ckb;
  for(index = 0; index < sizeof(MON_VER); ++index) {
    M5.Lcd.printf("%02x ", MON_VER[index]);
    Serial2.write(MON_VER[index]);
  }
  loggingtime.restart();
}

void loop() {
  uint8_t c = 0;
  if(!loggingtime.isExpired()) {
    while (Serial2.available() > 0) {
      c = Serial2.read();
      if((0 == c) || (0x0d == c)) {
        // do nothing
      }
      else if (0x0a == c) {
        M5.Lcd.println("");
        Serial.println("");
      }
      else if(0x20 <= c && c <= 0x7f) {
        Serial.printf("%c", c);
        M5.Lcd.printf("%c", c);
      } 
      else {
        Serial.printf("[%02x]", c);
        M5.Lcd.printf("[%02x]", c);
      }
    }
  }
}


/* void loop() {
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

}

 */// 8-bit checksum
void fletcher8(Checksum& cksum, const uint8_t* payload, size_t len) {
  uint8_t cka = 0;
  uint8_t ckb = 0;

  for(int i=0; i<len; ++i)
  {
    cka = cka + payload[i];
    ckb = ckb + cka;
  }
  cksum.cka = cka;
  cksum.ckb = ckb;
  return;
}