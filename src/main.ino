/**
 * Simple example to demo the El-Client REST calls
 */
#include <Arduino.h>
#include <RCSwitch.h>
#include "controller.h"

#define DEVICE_MAGIC 0x407

Controller ctr;
RCSwitch mySwitch = RCSwitch();

#define USE_MESSENGER
#ifdef USE_MESSENGER
#include "Messenger.h"
#else
#include <ELClient.h>
#include <ELClientWebServer.h>

// Initialize a connection to esp-link using the normal hardware serial port both for
// SLIP and for debug messages. Esp-link will show the debug messages in the uC console
// because they are not SLIP framed.
ELClient esp(&Serial);

// Initialize the Web-Server client
ELClientWebServer webServer(&esp);

// Initialize CMD client (for GetTime)
//ELClientCmd cmd(&esp);

void curtainsInit() { }

// called at page refreshing
void curtainsRefreshCb(char * url)
{
  if( ctr.isRunning() )
    webServer.setArgString(F("text"), F("Curtain is running"));
  else
    webServer.setArgString(F("text"), F("Curtain is stopped"));

}

// called at page loading
void curtainsLoadCb(char * url)
{
    webServer.setArgBoolean(F("Running"), ctr.isRunning());
    webServer.setArgString(F("Direction"), ctr.movingUp()? "up": "down");
    //webServer.setArgInt(F("Min pos"), ctr.getMinPosition());
    //webServer.setArgInt(F("Max pos"), ctr.getMaxPosition());
    webServer.setArgInt(F("Current pos"), ctr.getPosition());
    curtainsRefreshCb( url );
}

// called at button pressing
void curtainsButtonPressCb(char * button)
{
  String btn = button;
  if( btn == F("btn_up") )
  {
    ctr.moveToPerc(0);
  }
  else if( btn == F("btn_down") )
  {
    ctr.moveToPerc(100);
  }
  else if( btn == F("btn_toggle") )
  {
    ctr.toggle();
  }
}
// Callback made from esp-link to notify of wifi status changes
// Here we just print something out for grins
void wifiCb(void* response) {
  ELClientResponse *res = (ELClientResponse*)response;
  if (res->argc() == 1) {
    uint8_t status;
    res->popArg(&status, 1);

    if(status == STATION_GOT_IP) {
      Serial.println("WIFI CONNECTED");
    } else {
      Serial.print("WIFI NOT READY: ");
      Serial.println(status);
    }
  }
}

// Callback made form esp-link to notify that it has just come out of a reset. This means we
// need to initialize it!
void resetCb(void) {
  Serial.println("EL-Client (re-)starting!");
  bool ok = false;
  do {
    ok = esp.Sync();      // sync up with esp-link, blocks for up to 2 seconds
    if (!ok) Serial.println("EL-Client sync failed!");
  } while(!ok);
  Serial.println("EL-Client synced!");
  webServer.setup();
}

#endif

// creazione dell'istanza della classe ctr
/*
   IN1 -> 9
   IN2 -> 10
   IN3 -> 11
   IN4 -> 12
*/
#ifdef USE_MESSENGER
Messenger message(';');

void power(char* param[]) {
  if (atoi(param[0]) == 1) {
    unsigned int perc = atoi(param[1])? map(254-constrain(atoi(param[2]), 0 , 254), 0, 254, 0, 100): 100;
    ctr.moveToPerc(perc);
  }
}

void move(char* param[]) {
  unsigned int perc = !atoi(param[1])? constrain(atoi(param[0]), 0 , 100): atoi(param[0]);
  ctr.moveToPerc(perc);
}

void moveUp(char* param[]) {
  //ctr.moveToPerc(0);
  ctr.move(-2048);
  ctr.runToPosition();
}

void moveDown(char* param[]) {
  //ctr.moveToPerc(100);
  ctr.move(2048);
  ctr.runToPosition();
}

void saveTop(char* param[]) {
  ctr.setMinPosition(ctr.getPosition());
}

void saveBottom(char* param[]) {
  ctr.setMaxPosition(ctr.getPosition());
}

void toggle(char* param[]) {
  ctr.toggle();
}

void print(char* param[]) {
  ctr.printInfo();
}

void reset(char* param[]) {
  ctr.reset();
}

void blink(char* param[]) {
  unsigned int times = atoi(param[0]);
  ctr.blink(times);
}  

void device(char* param[]) {
  unsigned int deviceId = atoi(param[0]);
  unsigned int deviceState = atoi(param[1]);
  unsigned int deviceBri = atoi(param[2]);
  Serial.printf("device %d %s\r\n", deviceId, deviceState==0?"off":"on");
  digitalWrite(LED_BUILTIN, deviceState? HIGH: LOW );
}  

cmdlist_t cmdlist[] = {
  { "power",      power       },    
  { "reset",       reset       },  
  { "move",        move        },  
  { "up",          moveUp      },
  { "down",        moveDown    },
  { "toggle",      toggle      },
  { "saveTop",     saveTop     },
  { "saveBottom",  saveBottom  },
  { "print",       print       },
  { "blink",       blink       },
  { "device",       device       },
  { NULL, NULL }
};

#else
#endif


void setup() {

  Serial.begin(9600);

#ifdef USE_MESSENGER  
  message.attachCommands((cmdlist_t*)&cmdlist);
#else
  // Sync-up with esp-link, this is required at the start of any sketch and initializes the
  // callbacks to the wifi status change callback. The callback gets called with the initial
  // status right after Sync() below completes.
  esp.wifiCb.attach(wifiCb); // wifi status change callback, optional (delete if not desired)
  esp.resetCb = resetCb;
  curtainsInit();

  URLHandler *ledHandler = webServer.createURLHandler("/CURTAINS.html.json");
  ledHandler->loadCb.attach(&curtainsLoadCb);
  ledHandler->refreshCb.attach(&curtainsRefreshCb);
  ledHandler->buttonCb.attach(&curtainsButtonPressCb);
  //ledHandler->setFieldCb.attach(ledSetFieldCb);
  
  resetCb();
#endif
  mySwitch.enableReceive(0);  // Receiver on interrupt 0 => that is pin #2
  //Serial.println( "Started" );
}

unsigned long previousMillis;

void loop()
{
#ifdef USE_MESSENGER  
  if (Serial.available()) message.process(Serial.read());
#else
  esp.Process();

  //if ((millis()-previousMillis) > 4000) {
  //  Serial.println("requesting time");
  //  uint32_t t = cmd.GetTime();
  //  Serial.print("Time: "); Serial.println(t);
  //
  //  previousMillis = millis();
  //}
  
#endif  

  if (mySwitch.available()) { 
    unsigned long value = mySwitch.getReceivedValue();

    char response[100];
    sprintf(response,"Received %lp", value);
    Serial.println(response);

    if ((value>>12) == DEVICE_MAGIC) {
      if ((value>>8) & 0xF) {
        // 0x4071**
        ctr.toggle();
      } else {
        // 0x407000-0x4070FF
        unsigned int pos = map(value & 0xFF, 0, 0xFF, 0, 100);
        ctr.moveToPerc(pos);
      }
    } 

    //Serial.print(" / ");
    //Serial.print( mySwitch.getReceivedBitlength() );
    //Serial.print("bit ");
    //Serial.print("Protocol: ");
    //Serial.println( mySwitch.getReceivedProtocol() );
    
    mySwitch.resetAvailable();
  }
  // distanceToGo restituisce il numero di passi compiuti.
  // Se distanceToGo raggiunge lo zero, cioè numero di passi è uguale a zero
  // inverte il senso di rotazione assegnando un valore negativo al numero di passi
  // da compiere.

  //if (ctr.distanceToGo() == 0)
  //  ctr.moveTo(-ctr.currentPosition());

  // se non è stato raggiunto il valore zero, muove lo stepper di un passo
  ctr.process();
  
}