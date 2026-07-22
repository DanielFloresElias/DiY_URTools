#include "DiY_URtool.h"
#include "MotorControl.h"
#include "EffectsEngine.h"

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x01 };

enum ToolStatusID {
  NORMAL_MODE,
  CONFIGURATION_MODE,
  RESET_MODE,
};

ToolStatusID currentToolStatus = NORMAL_MODE;
DiY_URtool ur(mac, 30002, 5);

// --- PINS DEL SISTEMA ---
#define PIN_ENCODER_A 32
#define PIN_ENCODER_B 33
#define PIN_PWM_MOTOR 25
#define PIN_DIR1 26
#define PIN_DIR2 27
#define PIN_CORRENT 34  // ADC1, segur amb WiFi
#define PIN_LEDS 22
#define LED_COUNT 24

EffectsEngine effects(LED_COUNT, PIN_LEDS);
// Pins: A, B, PWM, Dir1, Dir2, Corrent
MotorControl pinces(PIN_ENCODER_A, PIN_ENCODER_B, PIN_PWM_MOTOR, PIN_DIR1, PIN_DIR2, PIN_CORRENT);

unsigned long old_time = 0;
bool f_Open_Gripper = false;
bool Open_Gripper_Old = false;
bool f_Close_Gripper = false;
bool Close_Gripper_Old = false;
bool Openning_Gripper_Old = false;
bool Closing_Gripper_Old = false;


bool f_command = false;
uint8_t old_command = 0;

bool configStatus = false;
uint8_t effect_old = 0;


// Definició variables Digital Input
const int buttonPin = 13;
const unsigned long debounceDelay = 50;
const unsigned long longPressTime = 2000;
const unsigned long longPressResetTime = 10000;

bool lastStableState = HIGH;
bool lastReading = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long pressStartTime = 0;
bool longPressHandled = false;
bool longPressResetHandled = false;



void setup() {
  Serial.begin(115200);
  ur.begin();
  pinces.begin();
  effects.begin();

  Serial.print("IP asignada: ");
  Serial.println(Ethernet.localIP());

  // Tuning (Sintonitza aquests valors!)
  pinces.setPID(0.4, 0.05, 2.0);
  pinces.setForceLimit(0.5);     // Màxim corrent permès
  pinces.setGripThreshold(1.0);  // Corrent per detectar peça
  pinces.setMaxAccel(8);
  pinces.setDeadband(50);

  pinMode(buttonPin, INPUT_PULLUP);
}

void loop() {
  polling();

  ur.handler();      // Supervision comunication over TCP/IP
  pinces.handler();  // Supervision Gripper Motion Control
  selectEffect();
  gripperFunction();

  switch (currentToolStatus) {
    case NORMAL_MODE:
      break;
    case CONFIGURATION_MODE:
      ur.handleSerialCommands();
      break;
    case RESET_MODE:
      ESP.restart();
      break;
    default:
      currentToolStatus = NORMAL_MODE;
      break;
  }


  effects.update();  // Supervision Indicator Light Effects
}

void gripperFunction() {
  if ((ur.getInByte(0) & 0x01) && !Open_Gripper_Old) {  // Open Flag detectet
    //pinces.openGripper(map(ur.getInByte(2), 0, 255, 0, 100));
    pinces.openGripper(ur.getInByte(2));
  }
  Open_Gripper_Old = ur.getInByte(0) & 0x01;
  if ((ur.getInByte(0) & 0x02) && !Close_Gripper_Old) {  // Open Flag detectet
    //pinces.openGripper(map(ur.getInByte(2), 0, 255, 0, 100));
    pinces.closeGripper(ur.getInByte(2));
  }
  Close_Gripper_Old = ur.getInByte(0) & 0x02;

  if ((ur.getInByte(0) & 0x04) && !Openning_Gripper_Old) {
    Serial.println("Openning Press detectada!");
    pinces.openGripperManual();
  }
  if (!(ur.getInByte(0) & 0x04) && Openning_Gripper_Old){
    Serial.println("Opening Release detectada!");
    pinces.stopGripper();
  }
  if ((ur.getInByte(0) & 0x04) && Openning_Gripper_Old) {
    // Control dels límits
    ;
  }
  if ((ur.getInByte(0) & 0x04)) Openning_Gripper_Old = true;
  else Openning_Gripper_Old = false;

  if ((ur.getInByte(0) & 0x08) && !Closing_Gripper_Old){
    Serial.println("Closing Press detectada!");
    pinces.closeGripperManual();
  }
  if (!(ur.getInByte(0) & 0x08) && Closing_Gripper_Old){
    Serial.println("Closing Release detectada!");
    pinces.stopGripper();
  }
  if ((ur.getInByte(0) & 0x08) && Closing_Gripper_Old) {
    // Control dels límits
    ;
  }
  if ((ur.getInByte(0) & 0x08)) Closing_Gripper_Old = true;
  else Closing_Gripper_Old = false;
  
}

void selectEffect() {
  // EFFECTS LIGHT INDICATOR
  uint8_t effect = ur.getInByte(3);
  effects.setColor_1(ur.getInByte(5));
  effects.setColor_2(ur.getInByte(6));
  effects.setInterval(ur.getInByte(4));
  effects.setBrightness(255);
  //effects.setBrightness(ur.getInByte(7));
  if (effect != effect_old) {

    switch (effect) {
      case 0:
        effects.setEffect(NO_EFFECT);
        break;

      case 1:
        effects.setEffect(EFFECT_SOLID);
        break;

      case 2:
        effects.setEffect(EFFECT_BREATHING);
        break;

      case 3:
        effects.setEffect(EFFECT_KNIGHT_RIDER);
        break;

      case 4:
        effects.setEffect(EFFECT_POLICE);
        break;

      case 5:
        effects.setEffect(EFFECT_FLASH);
        break;
      case 6:
        effects.setEffect(EFFECT_RAINBOW);
        break;

      case 7:
        effects.setEffect(EFFECT_ROLL_RING);
        break;

      default:
        effects.setEffect(NO_EFFECT);
        break;
    }
  }
  effect_old = effect;
}


void polling() {
  bool reading = digitalRead(buttonPin);

  // Debounce
  if (reading != lastReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != lastStableState) {
      lastStableState = reading;

      if (lastStableState == LOW) {  // Botó premut
        pressStartTime = millis();
        longPressHandled = false;
        longPressResetHandled = false;
      } else {  // Botó alliberat
        longPressHandled = false;
        longPressResetHandled = false;
      }
    }
  }

  // Comprovació de long press
  if (lastStableState == LOW && !longPressHandled) {
    if (millis() - pressStartTime >= longPressTime) {
      onLongPress();
      longPressHandled = true;  // Evita múltiples crides
    }
  }

  // Comprovació de long press Reset
  if (lastStableState == LOW && !longPressResetHandled) {
    if (millis() - pressStartTime >= longPressResetTime) {
      onLongPressReset();
      longPressResetHandled = true;  // Evita múltiples crides
    }
  }

  lastReading = reading;
}

void onLongPress() {
  configStatus = !configStatus;
  if (configStatus) {
    currentToolStatus = CONFIGURATION_MODE;
  } else {
    currentToolStatus = NORMAL_MODE;
  }
}

void onLongPressReset() {
  currentToolStatus = RESET_MODE;
}