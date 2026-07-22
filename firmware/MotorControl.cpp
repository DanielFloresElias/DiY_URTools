#include "MotorControl.h"

MotorControl::MotorControl(uint8_t pA, uint8_t pB, uint8_t pPWM, uint8_t pD1, uint8_t pD2, uint8_t pCurr)
  : pinA(pA), pinB(pB), pinPWM(pPWM), pinDir1(pD1), pinDir2(pD2), pinCorrent(pCurr) {
  targetPos = 0;
  Kp = 1.0;
  Ki = 0.0;
  Kd = 0.0;
  errorIntegral = 0;
  errorAnterior = 0;
  correntLimit = 0.8;
  correntHoming = 0.3;
  correntLimitOpen = 0.3;
  correntLimitClose = 0.15;
  gripCurrentThreshold = 0.5;
  maxAccel = 5;
  currentPwm = 0;
  currentState = HOMING;
  deadband = 15;  // Valor per defecte
  firstStateCycle = true;
  overCurrent = false;
  movingTimeOut = 3800;  //(5 segons)
  lastTimeCounter = 0;
}

void MotorControl::begin() {
  pinMode(pinDir1, OUTPUT);
  pinMode(pinDir2, OUTPUT);

  // Inicialitzem l'encoder amb la llibreria (sense ISR!)
  encoder.attachFullQuad(pinA, pinB);
  encoder.setCount(0);

  ledcAttach(pinPWM, freq, resolucio);
}

void MotorControl::home() {
  currentState = HOMING;
  firstStateCycle = true;
  setTarget(-50000);
}

void MotorControl::setPID(float p, float i, float d) {
  Kp = p;
  Ki = i;
  Kd = d;
}
void MotorControl::setKp(float p) {
  Kp = p;
}
void MotorControl::setKi(float i) {
  Ki = i;
}
void MotorControl::setKd(float d) {
  Kd = d;
}
void MotorControl::setTarget(long target) {
  targetPos = target;
  errorIntegral = 0;
}
void MotorControl::setForceLimit(float amps) {
  correntLimit = amps;
}
void MotorControl::setGripThreshold(float amps) {
  gripCurrentThreshold = amps;
}
void MotorControl::setMaxAccel(int accel) {
  maxAccel = accel;
}

long MotorControl::getPosition() {
  return (long)encoder.getCount();
}

float MotorControl::getCurrent() {
  // Mitjana mòbil per eliminar soroll elèctric (Software Filter)
    long adcSum = 0;
    for(int i = 0; i < 8; i++) {
        adcSum += analogRead(pinCorrent);
    }
    return ((adcSum >> 3) * AnalogToVoltage);
    //return ((adcSum / 8.0) * 3.3 / 4095.0);

  //currentSum = (currentSum + (analogRead(pinCorrent) / 500.0)) * 0.5;
  //return ((adcSum / 8.0) / 500.0);
  //return currentSum;
}

float MotorControl::getCurrentSum() {
  return currentSum;
}

void MotorControl::setDeadband(int val) {
  deadband = val;
}

long MotorControl::directConvert(int percent) {
  if (percent > 100) percent = 100;
  if (percent < 0) percent = 0;
  return 17000L - (long)percent * 170L;
}

int MotorControl::inversConvert(long ticks) {
  if (ticks > 17000) ticks = 17000;
  if (ticks < 0) ticks = 0;
  return (17000L - ticks) / 170L;
}

void MotorControl::stopGripper() {
  setTarget(encoder.getCount());
  ledcWrite(pinPWM, 0);
  digitalWrite(pinDir1, LOW);
  digitalWrite(pinDir2, LOW);
  currentPwm = 0;
  errorIntegral = 0;  // Netejem l'integral perquè no s'acumuli mentres parem
  currentState = IDLE;
}

void MotorControl::openGripper(int pos) {
  if (pos > 100) pos = 100;
  if (pos < 0) pos = 0;
  setTarget(directConvert(pos));
  currentState = OPENING;
  firstStateCycle = true;
}
void MotorControl::openGripperManual(){
  currentState = MANUAL_OPEN;
}

void MotorControl::closeGripper(int pos) {
  if (pos > 100) pos = 100;
  if (pos < 0) pos = 0;
  setTarget(directConvert(pos));
  currentState = CLOSING;
  firstStateCycle = true;
}

void MotorControl::closeGripperManual(){
  currentState = MANUAL_CLOSE;
}

void MotorControl::handler() {
  if (millis() - lastTick >= 10) {
    lastTick = millis();
    float current = getCurrent();
    long posicio = getPosition();  // Obtenim posició del hardware PCNT
    long error = targetPos - posicio;

    // 1. Lògica Homing
    if (currentState == HOMING) {

      if (firstStateCycle) {
        firstStateCycle = false;
        lastTimeCounter = millis();
        setTarget(-50000);
      }

      if (millis() - lastTimeCounter > 400) {
        if (((current > correntHoming) || (millis() - lastTimeCounter > movingTimeOut)) && !overCurrent) {
          overCurrent = true;
          encoder.setCount(-2000);
          setTarget(0);
        }
        if (overCurrent) {
          // --- LA ZONA MORTA ---
          if (abs(error) <= deadband) {
            // Estem prou a prop, parem el control
            stopGripper();
            overCurrent = false;
            //Serial.println("HOMING Done");
            return;  // Sortim del handler, no cal calcular PID
          }
        }
      }
    }

    // 2. Lògica IDLE
    if (currentState == IDLE) {
      ;
    }

    // 3. Lògica CLOSING
    if (currentState == CLOSING) {
      if (firstStateCycle) {
        firstStateCycle = false;
        lastTimeCounter = millis();
      }
      if (millis() - lastTimeCounter > 400) {
        if (current > correntLimitClose) {
          //Serial.printf("overCurrent detectat -> setPoint:%d, actualPos:%d\n", inversConvert(targetPos), inversConvert(getPosition()));
          stopGripper();
          return;  // Sortim del handler, no cal calcular PID
        }
        if (millis() - lastTimeCounter > movingTimeOut) {
        //Serial.printf("timeOut detectat -> setPoint:%d, actualPos:%d\n", inversConvert(targetPos), inversConvert(getPosition()));
          stopGripper();
          return;  // Sortim del handler, no cal calcular PID
        }
      }
      if (abs(error) <= deadband) {
        // Estem prou a prop, parem el control
        //Serial.println("CLOSING Done");
        stopGripper();
        return;  // Sortim del handler, no cal calcular PID
      }
    }

    // 4. Lògica OPENING
    if (currentState == OPENING) {
      if (firstStateCycle) {
        firstStateCycle = false;
        lastTimeCounter = millis();
      }
      if (millis() - lastTimeCounter > 400) {
        if (current > correntLimitOpen) {
          //Serial.printf("overCurrent detectat -> setPoint:%d, actualPos:%d\n", inversConvert(targetPos), inversConvert(getPosition()));
          stopGripper();
          return;  // Sortim del handler, no cal calcular PID
        }
        if (millis() - lastTimeCounter > movingTimeOut) {
          //Serial.printf("timeOut detectat -> setPoint:%d, actualPos:%d\n", inversConvert(targetPos), inversConvert(getPosition()));
          stopGripper();
          return;  // Sortim del handler, no cal calcular PID
        }
      }
      if (abs(error) <= deadband) {
        // Estem prou a prop, parem el control
        //Serial.println("OPENING Done");
        stopGripper();
        return;  // Sortim del handler, no cal calcular PID
      }
    }

    
    // 5. Seguretat: Control de sobrecorrent
    if (current > correntLimit) {
      stopGripper();
      return;  // Sortim del handler, no cal calcular PID
    }


    // 6. PID
    if (currentState != IDLE && currentState != MANUAL_CLOSE && currentState != MANUAL_OPEN) {
      errorIntegral = constrain(errorIntegral + error, -500, 500);
      float pidOutput = (Kp * error) + (Ki * errorIntegral) + (Kd * (error - errorAnterior));
      errorAnterior = error;
      pidOutput = constrain(pidOutput, -255, 255);  // Important per no saturar

      // 5. Rampa
      int targetPwm = (int)pidOutput;
      int diff = targetPwm - currentPwm;
      if (diff > maxAccel) diff = maxAccel;
      else if (diff < -maxAccel) diff = -maxAccel;
      currentPwm += diff;
    }


    // 7. Sortida
    int pwmMagnitude = constrain(abs(currentPwm), 0, 255);
    if (currentPwm > 0) {
      digitalWrite(pinDir1, HIGH);
      digitalWrite(pinDir2, LOW);
    } else if (currentPwm < 0) {
      digitalWrite(pinDir1, LOW);
      digitalWrite(pinDir2, HIGH);
    } else {
      digitalWrite(pinDir1, LOW);
      digitalWrite(pinDir2, LOW);
    }
    ledcWrite(pinPWM, pwmMagnitude);

       // Lògica CLOSE MANUAL
    if (currentState == MANUAL_CLOSE) {
        ledcWrite(pinPWM, 255);
        digitalWrite(pinDir1, HIGH);
        digitalWrite(pinDir2, LOW);
    }

    // Lògica OPEN MANUAL
    if (currentState == MANUAL_OPEN) {
        ledcWrite(pinPWM, 255);
        digitalWrite(pinDir1, LOW);
        digitalWrite(pinDir2, HIGH);
    }

  }

}