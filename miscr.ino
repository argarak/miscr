 
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
}

Command::Command() {
  pin = 0;
  times = 0;
}

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

Stepper::Stepper(int stepPinIn, int dirPinIn, int enablePinIn,
                 int minPinIn, int maxPinIn) {
  stepPin = stepPinIn;
  dirPin = dirPinIn;
  enablePin = enablePinIn;
  minPin = minPinIn;
  maxPin = maxPinIn;
  update = false;
  incr = 0;

  pm(stepPin, OUTPUT);
  pm(dirPin, OUTPUT);
  pm(enablePin, OUTPUT);
  dw(enable, LOW);
}

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

/*
 * This function does not yet have any GCode parsing functionality,
 * it is currently only used for testing...
 */
bool parseGCode(String in) {
  if(in == "stopall") {
    Serial.println("TODO Implement");
    return true;
  } else if(in == "startall") {
    sX.testSpin(true, 360);
    return true;
  } else if(in == "?") {
    Serial.println("TODO Display info...");
  } else {
    return false;
  }
}


/*
 * Gets user serial input to be put into the parseGCode() function.
 */
void getSerialInput() {
  String out = "";
  char c;

  while(Serial.available()) {
      c = Serial.read();
      out.concat(character);
  }

  if(content != "") {
    if(!parseGCode()) {
      Serial.println("Invalid GCode entered!");
    }
  }
}

void setup() {
  // Begin serial connection
  Serial.begin(9600);

  // Print welcome message
  Serial.println("miscr v0.1 [enter \"?\" for commands and \"$\" for status]\n\n");
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