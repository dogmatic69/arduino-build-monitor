#include <EtherCard.h>

static byte macAdd[] = {0x74, 0x69, 0x69, 0x2D, 0x30,0x31};

/**
 * PWM for 3 colour LED
 *
 * purple (128, 0, 128): powered on
 * red: cant acceess ethernet controller
 * yellow: DHCP failed
 * green: All good
 */
#define LED_R 9
#define LED_G 6
#define LED_B 5

/**
 * Slave Select for Ethernet SPI
 */
#define SLAVE_SELECT 10

/**
 * Digital pin to set alarm off (trigger Transistor)
 */
#define BUILD_ALERT 8

/**
 * Flag for storing build status
 *
 * @var boolean
 */
bool buildHasPassed = true;

/**
 * Flag to store if the alarm has been triggered (wont run untill build passes then breaks again)
 */
bool alerted = true;

/**
 * Buffer for sending / recieving request
 */
byte Ethernet::buffer[500];

BufferFiller bfill;

/**
 * timer variable
 *
 * tracks how long the alert has been running
 */
unsigned long alertStarted = 0;

/**
 * 200 status header
 */
const char http_OK[] PROGMEM =
"HTTP/1.0 200 OK\r\n"
"Content-Type: text/html\r\n"
"Pragma: no-cache\r\n\r\n";

/**
 * 302 status header
 */
const char http_Found[] PROGMEM =
"HTTP/1.0 302 Found\r\n"
"Location: /\r\n\r\n";

/**
 * 401 status header
 */
const char http_Unauthorized[] PROGMEM =
"HTTP/1.0 401 Unauthorized\r\n"
"Content-Type: text/html\r\n\r\n"
"<h1>401 Unauthorized</h1>";

/**
 * Set up the ethernet and LEDS
 */
void setup(){
  digitalWrite(BUILD_ALERT, LOW);
  _statusLED(128, 0, 128);
  
  delay(2000);
  Serial.begin(57600);
  Serial.println("\n[backSoon]");

  if (ether.begin(sizeof Ethernet::buffer, macAdd, SLAVE_SELECT) == 0) {
    Serial.println( "Failed to access Ethernet controller");
    _statusLED(255, 0, 0);
  }

  if (!ether.dhcpSetup()) {
    Serial.println("DHCP failed");
    _statusLED(255, 255, 0);
  } else {
    _statusLED(0, 255, 0);
  }
  
  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);  
  ether.printIp("DNS: ", ether.dnsip);  
}

/**
 * Change the colour of the status LED
 *
 * @param byte r the value of red (0 - 255)
 * @param byte g the value of green (0 - 255)
 * @param byte b the value of blue (0 - 255)
 */
void _statusLED(byte r, byte g, byte b) {
  analogWrite(LED_R, r);
  analogWrite(LED_G, g);
  analogWrite(LED_B, b);
}

/**
 * Show the status page with last build status
 *
 * @return void
 */
void statusPage() {
  bfill.emit_p(PSTR("$F"
    "<meta http-equiv='refresh' content='30'/>"
    "<title>Jenkins Build Status</title>" 
    "Last Build: $F [<a href='?build=pass'>pass</a>][<a href='?build=fail'>fail</a>]"),
  http_OK,
  buildPassed ? PSTR("PASS") : PSTR("FAIL"));
}

/**
 * DHCP expiration is a bit brutal
 * 
 * because all other ethernet activity and incoming packets will be ignored until a new lease has been acquired
 */
void _checkDhcp() {
  /*if (ether.dhcpState != DHCP_STATE_BOUND) {
    Serial.println("Acquiring DHCP lease again");
    ether.dhcpSetup();
  }*/
}

/**
 * Main loop that watches for requests
 *
 * If there is a fail and its not alerted the alert is run.
 * If there is no data nothing happens.
 */
void loop() {
  _checkDhcp();
  
  checkRequest();
  
  if (!alerted) {
     brokenBuildAlert(); 
  }
}

/**
 * Check what the request is for
 */
void checkRequest() {
  word pos = ether.packetLoop(ether.packetReceive()); 
  if (!pos) {
    return;
  }
  
  delay(1);
  
  bfill = ether.tcpOffset();
  char *data = (char *) Ethernet::buffer + pos;
  
  if (strncmp("GET /", data, 5) != 0) {
    badRequest();
    return;
  }
 
  Serial.println("Parsing GET");
  data += 5;
  if (data[0] == ' ') {
    statusPage();
  } else if (strncmp("?build=pass ", data, 12) == 0) {
    buildPassed();
  } else if (strncmp("?build=fail ", data, 12) == 0) {
    buildFailed();
  } else {
    badRequest();
    return;
  }

  bfill.emit_p(http_Found);
  ether.httpServerReply(bfill.position());
}

/**
 * Build Passed
 */
void buildPassed() {
  Serial.println("Build passed");
  buildHasPassed = true;
  alerted = true;
  analogWrite(BUILD_ALERT, 0);
}

/**
 * Build Failed
 */
void buildFailed() {
  Serial.println("Build failed");
  if (buildPassed) {
    alertStarted = 0;
    alerted = false;
  }
  buildHasPassed = false;
}

/**
 * Set the alarm pin high so things light up / make a noise
 *
 * @return void
 */
void brokenBuildAlert() {
  if (!alertStarted) {
    Serial.println("Broken build alert start");
    alertStarted = millis();
    analogWrite(BUILD_ALERT, 255);
  }
  
  if (millis() - alertStarted >= 3000) {
    Serial.println("Broken build alert end");
    alertStarted = 0;
    analogWrite(BUILD_ALERT, 0);
    alerted = true;
  }
}

/**
 * Send a bad request page
 */
void badRequest() {
  Serial.println("not GET");
  bfill.emit_p(http_Unauthorized);
  ether.httpServerReply(bfill.position()); 
}
