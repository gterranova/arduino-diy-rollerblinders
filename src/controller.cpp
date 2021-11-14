#include "controller.h"
#include <EEPROM.h>

Controller::Controller() 
    : AccelStepper(AccelStepper::FULL4WIRE, 12, 10, 11, 9) {
    // impostare la velocità massima,
    // accelerazione,
    // velocità iniziale
    // numero di passi da compiere
    setMaxSpeed(400);
    setAcceleration(50);
    setSpeed(100);

    // set the digital pin as output:
    pinMode(ledPin, OUTPUT);

    EEPROM.get(EEPROM_ADDR, settings);
    powerOff();
}

void Controller::reset() {
    setPosition(0);
    setMinPosition(0);
    setMaxPosition(DEFAULT_BOTTOM);
#if DEBUG                           
    printInfo();
#endif
}

bool Controller::isRunning() {  // Method/function defined inside the class
    return settings.state.info.run;
}
bool Controller::movingUp() {
    return settings.state.info.direction == DIR_UP;
}
bool Controller::movingDown() {
    return settings.state.info.direction == DIR_DOWN;
}
unsigned int Controller::getPosition() {
    settings.currentPos = currentPosition();
    return settings.currentPos;
}
void Controller::setPosition(unsigned int position) {
    settings.currentPos = position;
    setCurrentPosition(position);
}
void Controller::setMinPosition(unsigned int position) {
    settings.minPos = position;
}
void Controller::setMaxPosition(unsigned int position) {
    settings.maxPos = position;
}
void Controller::toggle() {  // Method/function defined inside the class
    blink(1);
    settings.state.value += 1;
    if ((movingUp() && settings.minPos == settings.currentPos) || (movingDown() && settings.maxPos == settings.currentPos)) {
        settings.state.value += 2;
    }
    if (!isRunning()) {
        stop();
        runToPosition();
        getPosition();
        powerOff();
        store();
    }
    else moveTo(movingUp()? settings.minPos: settings.maxPos);            
}
void Controller::powerOff() {                  // This writes all motor pins LOW, preventing the motor from drawing current when idle.
  digitalWrite(9, LOW);
  digitalWrite(10, LOW);
  digitalWrite(11, LOW);
  digitalWrite(12, LOW);       
}
void Controller::moveToPerc(unsigned int perc) {
    unsigned int pos = settings.minPos + (settings.maxPos - settings.minPos) * ((float)perc / 100);
    if (isRunning()) toggle();

    blink(1);
    settings.state.info.direction = pos > getPosition()? DIR_DOWN: DIR_UP;
    settings.state.info.run = 1;
    moveTo(pos);
}
void Controller::moveRel(long pos) {
    if (isRunning()) toggle();

    blink(1);
    settings.state.value += (pos > 0 && movingDown())? 1: -1;
    move(pos);
}
void Controller::store() {
    EEPROM.put(EEPROM_ADDR, settings);
}
void Controller::process() {  // Method/function defined inside the class
    unsigned int pos = getPosition();
    if (isRunning()) {
        if (distanceToGo() == 0 || (pos <= settings.minPos && movingUp()) || (pos >= settings.maxPos && movingDown())) {
            toggle();
#if DEBUG                           
            printInfo();
#endif
        } 
    } else {
        powerOff();
    }
    if (blinkCount>0) 
        blinkLoop();
    run();
}

void Controller::blink(unsigned int times) {
  blinkCount = times;
}  

void Controller::blinkLoop() {
  // here is where you'd put code that needs to be running all the time.

  // check to see if it's time to blink the LED; that is, if the difference
  // between the current time and last time you blinked the LED is bigger than
  // the interval at which you want to blink the LED.
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
      blinkCount -= 1;
    }

    // set the LED with the ledState of the variable:
    digitalWrite(ledPin, ledState);
  }
}

void Controller::printInfo() {
    char response[255];
    sprintf(response,"{" \
    "\"run\": \"%s\", " \
    "\"direction\": \"%s\", " \
    "\"currentPos\": %d, " \
    "\"minPos\": %d, " \
    "\"maxPos\": %d " \
    "}", isRunning()? "true": "false", movingUp()? "up": "down", settings.currentPos, settings.minPos, settings.maxPos);
    Serial.println( response );
}
