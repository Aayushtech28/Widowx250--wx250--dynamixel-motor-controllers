/*
 * wx250.ino (Code for arduino)
 *
 *  Created on: April 4, 2025
 *      Author: Aayush S. kulkarni
 */

#include "wx250.h"

// How many motors we want to find
const uint8_t TARGET_MOTOR_COUNT = 8;

// How often to update positions (milliseconds)
const unsigned long POSITION_UPDATE_INTERVAL = 2000;
unsigned long lastPositionUpdate = 0;

void setup() {
  Serial.begin(57600);
  
  // Wait for Serial Monitor to open
  while (!Serial) {
    delay(100);
  }
  
  Serial.println("Widow X Robot Arm Controller");
  Serial.println("===========================");
  
  // Initialize and scan for motors
  int foundMotors = initDevice(false);
  
  Serial.print("Initial scan found ");
  Serial.print(foundMotors);
  Serial.println(" motors");
  
  // Continue scanning until we find the target number of motors
  if (foundMotors < TARGET_MOTOR_COUNT) {
    Serial.println("Not all motors found. Starting continuous scanning...");
    scanUntilFound(TARGET_MOTOR_COUNT);
  }
  
  // Print initial positions
  printAllPositions();
  
  // Attempt recovery for any non-responsive motors
  for (int i = 0; i < dxl_cnt; i++) {
    uint8_t id = scannedID[i];
    const char *log;
    int32_t position = 0;
    
    if (!dxl_wb.getPresentPositionData(id, &position, &log)) {
      Serial.print("Motor ID ");
      Serial.print(id);
      Serial.println(" not responding to position queries. Attempting recovery...");
      
      if (forceEnableMotor(id)) {
        Serial.println("Recovery successful!");
      } else {
        Serial.println("Recovery failed. Motor may need service.");
      }
    }
  }
  
  Serial.println("Setup complete!");
}

void loop() {
  // Test all motors with small movements
  for (int i = 0; i < dxl_cnt; i++) {
    uint8_t motorId = scannedID[i];
    
    Serial.print("Testing motor ID: ");
    Serial.println(motorId);
    
    // Move forward
    moveJoint(motorId, 50);
    delay(1000);
    
    // Move back
    moveJoint(motorId, -50);
    delay(1000);
  }
  
  // Display real-time position updates periodically
  unsigned long currentTime = millis();
  if (currentTime - lastPositionUpdate >= POSITION_UPDATE_INTERVAL) {
    printAllPositions();
    lastPositionUpdate = currentTime;
  }
  
  // Add a delay to prevent flooding the serial monitor
  delay(500);
  
  // Optional: Check Serial for commands
  checkSerialCommands();
}

// Process any commands sent via Serial
void checkSerialCommands() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command.startsWith("move")) {
      // Format: move [motor_id] [position_delta]
      int spaceIndex = command.indexOf(' ', 5);
      if (spaceIndex > 0) {
        uint8_t motorId = command.substring(5, spaceIndex).toInt();
        int delta = command.substring(spaceIndex + 1).toInt();
        
        Serial.print("Command: Move motor ");
        Serial.print(motorId);
        Serial.print(" by ");
        Serial.println(delta);
        
        moveJoint(motorId, delta);
      }
    } 
    else if (command == "status") {
      printAllPositions();
    }
    else if (command.startsWith("reset")) {
      // Format: reset [motor_id]
      uint8_t motorId = command.substring(6).toInt();
      resetMotor(motorId);
    }
    else if (command.startsWith("scan")) {
      initDevice(false);
      printAllPositions();
    }
    else {
      Serial.println("Unknown command. Available commands:");
      Serial.println("- move [motor_id] [position_delta]");
      Serial.println("- status");
      Serial.println("- reset [motor_id]");
      Serial.println("- scan");
    }
  }
}
