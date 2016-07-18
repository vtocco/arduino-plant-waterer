/*
  SendDataToGoogleSpreadsheet

  Demonstrates appending a row of data to a Google spreadsheet from the Arduino Yun
  using the Temboo Arduino Yun SDK.

  This example code is in the public domain.
*/

#include <Bridge.h>
#include <FileIO.h>
#include <Process.h>

//plant stuff
const int plantSensorPINA = A0;
const int plantSensorPINB = A1;
//moved it from A2
const int waterSensorPIN = A3;

const int plantSensorPowerPINA = 6;
const int plantSensorPowerPINB = 7;
const int waterSensorPowerPIN = 8;

//variables
const int plantAThreshold = 590;  //what sensor level to start watering
const int plantBThreshold = 520;
const int wateringCycleThreshold = 3000;  //how many runs without watering before we alert, 2000 is about 2 days
//how long to water in seconds
const int wateringTime = 15000;  //must be in milliseconds, 15000 is 15 seconsd

const int waterPumpPowerPIN = 9;
const int waterValvePINA = 10;
const int waterValvePINB = 11;

//program counters
unsigned long wateringCountsA = 0;
unsigned long wateringCountsB = 0;
//keep track of the number of program cycles
unsigned long numberRuns = 0;
unsigned long waterLow = 0;

//int plantSensorPowerStateA = HIGH;
//int plantSensorPowerStateB = HIGH;

//time setting stuff
String getTime() {
  String result;
  Process time;
  // Linux command line to get time
  // command date
  // parameters D for the complete date mm/dd/yy
  // T for the time hh:mm:ss
  time.begin("date");
  //time.addParameter("+%rq");
  time.run(); // run the command
  // read the output of the command
  while (time.available() > 0) {
    char c = time.read();
    if (c != '\n')
      result += c;
  }
  return result;
}

void setClock(void) {
  Process p;
  String result;

  Console.println("Setting clock.");

  p.runShellCommand("ntpd -qn -p 0.pool.ntp.org");

  result = "";
  while (p.available() > 0) {
    char c = p.read();
    result += c;
  }
  Console.println(result);
}


void sendEmail(String Subject) {
  Process p;

  String dataString;
  dataString += "From: vtocco@gmail.com\n";
  dataString += "To:vtocco@gmail.com\n";
  dataString += "Subject: " + Subject + "\n\n";
  dataString += "This is a message from Vince's Yun\n";
  dataString += Subject;

  File dataFile = FileSystem.open("/tmp/mail.txt", FILE_WRITE);
  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    // print to the serial port too:
    Console.println(dataString);
  }
  // if the file isn't open, pop up an error:
  else {
    Console.println("error opening mail.txt");
  }

  p.runShellCommand("cat /tmp/mail.txt | ssmtp vtocco@gmail.com  2>&1");

  while (p.running());

  // Read command output.
  while (p.available() > 0) {
    char c = p.read();
    Console.print(c);
  }

  // Ensure the last bit of data is sent.
  Console.flush();
  Console.println("Email Sent - " + Subject);
}


void setup() {
  Bridge.begin();
  Console.begin();
  FileSystem.begin();

  delay(4000);
  Console.println("Console available!\n");

  //make all the pins output mode
  pinMode(plantSensorPowerPINA, OUTPUT);
  pinMode(plantSensorPowerPINB, OUTPUT);
  pinMode(waterPumpPowerPIN, OUTPUT);
  pinMode(waterValvePINA, OUTPUT);
  pinMode(waterValvePINB, OUTPUT);
  pinMode(waterSensorPowerPIN, OUTPUT);

  //make the analog pins input mode (not really needed)
  pinMode(plantSensorPINA, INPUT);
  pinMode(plantSensorPINB, INPUT);
  pinMode(waterSensorPIN, INPUT);


  //for the relay, HIGH will de-activate it, turn off everything
  digitalWrite(waterPumpPowerPIN, HIGH);
  digitalWrite(waterValvePINA, HIGH);
  digitalWrite(waterValvePINB, HIGH);

  //turn off sensors
  digitalWrite(plantSensorPowerPINA, LOW);
  digitalWrite(plantSensorPowerPINB, LOW);
  digitalWrite(waterSensorPowerPIN, LOW);


  setClock();

  sendEmail("Arduino Yun Plant Watering - Starting Up");
}

void loop()
{

  // get the number of milliseconds this sketch has been running
  //unsigned long now = millis();
  String theTime = getTime();
  Console.println("Getting sensor values...");

  digitalWrite(plantSensorPowerPINA, HIGH);
  delay(1000); //let the sensor take a bearing

  unsigned long sensorValuePlantA = analogRead(plantSensorPINA);

  //turn off sensor A
  digitalWrite(plantSensorPowerPINA, LOW);


  if (sensorValuePlantA < plantAThreshold) {
    
    //make sure not to water too much
    if (wateringCountsA < 11) {
      //lets water the plant
      Console.println("Watering Plant A");
      //turn on pump - LOW turns it on from the relay

      digitalWrite(waterPumpPowerPIN, LOW);
      //let it kick in
      delay(500);

      //turn on valve
      digitalWrite(waterValvePINA, LOW);

      //let it water
      delay(wateringTime);

      //turn off valve A - pump can stay on while we check sensor B
      digitalWrite(waterValvePINA, HIGH);

      //increment counter
      wateringCountsA++;

      //reset runs without watering
      numberRuns = 0;

      //turn off the pump
      digitalWrite(waterPumpPowerPIN, HIGH);
    }
  }
  else {
    wateringCountsA = 0;
  }


  digitalWrite(plantSensorPowerPINB, HIGH);
  delay(1000);
  unsigned long sensorValuePlantB = analogRead(plantSensorPINB);
  //turn off sensor B
  digitalWrite(plantSensorPowerPINB, LOW);

  if (sensorValuePlantB < plantBThreshold) {

    //make sure not to water too much
    if (wateringCountsB < 11) {
      //lets water the plant
      Console.println("Watering Plant B");

      //turn on pump
      digitalWrite(waterPumpPowerPIN, LOW);
      delay(500);

      //turn on valve
      digitalWrite(waterValvePINB, LOW);

      //let it water
      delay(wateringTime);

      //turn off the valve
      digitalWrite(waterValvePINB, HIGH);

      //counter increment
      wateringCountsB++;

      //reset runs without watering
      numberRuns = 0;

      //always turn off the pump
      digitalWrite(waterPumpPowerPIN, HIGH);
    }

  }
  else {
    wateringCountsB = 0;
  }

  digitalWrite(waterSensorPowerPIN, HIGH);
  delay(1000);
  unsigned long sensorValueWater = analogRead(waterSensorPIN);

  //turn off sensor
  digitalWrite(waterSensorPowerPIN, LOW);

  if (sensorValuePlantB < 200) {
    //lets water the plant
    Console.println("Water is low");
    waterLow++;
  }
  else {
    waterLow = 0;
  }


  if (wateringCountsA == 10) {
    sendEmail("Plant A watering hit 10 rounds, check the sensor. Watering stopped. ");
  }
  if (wateringCountsB == 10) {
    sendEmail("Plant B watering hit 10 rounds, check the sensor. Watering stopped. ");
  }
  if (numberRuns == wateringCycleThreshold) {
    sendEmail("Have not watered in " + String(wateringCycleThreshold) + " runs, check the sensors");
  }
  if (waterLow == 10) {
    sendEmail("Water is low, please check it");
  }


  String rowData = theTime + " A:" + sensorValuePlantA + " B:" + sensorValuePlantB + " Water: " + sensorValueWater;
  Console.println("Success: " + rowData);

  Console.println("Waiting...");

  //increment counter to check if we ever get to water
  numberRuns++;

  delay(20000);
}

