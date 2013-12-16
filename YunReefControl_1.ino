/*

Yun Reef Control Sketch

 */

#include <Bridge.h>
#include <Process.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Console.h>
#include <YunServer.h>
#include <YunClient.h>

Process date;                 // process used to get the date
int hours, minutes, seconds;  // for the results
int lastSecond = -1;          // need an impossible value for comparison

const int rampMinutes = 240;
const int peakMinutes = 360;
const int fadeMinutes = 240;
const int whiteMax = 255;
const int whiteMin = 0;
const int blueMax = 235;
const int blueMin = 0;
const int startHours = 6;
const int startMinutes = 0;

const int whitePin = 5;
const int bluePin = 6;
const int relay1Pin = 2; // Kalk mixer
const int relay2Pin = 3; // Kalk delivery
const int relay3Pin = 4;  
const int relay4Pin = 7; // Cooling fan for LEDs

const int rampStart = (startHours *60) + startMinutes;
const int peakStart = rampStart + rampMinutes;
const int fadeStart = peakStart + peakMinutes;
const int fadeEnd = fadeStart + fadeMinutes;

int relay1Status = 1;
int relay2Status = 1;
int relay3Status = 1;
int relay4Status = 0; // reversed as relay is NC

int currentPosition = 0;
int whiteLastLevel = 0;
int blueLastLevel = 0;

long overrideTime = 0;
#define OVERRIDE_TIMEOUT 180 // value in mins

#define ONE_WIRE_BUS A4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

YunServer server;

#define FANTEMP 50
#define DEBUG false

void setup() {  
  pinMode(relay1Pin, OUTPUT);
  pinMode(relay2Pin, OUTPUT);
  pinMode(relay3Pin, OUTPUT);
  pinMode(relay4Pin, OUTPUT); 
  
  // setting high turns off the relay
  digitalWrite(relay1Pin, HIGH);
  digitalWrite(relay2Pin, HIGH);
  digitalWrite(relay3Pin, HIGH);
  digitalWrite(relay4Pin, HIGH); // turns on as this relay is NC
  
  relay1Status = 0;
  relay2Status = 0;
  relay3Status = 0;
  relay4Status = 1; // reversed as relay is NC
  
  Bridge.put("RELAY1", "0");
  Bridge.put("RELAY2", "0");
  Bridge.put("RELAY3", "0");
  Bridge.put("RELAY4", "1"); // reversed as relay is NC
  
  Bridge.begin();
  Serial.begin(9600);      
  Console.begin(); 
  sensors.begin(); //dallas temp sensors
  
  // start REST server
  server.listenOnLocalhost();
  server.begin();
  
  if (!date.running())  {
    date.begin("date");
    date.addParameter("+%T");
    date.run();
  }
}

void loop() {

  if(lastSecond != seconds) {  // if a second has passed

    // restart the date process:
    if (!date.running())  {
      date.begin("date");
      date.addParameter("+%T");
      date.run();
    }
    
    currentPosition = (hours *60) + minutes;
    updateDimmer();
    checkTemp();
    setRelays();
  }

  //if there's a result from the date process, parse it:
  while (date.available()>0) {
    // get the result of the date process (should be hh:mm:ss):
    String timeString = date.readString();    

    // find the colons:
    int firstColon = timeString.indexOf(":");
    int secondColon= timeString.lastIndexOf(":");

    // get the substrings for hour, minute second:
    String hourString = timeString.substring(0, firstColon); 
    String minString = timeString.substring(firstColon+1, secondColon);
    String secString = timeString.substring(secondColon+1);

    // convert to ints,saving the previous second:
    hours = hourString.toInt();
    minutes = minString.toInt();
    lastSecond = seconds;          // save to do a time comparison
    seconds = secString.toInt();
  } 
  
  // Get clients coming from server
  YunClient client = server.accept();
  if (client) {
    process(client);
    client.stop();
  }
  
  delay(50);
}


void printTime() {
  if (hours <= 9) Console.print("0");    // adjust for 0-9
    Console.print(hours);    
    Console.print(":");
    if (minutes <= 9) Console.print("0");  // adjust for 0-9
    Console.print(minutes);
    Console.print(":");
    if (seconds <= 9) Console.print("0");  // adjust for 0-9
    Console.print(seconds);
}
  
void updateDimmer() {
  
  int blueLevel = blueMin;
  int whiteLevel = whiteMin;
  
  if(DEBUG){
    Console.print("Current position: ");
    Console.print(currentPosition);
    Console.print(", Ramp start: ");
    Console.print(rampStart);
    Console.print(", Peak start: ");
    Console.print(peakStart);
    Console.print(", Fade start: ");
    Console.print(fadeStart);
    Console.print(", Fade end: ");
    Console.print(fadeEnd);
    Console.print(", White max: ");
    Console.print(whiteMax);
    Console.print(", Blue max: ");
    Console.println(blueMax);
  }
  
  if ((currentPosition > rampStart) && (currentPosition < peakStart)) {
    float phasePercent = float(currentPosition - rampStart) / float(peakStart - rampStart);
    if(DEBUG){Console.print("In the ramp up phase, position ");
    Console.println(phasePercent);}
    whiteLevel = ((whiteMax - whiteMin) * phasePercent) + whiteMin;
    blueLevel = ((blueMax - blueMin) * phasePercent) + blueMin;
   
  }
  if ((currentPosition >= peakStart) && (currentPosition < fadeStart)) {
    if(DEBUG){Console.print("In the peak brightness phase ");}
    whiteLevel = whiteMax;
    blueLevel = blueMax;
  }
  if ((currentPosition >= fadeStart) && (currentPosition < fadeEnd)) {
    float phasePercent = float(fadeEnd - currentPosition) / float(fadeEnd - fadeStart);
    if(DEBUG){Console.print("In the fade out phase, position ");
    Console.println(phasePercent);}
    whiteLevel = ((whiteMax - whiteMin) * phasePercent) + whiteMin;
    blueLevel = ((blueMax - blueMin) * phasePercent) + blueMin;
  }
  
  if (overrideTime < (millis() - (OVERRIDE_TIMEOUT *60000))) {
   if (whiteLevel != whiteLastLevel) {
    printTime();
    Console.print(" Setting white pin to ");
    Console.println(whiteLevel);
    analogWrite(whitePin, whiteLevel);
    String key = "PIN";
    key += whitePin;
    Bridge.put(key, String(whiteLevel));
    whiteLastLevel = whiteLevel;
   }
   if (blueLevel != blueLastLevel) {
    printTime();
    Console.print(" Setting blue pin to ");
    Console.println(blueLevel);
    analogWrite(bluePin, blueLevel);
    String key = "PIN";
    key += bluePin;
    Bridge.put(key, String(blueLevel));
    blueLastLevel = blueLevel;
   }
  }
}

void checkTemp() {
  if(DEBUG){
    printTime();
    Console.println(" Requesting temperatures...");
  }
  sensors.requestTemperatures(); // Send the command to get temperatures
  float temp1 = sensors.getTempCByIndex(0);
  float temp2 = sensors.getTempCByIndex(1);
  float temp3 = sensors.getTempCByIndex(2);
  float temp4 = sensors.getTempCByIndex(3);
  
  Bridge.put("TEMP1", String(temp1));
  Bridge.put("TEMP2", String(temp2));
  Bridge.put("TEMP3", String(temp3));
  Bridge.put("TEMP4", String(temp3));

  if(DEBUG){
    Console.print(" Temperature for Device 1 is: "); // Tank 
    Console.println(temp1);  
    Console.print("Temperature for Device 2 is: "); // Sump
    Console.println(temp2); 
    Console.print("Temperature for Device 3 is: "); // White heatsink
    Console.println(temp3); 
    Console.print("Temperature for Device 4 is: "); // Blue heatsink
    Console.println(temp4); 
  }
  
  if ((temp3 > FANTEMP) and (relay4Status == 0)) {
    printTime();
    Console.print(" Turning fan on, heatsink temp is now ");
    Console.println(sensors.getTempCByIndex(2));
    digitalWrite(relay4Pin, HIGH); // This is reversed as we use the NC contacts as an extra safeguard
    String key = "PIN";
    key += relay4Pin;
    Bridge.put(key, "1");
    relay4Status = 1;
  } 
  if ((temp3 < (FANTEMP -10)) and (relay4Status == 1)) {
    printTime();
    Console.print(" Turning fan off, heatsink temp is now ");
    Console.println(sensors.getTempCByIndex(2));
    digitalWrite(relay4Pin, LOW); // Reversed, see above
    String key = "PIN";
    key += relay4Pin;
    Bridge.put(key, "0");
    relay4Status = 0;
  }
}

void setRelays() {
  // we subtracted 60 mins from dusk to allow Kalk to settle before delivery. Mix for 5 mins every hour. Safeguard check that delivery relay is not on.
  if ((currentPosition > rampStart) and (currentPosition < (fadeEnd -60)) and (minutes < 5) and (relay2Status == 0)) {
    if (relay1Status == 0) {
      printTime();
      Console.println(" Kalk Mixer ON"); 
      digitalWrite(relay1Pin, LOW);
      String key = "PIN";
      key += relay1Pin;
      Bridge.put(key, "1");
      relay1Status = 1;
    }
  } else {
    if (relay1Status == 1) {
      printTime();
      Console.println(" Kalk Mixer OFF"); 
      digitalWrite(relay1Pin, HIGH);
      String key = "PIN";
      key += relay1Pin;
      Bridge.put(key, "0");
      relay1Status = 0;
    }
  }
  
  if ((currentPosition < rampStart) or (currentPosition > fadeEnd) and (relay1Status == 0)) { // safeguard to check that Kalk mixer is not running
    if (relay2Status == 0) {
      printTime();
      Console.println(" Kalk Delivery ON"); 
      digitalWrite(relay2Pin, LOW);
      String key = "PIN";
      key += relay2Pin;
      Bridge.put(key, "1");
      relay2Status = 1;
    }
  } else {
    if (relay2Status == 1) {
      printTime();
      Console.println(" Kalk Delivery OFF");
      digitalWrite(relay2Pin, HIGH);
      String key = "PIN";
      key += relay2Pin;
      Bridge.put(key, "0");
      relay2Status = 0;
    }
  }
}

void process(YunClient client) {
  // read the command
  String command = client.readStringUntil('/');
  // is "analog" command?
  if (command == "analog") {
    analogCommand(client);
  }
}

void analogCommand(YunClient client) {
  int pin, value;

  // Read pin number
  pin = client.parseInt();

  // If the next character is a '/' it means we have an URL
  // with a value like: "/analog/5/120"
  if (client.read() == '/') {
    // Read value and execute command
    value = client.parseInt();
    analogWrite(pin, value);

    // Send feedback to client
    client.println(value);

    // Update datastore key with the current pin value
    String key = "PIN";
    key += pin;
    Bridge.put(key, String(value));
    
    overrideTime = millis();
  }
}


