#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>

void motorInit();
void motorUpdate();
void motorDrive(const String& mode, int targetL = -1, int targetR = -1);
void motorStop();
void motorFeedWatchdog();

String motorGetMode();
int motorGetPwmLeft();
int motorGetPwmRight();
bool motorIsMoving();

#endif // MOTOR_H
