#include <EtherCard.h>
#include <avr/wdt.h>

// ethernet interface mac address - must be unique on your network
static byte mymac[] = { 0x74, 0x69, 0x69, 0x2D, 0x30, 0x31 };

static byte homeip[] = { 10, 0, 0, 10 };
static byte homegwip[] = { 10, 0, 0, 138 };

static byte officeip[] = { 192, 168, 210, 99 };
static byte officegwip[] = { 192, 168, 200, 254 };

static byte myip[] = { 10, 0, 0, 10 };
static byte gwip[] = { 10, 0, 0, 138 };

const int LED      = 8;
const int HOME_PIN = 7; // For this to work, connect (digital) GND to 7
const int PIR      = 2; // PIR Out pin

static BufferFiller bfill;  // used as cursor while filling the buffer
byte Ethernet::buffer[1000];   // tcp/ip send and receive buffer

void setup() {
  Serial.begin(9600);
  Serial.println("\n[HTTP RELAY]");

  pinMode(HOME_PIN, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  pinMode(PIR, INPUT);
  digitalWrite(LED, LOW);

  connectToInternet();
}

unsigned long endTime = 0;
bool shouldCheck = false;
unsigned long restartTime = 14400000; // 4 hours

void loop() {
  bool shouldOpen = didReceiveHttpRequest() || IsSomeoneClose();

  // check if valid tcp data is received
  if (shouldOpen) {
    digitalWrite(LED, HIGH);
    endTime = millis() + 1000;
    shouldCheck = true;
  }

  if (shouldCheck && millis() > endTime) {
    digitalWrite(LED, LOW);
    shouldCheck = false;
  }

  handleReboot();
}

void connectToInternet() {
  if (ether.begin(sizeof Ethernet::buffer, mymac, SS) == 0)
    Serial.println( "Failed to access Ethernet controller");

  if (digitalRead(HOME_PIN) == LOW) {
    ether.staticSetup(homeip, homegwip);
  } else {
    ether.staticSetup(officeip, officegwip);
  }

  ether.printIp("IP: ", ether.myip);
}

bool IsSomeoneClose() {
  return digitalRead(PIR) == HIGH;
}

bool didReceiveHttpRequest() {
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);

  if (pos) {
    bfill = ether.tcpOffset();
    char* data = (char *) Ethernet::buffer + pos;

    if (strncmp("GET /noclose", data, 12) == 0) {
      bfill.emit_p(PSTR(
                     "HTTP/1.0 200 OK\r\n"
                     "Content-Type: text/html\r\n"
                     "\r\n"
                     "<html><head></head><body><h1>200 OK</h1></body></html>"));
      ether.httpServerReply(bfill.position()); // send web page data
    } else {
      bfill.emit_p(PSTR(
                     "HTTP/1.0 200 OK\r\n"
                     "Content-Type: text/html\r\n"
                     "\r\n"
                     "<html><head><script type=\"text/javascript\">window.close();</script></head><body><h1>200 OK</h1></body></html>"));
      ether.httpServerReply(bfill.position()); // send web page data
    }

    return true;
  } else {
    return false;
  }
}

void handleReboot() {
  if (millis() >= restartTime) {
    wdt_disable();
    wdt_enable(WDTO_15MS);
    while (1) {}
  }
}
