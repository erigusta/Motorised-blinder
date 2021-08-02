// working, now potentially to many but sends data to thingspeak, can use stepper,servo and sensors
// Steppper motor library and servo library
#include <Servo.h>
#include <Stepper.h>
//declare servos
Servo closer;
Servo Rotater;

#include <ESP8266WiFi.h> // for webserver
#include <NTPClient.h> // for not spaming updates to thingspeak (time) // https://github.com/sstaub/NTP for documentation (MIT licence Copyright (c) 2018 Stefan Staub) for documentation
#include <WiFiUdp.h> // to communicate with timeserver
// temperature sensor ---
#include "DHT.h"
#define DHTPIN 2
#define DHTTYPE DHT11   // DHT 11
// initialise DHT
DHT dht(DHTPIN, DHTTYPE);

// manual ip adresss stuff, change to one that works for you form .25
IPAddress local_IP(192, 168, 1, 25);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(192, 168, 1, 1);
IPAddress SecondaryDNS(192, 168, 1, 1);

// wifi, config your own. set in your own stuff
String apiKey = "API-key";
const char* ssid     = "SSID";
const char* password = "PSWD";
const char* IOTServer = "api.thingspeak.com";// if using another server change this..
const int windowheight = 1000;

const int advancedAngleFromTemp = 0;

// for HTTP request (will do for now on "safe" local networks)
WiFiServer server(80);
// to store header (http stuff)
String header;
// to manage timout
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 3000;

// sensor stuff
const int photoR = A0;
const int Contact1 = 16;//d0 insted of interrupt...

int light = 0;
int buttonState1 = 0;

float h = 0;// dht humidity
float t = 0;//dht celsius

// servo stuff
// writing to pin can change the output servo but a delay is needed to aviod the lag from the ICs.
const int Servo = 4 ;// need 14,12,15 for stepper pwn
const int servoSwappPin = 5;
int debugMovement = 1;

//stepper stuff
const int rolePerMinute = 15;
const int stepsPerRevolution = 2048;
const int StepsperMM = 1;//change after calibrating iwth gearbox

// time stuff
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// close-time & opentime -- change to fit timer stuff...
int autoMovements = 1;
int closeTimeH = 22;
int closeTimeM = 30;
int openTimeH = 9;
int openTimeM = 30;

int statusBlinder = 0;// 1 is closed
int breakMaxTemperature = 30;
int breakMinTemperature = 25;
// webpage in program memmory (ADD LATER IF TIME REMAINS)- store in flash, can send with client.print ...
// R"=== to enable it to be stored propperly but in spererate liens
char webpage[] PROGMEM = R"=====(

)=====";
// to use in movement funcitons
int windowSstatus = 0;
//int desiredWindowStatus = 0;

int rotatedAngle = 0;
//int desiredRotatedAngle = 0;

 void blinderMove(int rotate, int blinder){
  digitalWrite(servoSwappPin,LOW);
  delay(100);
  closer.attach(4);
  closer.write(180);// move servo
  delay(1000);
  // blocker disabled, now time to move to height
  //int steps = stepsPerRevolution * StepsperMM * blinder;
  stepperTest(blinder);
  Serial.println("Servo ready");

  delay(1000);
  Serial.println("Starts closing");

  //close it
  closer.attach(4);
  closer.write(0);// move servo
  delay(1000);
  Serial.println("Close");
  // rotating part
  digitalWrite(servoSwappPin,HIGH);
  delay(100);
  Serial.println(" rotator");
  Rotater.attach(4);
  delay(100);
  Rotater.write(rotate);// move servos
  delay(1000);

  //change toggle
  if (statusBlinder ==1){
    statusBlinder = 0;
    }
  else{
    statusBlinder = 1;
    }

    rotatedAngle = rotate;
    windowSstatus = blinder;
  }

 void advance_rotate( int a){
  int maxligth = 0;
  int angleatmaxlight;
  Serial.println("Running advanced rotate");
  digitalWrite(servoSwappPin,LOW);
  delay(100);
  Rotater.attach(4);
  int positionServo = 0;
  Serial.println("Loop st");
  while (positionServo<180){
    delay(200);
    updateLight();
    if (light > maxligth && a>0){
      maxligth = light;
      angleatmaxlight = positionServo;
      }
     else if (light < maxligth && a<0){
      maxligth = light;//in this case shoul be min light
      angleatmaxlight = positionServo;
      }
     positionServo = positionServo +10;
     Rotater.write(positionServo);
    }
    Serial.println("Done adv rotara");
    Rotater.write(angleatmaxlight);
    delay(500);
  }


void updateLight(){
  light = analogRead(photoR);
  }

void updateContact(){
  buttonState1 = digitalRead(Contact1);// ok
  }

void sensorDebug(){
  updateContact();
  updateLight();
  updateDHT();
  Serial.println("light = ");
  Serial.println(light);
  Serial.println("Contact status");
  Serial.println(buttonState1);
  Serial.println("humidity");
  Serial.println(h);
  Serial.println("Temp");
  Serial.println(t);
  delay(1000);
  }

void updateDHT(){
  h = dht.readHumidity();
  t = dht.readTemperature();
  // checking values
  if (isnan(h) || isnan(t)) {
    //Serial.println(F("Error with DHT sensor"));
    return;
    }
  }

void updateAll(){
  updateDHT();
  updateLight();
  updateContact();
 }

void servotest(){
  digitalWrite(servoSwappPin,LOW);
  delay(100);
  closer.attach(4);
  closer.write(30);// move servos
  delay(1000);
  closer.write(90);// move servos
  delay(1000);
  //test the other servo
  digitalWrite(servoSwappPin,HIGH);
  delay(100);
  Rotater.attach(4);
  Rotater.write(30);// move servos
  delay(1000);
  Rotater.write(90);// move servos
  delay(1000);

  }
void stepperTest(int a){
  //tested first, then used for stepper movement
  //initialse
  Stepper myStepper(stepsPerRevolution, 13, 12, 15, 14);
  // gpio IN4 - 13, IN3 -15, IN2 - 12 , IN1 -14
  myStepper.setSpeed(rolePerMinute);
  delay(500);
  myStepper.step(stepsPerRevolution * a);
  delay(500);
  //myStepper.step(-stepsPerRevolution);


  }

void setup() {
  // if changeing this use https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html for usage
  Serial.begin(115200);
  Serial.println(" ");
  Serial.print("Connecting to : ");
  Serial.print(ssid);
  //starting wifi if error then and tries to connect
  WiFi.begin(ssid, password);
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, SecondaryDNS)) {
    Serial.println("Failed to configure");
  }
  while(WiFi.status()!= WL_CONNECTED){
    delay(1000);
    Serial.print(".");
  }
  // print some data such as the fixed ip adress
  Serial.println("");
  Serial.println("IP adress : ");
  Serial.println(WiFi.localIP());
  server.begin();

  // sensor
  pinMode(photoR, INPUT);
  pinMode(Contact1, INPUT);
  // start dht 11
  // stuff test
  delay(200);
  dht.begin();

  // start ntp client
   timeClient.setTimeOffset(2); // due stocholm timezone, could in theory have if and geting month to add or subtract for winter time..
   // only needs secounds...
  timeClient.begin();

  // servo connect
    pinMode(servoSwappPin, OUTPUT);// used for swapping form servo 1 to servo 2

  Serial.println("Setup complete");

}


void loop() {
  // disable light sleep
    //wifi_set_sleep_type(NONE_SLEEP_T); not used
    delay(500);

  int currentSecond = timeClient.getSeconds();

  // webserver stuff
 WiFiClient client = server.available();
  if (client) {                             // So when a client connencts to the web server
    currentTime = millis();                 // update times for timeout checking...
    previousTime = currentTime;
    Serial.println("New Connection ");          // data...
    String currentLine = "";                // make a String to hold incoming data from the client
    int timeoutmaybe = currentTime - previousTime;
    while (client.connected() && timeoutmaybe < timeoutTime) {
      // so when it connectes
      currentTime = millis();

      if (client.available()) {
          // from client
        // bunch of data from client
        char c = client.read();
        Serial.write(c);
         header += c;
        if (c == '\n') {                    // newline character ?
          // blank? then 2 new line charachters ( end of http request)
          if (currentLine.length() == 0) {
            // client print line esentially send a a file row by row to the client to display...
            client.println("HTTP/1.1 200 OK"); //[https://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html]   //standard respone, ok
            client.println("Content-type:text/html");//type info
            client.println("Connection: close");
            client.println();

            // // HTML stuff for displaying web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<title>ESPN8266 window controlling web server</title>");
            // Web Page Heading
            client.println("<body><h1>ESP8266 Window web Server</h1>");
             // update and print light value
            updateLight();
            client.print("<p>The light value is  : ");
            client.print(light);
            client.println("</p>");
            // same for contact ish
            updateContact();
            client.print("<p>The boolena contact status is  : ");
            client.print(buttonState1);
            client.println("</p>");
            updateDHT();
            client.print("<p>Temperature is  : ");
            client.print(t);
            client.println(" Degrees celsius.</p>");
            client.print("<p>The Humidity is  : ");
            client.print(h);
            client.println(" percent.</p>");
            //client.println("<p> Desired status ");
            //client.println(desiredWindowStatus);
            //client.print(".<p>");<p>
            client.print("Current  status ");
            client.println(windowSstatus);
            client.print(".</p>");
            //client.print("<p> Desired  status ");
            //client.print(desiredRotatedAngle);
            //client.print(".<p>");<p>
            client.print("Current  status ");
            client.print(rotatedAngle);
            client.print(".</p>");

            if (advancedAngleFromTemp == 0 ){
             client.print(" Temperature limited angeling is disabled");
             client.print(".</p>");
              }
            else{
             client.print(" Temperature limited angeling is enabled , advancedAngleFromTemp : ");
             client.print(advancedAngleFromTemp);
             client.print(".</p>");
             client.print("BreakMaxTemperature : ");
             client.print(breakMaxTemperature);
              client.print("BreakMinTemperature : ");
             client.print(breakMinTemperature );
             client.print(".</p>");
              }
            client.println();
            // yeah abouth that...
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // so abouth that carage return...
          currentLine += c;      //currentline + char
        }
      }
    }

    // reset header
    header = "";
    // end it ( for when disconnected
    client.stop();
    Serial.println("http disconnected.");
  }
  else if (currentSecond == 31){
    // the information in this from https://github.com/arduino/tutorials/blob/master/WiFi101ThingSpeakDataUploader/WiFi101ThingSpeakDataUploader.ino , by Helena Bisby <support@arduino.cc>
     //This example code is in the public domain
    if (client.connect(IOTServer,80))   //   "184.106.153.149" or api.thingspeak.com
                      {
        updateDHT();
        String sendStr = apiKey;
        sendStr +="&field1=";
        sendStr += String(t);
        sendStr +="&field2=";
        sendStr += String(h);
        sendStr +="&field3="; // added! this section untill next comment
        sendStr += String(rotatedAngle);
        sendStr +="&field4=";
        sendStr += String(windowSstatus / windowheight);
        sendStr += "\r\n\r\n\r\n\r\n";// nextcoment, this line was slightly altered

        client.print("POST /update HTTP/1.1\n");
        client.print("Host: api.thingspeak.com\n");
        client.print("Connection: close\n");
        client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
        client.print("Content-Type: application/x-www-form-urlencoded\n");
        client.print("Content-Length: ");
        client.print(sendStr.length());
        client.print("\n\n\n\n");// dubbeled ns sequence!
        client.print(sendStr);
        Serial.println("%. Send to Thingspeak.");
        Serial.println(currentSecond);
        Serial.println(sendStr);
                               }
        client.stop();
        //end of the linked by Helena Bisbby
        delay(1000);
    }
    // condition using fixed values in the top for logic
    else if(currentSecond == 00 && timeClient.getMinutes() == closeTimeM && timeClient.getHours() == closeTimeH && statusBlinder!= 1 && autoMovements != 0){
      // checks time and close
      blinderMove(90,windowheight);
    }
    else if(currentSecond == 00 &&timeClient.getMinutes() == openTimeM && timeClient.getHours() == openTimeH && statusBlinder != 0 && autoMovements != 0){
       // checks time and open
       blinderMove(0,-windowheight);
    }
    else if (t>breakMaxTemperature && statusBlinder != 0 && advancedAngleFromTemp != 0 && (timeClient.getHours()+1 > openTimeH || timeClient.getHours()< closeTimeH -1 )){
        // yeah, the + and - 1 is a simple limitation, could add more but not neeed
        advance_rotate(1);
      }
    else if (t<breakMinTemperature && statusBlinder != 0 && advancedAngleFromTemp != 0 && (timeClient.getHours() +1 > openTimeH || timeClient.getHours()< closeTimeH -1 )) {
       // yeah, the + and - 1 is a simple limitation, could add more but not neeed
       advance_rotate(-1);
      }
    //wifi_set_sleep_type(LIGHT_SLEEP_T); // requires changes from second based to secound + checks...
    //delay(500);
}
