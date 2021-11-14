#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

#include <AccelStepper.h>

#define EEPROM_ADDR 0
#define DIR_UP   1
#define DIR_DOWN 0

#define DEFAULT_TOP 0
#define DEFAULT_BOTTOM 10*2048

#define DEBUG 1


typedef struct tState {
    unsigned run : 1, direction : 1;
} tState;

union uState {
    tState info;
    unsigned value : 2;
};

typedef struct tSettings {
    uState state;
    unsigned int currentPos;
    unsigned int minPos = DEFAULT_TOP;
    unsigned int maxPos = DEFAULT_BOTTOM;
} Settings;

class Controller: public AccelStepper {        // The class
    private:
        Settings settings;
        AccelStepper *motor;

        // constants won't change. Used here to set a pin number:
        const int ledPin =  LED_BUILTIN;// the number of the LED pin

        // Variables will change:
        int ledState = LOW;             // ledState used to set the LED

        // Generally, you should use "unsigned long" for variables that hold time
        // The value will quickly become too large for an int to store
        unsigned long previousMillis = 0;        // will store last time LED was updated

        // constants won't change:
        const unsigned long interval = 1000;           // interval at which to blink (milliseconds)

        // constants won't change:
        unsigned int blinkCount = 0;

    public:              // Access specifier
        Controller();
        void reset();
        bool isRunning();
        bool movingUp();
        bool movingDown();
        unsigned int getPosition();
        void setPosition(unsigned int position);
        void toggle();
        void powerOff();
        void process();
        void store();
        void moveToPerc(unsigned int perc);
        void moveRel(long pos);
        void setMinPosition(unsigned int position);
        void setMaxPosition(unsigned int position);
        void blink(unsigned int times);
        void blinkLoop();
        void printInfo();
};

#endif