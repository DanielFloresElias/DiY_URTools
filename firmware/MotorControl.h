#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <Arduino.h>
#include <ESP32Encoder.h>

enum GripperState { IDLE, CLOSING, OPENING, MOVING, GRIPPED, NO_OBJECT, HOMING, MANUAL_CLOSE, MANUAL_OPEN };

class MotorControl {
public:
    MotorControl(uint8_t pA, uint8_t pB, uint8_t pPWM, uint8_t pD1, uint8_t pD2, uint8_t pCurr);
    
    void begin();
    void setPID(float p, float i, float d);
    void setKp(float p);
    void setKi(float i);
    void setKd(float d);
    void setTarget(long target);
    void setForceLimit(float amps);
    void setGripThreshold(float amps);
    void setMaxAccel(int accel);
    void home();
    void handler();
    void setDeadband(int val);
    void stopGripper();
    void openGripper(int pos);
    void closeGripper(int pos);
    void openGripperManual();
    void closeGripperManual();
    long directConvert(int percent);
    int inversConvert(long ticks);
    
    long getPosition();
    long getTarget() { return targetPos; }
    GripperState getState() { return currentState; }
    float getCurrent();
    float getCurrentSum();
    

private:
    uint8_t pinA, pinB, pinPWM, pinDir1, pinDir2, pinCorrent;
    ESP32Encoder encoder; // Ara l'encoder és part de la classe
    
    // PID & Control
    float Kp, Ki, Kd;
    float errorIntegral, errorAnterior;
    long targetPos;
    
    float correntHoming;
    float correntLimit;
    float correntLimitOpen;
    float correntLimitClose;
    float currentSum;
    bool overCurrent;
    float gripCurrentThreshold;
    GripperState currentState;
    bool firstStateCycle=true;
    int posTolerance = 10;
    unsigned long lastTimeCounter;
    unsigned long movingTimeOut;
    unsigned long lastTick = 0; 
    
    int maxAccel;
    int currentPwm;
    int deadband;

    const int freq = 1000;
    const int resolucio = 8;
    const float AnalogToVoltage = 3.3 / 4095;
};

#endif