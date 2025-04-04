/*
 * wx250.h
 *
 *  Created on: April 4, 2025
 *      Author: Aayush S. kulkarni
 */

#ifndef WX250_H_
#define WX250_H_

#include <DynamixelWorkbench.h>

#if defined(__OPENCM904__)
  #define DEVICE_NAME "1" //Dynamixel on Serial1(USART1) <-OpenCM 485EXP
#elif defined(__OPENCR__)
  #define DEVICE_NAME ""
#endif

#define BAUDRATEWX250 1000000
#define MAX_MOTORS 8
#define MOTOR_RETRY_COUNT 3
#define MIN_REQUIRED_MOTORS 4  // Minimum number of motors required to proceed

// Function prototypes
int initDevice(bool waitForMinMotors = true);
bool moveJoint(uint8_t idx, int positionDelta);
bool resetMotor(uint8_t dxl_id);
void printAllPositions();
bool forceEnableMotor(uint8_t dxl_id);
void scanUntilFound(uint8_t minMotors);

// Global data declarations
extern DynamixelWorkbench dxl_wb;
extern int jPositions[MAX_MOTORS];
extern uint8_t scannedID[MAX_MOTORS];
extern uint8_t dxl_cnt;

#endif /* WX250_H_ */
