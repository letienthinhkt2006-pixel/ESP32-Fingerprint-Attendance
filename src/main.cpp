#include <Arduino.h>
#include <Adafruit_Fingerprint.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ================= CẤU HÌNH PHẦN CỨNG =================
#define RX2_PIN 16
#define TX2_PIN 17
#define BUZZER_PIN 4
#define LED_GREEN_PIN 18
#define LED_RED_PIN 19

// ================= CẤU HÌNH WIFI =================
const char* ssid = "Thinh";
const char* password = "12345678";

// ================= CẤU HÌNH FIREBASE =================
// QUAN TRỌNG: URL REST phải có .json
const char* FIREBASE_URL =
  "https://hihihaha-6f707-default-rtdb.asia-southeast1.firebasedatabase.app/attendance_test.json";

HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

bool isEnrolling = false;

// ================= KHAI BÁO HÀM =================
void connectWiFi();
void beepSuccess();
void beepError();
void checkFingerprint();
uint8_t getFingerprintEnroll(uint8_t id);
void sendAttendanceToFirebase(int id);

// ================= SETUP =================
void setup() {

  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_GREEN_PIN, LOW);
  digitalWrite(LED_RED_PIN, LOW);

  // Kết nối WiFi
  connectWiFi();

  // Kết nối cảm biến vân tay
  mySerial.begin(57600, SERIAL_8N1, RX2_PIN, TX2_PIN);
  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("\n[OK] Da ket noi voi cam bien van tay!");
  } 
  else {
    Serial.println("\n[LOI] Khong tim thay cam bien!");
    Serial.println("Kiem tra lai day TX/RX.");

    while (1) {
      delay(1);
    }
  }

  Serial.println("\n=========== HE THONG CHAM CONG ===========");
  Serial.println("-> Dat tay de CHAM CONG.");
  Serial.println("-> Nhap ID 1-127 de DANG KY van tay.");
  Serial.println("===========================================\n");
}

// ================= VÒNG LẶP CHÍNH =================
void loop() {

  // Nhập ID từ Serial để đăng ký vân tay
  if (Serial.available() > 0) {

    int id = Serial.parseInt();

    while (Serial.available() > 0) {
      Serial.read();
    }

    if (id > 0 && id <= 127) {

      isEnrolling = true;

      Serial.print("\n>>> Bat dau dang ky van tay ID #");
      Serial.println(id);

      getFingerprintEnroll(id);

      isEnrolling = false;
    }
  }

  // Chấm công
  if (!isEnrolling) {
    checkFingerprint();
    delay(50);
  }
}

// ================= KẾT NỐI WIFI =================
void connectWiFi() {

  Serial.print("\nDang ket noi WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 20) {

    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("\n[OK] Da ket noi WiFi!");

    Serial.print("Dia chi IP: ");
    Serial.println(WiFi.localIP());

  } 
  else {

    Serial.println("\n[LOI] Khong the ket noi WiFi!");
  }
}

// ================= XÁC THỰC VÂN TAY =================
void checkFingerprint() {

  uint8_t p = finger.getImage();

  if (p != FINGERPRINT_OK) {
    return;
  }

  p = finger.image2Tz();

  if (p != FINGERPRINT_OK) {

    Serial.println("Hinh anh cham cong qua mo, thu lai.");

    beepError();

    return;
  }

  p = finger.fingerSearch();

  if (p == FINGERPRINT_OK) {

    int matchedId = finger.fingerID;

    Serial.print("\n[THANH CONG] Nhan vien ID #");
    Serial.println(matchedId);

    // Báo hiệu phần cứng
    beepSuccess();

    // Gửi ID lên Firebase
    sendAttendanceToFirebase(matchedId);

  }

  else if (p == FINGERPRINT_NOTFOUND) {

    Serial.println("\n[THAT BAI] Van tay chua duoc dang ky!");

    beepError();
  }

  else {

    Serial.println("\n[LOI] Loi doc du lieu!");

    beepError();
  }
}

// ================= GỬI DỮ LIỆU LÊN FIREBASE =================
void sendAttendanceToFirebase(int id) {

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("[LOI] Mat ket noi WiFi!");

    return;
  }

  HTTPClient http;

  Serial.println("=================================");
  Serial.println("Dang gui du lieu len Firebase...");
  Serial.print("Fingerprint ID: ");
  Serial.println(id);

  // URL đã có .json
  http.begin(FIREBASE_URL);

  // Header JSON
  http.addHeader("Content-Type", "application/json");

  // Dữ liệu gửi lên Firebase
  String jsonPayload =
    "{\"fingerprintId\":" + String(id) + "}";

  Serial.print("JSON gui di: ");
  Serial.println(jsonPayload);

  // POST
  int httpResponseCode = http.POST(jsonPayload);

  Serial.print("HTTP Response Code: ");
  Serial.println(httpResponseCode);

  if (httpResponseCode > 0) {

    String response = http.getString();

    Serial.print("Firebase response: ");
    Serial.println(response);

    if (httpResponseCode == 200 ||
        httpResponseCode == 201) {

      Serial.println("[OK] DA GUI DU LIEU LEN FIREBASE!");

    }
    else {

      Serial.println("[LOI] Firebase khong chap nhan request!");
    }

  }
  else {

    Serial.print("[HTTP ERROR] ");
    Serial.println(http.errorToString(httpResponseCode));
  }

  http.end();

  Serial.println("=================================");
}

// ================= ĐĂNG KÝ VÂN TAY =================
uint8_t getFingerprintEnroll(uint8_t id) {

  int p = -1;

  Serial.print("Buoc 1: Dat ngon tay len cam bien cho ID #");
  Serial.println(id);

  while (p != FINGERPRINT_OK) {

    p = finger.getImage();

    delay(50);
  }

  p = finger.image2Tz(1);

  if (p != FINGERPRINT_OK) {

    beepError();

    return p;
  }

  Serial.println("-> OK lan 1. Bo tay ra...");

  beepSuccess();

  delay(2000);

  p = 0;

  while (p != FINGERPRINT_NOFINGER) {

    p = finger.getImage();

    delay(50);
  }

  Serial.println("Buoc 2: Dat lai cung ngon tay do...");

  p = -1;

  while (p != FINGERPRINT_OK) {

    p = finger.getImage();

    delay(50);
  }

  p = finger.image2Tz(2);

  if (p != FINGERPRINT_OK) {

    beepError();

    return p;
  }

  p = finger.createModel();

  if (p != FINGERPRINT_OK) {

    beepError();

    return p;
  }

  p = finger.storeModel(id);

  if (p == FINGERPRINT_OK) {

    Serial.println(">>> DANG KY THANH CONG! <<<");

    beepSuccess();

  }
  else {

    Serial.println(">>> DANG KY THAT BAI! <<<");

    beepError();
  }

  return p;
}

// ================= BÁO THÀNH CÔNG =================
void beepSuccess() {

  digitalWrite(LED_GREEN_PIN, HIGH);

  digitalWrite(BUZZER_PIN, HIGH);

  delay(150);

  digitalWrite(BUZZER_PIN, LOW);

  digitalWrite(LED_GREEN_PIN, LOW);

  delay(1500);
}

// ================= BÁO LỖI =================
void beepError() {

  digitalWrite(LED_RED_PIN, HIGH);

  for (int i = 0; i < 3; i++) {

    digitalWrite(BUZZER_PIN, HIGH);

    delay(100);

    digitalWrite(BUZZER_PIN, LOW);

    delay(100);
  }

  digitalWrite(LED_RED_PIN, LOW);

  delay(1500);
}