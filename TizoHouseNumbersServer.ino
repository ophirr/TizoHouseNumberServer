/* version 0.2 */

#include "SPI.h"
#include "Ethernet.h"
#include "LPD8806.h"
#include "WebServer.h"


/* CHANGE THIS TO YOUR OWN UNIQUE VALUE.  The MAC number should be
 * different from any other devices on your network or you'll have
 * problems receiving packets. */
static uint8_t mac[] = { 0x02, 0xAA, 0xBB, 0xCC, 0xAA, 0x22 };

/* CHANGE THIS TO MATCH YOUR HOST NETWORK.  Most home networks are in
 * the 192.168.0.XXX or 192.168.1.XXX subrange.  Pick an address
 * that's not in use and isn't going to be automatically allocated by
 * DHCP from your router. */
static uint8_t ip[] = { 192, 168, 199, 27 };

/* all URLs on this server will start with /buzz because of how we
 * define the PREFIX value.  We also will listen on port 80, the
 * standard HTTP service port */
#define PREFIX "/tizo"
WebServer webserver(PREFIX, 80);

/* What do we want to do? */
int mode = 0;
int interval = 0;
int debug = 1;

// Choose which 2 pins you will use for output.
// Can be any valid output pins. If using SPI (fast) use pins 11 (data) and 13 (clock)
int dataPin = 2;   
int clockPin = 3; 

// Set the first variable to the NUMBER of pixels. 32 = 32 pixels in a row
// The LED strips are 32 LEDs per meter but you can extend/cut the strip
 LPD8806 strip = LPD8806(24, dataPin, clockPin);
// LPD8806 strip = LPD8806(8);

// Previous time
long previousMillis = 0; 


/* This command is set as the default command for the server.  It
 * handles both GET and POST requests.  For a GET, it returns a simple
 * page with some buttons.  For a POST, it saves the value posted to
 * the buzzDelay variable, affecting the output of the speaker */
void buzzCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  if (type == WebServer::POST)
  {
    bool repeat;
    char name[16], value[16];
    do
    {
      /* readPOSTparam returns false when there are no more parameters
       * to read from the input.  We pass in buffers for it to store
       * the name and value strings along with the length of those
       * buffers. */
      repeat = server.readPOSTparam(name, 16, value, 16);
      
      Serial.print(name);
      Serial.print(" : ");
      Serial.println(value);
      
      /* this is a standard string comparison function.  It returns 0
       * when there's an exact match.  We're looking for a parameter
       * named "tizo" here. */
      if (strcmp(name, "mode") == 0)
      {
	/* use the STRing TO Unsigned Long function to turn the string
	 * version of the delay number into our integer modeLitel
	 * variable */
        mode = strtoul(value, NULL, 10);
      } else if (strcmp(name, "interval") == 0)
      {
        interval = strtoul(value, NULL, 10);
      }
    } while (repeat);

    // after procesing the POST data, tell the web browser to reload
    // the page using a GET method. 
    server.httpSeeOther(PREFIX);
    return;
  }    

  /* for a GET or HEAD, send the standard "it's all OK headers" */
  server.httpSuccess();

  /* we don't output the body for a HEAD request */
  if (type == WebServer::GET)
  {
    /* store the HTML in program memory using the P macro */
    P(message) = 
      "<html><head><title>Tizo Numbers Control</title>"
      "<body>"
      "<h1><center>Tizo Numbers Control Panel</center></h1>"
      "<form action='/tizo' method='POST'>"
      "<p><p><p>"
      "<center>"
      "Choose Color Mode "
      "<select name=\"mode\"<p><p>"
      "<p><option value=\"100\" selected=\"selected\">Off</option><p><p>"
      "<p><option value=\"110\">Red</option><p><p>"
      "<p><option value=\"120\">Yellow</option><p><p>"
      "<p><option value=\"130\">Green</option><p><p>"
      "<p><option value=\"140\">Teal</option><p><p>"
      "<p><option value=\"150\">Blue</option><p><p>"
      "<p><option value=\"160\">Violet</option><p><p>"
      "<p><option value=\"300\">Rainbow Transitions</option><p><p>"
      "<p><option value=\"310\">Rainbow Letters</option><p><p>"
      "<p><option value=\"320\">EMERGENCY</option><p><p>"
      "<p></select></p></p>"
      "Choose Interval in Seconds "
      "<select name=\"interval\"<p><p>"
      "<p><option value=\"1\" selected=\"selected\">instant</option><p><p>"
      "<p><option value=\"5\" >5 seconds</option><p><p>"
      "<p><option value=\"10\">10 seconds</option><p><p>"
      "<p><option value=\"20\">20 seconds</option><p><p>"
      "<p><option value=\"30\">30 seconds</option><p><p>"
      "<p><option value=\"60\">60 seconds</option><p><p>"
      "<p><option value=\"120\">120 seconds</option><p><p>"
      "<p></select></p></p>"
      "<INPUT TYPE=submit VALUE=\"Submit\">"
      "</center>"
      "</form></body></html>";
      
    server.printP(message);
  }
}

/* Helper functions */

//Input a value 0 to 384 to get a color value.
//The colours are a transition r - g -b - back to r

uint32_t Wheel(uint16_t WheelPos)
{
  byte r, g, b;
  switch(WheelPos / 128)
  {
    case 0:
      r = 127 - WheelPos % 128;   //Red down
      g = WheelPos % 128;      // Green up
      b = 0;                  //blue off
      break; 
    case 1:
      g = 127 - WheelPos % 128;  //green down
      b = WheelPos % 128;      //blue up
      r = 0;                  //red off
      break; 
    case 2:
      b = 127 - WheelPos % 128;  //blue down 
      r = WheelPos % 128;      //red up
      g = 0;                  //green off
      break; 
  }
  return(strip.Color(r,g,b));
}



void rainbow(uint8_t wait) {
  int i, j;
   
  for (j=0; j < 384; j++) {     // 3 cycles of all 384 colors in the wheel
    for (i=0; i < strip.numPixels(); i++) {
          // current time in milliseconds
      unsigned long currentMillis = millis();
  
       if(currentMillis - previousMillis > wait) {
        // save the last time you blinked the LED 
        previousMillis = currentMillis;   
        strip.setPixelColor(i, Wheel( (i + j) % 384));
      }  
      strip.show();   // write all the pixels out
    }
  }
}

// Slightly different, this one makes the rainbow wheel equally distributed 
// along the chain
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;
  
  for (j=0; j < 384 * 1; j++) {     // 5 cycles of all 384 colors in the wheel
    for (i=0; i < strip.numPixels(); i++) {
      // current time in milliseconds
      unsigned long currentMillis = millis();
  
       if(currentMillis - previousMillis > wait) {
        // save the last time you blinked the LED 
        previousMillis = currentMillis;   
        // tricky math! we use each pixel as a fraction of the full 384-color wheel
        // (thats the i / strip.numPixels() part)
        // Then add in j which makes the colors go around per pixel
        // the % 384 is to make the wheel cycle around
        strip.setPixelColor(i, Wheel( ((i * 384 / strip.numPixels()) + j) % 384) );
      }  
    strip.show();   // write all the pixels out
    }
  }
}

// fill the dots one after the other with said color
// good for testing purposes
void colorWipe(uint32_t c, uint8_t wait) {
  int i;
  
  for (i=0; i < strip.numPixels(); i++) {
    // current time in milliseconds
    unsigned long currentMillis = millis();
  
     if(currentMillis - previousMillis > wait) {
      // save the last time you blinked the LED 
      previousMillis = currentMillis;     
      strip.setPixelColor(i, c);
      strip.show();
     }  
   }
}

// Chase a dot down the strip
// good for testing purposes
void colorChase(uint32_t c, uint8_t wait) {
  int i;
  
  for (i=0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 0);  // turn all pixels off
  } 
  
  for (i=0; i < strip.numPixels(); i++) {
      // current time in milliseconds
      unsigned long currentMillis = millis();
  
      if(currentMillis - previousMillis > wait) {
      // save the last time you blinked the LED 
        previousMillis = currentMillis;   
        strip.setPixelColor(i, c);
        if (i == 0) { 
          strip.setPixelColor(strip.numPixels()-1, 0);
        } else {
          strip.setPixelColor(i-1, 0);
        }
        strip.show();
        delay(wait);
      }
  }
}


void setup()
{
  // Start console  
  Serial.begin(9600);

  // Start up the LED strip
  strip.begin();

  // Update the strip, to start they are all 'off'
  strip.show();
  
  // setup the Ehternet library to talk to the Wiznet board
  Ethernet.begin(mac, ip);

  /* register our default command (activated with the request of
   * http://x.x.x.x/buzz */
  webserver.setDefaultCommand(&buzzCmd);

  /* start the server to wait for connections */
  webserver.begin();

  Serial.print("server at "); Serial.println(Ethernet.localIP()); //debug to serial if server is 0.0.0.0 web boot failed check sheld connections - See more at: http://kellykeeton.com/2013/08/06/webpower/#sthash.RaVG6diY.dpuf
}




void loop()
{
  
  // process incoming connections one at a time forever
  webserver.processConnection();

  // current time in milliseconds
  unsigned long currentMillis = millis();
   
     /* Our interval Case */
    switch (mode) {
    case 1:
      // 0 seconds
        interval = 0;
        break;
    case 5:
      // 5 seconds
        interval = 5;
        break;
    case 10:
      // 10 seconds
       interval = 10;
      break;
   case 20:
      // 20 seconds
       interval = 20;
      break;
    case 30:
      // 30 seconds
       interval = 30;
      break;
    case 60:
      // 60 seconds
       interval = 60;
      break;
   case 120:
      // 120 seconds
       interval = 120;
      break;
    }


     /* Our Color Case for the Lights! */
    switch (mode) {
    case 100:
      // All Pixels OFF
      int i;
      for(i=0; i<strip.numPixels(); i++) strip.setPixelColor(i, 0);
      break;
    case 110:
      //Red Fill
       colorWipe(strip.Color(127,0,0), interval);
      break;
    case 120:
      //Yellow Fill
       colorWipe(strip.Color(127,127,0), interval);
      break;
    case 130:
      //Green Fill
       colorWipe(strip.Color(0,127,0), interval);
      break;
    case 140:
      //Teal Fill
       colorWipe(strip.Color(0,127,127), interval);
      break;
    case 150:
      //Blue Fill
       colorWipe(strip.Color(0,0,127), interval);	
      break;
    case 160:
      //Violet Fill
      colorWipe(strip.Color(127,0,127), interval);
      break;  
     case 300:
      // Rainbow Fill
      rainbow(interval);
      break;    
    case 310:
      //Rainbow Numbers
      rainbowCycle(interval);
      break;    
    case 320:
      //EMERGENCY MODE - RED
      colorChase(strip.Color(127,0,0), 20); 
      break;    
    }
    strip.show();   // write all the pixels out
}


