#include <Wire.h>
#include <MPU6050.h>
#include <Crack_predictor_inferencing.h>
#include <WiFi.h>
#include <ESP_Mail_Client.h>

// WiFi
const char* ssid = "moto g34 5G";
const char* password = "karthi124";

// Email config
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

#define AUTHOR_EMAIL "ppskarthi12@gmail.com"
#define AUTHOR_PASSWORD "endbvnchanjctune"
#define RECIPIENT_EMAIL "karthidevanece@gmail.com"

SMTPSession smtp;

MPU6050 mpu;
float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];

// Ultrasonic
#define TRIG 5
#define ECHO 18

// Buzzer
#define BUZZER 4

float baseDistance = 0;

// 📧 Email function
void sendEmail(String message) {

  SMTP_Message msg;

  msg.sender.name = "Crack Detector";
  msg.sender.email = AUTHOR_EMAIL;
  msg.subject = "⚠️ Crack Detected!";
  msg.addRecipient("User", RECIPIENT_EMAIL);

  msg.text.content = message.c_str();

  ESP_Mail_Session session;
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;

  if (!smtp.connect(&session)) {
    Serial.println("Email connection failed");
    return;
  }

  if (!MailClient.sendMail(&smtp, &msg)) {
    Serial.println("Error sending Email");
  } else {
    Serial.println("Email sent successfully!");
  }

  smtp.closeSession();
}

// 📏 Ultrasonic distance
float getDistance() {

  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH);
  float distance = duration * 0.034 / 2;

  return distance;
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");

  mpu.initialize();

  // Calibration
  Serial.println("Calibrating distance...");
  delay(2000);
  baseDistance = getDistance();

  Serial.print("Base Distance: ");
  Serial.println(baseDistance);
}

void loop() {

  // 🔹 Collect MPU6050 data
  for (int i = 0; i < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE / 6; i++) {

    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    features[i * 6 + 0] = ax / 16384.0;
    features[i * 6 + 1] = ay / 16384.0;
    features[i * 6 + 2] = az / 16384.0;
    features[i * 6 + 3] = gx / 131.0;
    features[i * 6 + 4] = gy / 131.0;
    features[i * 6 + 5] = gz / 131.0;

    delay(10);
  }

  // 🔹 Run AI model
  signal_t signal;
  numpy::signal_from_buffer(features, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);

  ei_impulse_result_t result = {0};
  run_classifier(&signal, &result, false);

  float max_value = 0;
  String detected_class = "";

  for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
    float value = result.classification[ix].value;

    if (value > max_value) {
      max_value = value;
      detected_class = String(result.classification[ix].label);
    }
  }

  // 🔹 Ultrasonic reading
  float currentDistance = getDistance();

  Serial.print("Detected: ");
  Serial.println(detected_class);

  Serial.print("Distance: ");
  Serial.println(currentDistance);

  // 🚨 FINAL CONDITION (AI + Distance)
  if (detected_class == "micro_crack" &&
      abs(currentDistance - baseDistance) > 2.0) {

    Serial.println("🚨 ALERT TRIGGERED");

    // 🔔 Buzzer pattern
    for (int i = 0; i < 3; i++) {
      digitalWrite(BUZZER, HIGH);
      delay(300);
      digitalWrite(BUZZER, LOW);
      delay(300);
    }

    // 📧 Email
    String msg = "⚠️ Crack detected!\n\n";
    msg += "Confidence: " + String(max_value) + "\n";
    msg += "Distance change: " + String(currentDistance - baseDistance);

    sendEmail(msg);

    delay(10000); // avoid spam
  }

  delay(2000);
}