// ============================================================
//  EDGE IMPULSE DATA COLLECTION SKETCH — MPU-6050
//  Hardware : ESP32 + MPU-6050 (SDA=GPIO21, SCL=GPIO22)
//  Purpose  : Collect vibration training data for Edge Impulse
//  Output   : CSV via Serial Monitor → copy → save as .csv
// ============================================================
//
//  WIRING:
//  MPU-6050 VCC → ESP32 3.3V
//  MPU-6050 GND → ESP32 GND
//  MPU-6050 SDA → ESP32 GPIO 21
//  MPU-6050 SCL → ESP32 GPIO 22
//  MPU-6050 AD0 → ESP32 GND  (important!)
// ============================================================

#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

// ── Settings ──
#define SAMPLE_RATE_HZ    100     // 100 samples per second
#define WINDOW_SAMPLES    200     // 200 samples = 2 second window

// ── CHANGE THIS before each recording session ──
// Use exactly one of:
//   normal
//   micro_crack
//   critical_crack
String LABEL = "critical_crack";

void setup() {
  Serial.begin(115200);
  delay(500);
  Wire.begin(21, 22);  // SDA=21, SCL=22

  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("ERROR: MPU-6050 not found!");
    Serial.println("Check: VCC→3.3V, GND→GND, SDA→21, SCL→22, AD0→GND");
    while (1) delay(1000);
  }

  // Configure MPU-6050
  mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_4);   // ±4g range
  mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_500);   // ±500 deg/s
  mpu.setDLPFMode(MPU6050_DLPF_BW_42);              // low pass filter

  Serial.println("==========================================");
  Serial.println("  Edge Impulse Data Collection — MPU6050");
  Serial.println("==========================================");
  Serial.println("Current label : " + LABEL);
  Serial.println();
  Serial.println("Commands (type in Serial Monitor + Send):");
  Serial.println("  s → collect one 2-second sample");
  Serial.println("  r → auto-collect 20 samples in a row");
  Serial.println();
  Serial.println("STEPS:");
  Serial.println("1. Keep sensor on target surface");
  Serial.println("2. Type r → press Send → wait for 20 samples");
  Serial.println("3. Ctrl+A → Ctrl+C in Serial Monitor");
  Serial.println("4. Paste in Notepad → Save as label_data.csv");
  Serial.println("5. Change LABEL in code → re-upload → repeat");
  Serial.println("==========================================");
  Serial.println("Waiting for command...");
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 's' || cmd == 'S') {
      collectOneSample();
    }
    else if (cmd == 'r' || cmd == 'R') {
      Serial.println("Auto-collecting 20 samples — label: " + LABEL);
      Serial.println("Keep sensor in position...");
      delay(1500);
      for (int i = 1; i <=20; i++) {
        Serial.println("--- Sample " + String(i) + " of 20 ---");
        collectOneSample();
        delay(300);
      }
      Serial.println("=== DONE — 20 samples collected for: " + LABEL + " ===");
      Serial.println("Now copy all output and save as " + LABEL + "_data.csv");
    }
  }
}

void collectOneSample() {
  Serial.println();
  Serial.println("Collecting: " + LABEL);
  delay(200);

  // CSV header — 6 axes: accel XYZ + gyro XYZ
  Serial.println("accX,accY,accZ,gyroX,gyroY,gyroZ");

  unsigned long startTime = millis();

  for (int i = 0; i < WINDOW_SAMPLES; i++) {
    unsigned long t = millis() - startTime;

    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    // Convert raw to real units
    // Accel: raw / 8192.0 for ±4g range → m/s²  (* 9.81)
    // Gyro:  raw / 65.5   for ±500 range → deg/s
    float accX  = (ax / 8192.0) * 9.81;
    float accY  = (ay / 8192.0) * 9.81;
    float accZ  = (az / 8192.0) * 9.81;
    float gyroX = gx / 65.5;
    float gyroY = gy / 65.5;
    float gyroZ = gz / 65.5;

    Serial.print(t);
    Serial.print(",");
    Serial.print(accX,  4);  Serial.print(",");
    Serial.print(accY,  4);  Serial.print(",");
    Serial.print(accZ,  4);  Serial.print(",");
    Serial.print(gyroX, 4);  Serial.print(",");
    Serial.print(gyroY, 4);  Serial.print(",");
    Serial.println(gyroZ, 4);

    // Maintain 100 Hz — 10ms per sample
    unsigned long elapsed = millis() - startTime - (i * 10);
    if (elapsed < 10) delay(10 - elapsed);
  }

  Serial.println("--- END: " + LABEL + " ---");
  Serial.println();
  Serial.println("Ready. Type s or r for next sample.");
  Serial.println();
} 