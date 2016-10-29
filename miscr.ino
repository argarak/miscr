 
/****************************************************************************/
/*                   "                                                      */
/*          mmmmm  mmm     mmm    mmm    m mm                               */
/*          # # #    #    #   "  #"  "   #"  "                              */
/*          # # #    #     """m  #       #                                  */
/*          # # #  mm#mm  "mmm"  "#mm"   #    v0.1                          */
/*                                                                          */
/* Microcontroller Interfaced Stepper Control for Ramps                     */
/*                                                                          */
/* This is just a prototype version for testing, without many features,     */
/* it currently only includes a Stepper class definition and a few          */
/* simple non-GCode commands which may be issued with a Serial Monitor.     */
/* These include:                                                           */
/*                                                                          */
/*     * startall - Spins all of the motors clockwise and then              */
/*                  anticlockwise 360 degrees;                              */
/*     * stopall  - Terminate the spinning process - if there is one;       */
/*     * startx   - Only spin the X Stepper;                                */
/*     * starty   - Only spin the Y Stepper;                                */
/*                                                                          */
/* (Only two start functions as I don't have more than two Steppers         */
/* connected in my current setup)                                           */
/*                                                                          */
/* Copyright 2016 Jakub Kukiełka                                            */
/*                                                                          */
/* Licensed under the Apache License, Version 2.0 (the "License");          */
/* you may not use this file except in compliance with the License.         */
/* You may obtain a copy of the License at                                  */
/*                                                                          */
/*    http://www.apache.org/licenses/LICENSE-2.0                            */
/*                                                                          */
/* Unless required by applicable law or agreed to in writing, software      */
/* distributed under the License is distributed on an "AS IS" BASIS,        */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. */
/* See the License for the specific language governing permissions and      */
/* limitations under the License.                                           */
/****************************************************************************/


// Taken from the RAMPS test code file...
#define SDPOWER            -1
#define SDSS               53
#define LED_PIN            13

#define FAN_PIN            9

#define PS_ON_PIN          12
#define KILL_PIN           -1

/*
 * Alias for the digitalWrite and pinMode functions
 */
void dw(int pin, int mode) {digitalWrite(pin, mode);}
void pm(int pin, int mode) {pinMode(pin, mode);}

/*
 * Command class, still a concept, but the pin will alternate once every
 * millisecond - as defined by the loop() function. Times defines the
 * amount of times this should be done.
 */
class Command {
 public:
  int pin;
  int times; // TODO double data type for half-steps
  Command() {
    pin = 0;
    times = 0;
  }
};

/*
 * Stepper class, defines pins, whether it should be updated and a few control
 * functions - one for each stepper.
 */
class Stepper {
 public:
  int stepPin;
  int dirPin;
  int enablePin;
  int minPin;
  int maxPin;
  bool update;
  Command command;
  int incr;

  Stepper(int stepPinIn, int dirPinIn, int enablePinIn,
          int minPinIn, int maxPinIn) {
    stepPin = stepPinIn;
    dirPin = dirPinIn;
    enablePin = enablePinIn;
    minPin = minPinIn;
    maxPin = maxPinIn;
    update = false;
    incr = 0;
  }

  void begin() {
    pm(stepPin, OUTPUT);
    pm(dirPin, OUTPUT);
    pm(enablePin, OUTPUT);
    dw(enablePin, LOW);
  }
  //void step(double times) {}
  void testSpin(bool clockwise, int amount) {
    if(clockwise)
      dw(dirPin, LOW);
    else
      dw(dirPin, HIGH);
    update = true;
    command.pin = stepPin;
    command.times = amount;
  }
};


// Initialise Steppers
//         stepPin | dirPin | enablePin | minPin | maxPin 
Stepper sX(54,       55,      38,         3,       2     );
Stepper sY(60,       61,      56,         14,      15    );
Stepper sZ(46,       48,      62,         18,      19    );
Stepper sE(26,       28,      24,         NULL,    NULL  );
Stepper sQ(36,       34,      30,         NULL,    NULL  );

// TODO Implement these functions
/* void flashled() {} */
/* void fan(bool operation) {} */
/* bool xswitch() {} */
/* bool yswitch() {} */
/* bool zswitch() {} */

int getIndex(char code, String in) {
  int codeIndex = in.indexOf(code);
  int spaceIndex = in.indexOf(" ");
  bool toEnd = false;

  if(spaceIndex == -1) {
    toEnd = true;
    Serial.println("toEnd is true!");
  }

  if(codeIndex == -1) {
    Serial.println("nope not found");
    Serial.println("code = " + code);
    return -1;
  }

  if(!toEnd) {
    String toconvert = in.substring(codeIndex + 1, spaceIndex - 1);
    Serial.println("convert :: " + toconvert);
    return toconvert.toInt();
  } else {
    String toconvert = in.substring(codeIndex + 1);
    Serial.println("convert :: " + toconvert);
    return toconvert.toInt();
  }

  return -2;
}

int strtoint(String in) {return in.toInt();}

/*
 * This function does not yet have any GCode parsing functionality,
 * it is currently only used for testing...
 *
 * Thanks to the blog post at:
 * https://www.marginallyclever.com/2013/08/how-to-build-an-2-axis-arduino-cnc-gcode-interpreter/
 * for helping me writing this function!
 */
bool parseGCode(String in) {
  int index = getIndex('X', in);
  if(index != -1) {
    sX.testSpin(true, index);
  } else
    return false;
}

String serialStr = "";

/*
 * Gets user serial input to be put into the parseGCode() function.
 */
void getSerialInput() {
  if(Serial.available() > 0) {
    serialStr = Serial.readString();
    serialStr.trim();
    if(Serial.read() == -1) {
      Serial.println(serialStr);
      if(!parseGCode(serialStr))
        Serial.println("Invalid GCode entered!");
    }
  }
}

void setup() {
  // Begin serial connection
  Serial.begin(115200);

  // Print welcome message
  Serial.println("miscr v0.1 [enter \"?\" for commands and \"$\" for status]\n");

  sX.begin();
  //sY.begin();
}

void loop() {
  getSerialInput();

  if(sX.update) {
    if(sX.incr < sX.command.times) {
      // Alternate
      dw(sX.stepPin, HIGH);
      delay(1);
      dw(sX.stepPin, LOW);

      // Add one to increment
      ++sX.incr;
    } else {
      sX.update = false;
      sX.incr = 0;
    }
  }

  // TODO other motors
}
