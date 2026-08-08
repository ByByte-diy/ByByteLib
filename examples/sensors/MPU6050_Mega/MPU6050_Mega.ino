/*
 * ByByteLib - MPU6050 (Accel/Gyro) Demo using MPU6050_tockn library
 *
 * Mega only: on-board MPU6050 (I2C).
 * Library: MPU6050 by tockn (API: MPU6050 mpu(Wire); mpu.begin(); mpu.update(); getters)
 * 
 * LCD Display:
 * - Top row: forward/backward tilt indicator (filled rectangle moves left/right)
 * - Bottom row: left/right tilt indicator (filled rectangle moves left/right)
 */

#include <Wire.h>
#include <MPU6050_tockn.h>
#include <LiquidCrystal_I2C.h>

static LiquidCrystal_I2C lcd(0x27, 16, 2);
MPU6050 mpu(Wire);

void setup() {
  Serial.begin(115200);
  Serial.println(F("=== MPU6050 (tockn) Demo - Mega Only ==="));
  Wire.begin();
  Wire.setClock(400000);
  mpu.begin();
  // Optional: calibrate with static offsets if needed
  // mpu.calcGyroOffsets(true);
  
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("F/B:");
  lcd.setCursor(0,1); lcd.print("L/R:");
}

void loop() {
  mpu.update();
  // Read linear accel (g) and angular velocity (deg/s)
  float ax = mpu.getAccX();
  float ay = mpu.getAccY();
  float az = mpu.getAccZ();
  float gx = mpu.getGyroX();
  float gy = mpu.getGyroY();
  float gz = mpu.getGyroZ();

  Serial.print(F("A[g] "));
  Serial.print(ax, 3); Serial.print(' ');
  Serial.print(ay, 3); Serial.print(' ');
  Serial.print(az, 3);
  Serial.print(F("  G[dps] "));
  Serial.print(gx, 1); Serial.print(' ');
  Serial.print(gy, 1); Serial.print(' ');
  Serial.println(gz, 1);

  // Convert accelerometer values to tilt indicators
  // Map accelerometer values to LCD positions (0-15)
  // Forward/Backward tilt (Ax): positive = forward, negative = backward
  int fbPos = map(constrain(ax * 100, -90, 90), -90, 90, 0, 12);
  
  // Left/Right tilt (Ay): positive = right, negative = left  
  int lrPos = map(constrain(ay * 100, -90, 90), -90, 90, 0, 12);
  
  // Clear and redraw indicators
  lcd.setCursor(4, 0);
  lcd.print("            "); // Clear 12 chars
  lcd.setCursor(4, 1);
  lcd.print("            "); // Clear 12 chars
  
  // Draw filled rectangle at calculated position
  lcd.setCursor(4 + fbPos, 0);
  lcd.write(0xFF); // Filled block character
  
  lcd.setCursor(4 + lrPos, 1);
  lcd.write(0xFF); // Filled block character

  delay(50);
}


