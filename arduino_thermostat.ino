/*
    thermostat (p-controller)
    
    A trivial way to control you heating with your arduino,
    a 1-wire temperature sensor (DS1820) and a network shield.
    
    created 4 Jan 2017
    by reachC
    
    This code is in the public domain.
*/

#include <OneWire.h>
#include <SPI.h>
#include <Ethernet.h>

// one-wire temperature sensor pin
int iTempPin1 = 2;

// relay pin
int iRelayPin1 = 3;

// mac and ip for the network connection
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEF };
IPAddress ip(192,168,0,1);

// define a webserver on port 80
EthernetServer server(80);

// define a one wire temperature sensor
OneWire ds(iTempPin1);

const float fHysteresis = 1.0;     // switching hysteresis +/- from set point
float fSetPoint = 21.0;            // control to this temperature
float fActualValue = 0;            // current room temperature
boolean bRelayState = false;       // current relay state

void setup(void) {
  // start serial connection @ 9600 BAUD
  Serial.begin(9600);
  
  // connecting to the network
  Ethernet.begin(mac, ip);
  
  // start webserver
  server.begin();
  
  // print network ip
  Serial.println("IP: "+Ethernet.localIP());
  
  // define relay pin
  pinMode(iRelayPin1, OUTPUT);
}

void loop(void) {
  // read current temperature
  fActualValue = getTemp();
  
  // check if the relay should be opened or closed
  if (fActualValue <= (fSetPoint - fHysteresis)) {
    // close relay circuit
    digitalWrite(iRelayPin1, HIGH);
    bRelayState = true;
  } else if (fActualValue >= (fSetPoint + fHysteresis)) {
    // open relay circuit
    digitalWrite(iRelayPin1, LOW); 
    bRelayState = false;
  }
  
  // print the current temperature
  Serial.println(fActualValue);
  
  EthernetClient client = server.available();
  
  if (client) {
    Serial.println("client connected");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        // read a character sent from the client
        char c = client.read();

        if (c == '\n' && currentLineIsBlank) {
          // send header and html page
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
	  client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.println("<head>");
          client.println("<title>Temperatur</title>");
          client.println("</head>");
          client.print("<body>");
          client.print("actual value: ");
          client.println(fActualValue);
          client.println("<br />");
          client.print("set point: ");
          client.println(fSetPoint);
          client.println("<br />");
          client.print("relay state: ");
          client.println(bRelayState);
          client.println("</body>");
          client.println("</html>");
          break;
        } else {
          if (c == '[') {
            // send "[" character to increment setpoint at +0.5 °C
            fSetPoint = fSetPoint+0.5;
          } else if (c == ']') {
            // send "]" character to decrement setpoint at -0,5 °C
            fSetPoint = fSetPoint-0.5;
          }
        }
        
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    
    // give the web browser time to receive the data
    delay(1);
    
    // close the connection:
    client.stop();
    
    Serial.println("client disonnected");
  }
  
  delay(300);
}

float getTemp() {
  byte data[12];
  byte addr[8];

  if ( !ds.search(addr)) {
      //no more sensors on chain, reset search
      ds.reset_search();
      return -1000;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
      Serial.print("Device is not recognized");
      return -1000;
  }
  
  ds.reset();
  ds.select(addr);
  ds.write(0x44,1); // start conversion, with parasite power on at the end

  byte present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE); // Read Scratchpad

  
  for (int i = 0; i < 9; i++) {
    data[i] = ds.read();
  }
  
  ds.reset_search();
  
  byte MSB = data[1];
  byte LSB = data[0];
  
  //using two's compliment
  float tempRead = ((MSB << 8) | LSB);
  float TemperatureSum = tempRead / 16;
  
  return TemperatureSum;
}

