
/****************************************************************************/
/*                   "                                                      */
/*          mmmmm  mmm     mmm    mmm    m mm                               */
/*          # # #    #    #   "  #"  "   #"  "                              */
/*          # # #    #     """m  #       #                                  */
/*          # # #  mm#mm  "mmm"  "#mm"   #    v0.11 pre-release             */
/*                                                                          */
/* Microcontroller Interfaced Stepper Control for Ramps                     */
/*                                                                          */
/* Copyright 2016 Jakub Kukiełka                                            */
/*                                                                          */
/* Please see https://github.com/argarak/miscr/wiki/TODO-List for a list of */
/* the features to be done and the features which are already implemented.  */
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

#include <LiquidCrystal.h> // Include the LiquidCrystal library to interface
                           // with an LCD screen

// Taken from the RAMPS test code file...
#define SDPOWER            -1
#define SDSS               53
#define LED_PIN            13

#define FAN_PIN            9

#define BUZZ_PIN           31
#define MAX_TEMP           200 // Maximum temperature in celcius

#define PS_ON_PIN          12
#define KILL_PIN           -1
#define INT_MIN            -10000 // Unfortunately needed for error-checking
                                  // Not technically the lowest value for int
                                  // however, got overflow errors when I did
                                  // enter the lowest value

// Define custom characters for miscr logo
byte downline[8] = {
  B10000,
  B01000,
  B10100,
  B01010,
  B00101,
  B00010,
  B00001,
};

byte midline[8] = {
  B10101,
  B10101,
  B10101,
  B10101,
  B10101,
  B10101,
  B10101,
};

/*
 * Alias for the digitalWrite and pinMode functions.
 */
void dw(int pin, int mode) {digitalWrite(pin, mode);}
void pm(int pin, int mode) {pinMode(pin, mode);}

// Constructor for the updateLCD() function
int updateLCD();

class TimeControl {
 public:
  bool pause; // Whether the machine is paused or not
  unsigned long pausetime; // Stamp for when the machine has been paused
  unsigned int increment; // How long it should wait for
  TimeControl() {
    pause = false;
    pausetime = -1;
    increment = 0;
  }
};

TimeControl time;

/*
 * Feedrate class, stores information about the current delay the motors should
 * run at and the default value on startup.
 */
class Feedrate {
 public:
  float defaultVal;
  float val;
  Feedrate() {
    defaultVal = 500;
    val = defaultVal;
  }
};

Feedrate globalDelay;

/*
 * Command class, still a concept, but the pin will alternate once every
 * millisecond - as defined by the loop() function. Times defines the
 * amount of times this should be done.
 */
class Command {
 public:
  float times; // TODO double data type for half-steps
  Command() {
    times = 0;
  }
};

/*
 * Position class, keeps a global position of where each motor is. miscr does not yet
 * feature any calibration functionality as I do not have a fully built system with
 * endstops for testing, therefore, the firmware thinks it's at (0, 0, 0) every time
 * it resets.
 */
class Position {
 public:
  float x;
  float y;
  float z;
  Position() {
    x = 0; y = 0; z = 0;
  }
};

// Initialise global position object
Position globalPos;

/*
 * Temperature class, keeps information about current temperature (currently just a
 * value, not based from real temperatures yet) and whether the fan should turn on
 * or off. Also includes a function which converts current temperature into an 8-bit
 * value which can be used in the analogWrite() function, when the fan activates.
 */
class Temperature {
 public:
  int celcius;
  bool operation; // If the fan should be turned on

  int getFanSpeed() {
    return map(celcius, 0, MAX_TEMP, 0, 255);
  }

  Temperature() {
    celcius = 0;
    operation = false;
  }
};

Temperature temperature;

/*
 * Message class, provides functions to print out standard messages which, in theory,
 * will make miscr compatible with other G-Code senders.
 */
class Message {
 private:
  void getStatus(int status) {
    if(status == 0)
      Serial.print("ok");
    else if(status == 1)
      Serial.print("rs");
    else
      Serial.print("!!");
  }
 public:
  void msg(int status) {
    getStatus(status);
    Serial.println("");
  }
  void pos(int status) {
    getStatus(status);
    //Serial.print(" L000");
    Serial.print(" C: X: ");
    Serial.print(globalPos.x);
    Serial.print(" Y: ");
    Serial.print(globalPos.y);
    Serial.print(" Z: ");
    Serial.print(globalPos.z);
    Serial.println(" E: N/A");
    //Serial.println("");
  }
  void msg(int status, String info) {
    getStatus(status);
    //Serial.print(" L000");
    Serial.print(" // ");
    Serial.println(info);
    //Serial.println("");
  }
  void pos(int status, String info) {
    getStatus(status);
    //Serial.print(" L000");
    Serial.print(" C: X: ");
    Serial.print(globalPos.x);
    Serial.print(" Y: ");
    Serial.print(globalPos.y);
    Serial.print(" Z: N/A E: N/A");
    Serial.print(" // ");
    Serial.println(info);
    //Serial.println("");
  }
  void tmp(int status) {
    getStatus(status);
    Serial.println(" T:0 B:0");
  }
};

// Initialise global message object
Message out;

/*
 * Stepper class, defines pins, whether it should be updated and a few control
 * functions - one for each stepper.
 */
class Stepper {
 private:
  bool update;
  Command command;
  int incr;
  int stepPin;
  int dirPin;
  int enablePin;
  int minPin;
  int maxPin;
 public:
  int prevpos;
  int pos;
  Stepper(int stepPinIn, int dirPinIn, int enablePinIn,
          int minPinIn, int maxPinIn) {
    stepPin = stepPinIn;
    dirPin = dirPinIn;
    enablePin = enablePinIn;
    minPin = minPinIn;
    maxPin = maxPinIn;
    update = false;
    incr = 0;
    pos = 0;
    prevpos = 0;
  }

  void begin() {
    pm(stepPin, OUTPUT);
    pm(dirPin, OUTPUT);
    pm(enablePin, OUTPUT);
    dw(enablePin, LOW);
  }
  //void step(double times) {}
  void testSpin(float amount) {
    amount *= 80;
    if(amount > 0)
      dw(dirPin, LOW);
    else
      dw(dirPin, HIGH);
    prevpos = pos;
    pos += amount / 80;
    update = true;
    command.times = abs(amount);
  }

  void enable() {
    dw(enablePin, HIGH);
  }

  void disable() {
    dw(enablePin, LOW);
  }

  void updateLoop() {
    if(update) {
      if(incr < command.times) {
        // Alternate
        dw(stepPin, HIGH);
        delayMicroseconds(globalDelay.val);
        dw(stepPin, LOW);

        // Add one to increment
        ++incr;
      } else {
        update = false;
        incr = 0;
      }
    }
  }
};

// Initialise LCD
LiquidCrystal lcd(16, 17, 23, 25, 27, 29);


// Initialise Steppers
//         stepPin | dirPin | enablePin | minPin | maxPin 
Stepper sX(54,       55,      38,         3,       2     );
Stepper sY(60,       61,      56,         14,      15    );
Stepper sZ(46,       48,      62,         18,      19    );
Stepper sE(26,       28,      24,         -1,      -1    );
Stepper sQ(36,       34,      30,         -1,      -1    );

// TODO Implement these functions
/* void flashled() {} */
void fanOn(bool operation) {
  if(operation)
    analogWrite(FAN_PIN, temperature.getFanSpeed());
  else
    digitalWrite(FAN_PIN, LOW);
}
/* bool xswitch() {} */
/* bool yswitch() {} */
/* bool zswitch() {} */

float getIndex(char code, String in) {
  int codeIndex = in.indexOf(code);
  int spaceIndex = in.indexOf(" ");
  bool toEnd = false;

  if(in.length() < 2) {
    return INT_MIN;
  }

  if(spaceIndex == -1) {
    toEnd = true;
  }

  if(codeIndex == -1) {
    return INT_MIN;
  }

  if(!toEnd) {
    String toconvert = in.substring(codeIndex + 1, spaceIndex);
    return toconvert.toFloat();
  } else {
    String toconvert = in.substring(codeIndex + 1);
    return toconvert.toFloat();
  }

  return INT_MIN;
}

String trim(String in) {
  int spaceIndex = in.indexOf(" ");
  return in.substring(spaceIndex + 1, in.length());
}

/*
 * Feedrate functions. These calculate the delay used for each step of each motor
 * from the feedrate and distance. These can convert delays from only one
 * dimension to all three dimensions. The delay is output in microseconds.
 * Not sure if this is the correct way of doing this, but I don't have any
 * experience in writing CNC firmware, so this may be completely non-standard
 * and non-functional.
 */
float calc1DFeedrate(int feedrate, float distance) {
  // Feedrate in mm/min, distance in mm
  float rateseconds = feedrate / 60; // Convert to mm/s

  // Calculate the time
  float time = distance / rateseconds;

  // Calculate delay
  float delay = time / (distance * 80); // Delay for each step, convert mm to
                                        // steps of each motor
  delay *= (float)1000000; // Convert to microseconds

  return delay;
}

float calc2DFeedrate(int feedrate, float x, float y) {
  // Distance formula of two points
  // Squares the difference between the final position and the current position
  // Adds both axes and finally square roots everything
  float distance = sqrt(sq(x - globalPos.x) + sq(y - globalPos.y));

  // Same as before, convert feedrate to mm/s, calculate the time and finally
  // the delay, see function calc1DFeedrate() above for better documentation
  // as it basically does the same thing I need for this function. Therefore,
  // I can just reference calc1DFeedrate(), without copying and pasting the
  // same code!
  return calc1DFeedrate(feedrate, distance);
}

float calc3DFeedrate(int feedrate, float x, float y, float z) {
  // Distance formula of three points
  // Same thing as before, this time also adding the squared difference of the
  // Z value, yet Z values have not been implemented, so I'll use this function
  // later.
  float distance = sqrt(sq(x - globalPos.x) + sq(y - globalPos.y) + sq(z - globalPos.z));

  // Again, return the fnuction calc1DFeedrate()
  return calc1DFeedrate(feedrate, distance);
}

/*
 * Remove the checksum (*) at the end of a line
 */
String removeChecksum(String in) {
  int checksumIndex = in.indexOf('*');

  if(checksumIndex == -1)
    return "";

  return in.substring(0, checksumIndex);
}

/*
 * Provides G-Code parsing functionality
 */
bool parseGCode(String in) {
  int nindex = getIndex('N', in);

  if(nindex != INT_MIN)
    in = trim(in);

  if(in.indexOf('*') != -1)
    in = removeChecksum(in);

  int gindex = getIndex('G', in);

  if(gindex == 1 || gindex == 0) {
    in = trim(in);
    int xindex = getIndex('X', in);

    if(xindex != INT_MIN)
      in = trim(in);

    int yindex = getIndex('Y', in);

    if(yindex != INT_MIN)
      in = trim(in);

    int findex = getIndex('F', in);

    if(findex != INT_MIN) {
      if(xindex != INT_MIN && yindex == INT_MIN) {
        // Temporarily replace globalDelay value and calculate distance for X
        globalDelay.val = calc1DFeedrate(findex, xindex);
      } else if(xindex == INT_MIN && yindex != INT_MIN) {
        // Temporarily replace globalDelay value and calculate distance for X
        globalDelay.val = calc1DFeedrate(findex, yindex);
      } else if(xindex != INT_MIN && yindex != INT_MIN) {
        // Temporarily replace globalDelay value and calculate distanc
        // from the current position to the position specified
        globalDelay.val = calc2DFeedrate(findex, xindex, yindex);
      } else if(xindex == INT_MIN && yindex == INT_MIN) {
        // TODO Permanently replace globalDelay default value
        // globalDelay.defaultVal = findex;
      }
    }

    if(xindex != INT_MIN) sX.testSpin(xindex);
    if(yindex != INT_MIN) sY.testSpin(yindex);

    if(xindex == INT_MIN && yindex == INT_MIN) {
      out.msg(1, "No parameters/Invalid values for G0/G1");
      return false;
    }

    return true;
  } else if(gindex == 28) {
    int xindex = in.indexOf("X");
    int yindex = in.indexOf("Y");

    if(xindex == -1 && yindex == -1) {
      if(globalPos.x != 0)
        sX.testSpin(globalPos.x - (globalPos.x * 2));
      if(globalPos.y != 0)
        sY.testSpin(globalPos.y - (globalPos.y * 2));
    } else if(xindex != -1 && globalPos.x != 0) {
      sX.testSpin(globalPos.x - (globalPos.x * 2));
    } else if(yindex != -1 && globalPos.y != 0) {
      sY.testSpin(globalPos.y - (globalPos.y * 2));
    }

    return true;
  } else if(gindex == 91) {
    return true;
  } else if(gindex == 90) {
    return true; // Not implemented, yet needed to test support for G-Code senders
  } else if(gindex == 92) {
    in = trim(in);

    float xindex = getIndex('X', in);

    if(xindex != INT_MIN) {
      sX.pos = xindex;
      in = trim(in);
    }

    float yindex = getIndex('Y', in);

    if(yindex != INT_MIN)
      sY.pos = yindex;

    if(xindex == INT_MIN && yindex == INT_MIN) {
      sX.pos = 0;
      sY.pos = 0;
      sZ.pos = 0;
    }

    return true;
  } else if(gindex == 4) {
    in = trim(in);

    float pindex = getIndex('P', in);

    if(pindex == INT_MIN) {
      out.msg(1, "Parameter P Missing");
      return false;
    }

    time.pause = true;
    time.increment = pindex;

    return true;
  }

  int mindex = getIndex('M', in);

  if(mindex == 114) {
    out.pos(0);
    return false;
  } else if(mindex == 115) {
    Serial.print("ok PROTOCOL_VERSION:0.11 FIRMWARE_NAME:miscr ");
    Serial.print("FIRMWARE_URL:github.com/argarak/miscr MACHINE_TYPE:N/A ");
    Serial.println("EXTRUDER_COUNT:0");
    return false;
  } else if(mindex == 70 || mindex == 117) {
    int pindex = getIndex('P', in);

    if(pindex == INT_MIN) {
      if(mindex == 70) {
        out.msg(1, "No time specified");
        return false;
      } else {
        in = trim(in);
        lcd.clear();
        lcd.print(in);
        return true;
      }
    } else {
      in = trim(trim(in));
      lcd.clear();
      lcd.print(in);
      delay(pindex);
      return true;
    }
  } else if(mindex == 105) {
    out.tmp(0);
    return false;
  } else if(mindex == 17) {
    // Disable all stepper motors
    sX.disable();
    sY.disable();
    // TODO add more motors
  } else if(mindex == 18) {
    // Enable all stepper motors
    sX.enable();
    sY.enable();
    // TODO add more motors
  } else if(mindex == 300) {
    // Beep sound
    in = trim(in);

    int sindex = getIndex('S', in);

    if(sindex == INT_MIN) {
      out.msg(1, "Frequency not specified");
      return false;
    }

    in = trim(in);
    int pindex = getIndex('P', in);

    if(pindex == INT_MIN) {
      out.msg(1, "Period not specified");
      return false;
    }

    tone(BUZZ_PIN, sindex, pindex);
    return true;
  } else if(mindex == 104) {
    // Set temperature
    in = trim(in);
    int sindex = getIndex('S', in);

    if(sindex == INT_MIN) {
      out.msg(1, "Parameter S Missing");
      return false;
    }

    temperature.celcius = sindex;
    return true;
  } else if(mindex == 106) {
    // Fan on
    temperature.operation = true;
    return true;
  } else if(mindex == 107) {
    // Fan off
    temperature.operation = false;
    return true;
  }

  out.msg(1, "Incorrect/Unsupported G-Code entered");
  return false;
}

void printSupportedCommands() {
  Serial.println("G0/G1  Move");
  Serial.println("G28    Move to Origin");
  Serial.println("M114   Get Current Position");
  Serial.println("M115   Get Firmware Version and Capabilities");
  Serial.println("See more detailed information in the miscr wiki");
  Serial.println("");
}

String serialStr = "";

/*
 * Gets user serial input to be put into the parseGCode() function.
 */
int getSerialInput() {
  if(Serial.available() > 0) {
    serialStr = Serial.readString();
    serialStr.trim();
    if(Serial.read() == -1) {
      Serial.println(serialStr); // debug
      if(serialStr == "?") {
        printSupportedCommands();
        return 0;
      }

      if(parseGCode(serialStr))
        out.msg(0);
    }
  }
  return 0;
}

void initLCD() {
  lcd.createChar(0, downline);
  lcd.createChar(1, midline);

  lcd.begin(16, 2);

  // write top of logo
  lcd.write(byte(0));
  lcd.write(byte(1));
  lcd.write(byte(0));

  // set to bottom
  lcd.setCursor(0, 1);

  // write bottom of logo
  lcd.write(byte(0));
  lcd.write(byte(1));
  lcd.write(byte(0));

  lcd.setCursor(3, 0);

  lcd.print("miscr v0.11");

  lcd.setCursor(3, 1);

  lcd.print("pre-release");

  delay(1000);

  lcd.clear();
  lcd.setCursor(0, 0);

  lcd.print("X: ");
  lcd.print(globalPos.x);

  lcd.setCursor(0, 1);

  lcd.print("Y: ");
  lcd.print(globalPos.y);
}

int updateLCD() {
  if(sX.prevpos == globalPos.x || sY.prevpos == globalPos.y) {
    return -1;
  }

  lcd.clear();
  lcd.setCursor(0, 0);

  lcd.print("X: ");
  lcd.print(globalPos.x);

  lcd.setCursor(0, 1);

  lcd.print("Y: ");
  lcd.print(globalPos.y);

  return 0;
}

void updatePos() {
  globalPos.x = sX.pos;
  globalPos.y = sY.pos;

  //updateLCD();
}

void setup() {
  // Begin serial connection
  Serial.begin(115200);

  // Print welcome message to serial connection
  Serial.println("miscr v0.11 ready [enter \"?\" for supported commands]\n");

  // Initialise LCD
  initLCD();

  // Initialise fan
  pm(FAN_PIN, OUTPUT);

  // Initialise motors
  sX.begin();
  sY.begin();
}

void loop() {
  if(!time.pause) {
    getSerialInput();
    sX.updateLoop();
    sY.updateLoop();
    fanOn(temperature.operation);
    updatePos();
  } else {
    if(time.pausetime == 0)
      time.pausetime = millis();
    if(millis() > time.pausetime + time.increment) {
      time.pausetime = 0;
      time.pause = false; // Return to regular operation
    }
  }
}
