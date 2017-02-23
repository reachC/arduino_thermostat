/*
    Arduino Thermostat

    Control your heating with your arduino uno,
    a 1-wire temperature sensor (DS18B20),
    a relay (shield) and a network shield.

    created 23 Feb 2017
    by reachC
    
    https://reachcoding.eu
*/

#include <OneWire.h>
#include <Ethernet.h>

// -----------------------------------------------------------------------------------------------------
// CONFIG
// -----------------------------------------------------------------------------------------------------
int iTempPin1 = 2;                                    // one-wire temperature sensor pin
int iRelayPin1 = 4;                                   // relay pin
const float fHysteresis = 1.0;                        // switching hysteresis +/- from the set point
float fSetPoint = 21.0;                               // default set point
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEF };  // mac address
IPAddress ip(192, 168, 0, 5);                        // ip address
// -----------------------------------------------------------------------------------------------------

// webserver on port 80
EthernetServer server(80);

// one wire temperature sensor
OneWire ds(iTempPin1);

// var that holds the current relay state
boolean bRelayState = false;

// var that holds the current room temperature
float fActualValue = 0;

// holds the http request string
String reqString = "";

void setup(void) {
  // connecting to the network
  Ethernet.begin(mac, ip);

  // start webserver
  server.begin();

  // define relay pin
  pinMode(iRelayPin1, OUTPUT);
}

void loop(void) {
  // read current temperature
  fActualValue = getTemp();

  // check if the relay should be opened or closed
  if (fActualValue <= (fSetPoint - fHysteresis)) {
    // heating on
    digitalWrite(iRelayPin1, HIGH);
    bRelayState = true;
  } else if (fActualValue >= (fSetPoint + fHysteresis)) {
    // heating off
    digitalWrite(iRelayPin1, LOW);
    bRelayState = false;
  }

  EthernetClient client = server.available();

  if (client) {
    // http request ends with a blank line
    boolean currentLineIsBlank = true;

    while (client.connected()) {
      if (client.available()) {
        // read a character sent from the client
        char c = client.read();

        if (reqString.length() < 100) {
          reqString += c;
        }

        if (c == '\n' && currentLineIsBlank) {
          // check if the client wants to change the set point
          if (reqString.indexOf("temp=up") >= 0) {
            if (fSetPoint < 99.0) {
              // increment set point
              fSetPoint = fSetPoint + 0.5;
            }
          } else if (reqString.indexOf("temp=down") >= 0) {
            if (fSetPoint > 0.0) {
              // decrement set point
              fSetPoint = fSetPoint - 0.5;
            }
          }

          // clear the request string
          reqString = "";

          // send header and html page
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.println("<head>");
          client.println(F("<style>html { width: 100%; min-height: 100%; margin: 0 auto; } body { float: left; background: #cedce7; background: -webkit-linear-gradient(45deg, rgba(0,51,51,1) 0%, rgba(5,193,255,1) 50%, rgba(0,51,51,1) 100%); } .box { width: 100vw; height: 23px; display:table-cell; vertical-align:middle; background-color: red; border-radius: 12px 12px 12px 12px; padding: 5px; background: #6B6B6B; background:-webkit-radial-gradient(center, ellipse cover, rgba(107,107,107,1) 0%, rgba(0,0,0,1) 100%); color: white; box-shadow: 3px 3px 1px rgba(0, 0, 0, 0.4); }</style>"));
          client.println("<title>Arduino Thermostat</title>");
          client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
          client.print("<meta http-equiv='refresh' content='10; URL=http://");
          client.print(Ethernet.localIP());
          client.println("'></head>");
          client.print(F("<body><div class='box'><img src='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAcAAAAWCAYAAAAM2IbtAAAABmJLR0QA/wD/AP+gvaeTAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAB3RJTUUH4QIMARYvl1Fs5gAAADt0RVh0Q29tbWVudABFZGl0ZWQgYnkgUGF1bCBTaGVybWFuIGZvciBXUENsaXBhcnQsIFB1YmxpYyBEb21haW5wJ3oAAAABfklEQVQoz12QzUocQRSFv3trQqYrsRmRHv8iQjZKZiMSQ0Q0rpVA9oEwEFfJCyR5heQJ4sZtzOgLCIFAshHxEWbhIooyIMSR6XG6bxbdnR69UBRV3z23Th0hK202mxtm6atu93qn1Wr9BgyAZ0tLb7/vfrPz8zPb3v5qwCqAAjIxOTUzNz+P955Go0EQVGsFBMwwQ6Q8DUEEBDMwM1SzrkIJYkUXaZoOQ8lXtqncVebO89ekhKpI4BHvkSD431jBzJIX6/2rvT3+jo9zfXKCmaUZFGEwPX118eEjVaAD4FxSjnVOpEhEBDPT4X8WXvOk9VYIhVcEcKqlUkuWCSuVe9n91pZzcfykmqurZsyFIwscHol7o/pys93+MmtGCjwEpm4G649P/xy42X5//3W3GyVDY6MkYb/TGVUfxzP3uV0hMOj1Hmlcqx1f3oFnQFivH2oSRe9/et8bACnQB36FYedi5MFnPrXb8nR5+flGPdp9FwQ/1sbGdhZXVhoA/wBRHIKn3yIlPgAAAABJRU5ErkJggg=='>"));
          client.print("&nbsp;&nbsp;&nbsp;actual value: ");
          client.print(fActualValue);
          client.println(" &#176;C</div><br />");
          client.print(F("<div class='box'><img src='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAIAAACQkWg2AAAABmJLR0QA/wD/AP+gvaeTAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAB3RJTUUH4QIMAQ4Jx0dxQgAAAWhJREFUKM99UT1rwlAUvS++SlvR2CFChmKHUugi+gN0UTM5CIIgzk4uooODizjq4OIkjpLZIOLgKHSXDhWhg5UOSUSplgomvtchrd/mTufAOe+cdy96qddX3S5CCEwGIaAUCLEKAv7pdJySdFZGAW48HjYe36jqtFaDzUbTNIwY5tK7jkjkqd02MF8oDFwuQOiimgK4crkttXLcXSKBABiz6oQc/8TEgADkSmVL19PpTBQpAD4Q2e02v9/Cslc8b2HZeav16vNxqZQmy3KpZAEgRwa6XDqCQT6bNag2mcwajc902gg05qASAdAXi10gxqdVmf21cJnMfbG4M1itZgZnMvlQrRp4JkmjWAzZbKeGv9DbUOix2TTwV7//Ho0yAIu96gcJ2O1+7vUM/j0YjAIB5n+z5yutx+OPfB4AVsPhm9drekvAQAgDoJTLuqrORdFcDZTia0HQdB0QUhQFBAEovXh7SvVw+Beme3ng6sWBoAAAAABJRU5ErkJggg=='>"));
          client.print("&nbsp;set point: ");
          client.print(fSetPoint);
          client.println(" &#176;C</div><br />");
          client.print(F("<div class='box'><img src='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAABmJLR0QA/wD/AP+gvaeTAAAACXBIWXMAAEDhAABA4QFEieSXAAAAB3RJTUUH4QIMAQ4eRJT0hQAAAw9JREFUOMttk11om3UUxn//N+m7YkgaDaybFlwrDdqlDMRKOlPrsqqjbNlFEdMKrVJcx6jFIth454XgvBnsZh2jklqq1QsFBQcyXepH2qSNUBmMdksplaqZsYRm9iPJ/32PNx3MbQ88Fwee5zycwzmK++Hr7e093dDQcMTj8RwA2NjYWFleXk5MTk6OAoW7xeruIhwOx/r6+j5s9Pu5kkySz2YR4FG/nyPBIDezWcbj8XcSicS5+2J7enq+zmQy0nfqlPSbplxxueQnj0e+dbnkvFLyIkj/mTOSTqelu7v7U+5JHp6bm5OXQyGZq6mRW/v3y8revXLN55Ofa2rkS5dLPjZNeQ+k/fBhyWQy0tHREbszgntsbKyYWljgtfFxmtxuPE4nhm3z2eYmBa05pBRr5TLrWrNmWewMDXHo4EEGBgaUEY1G3wwEAhQvXWJfdTVahD+0pjWf54dSiV8qFYa2trgNGErxCPDrhQu0trYSDodHHJFIZOSf7e0nmy9fxmuaOIHj6+t84vWStywOOBwcczr5YGeH5wyDbdvGadtsNjXxUHW1aXi93sfXlpZwOxwUtebzrS2OmyZO26beMNgH1ADPO51cs20EaAQWrl+ntra2zhARWylF0bIoas2fWuMG1rXmMeBhETZtG58IOdsGQAClFCIiRqFQWKlvauJmpULBsvABqXKZf7WmoDVFy6KsNbOWRf3u1heBZ5qbyeVyvxvZbPbHlkCAhGFwu1Kh3rLI2TZflEr8pTW3tOYry6IsQi1gADdMkxdCIebn579RwJ7R0dGdG6urVM6epVYp9ijFjAi/iaCAABAEKkAeqIrFeKKuTgYHBw0DKE1NTfW/EomwFA7ztwhl2+ZpEU4DA0ALYO+aV0+c4NWTJ4nH42/97xq7urouplIpGR4ZkWcNQ94AeX+Xr4O0VFXJcCwms7Oz0tnZefGBz9Te3j4cjUbPhUIhvrt6lcVsFoCnGhs5dvQo09PTTExMvJ1Op88/sMEdtLW1vRsMBl/yeDx+QBWLxcWZmZnvk8nkR/dq/wN7M2iFEVDD6QAAAABJRU5ErkJggg=='>"));
          client.print("&nbsp;relay state: ");
          client.print(bRelayState);
          client.println("</div><br /><br />");
          client.print("<a href='http://");
          client.print(Ethernet.localIP());
          client.println("?temp=up'><div class='box' style='text-align: center; color: #09ff00;'><b>+</b></div></a><br /><br />");
          client.print("<a href='http://");
          client.print(Ethernet.localIP());
          client.println("?temp=down'><div class='box' style='text-align: center; color: red;'><b>-</b></div></a>");
          client.println("</body>");
          client.println("</html>");
          break;
        }

        if (c == '\n') {
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }

    // give the web browser time to receive the data
    delay(1);

    // close the connection:
    client.stop();
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
    return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
    return -1000;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1); // start conversion, with parasite power on at the end

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

