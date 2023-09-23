// Feather9x_TX
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging client (transmitter)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example Feather9x_RX

#include <SPI.h>
#include <RH_RF95.h>
//display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C  ///< See datasheet for Address; 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);


const byte numChars = 16;
const char mmArray[][16] = {"> Exit", "> Submenu", "> Encodertest", "> LED_state:", "> Clock", "> Item_6", "> Item_7", "> Item_8", "> Item_9"};
int line;
int startitem = 0;
int enditem;
int menuitem;

// First 3 here are boards w/radio BUILT-IN. Boards using FeatherWing follow.
#if defined (__AVR_ATmega32U4__)  // Feather 32u4 w/Radio
  #define RFM95_CS    8
  #define RFM95_INT   7
  #define RFM95_RST   4

#elif defined(ADAFRUIT_FEATHER_M0) || defined(ADAFRUIT_FEATHER_M0_EXPRESS) || defined(ARDUINO_SAMD_FEATHER_M0)  // Feather M0 w/Radio
  #define RFM95_CS    8
  #define RFM95_INT   3
  #define RFM95_RST   4

#elif defined(ARDUINO_ADAFRUIT_FEATHER_RP2040_RFM)  // Feather RP2040 w/Radio
  #define RFM95_CS   16
  #define RFM95_INT  21
  #define RFM95_RST  17

#elif defined (__AVR_ATmega328P__)  // Feather 328P w/wing
  #define RFM95_CS    4  //
  #define RFM95_INT   3  //
  #define RFM95_RST   2  // "A"

#elif defined(ESP8266)  // ESP8266 feather w/wing
  #define RFM95_CS    2  // "E"
  #define RFM95_INT  15  // "B"
  #define RFM95_RST  16  // "D"

#elif defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S2) || defined(ARDUINO_NRF52840_FEATHER) || defined(ARDUINO_NRF52840_FEATHER_SENSE)
  #define RFM95_CS   10  // "B"
  #define RFM95_INT   9  // "A"
  #define RFM95_RST  11  // "C"

#elif defined(ESP32)  // ESP32 feather w/wing
  #define RFM95_CS   33  // "B"
  #define RFM95_INT  27  // "A"
  #define RFM95_RST  13

#elif defined(ARDUINO_NRF52832_FEATHER)  // nRF52832 feather w/wing
  #define RFM95_CS   11  // "B"
  #define RFM95_INT  31  // "C"
  #define RFM95_RST   7  // "A"

#endif

/* Some other possible setups include:

// Feather 32u4:
#define RFM95_CS   8
#define RFM95_RST  4
#define RFM95_INT  7

// Feather M0:
#define RFM95_CS   8
#define RFM95_RST  4
#define RFM95_INT  3

// Arduino shield:
#define RFM95_CS  10
#define RFM95_RST  9
#define RFM95_INT  7

// Feather 32u4 w/wing:
#define RFM95_RST 11  // "A"
#define RFM95_CS  10  // "B"
#define RFM95_INT  2  // "SDA" (only SDA/SCL/RX/TX have IRQ!)

// Feather m0 w/wing:
#define RFM95_RST 11  // "A"
#define RFM95_CS  10  // "B"
#define RFM95_INT  6  // "D"
*/
#define RFM95_CS     17  // 
#define RFM95_INT    7  // RFM G0
#define RFM95_RST    8  // 
// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 868.0
bool debug = false;
//bool debug = true;
bool led_state = false;
// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }

  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
if (debug) {
    Serial.begin(115200);
    while (!Serial) delay(1);
    delay(100);
}

//  Serial.println("Feather LoRa TX Test!");

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    if (debug) {
    Serial.println("LoRa radio init failed");
    Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
    }
    while (1);


  }
  if (debug) {
    Serial.println("LoRa radio init OK!");
  }


  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    if (debug) {
      Serial.println("setFrequency failed");
    }
    while (1);
  }
  if (debug) {
    Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  }

  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
}

int16_t packetnum = 0;  // packet counter, we increment per xmission

void loop() {
  
  display.clearDisplay();
  display.setTextSize(1);               // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  //makemenu(startitem, menuitem, mmArray);
  delay(1000); // Wait 1 second between transmits, could also 'sleep' here!
  if (debug) {
    Serial.println("Transmitting..."); // Send a message to rf95_server
  }
  display.print("RFM 9x LoRa-Test");
  display.setCursor(0,16);
  display.print("Transmitting...");
  display.display();
  char radiopacket[20] = "Hello World #      ";
  itoa(packetnum++, radiopacket+13, 10);
  if (debug) {
    Serial.print("Sending "); Serial.println(radiopacket);
  }
  radiopacket[19] = 0;
  if (debug) {
    Serial.println("Sending...");
  }
  delay(10);
  digitalWrite(LED_BUILTIN,true) ;
  rf95.send((uint8_t *)radiopacket, 20);
  display.setCursor(0,26);
  display.print(radiopacket);
  display.display();
  if (debug) {
    Serial.println("Waiting for packet to complete...");
  }
  delay(10);
  rf95.waitPacketSent();
  //led_state = !led_state;
  // Now wait for a reply
  digitalWrite(LED_BUILTIN,false) ;
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  if (debug) {
    Serial.println("Waiting for reply...");
  }


  if (rf95.waitAvailableTimeout(1000)) {
    // Should be a reply message for us now
    if (rf95.recv(buf, &len)) {
      if (debug) {Serial.print("Got reply: ");}
      display.setCursor(0,36);
      display.print("Got reply ...");
      display.display();
      if (debug) {Serial.println((char*)buf);}
      display.setCursor(0,46);
      display.print((char*)buf);
      display.display();
      if (debug) {
        Serial.print("RSSI: ");
        Serial.print(rf95.lastRssi(), DEC);
        Serial.println(" dBm");
        Serial.print("FreqError: ");
        Serial.print(rf95.frequencyError(), DEC);
        Serial.println(" Hz");
        Serial.print("SNR: ");
        Serial.print(rf95.lastSNR(), DEC);
        Serial.println(" dB");
        Serial.print(rf95.printRegisters(), BIN);
      }
      display.setCursor(0,56);
      display.print("RSSI: ");
      display.print(rf95.lastRssi(), DEC);
      display.print(" dBm");
      display.display();
      // display.setCursor(0,25);
      
      // display.print("Receiving ...  ");                     
      // display.display();

    } else {
      if (debug) {Serial.println("Receive failed");}
    }
  } else {
    if (debug) {Serial.println("No reply, is there a listener around?");}
  }
}

// displays Menu handles dispaying selected Item inverted
void makemenu(int &stitem, int &actitem, const char (&menuarray)[][numChars]){    
  // Items Display can display 5 Items [0..4] + Title
    line = 15;
    enditem = stitem + 4;
    for (int item = stitem; item <= enditem ; item++){
      display.setCursor(0, line);
      if (actitem == item) {
        display.setTextColor(BLACK, WHITE);
        }
      else {
      display.setTextColor(SSD1306_WHITE);  
      }  
      display.print(menuarray[item]);
      line = line + 10;
    } 
  display.display();
}
