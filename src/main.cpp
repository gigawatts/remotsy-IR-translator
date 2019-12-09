/*
 * Infrared Translator
 * Running on Remotsy ESP-12E hardware
 * Some code borrowed from https://www.instructables.com/id/Arduino-Based-Remote-Translator/
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>

//#define NEOPIXEL_ENABLE  // Enables Neopixel stuff
//#define NEOPIXEL_DEBUG   // Enables Neopixel flashes with IR activity
//#define NEOPIXEL_TIMEOUT // Enables LED Amp timeout indicator
//#define SERIAL_ENABLE

#ifdef NEOPIXEL_ENABLE
  #include <Adafruit_NeoPixel.h>
  Adafruit_NeoPixel LED = Adafruit_NeoPixel(1, 14, NEO_GRB + NEO_KHZ800);
  uint32_t LED_ON = 0x000005;
  uint32_t LED_REPEAT = 0x050000;
  uint32_t LED_TIMEOUT = 0x050500;
#endif

#ifndef STASSID
#define STASSID "myNetwork"
#define STAPSK  "badPassword"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;
const char* host = "remotsy";

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

const int RECV_PIN = 5;
const int SEND_PIN = 4;
int repeat = 2; 

unsigned long currcode;
unsigned long prevcode;

unsigned long lastcode;
String lasttype;
unsigned long ircode;

static unsigned long lastAmpTime;
static unsigned long AMPtimeout = 4000;

//unsigned long REPEAT = 0xFFFFFFFF;
//uint64_t NEC_REPEAT = 0xFFFFFFFFFFFFFFFF;
//uint16_t rawRepeat[3] = {9026, 2248,  574};

// LG tv codes
unsigned long TVpower = 0x20DF10EF;
unsigned long TVvolup = 0x20DF40BF;
unsigned long TVvoldown = 0x20DFC03F;
unsigned long TVmute = 0x20DF906F;
unsigned long TVinfo = 0x20DF55AA;
unsigned long TVred = 0x20DF4EB1;
unsigned long TVgreen = 0x20DF8E71;
unsigned long TVyellow = 0x20DFC639;
unsigned long TVblue = 0x20DF8679;
unsigned long TVok = 0x20DF22DD;
unsigned long TVup = 0x20DF02FD;
unsigned long TVdown = 0x20DF827D;
unsigned long TVleft = 0x20DFE01F;
unsigned long TVright = 0x20DF609F;
unsigned long TVback = 0x20DF14EB;
unsigned long TVexit = 0x20DFDA25;
unsigned long TVhome = 0x20DF3EC1;
unsigned long TVnetflix = 0x20DF6A95;
unsigned long TVamazon = 0x20DF3AC5;
unsigned long TVrewind = 0x20DFF10E;
unsigned long TVfastfwd = 0x20DF718E;
unsigned long TVplay = 0x20DF0DF2;
unsigned long TVpause = 0x20DF5DA2;
unsigned long TVstop = 0x20DF8D72;
//unsigned long TV = 0x;

// Harmon/Kardon amplifier codes
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

// Start Functions ------------------------------------------------
void handleNotFound(void)
{
	String message = "File Not Found\n\n";
	message += "URI: ";
	message += httpServer.uri();
	message += "\nMethod: ";
	message += (httpServer.method() == HTTP_GET) ? "GET" : "POST";
	message += "\nArguments: ";
	message += httpServer.args();
	message += "\n";
	for (uint8_t i=0; i<httpServer.args(); i++)
	{
		message += " " + httpServer.argName(i) + ": " + httpServer.arg(i) + "\n";
	}
	httpServer.send(404, "text/plain", message);
}

void setupWebServer(void)
{
  httpServer.on("/rx", []() {
    char hexbuff[15];
    ltoa(lastcode, hexbuff, 16);

    for(int i=0; i<strlen(hexbuff); i++){
        hexbuff[i] = toupper(hexbuff[i]);
    }

    String message;
    message += "<html><body>Last code<br> \
    \nType: " + lasttype + 
    "\n<br>Code: 0x"+ (String) hexbuff +"<body></html>";

    httpServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    httpServer.sendHeader("Pragma", "no-cache");
    httpServer.sendHeader("Expires", "-1");
    httpServer.send(200, "text/html", message);
  });

	httpServer.on("/", []() {
		#ifdef SERIAL_ENABLE
      Serial.print("HTTP REQUEST > ");
    #endif

		for (uint8_t i = 0; i < httpServer.args(); i++) {
			/*
      if (httpServer.argName(i) == "ircode") {
        //ircode = (byte) httpServer.arg(i);
        //ircode = atol(httpServer.argName(i).c_str());
        //unsigned long ircode = strtoul(httpServer.argName(i), NULL, 16);

        unsigned long hexCode = (byte) httpServer.arg(i).toInt();
        //unsigned long intCode = (hexCode);
        //irsend.sendNEC(intCode, 32);
        #ifdef SERIAL_ENABLE
          Serial.print("hexCode==");
          Serial.print(hexCode);
          Serial.print("==  ");
        #endif
        irsend.sendNEC(hexCode, 32);
        delay(20);

				//ircode = (byte) httpServer.arg(i).toInt();
        //#ifdef SERIAL_ENABLE
        //  Serial.print("ircode: ");
        //  Serial.println(ircode);
        //#endif
			}
      
      else */ if (httpServer.argName(i) == "irname") {
        if ( httpServer.arg(i) == "TVpower") irsend.sendNEC(TVpower, 32);
        if ( httpServer.arg(i) == "TVup") irsend.sendNEC(TVup, 32);
        if ( httpServer.arg(i) == "TVdown") irsend.sendNEC(TVdown, 32);
        if ( httpServer.arg(i) == "TVleft") irsend.sendNEC(TVleft, 32);
        if ( httpServer.arg(i) == "TVright") irsend.sendNEC(TVright, 32);
        if ( httpServer.arg(i) == "TVhome") irsend.sendNEC(TVhome, 32);
        if ( httpServer.arg(i) == "TVback") irsend.sendNEC(TVback, 32);
        if ( httpServer.arg(i) == "TVexit") irsend.sendNEC(TVexit, 32);
        if ( httpServer.arg(i) == "TVok") irsend.sendNEC(TVok, 32);
        if ( httpServer.arg(i) == "TVplay") irsend.sendNEC(TVplay, 32);
        if ( httpServer.arg(i) == "TVpause") irsend.sendNEC(TVpause, 32);
        if ( httpServer.arg(i) == "TVstop") irsend.sendNEC(TVstop, 32);
        if ( httpServer.arg(i) == "TVrewind") irsend.sendNEC(TVrewind, 32);
        if ( httpServer.arg(i) == "TVfastfwd") irsend.sendNEC(TVfastfwd, 32);
        if ( httpServer.arg(i) == "TVnetflix") irsend.sendNEC(TVnetflix, 32);
        if ( httpServer.arg(i) == "TVamazon") irsend.sendNEC(TVamazon, 32);
        if ( httpServer.arg(i) == "AMPpoweron") irsend.sendNEC(AMPpoweron, 32);
        if ( httpServer.arg(i) == "AMPpoweroff") irsend.sendNEC(AMPpoweroff, 32);
        if ( httpServer.arg(i) == "AMPvolup") irsend.sendNEC(AMPvolup, 32);
        if ( httpServer.arg(i) == "AMPvoldown") irsend.sendNEC(AMPvoldown, 32);
        if ( httpServer.arg(i) == "AMPmute") irsend.sendNEC(AMPmute, 32);
        delay(20);
      }
			else {
				#ifdef SERIAL_ENABLE
          Serial.println("unknown argument! ");
        #endif
			}

			#ifdef SERIAL_ENABLE
        Serial.print(httpServer.argName(i));
        Serial.print(": ");
        Serial.print(httpServer.arg(i));
        Serial.print(" > ");
      #endif
		}
		#ifdef SERIAL_ENABLE
      Serial.println("done");
    #endif

		String message;
    message += "<html>\n<head>\n \
    <meta charset='utf-8'>\n \
    <meta name='apple-mobile-web-app-capable' content='yes' />\n \
    <meta name='viewport' content='width=device-width, initial-scale=1'>\n \
    <link rel='shortcut icon' type='image/png' href='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAA+ElEQVQ4EYWSTwrBQRTHPz85gK0s3cBCycIlrCwUJ1AWzqAUZ1AWcgoLWSg3sJSysraQvszzm9+Y4dU0773vn96bJiMeTWAMlBz8AGbAIaQbwe9XgAGwAm7uKB8CwgqRFaq8qAIjYOdabWABXHLKO0sZCK0DfSdYAqdQrDpm0HDiGF8mRx+IvcHEJwT5F1YOCFbeAZFtbK0zNdC/YxP4+N88NcEemAcTqNcLHVMGHaAFdJ3gHAqtTq2g3WvA1h3l9h6mfd0pgwLpV5FaQa+usbWKQrl6X5Ey0OgKG1ti9exNHAwpgyuw+bCC3+f1owb6RGuf5OXCCvEED6Ik3zsG3BQAAAAASUVORK5CYII=' />\n \
    <link rel='apple-touch-icon' sizes='72x72' href='data:image/png;base64,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' />\n \
    <style>\n \
      body {\n \
        background-color: black;\n \
        color: #ccc;\n \
      }\n \
      input[type='submit']{\n \
        background-color: #676767; color: white;\n \
        border: none;\n \
        text-align: center;\n \
        text-decoration: none;\n \
        display: inline-block;\n \
        font-size: 16px;\n \
        margin: 4px 2px;\n \
        cursor: pointer;\n \
      }\n \
    </style>\n \
    <title>IR</title>\n</head>\n\n";
    message += "<body>\n<center>\n<font face='sans-serif'>\n";
    message += "<h2>IR &#x1f4fa; Control</h2>\n"; // 📺 (Television)
    
    //int btn_w = 116;
    int btn_w = 87;
    int btn_h = 50;
    message += "<table><tr><td>";
    message += "<form action='/' method='post'><input type='hidden' name='irname' value='TVpower'><input type='submit' value='TV Power' style='height:"+(String) btn_h+"px; width:"+(String) btn_w+"px'></form>\n";
    message += "</td><td>";
    message += "<form action='/' method='post'><input type='hidden' name='irname' value='AMPpoweron'><input type='submit' value='Amp On' style='height:"+(String) btn_h+"px; width:"+(String) btn_w+"px'></form>\n";
    message += "</td><td>";
    message += "<form action='/' method='post'><input type='hidden' name='irname' value='AMPpoweroff'><input type='submit' value='Amp Off' style='height:"+(String) btn_h+"px; width:"+(String) btn_w+"px'></form>\n";
    message += "</td><td>";
    message += "<form action='/' method='post'><input type='hidden' name='irname' value='AMPmute'><input type='submit' value='Mute' style='height:"+(String) btn_h+"px; width:"+(String) btn_w+"px'></form>\n";
    message += "</td></tr>";
    
    message += "<tr><td>";
    message += "<form action='/' method='post'><input type='hidden' name='irname' value='TVnetflix'><input type='submit' value='Netflix' style='height:"+(String) btn_h+"px; width:"+(String) btn_w+"px'></form>\n";
    message += "</td><td>";
    message += "<form action='/' method='post'><input type='hidden' name='irname' value='TVhome'><input type='submit' value='Home' style='height:"+(String) btn_h+"px; width:"+(String) btn_w+"px'></form>\n";
    message += "</td><td>";
    message += "<form action='/' method='post'><input type='hidden' name='irname' value='TVamazon'><input type='submit' value='Amazon' style='height:"+(String) btn_h+"px; width:"+(String) btn_w+"px'></form>\n";
    message += "</td><td>";
    message += "<form action='/' method='post'><input type='hidden' name='irname' value='AMPvolup'><input type='submit' value='Vol +' style='height:"+(String) btn_h+"px; width:"+(String) btn_w+"px'></form>\n";
    message += "</td></tr>";
    
    message += "<tr><td>";
    message += "<form action='/' method='post'><input type='hidden' name='irname' value='TVrewind'><input type='submit' value='&#x25C0;&#x25C0;' style='height:"+(String) btn_h+"px; width:"+(String) btn_w+"px'></form>\n";
    message += "</td><td>";
    message += "</td><td>";
    message += "<form action='/' method='post'><input type='hidden' name='irname' value='TVfastfwd'><input type='submit' value='&#x25B6;&#x25B6;' style='height:"+(String) btn_h+"px; width:"+(String) btn_w+"px'></form>\n";
    message += "</td><td>";
    message += "<form action='/' method='post'><input type='hidden' name='irname' value='AMPvoldown'><input type='submit' value='Vol -' style='height:"+(String) btn_h+"px; width:"+(String) btn_w+"px'></form>\n";
    message += "</td></tr>";
    
    message += "<tr><td>";
    message += "</td><td>";
    message += "<form action='/' method='post'><input type='hidden' name='irname' value='TVup'><input type='submit' value='&#x21E7;' style='font-size: 25px; height:"+(String) btn_h+"px; width:"+(String) btn_w+"px'></form>\n";
    message += "</td><td>";
    message += "</td><td>";
    message += "<form action='/' method='post'><input type='hidden' name='irname' value='TVplay'><input type='submit' value='&#x25B6;' style='height:"+(String) btn_h+"px; width:"+(String) btn_w+"px'></form>\n";
    message += "</td></tr>";

    message += "<tr><td>";
    message += "<form action='/' method='post'><input type='hidden' name='irname' value='TVleft'><input type='submit' value='&#x21E6;' style='font-size: 25px; height:"+(String) btn_h+"px; width:"+(String) btn_w+"px'></form>\n";
    message += "</td><td>";
    message += "<form action='/' method='post'><input type='hidden' name='irname' value='TVok'><input type='submit' value='OK' style='height:"+(String) btn_h+"px; width:"+(String) btn_w+"px'></form>\n";
    message += "</td><td>";
    message += "<form action='/' method='post'><input type='hidden' name='irname' value='TVright'><input type='submit' value='&#x21E8;' style='font-size: 25px; height:"+(String) btn_h+"px; width:"+(String) btn_w+"px'></form>\n";
    message += "</td><td>";
    message += "<form action='/' method='post'><input type='hidden' name='irname' value='TVpause'><input type='submit' value='&#10074;&#10074;' style='height:"+(String) btn_h+"px; width:"+(String) btn_w+"px'></form>\n";
    message += "</td></tr>";

    message += "<tr><td>";
    message += "<form action='/' method='post'><input type='hidden' name='irname' value='TVback'><input type='submit' value='Back' style='height:"+(String) btn_h+"px; width:"+(String) btn_w+"px'></form>\n";
    message += "</td><td>";
    message += "<form action='/' method='post'><input type='hidden' name='irname' value='TVdown'><input type='submit' value='&#x21E9;' style='font-size: 25px; height:"+(String) btn_h+"px; width:"+(String) btn_w+"px'></form>\n";
    message += "</td><td>";
    message += "<form action='/' method='post'><input type='hidden' name='irname' value='TVexit'><input type='submit' value='Exit' style='height:"+(String) btn_h+"px; width:"+(String) btn_w+"px'></form>\n";
    message += "</td><td>";
    message += "<form action='/' method='post'><input type='hidden' name='irname' value='TVstop'><input type='submit' value='&#x25FC;' style='height:"+(String) btn_h+"px; width:"+(String) btn_w+"px'></form>\n";
    message += "</td></tr>";
    message += "</table>";

    //message += "<form action='/' method='get'><input type='text' name='ircode' size='15' value=''><input type='submit' value='Hex code'></form>\n";
    message += "</body>\n</center>\n</html>";

    httpServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    httpServer.sendHeader("Pragma", "no-cache");
    httpServer.sendHeader("Expires", "-1");
    httpServer.send(200, "text/html", message);
	});
  
  httpServer.onNotFound(handleNotFound);

	httpServer.begin();
	#ifdef SERIAL_ENABLE
    Serial.println("HTTP server started");
  #endif
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

  void doIrStuff(void)
  {
    unsigned long now = millis();
    // The amp eats the first volume code sent when idle, requiring you to send it twice
    // When not idle, the amp responds to each volume code sent
    // After 'AMPtimeout' seconds, the amp returns to an idle state
    if (repeat == 1 && now - lastAmpTime >= AMPtimeout) {
      repeat = 2;
      #ifdef SERIAL_ENABLE
        Serial.println("AMP Timeout");
      #endif
      #ifdef NEOPIXEL_TIMEOUT
        LED.setPixelColor(0, 0); LED.show();
      #endif
    }

    if (irrecv.decode(&results)) {
      if (results.decode_type != UNKNOWN ) {
        if (!results.repeat) {
          currcode = results.value;
          // Store the last recieved code value and type so it can be displayed on the /rx page
          lastcode = results.value;
          lasttype = results.decode_type;
        } else {
            currcode = 0;
        }

        // Translate and send codes to the amp when specific tv remote buttons are pressed
        // Also re-transmit some amp codes so the 'blocked' Amp IR receiver still works with its native remote
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
        
        else if (currcode == TVpower)
        {
          #ifdef SERIAL_ENABLE
            if (currcode == TVpower) Serial.print("Received TVpower, ");
            Serial.print("Sending Amplifier Power On, ");
            Serial.print("Repeat: "); Serial.println(repeat);
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
      
      // Reset timeout counter if any volume command is received
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
  }
// End Functions ------------------------------------------------

void setup()
{
  //WiFi.mode( WIFI_OFF );
  //WiFi.forceSleepBegin();

  WiFi.mode(WIFI_STA);
  WiFi.hostname(host);
  WiFi.begin(ssid, password);

  #ifdef SERIAL_ENABLE
    //Serial.begin(9600);
    Serial.begin(115200);
    Serial.println("");
    Serial.println("Starting ESP8266 Infrared Translator ...");
  #endif

  irrecv.enableIRIn();
  irsend.begin();
  #ifdef NEOPIXEL_ENABLE
    LED.begin();
  #endif
  //irrecv.blink13(true);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    #ifdef SERIAL_ENABLE
      Serial.println("WiFi failed, retrying.");
    #endif
  }

  #ifdef SERIAL_ENABLE
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  #endif

  MDNS.begin(host);
  httpUpdater.setup(&httpServer);
  setupWebServer();
  MDNS.addService("http", "tcp", 80);

  #ifdef SERIAL_ENABLE
    Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", host);
  #endif
} // End setup

void loop() {
  httpServer.handleClient();
  MDNS.update();
  doIrStuff();
} // END loop