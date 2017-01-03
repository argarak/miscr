
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

#include <LiquidCrystal.h>

// Taken from the RAMPS test code file...
#define SDPOWER            -1
#define SDSS               53
#define LED_PIN            13

#define FAN_PIN            9

#define PS_ON_PIN          12
#define KILL_PIN           -1

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
  void testSpin(int amount) {
    Serial.println(amount);
    if(amount > 0)
      dw(dirPin, LOW);
    else
      dw(dirPin, HIGH);
    update = true;
    command.pin = stepPin;
    command.times = abs(amount);
  }

  void updateLoop() {
    if(update) {
      if(incr < command.times) {
        // Alternate
        dw(stepPin, HIGH);
        delay(1);
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

#define LCD_PINS_RS      27
#define LCD_PINS_ENABLE  29
#define LCD_PINS_D4      37
#define LCD_PINS_D5      35
#define LCD_PINS_D6      33
#define LCD_PINS_D7 31

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
/* void fan(bool operation) {} */
/* bool xswitch() {} */
/* bool yswitch() {} */
/* bool zswitch() {} */

int getIndex(char code, String in) {
  int codeIndex = in.indexOf(code);
  int spaceIndex = in.indexOf(" ");
  bool toEnd = false;

  if(spaceIndex == -1) {
    Serial.println("toEnd is true!");
    toEnd = true;
  }

  Serial.print("spaceindex ");
  Serial.println(spaceIndex);

  if(codeIndex == -1) {
    Serial.print("datablast ");
    delay(200);
    Serial.println(code);
    delay(30);
    return -1;
  }

  if(!toEnd) {
    String toconvert = in.substring(codeIndex + 1, spaceIndex);
    Serial.println("convert :: " + toconvert);
    return toconvert.toInt();
  } else {
    String toconvert = in.substring(codeIndex + 1);
    Serial.println("convert :: " + toconvert);
    return toconvert.toInt();
  }

  return -2;
}

String trim(String in) {
  int spaceIndex = in.indexOf(" ");
  return in.substring(spaceIndex + 1, in.length());
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
  //int gindex = getIndex('G', in);
  int xindex = getIndex('X', in);
  Serial.println(" ");
  Serial.println(in);
  in = trim(in);
  Serial.println(in);
  int yindex = getIndex('Y', in);

  if(xindex != -1 && xindex != -2) sX.testSpin(xindex * 80);
  else Serial.println("getIndex::: x-error " + xindex);

  if(yindex != -1 && yindex != -2) sY.testSpin(yindex * 80);
  else Serial.println("getIndex::: y-error " + yindex);

  // fallback now, will be better implemented later
  return true;
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
  lcd.print("miscr ready.");
}

void setup() {
  // Begin serial connection
  Serial.begin(115200);

  // Print welcome message to serial connection
  Serial.println("miscr v0.1 [enter \"?\" for commands and \"$\" for status]\n");

  // Initialise LCD
  initLCD();

  // Initialise motors
  sX.begin();
  sY.begin();
}

void loop() {
  getSerialInput();
  sX.updateLoop();
  sY.updateLoop();
}
