#include <AWS_IOT.h>
#include <WiFi.h>
#include <ArduinoJson.h>

// AWS IoT 설정
char HOST_ADDRESS[] = "a2bx7ei8ft2iig-ats.iot.ap-northeast-2.amazonaws.com";
char CLIENT_ID[] = "Sensor_Device";
char sTOPIC_NAME[] = "$aws/things/Sensor_Device/shadow/update/delta"; // delta 주제
char pTOPIC_NAME[] = "$aws/things/Sensor_Device/shadow/update";      // shadow 업데이트 주제

AWS_IOT awsIot;
const char* ssid = "HGJ";
const char* password = "20010504";

// 모터 핀 설정
#define ENA 5  // PWM 핀 (속도 제어)
#define IN1 18 // 모터 A 방향 제어 핀 1
#define IN2 19 // 모터 A 방향 제어 핀 2

// 상태 변수
char rcvdPayload[512];
int msgReceived = 0;

// JSON 콜백 함수
void mySubCallBackHandler(char* topicName, int payloadLen, char* payLoad) {
  strncpy(rcvdPayload, payLoad, payloadLen);
  rcvdPayload[payloadLen] = 0;
  msgReceived = 1;
}

// 모터 동작 함수
void controlMotor(const char* windowState) {
  if (strcmp(windowState, "open") == 0) {
    Serial.println("Opening window...");
    digitalWrite(IN1, HIGH); // 시계 방향
    digitalWrite(IN2, LOW);
    analogWrite(ENA, 255);   // 최대 속도
    delay(2000);             // 2초 동안 작동
    stopMotor();
    Serial.println("Window opened.");
  } else if (strcmp(windowState, "close") == 0) {
    Serial.println("Closing window...");
    digitalWrite(IN1, LOW); // 반시계 방향
    digitalWrite(IN2, HIGH);
    analogWrite(ENA, 255);   // 최대 속도
    delay(2000);             // 2초 동안 작동
    stopMotor();
    Serial.println("Window closed.");
  }
}

// 모터 정지 함수
void stopMotor() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 0); // 속도 0
  Serial.println("Motor stopped.");
}

void setup() {
  Serial.begin(115200);

  // WiFi 연결
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  // AWS IoT 연결
  if (awsIot.connect(HOST_ADDRESS, CLIENT_ID) == 0) {
    Serial.println("Connected to AWS IoT");
    delay(1000);

    // delta 주제 구독
    if (0 == awsIot.subscribe(sTOPIC_NAME, mySubCallBackHandler)) {
      Serial.println("Subscribed to delta topic");
    } else {
      Serial.println("Subscription failed, check the topic name or credentials");
      while (1);
    }
  } else {
    Serial.println("AWS connection failed, check the HOST address or credentials");
    while (1);
  }

  // 모터 핀 초기화
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  // 모터 초기 상태: 정지
  stopMotor();                                   
}

void loop() {
  if (msgReceived == 1) {
    msgReceived = 0;
    Serial.print("Received Message: ");
    Serial.println(rcvdPayload);

    // JSON 파싱
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, rcvdPayload);
    if (error) {
      Serial.print("JSON parsing failed: ");
      Serial.println(error.c_str());
      return;
    }

    // desired.window 상태 확인
    const char* windowState = doc["state"]["window"];
    if (windowState) {
      Serial.print("Desired window state: ");
      Serial.println(windowState);

      // 모터 제어
      controlMotor(windowState);

      // reported 상태 업데이트
      StaticJsonDocument<512> reportedDoc;
      reportedDoc["state"]["reported"]["window"] = windowState;

      char buffer[512];
      size_t len = serializeJson(reportedDoc, buffer);
      if (awsIot.publish(pTOPIC_NAME, buffer) == 0) {
        Serial.println("Reported window state updated.");
      } else {
        Serial.println("Failed to update reported state.");
      }
    }
  }

  delay(10); // 메시지 처리 대기
}
