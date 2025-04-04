/*
 * wx250.cpp
 *
 *  Created on: April 4, 2025
 *      Author: Aayush S. kulkarni
 */

#include "wx250.h"

DynamixelWorkbench dxl_wb;
int jPositions[MAX_MOTORS] = {0};
uint8_t scannedID[MAX_MOTORS] = {0};
uint8_t dxl_cnt = 0;

// Move a joint by a relative position delta
bool moveJoint(uint8_t dxl_id, int positionDelta) {
  const char *log;
  bool result = dxl_wb.jointMode(dxl_id, 0, 0, &log);
  
  if (result == false) {
    Serial.println(log);
    Serial.println("Failed to change joint mode for motor ID: " + String(dxl_id));
    
    // Try to recover the motor
    if (forceEnableMotor(dxl_id)) {
      result = dxl_wb.jointMode(dxl_id, 0, 0, &log);
      if (result == false) {
        Serial.println("Failed to recover motor ID: " + String(dxl_id));
        return false;
      }
    } else {
      return false;
    }
  }
  
  Serial.println("Moving joint ID: " + String(dxl_id));
  
  // Apply movement with position feedback
  for (int count = 0; count < 3; count++) {
    // Update target position
    jPositions[dxl_id] += positionDelta;
    
    // Set goal position
    dxl_wb.goalPosition(dxl_id, (int32_t)jPositions[dxl_id]);
    
    // Read back current position
    int32_t current_position = 0;
    result = dxl_wb.getPresentPositionData(dxl_id, &current_position, &log);
    
    Serial.print("ID: ");
    Serial.print(dxl_id);
    Serial.print(" | Target: ");
    Serial.print(jPositions[dxl_id]);
    
    if (result) {
      Serial.print(" | Current: ");
      Serial.println(current_position);
      // Update position in our tracking array with actual position
      jPositions[dxl_id] = current_position;
    } else {
      Serial.println(" | Position read failed");
    }
    
    delay(100); // Brief delay to allow motor to move
  }
  
  return true;
}

// Initialize the Dynamixel device and scan for motors
int initDevice(bool waitForMinMotors) {
  const char *log;
  bool result = false;
  
  uint8_t scanned_id[100];
  uint8_t range = 253;
  
  // Initialize the Dynamixel workbench
  result = dxl_wb.init(DEVICE_NAME, BAUDRATEWX250, &log);
  if (result == false) {
    Serial.println(log);
    Serial.println("Failed to initialize Dynamixel workbench!");
    return 0;
  } else {
    Serial.print("Successfully initialized at baudrate: ");
    Serial.println(BAUDRATEWX250);
  }
  
  // Clear previous scan results
  dxl_cnt = 0;
  for (uint8_t num = 0; num < 100; num++) {
    scanned_id[num] = 0;
  }
  
  // Scan for Dynamixel motors
  result = dxl_wb.scan(scanned_id, &dxl_cnt, range, &log);
  if (result == false) {
    Serial.println(log);
    Serial.println("Failed to scan for Dynamixel motors!");
    return 0;
  } else {
    Serial.print("Found ");
    Serial.print(dxl_cnt);
    Serial.println(" Dynamixel motors");
    
    // If required, wait until minimum motors are found
    if (waitForMinMotors && dxl_cnt < MIN_REQUIRED_MOTORS) {
      Serial.println("Not enough motors found. Will continue scanning...");
      return dxl_cnt;
    }
    
    // Initialize position array and print info for each motor
    for (int cnt = 0; cnt < dxl_cnt; cnt++) {
      scannedID[cnt] = scanned_id[cnt];
      
      Serial.print("ID: ");
      Serial.print(scannedID[cnt]);
      Serial.print(" | Model: ");
      Serial.print(dxl_wb.getModelName(scannedID[cnt]));
      
      int32_t get_data = 0;
      result = dxl_wb.getPresentPositionData(scannedID[cnt], &get_data, &log);
      
      if (result) {
        jPositions[scannedID[cnt]] = get_data;
        Serial.print(" | Position: ");
        Serial.println(jPositions[scannedID[cnt]]);
      } else {
        Serial.println(" | Position read failed");
        // Try to recover the motor
        forceEnableMotor(scannedID[cnt]);
      }
    }
  }
  
  return dxl_cnt;
}

// Reset a motor that might be in error state
bool resetMotor(uint8_t dxl_id) {
  const char *log;
  bool result = false;
  
  Serial.print("Attempting to reset motor ID: ");
  Serial.println(dxl_id);
  
  // Try to reboot the motor
  result = dxl_wb.reboot(dxl_id, &log);
  if (!result) {
    Serial.println("Reboot failed: " + String(log));
    return false;
  }
  
  // Give it time to come back online
  delay(1000);
  
  // Check if it's responding
  uint16_t model_number = 0;
  result = dxl_wb.ping(dxl_id, &model_number, &log);
  
  if (result) {
    Serial.println("Motor reset successful");
    
    // Re-initialize the motor to joint mode
    result = dxl_wb.jointMode(dxl_id, 0, 0, &log);
    if (!result) {
      Serial.println("Failed to set joint mode after reset");
      return false;
    }
    
    // Get current position
    int32_t current_pos = 0;
    result = dxl_wb.getPresentPositionData(dxl_id, &current_pos, &log);
    if (result) {
      jPositions[dxl_id] = current_pos;
    }
    
    return true;
  } else {
    Serial.println("Motor reset failed - motor not responding");
    return false;
  }
}

// Print current positions of all detected motors
void printAllPositions() {
  const char *log;
  Serial.println("=== Current Motor Positions ===");
  
  for (int i = 0; i < dxl_cnt; i++) {
    uint8_t id = scannedID[i];
    int32_t current_pos = 0;
    bool result = dxl_wb.getPresentPositionData(id, &current_pos, &log);
    
    Serial.print("Motor ID: ");
    Serial.print(id);
    
    if (result) {
      jPositions[id] = current_pos;  // Update our tracking array
      Serial.print(" | Position: ");
      Serial.println(current_pos);
    } else {
      Serial.println(" | Position read failed");
    }
  }
  
  Serial.println("==============================");
}

// Try to force enable a motor that's not responding properly
bool forceEnableMotor(uint8_t dxl_id) {
  const char *log;
  bool result = false;
  
  Serial.print("Attempting to force enable motor ID: ");
  Serial.println(dxl_id);
  
  // First try a reset
  if (resetMotor(dxl_id)) {
    return true;
  }
  
  // If reset didn't work, try more aggressive recovery
  // First check if the motor responds to ping
  uint16_t model_number = 0;
  result = dxl_wb.ping(dxl_id, &model_number, &log);
  
  if (!result) {
    Serial.println("Motor not responding to ping");
    return false;
  }
  
  // Try to enable torque
  result = dxl_wb.torqueOn(dxl_id, &log);
  if (!result) {
    Serial.println("Failed to enable torque: " + String(log));
    return false;
  }
  
  // Set to joint mode with default values
  result = dxl_wb.jointMode(dxl_id, 0, 0, &log);
  if (!result) {
    Serial.println("Failed to set joint mode: " + String(log));
    return false;
  }
  
  // Try a small movement to test functionality
  int32_t current_pos = 0;
  result = dxl_wb.getPresentPositionData(dxl_id, &current_pos, &log);
  
  if (result) {
    jPositions[dxl_id] = current_pos;
    result = dxl_wb.goalPosition(dxl_id, current_pos + 10, &log);
    
    if (!result) {
      Serial.println("Failed to set test position");
      return false;
    }
    
    delay(500);
    
    int32_t new_pos = 0;
    result = dxl_wb.getPresentPositionData(dxl_id, &new_pos, &log);
    
    if (result && new_pos != current_pos) {
      Serial.println("Motor recovery successful!");
      return true;
    }
  }
  
  Serial.println("Motor recovery failed");
  return false;
}

// Keep scanning until minimum number of motors are found
void scanUntilFound(uint8_t minMotors) {
  int attempts = 0;
  int maxAttempts = 10;  // Limit the number of retry attempts
  
  while (dxl_cnt < minMotors && attempts < maxAttempts) {
    Serial.print("Scan attempt ");
    Serial.print(attempts + 1);
    Serial.print("/");
    Serial.print(maxAttempts);
    Serial.print(" - Found ");
    Serial.print(dxl_cnt);
    Serial.print("/");
    Serial.print(minMotors);
    Serial.println(" motors");
    
    // Try initializing again
    initDevice(false);
    
    // If we still don't have enough motors, wait and try again
    if (dxl_cnt < minMotors) {
      Serial.println("Waiting 2 seconds before next scan attempt...");
      delay(2000);
    }
    
    attempts++;
  }
  
  if (dxl_cnt >= minMotors) {
    Serial.print("Success! Found ");
    Serial.print(dxl_cnt);
    Serial.println(" motors");
  } else {
    Serial.print("Warning: Only found ");
    Serial.print(dxl_cnt);
    Serial.print(" out of ");
    Serial.print(minMotors);
    Serial.println(" required motors");
  }
}
