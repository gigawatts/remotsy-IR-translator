/*
 * Infrared Translator
 * Running on Remotsy ESP-12E hardware
 * Code mostly taken from https://www.instructables.com/id/Arduino-Based-Remote-Translator/
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>

//#define SERIAL_ENABLE
//#define NEOPIXEL_ENABLE  // Enables Neopixel stuff
//#define NEOPIXEL_DEBUG   // Enables Neopixel flashes with IR activity
//#define NEOPIXEL_TIMEOUT // Enables LED Amp timeout indicator

#ifdef NEOPIXEL_ENABLE
  #include <Adafruit_NeoPixel.h>
  Adafruit_NeoPixel LED = Adafruit_NeoPixel(1, 14, NEO_GRB + NEO_KHZ800);
  uint32_t LED_ON = 0x000005;
  uint32_t LED_REPEAT = 0x050000;
  uint32_t LED_TIMEOUT = 0x050500;
#endif

const int RECV_PIN = 5;
const int SEND_PIN = 4;
int repeat = 2; 

unsigned long currcode;
unsigned long prevcode;
static unsigned long lastAmpTime;
static unsigned long AMPtimeout = 4000;

//unsigned long REPEAT = 0xFFFFFFFF;
//uint64_t NEC_REPEAT = 0xFFFFFFFFFFFFFFFF;
//uint16_t rawRepeat[3] = {9026, 2248,  574};

// tv codes
unsigned long TVpower = 0x20DF10EF;
unsigned long TVvolup = 0x20DF40BF;
unsigned long TVvoldown = 0x20DFC03F;
unsigned long TVmute = 0x20DF906F;
//unsigned long TVinfo = 0x20DF55AA;
unsigned long TVred = 0x20DF4EB1;
unsigned long TVgreen = 0x20DF8E71;
//unsigned long TVyellow = 0x20DFC639;
//unsigned long TVblue = 0x20DF8679;
//unsigned long TV = 0x;

// amplifier codes
unsigned long AMPpoweron = 0x10E03FC;
unsigned long AMPpoweroff = 0x10EF906;
unsigned long AMPvolup = 0x10EE31C;
unsigned long AMPvoldown = 0x10E13EC;
unsigned long AMPmute = 0x10E837C;
//uint16_t AMPrawMute[68] = {9024, 4512, 564, 564, 564, 564, 564, 564, 564, 564, 564, 564, 564, 564, 564, 564, 564, 1692, 564, 564, 564, 564, 564, 564, 564, 564, 564, 1692, 564, 1692, 564, 1692, 564, 564, 564, 1692, 564, 564, 564, 564, 564, 564, 564, 564, 564, 564, 564, 1692, 564, 1692, 564, 564, 564, 1692, 564, 1692, 564, 1692, 564, 1692, 564, 1692, 564, 564, 564, 564, 564, 44268};
//unsigned long AMPavr = 0x10E03FC;
//unsigned long AMP = 0x;

IRrecv irrecv(RECV_PIN);
IRsend irsend(4); 
decode_results results;

void setup()
{
  WiFi.mode( WIFI_OFF );
  WiFi.forceSleepBegin();
  #ifdef SERIAL_ENABLE
    //Serial.begin(9600);
    Serial.begin(115200);
    Serial.println("");
    Serial.println("ESP8266 Infrared Traslator");
  #endif
  irrecv.enableIRIn();
  irsend.begin();
  #ifdef NEOPIXEL_ENABLE
    LED.begin();
  #endif
  //irrecv.blink13(true);
}

void dump(decode_results *results) {
    // Dumps out the decode_results structure.
    // Call this after IRrecv::decode()
    uint16_t count = results->rawlen;
    if (results->decode_type == UNKNOWN) {
      Serial.print("Unknown encoding: ");
    } else if (results->decode_type == NEC) {
      Serial.print("Decoded NEC: ");
    } else if (results->decode_type == SONY) {
      Serial.print("Decoded SONY: ");
    } else if (results->decode_type == RC5) {
      Serial.print("Decoded RC5: ");
    } else if (results->decode_type == RC5X) {
      Serial.print("Decoded RC5X: ");
    } else if (results->decode_type == RC6) {
      Serial.print("Decoded RC6: ");
    } else if (results->decode_type == RCMM) {
      Serial.print("Decoded RCMM: ");
    } else if (results->decode_type == PANASONIC) {
      Serial.print("Decoded PANASONIC - Address: ");
      Serial.print(results->address, HEX);
      Serial.print(" Value: ");
    } else if (results->decode_type == LG) {
      Serial.print("Decoded LG: ");
    } else if (results->decode_type == JVC) {
      Serial.print("Decoded JVC: ");
    } else if (results->decode_type == AIWA_RC_T501) {
      Serial.print("Decoded AIWA RC T501: ");
    } else if (results->decode_type == WHYNTER) {
      Serial.print("Decoded Whynter: ");
    }
    serialPrintUint64(results->value, 16);
    Serial.print(" (");
    Serial.print(results->bits, DEC);
    Serial.print(" bits) ");
    Serial.print("Raw (");
    Serial.print(count, DEC);
    Serial.print("): ");

    for (uint16_t i = 1; i < count; i++) {
      if (i % 100 == 0)
        yield();  // Preemptive yield every 100th entry to feed the WDT.
      Serial.print(" ");
    }
    Serial.println();
  } //END dump

void loop() {
  unsigned long now = millis();
  if (repeat == 1 && now - lastAmpTime >= AMPtimeout) {
    #ifdef SERIAL_ENABLE
      Serial.println("AMP Timeout");
    #endif
    repeat = 2;
      #ifdef NEOPIXEL_TIMEOUT
        LED.setPixelColor(0, 0); LED.show();
      #endif
  }

  if (irrecv.decode(&results)) {
    if (results.decode_type != UNKNOWN ) {
      if (!results.repeat) {
        currcode = results.value;
      } else {
          currcode = 0;
      }

      if (currcode == TVmute || currcode == AMPmute)
      {
        #ifdef SERIAL_ENABLE
          if (currcode == TVmute) Serial.print("Received TVmute, ");
          if (currcode == AMPmute) Serial.print("Received AMPmute, ");
          Serial.println("Sending Amplifier Volume Mute");
        #endif
        irsend.sendNEC(AMPmute, 32);
        //irsend.sendRaw(AMPrawMute,68,38000);
        #ifdef NEOPIXEL_DEBUG
          LED.setPixelColor(0, LED_ON); LED.show();
        #endif
        delay(50);
        #ifdef NEOPIXEL_DEBUG
          LED.setPixelColor(0, 0); LED.show();
        #endif
      }
      
      else if (currcode == TVvolup || currcode == AMPvolup)
      {
        #ifdef SERIAL_ENABLE
          if (currcode == TVvolup) Serial.print("Received TVvolup, ");
          if (currcode == AMPvolup) Serial.print("Received AMPvolup, ");
          Serial.print("Sending Amplifier Volume Up, ");
          Serial.print("Repeat: "); Serial.println(repeat);
        #endif
        for (int i = 0; i < repeat; i++)
        {
          irsend.sendNEC(AMPvolup, 32);
          #ifdef NEOPIXEL_DEBUG
            LED.setPixelColor(0, LED_ON); LED.show();
          #endif
          delay(50);
          #ifdef NEOPIXEL_DEBUG
            LED.setPixelColor(0, 0); LED.show();
          #endif
        }
      }
      
      else if (currcode == TVvoldown || currcode == AMPvoldown)
      {
        #ifdef SERIAL_ENABLE
          if (currcode == TVvoldown) Serial.print("Received TVvoldown, ");
          if (currcode == AMPvoldown) Serial.print("Received AMPvoldown, ");
          Serial.print("Sending Amplifier Volume Down");
          Serial.print("Repeat: "); Serial.println(repeat);
        #endif
        for (int i = 0; i < repeat; i++)
        {
          irsend.sendNEC(AMPvoldown, 32);
          #ifdef NEOPIXEL_DEBUG
            LED.setPixelColor(0, LED_ON); LED.show();
          #endif
          delay(50);
          #ifdef NEOPIXEL_DEBUG
            LED.setPixelColor(0, 0); LED.show();
          #endif
        }
      }
      
      else if (currcode == TVred || currcode == AMPpoweroff)
      {
        #ifdef SERIAL_ENABLE
          if (currcode == TVred) Serial.print("Received TVred, ");
          if (currcode == AMPpoweroff) Serial.print("Received AMPpoweroff, ");
          Serial.println("Sending Amplifier Power Off");
        #endif
        irsend.sendNEC(AMPpoweroff, 32);
        #ifdef NEOPIXEL_DEBUG
          LED.setPixelColor(0, LED_ON); LED.show();
        #endif
        delay(50);
        #ifdef NEOPIXEL_DEBUG
          LED.setPixelColor(0, 0); LED.show();
        #endif
      }
      
      else if (currcode == TVgreen || currcode == AMPpoweron)
      {
        #ifdef SERIAL_ENABLE
          if (currcode == TVgreen) Serial.print("Received TVgreen, ");
          if (currcode == AMPpoweron) Serial.print("Received AMPpoweron, ");
          Serial.println("Sending Amplifier Power On");
        #endif
        irsend.sendNEC(AMPpoweron, 32);
        #ifdef NEOPIXEL_DEBUG
          LED.setPixelColor(0, LED_ON); LED.show();
        #endif
        delay(50);
        #ifdef NEOPIXEL_DEBUG
          LED.setPixelColor(0, 0); LED.show();
        #endif
      }
      else if (currcode == 0)
      {
        #ifdef SERIAL_ENABLE
          Serial.println("Received Repeat");
        #endif
        // TODO: Send last code received?
        #ifdef NEOPIXEL_DEBUG
          LED.setPixelColor(0, LED_REPEAT); LED.show();
          delay(50);
          LED.setPixelColor(0, 0); LED.show();
        #endif
      }
      else
      {
        // do nothing
        #ifdef SERIAL_ENABLE
          dump(&results);
        #endif
      }
      
    } // END Decode results not unknown
    
    if (currcode == TVvolup || currcode == TVvoldown || currcode == AMPvolup || currcode == AMPvoldown) {
        lastAmpTime = now;
        repeat = 1;
        #ifdef NEOPIXEL_TIMEOUT
          LED.setPixelColor(0, LED_TIMEOUT); LED.show();
        #endif
      }
    prevcode = currcode;
    irrecv.resume();
    irrecv.enableIRIn(); //trust me this has to be done again

  } // END IR decode results

} // END loop