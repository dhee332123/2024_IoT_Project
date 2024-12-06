#include <WiFi.h>
#include <HTTPClient.h>
#include <AWS_IOT.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ArduinoJson.h>


// WiFi 설정
const char* ssid = "HGJ";
const char* password = "20010504";


// AWS IoT 설정
char HOST_ADDRESS[] = "a2bx7ei8ft2iig-ats.iot.ap-northeast-2.amazonaws.com";
char CLIENT_ID[] = "sensor_ID";
char pTOPIC_NAME[] = "$aws/things/Sensor_Device/shadow/update";
AWS_IOT awsIot;
char payload[1024];


// 한국환경공단 API 설정
const char* airQualityApiKey = "uW8ur4GRIDZymWMU7BSYsVC3tDgW%2BTQOXY4uwtqoBNTXTyD%2FQ00v1pB8PvAtxI4KH%2BIIhB%2FnG60FFzoSEm8pmw%3D%3D";
const char* airQualityApiUrl = "http://apis.data.go.kr/B552584/ArpltnInforInqireSvc/getCtprvnRltmMesureDnsty";


// BME280 설정
Adafruit_BME280 bme;


// MH-Z19B 설정 (CO2 센서)
HardwareSerial mySerial(1);  // UART1 사용
#define RXD2 16              // ESP32 RX 핀 (센서 TXD 연결)
#define TXD2 17              // ESP32 TX 핀 (센서 RXD 연결)


// MQ-135 핀
#define MQ135_PIN 34  // 아날로그 핀 (AOUT 연결)


// 위치 기반 URL
const char* ipLocationApiUrl = "http://ip-api.com/json";


// 주기 관리
unsigned long lastAirQualityTime = 118000;  // 공기질 데이터 주기
const long airQualityInterval = 120000;     // 2분
unsigned long lastSensorTime = 0;           // 센서 데이터 주기
const long sensorInterval = 2000;           // 2초


// PWM 부저 설정
const int ledChannel = 0;
const int resolution = 8; // 8비트 해상도
const int buzPin = 23;    // 부저 핀
const int duty = 128;     // 볼륨 (0~255)


// 음계 주파수 (C4 ~ A4)
int nFrq[] = {262, 294, 330, 349, 392, 440}; // C4, D4, E4, F4, G4, A4


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
  } else {
    Serial.println("AWS IoT connection failed!");
    while (1)
      ;
  }


  // BME280 초기화
  if (!bme.begin(0x76)) {
    Serial.println("BME280 센서를 찾을 수 없습니다!");
    while (1)
      ;
  }


  // MH-Z19B 초기화
  mySerial.begin(9600, SERIAL_8N1, RXD2, TXD2);


  // PWM 채널에 핀 연결
  ledcAttachPin(buzPin, ledChannel);
  pinMode(MQ135_PIN, INPUT);  


  Serial.println("시작 완료!");
}


// 지역 이름 변환 함수
const char* convertRegionToKorean(const char* region) {
  if (strcmp(region, "Gyeonggi-do") == 0) return "경기";
  else if (strcmp(region, "Seoul") == 0) return "서울";
  else if (strcmp(region, "Incheon") == 0) return "인천";
  else if (strcmp(region, "Busan") == 0) return "부산";
  else if (strcmp(region, "Daegu") == 0) return "대구";
  else if (strcmp(region, "Daejeon") == 0) return "대전";
  else if (strcmp(region, "Gwangju") == 0) return "광주";
  else if (strcmp(region, "Ulsan") == 0) return "울산";
  else if (strcmp(region, "Jeju-do") == 0) return "제주";
  else if (strcmp(region, "Chungcheongbuk-do") == 0) return "충북";
  else if (strcmp(region, "Chungcheongnam-do") == 0) return "충남";
  else if (strcmp(region, "Gangwon-do") == 0) return "강원";
  else if (strcmp(region, "Gyeongsangbuk-do") == 0) return "경북";
  else if (strcmp(region, "Gyeongsangnam-do") == 0) return "경남";
  else if (strcmp(region, "Jeollabuk-do") == 0) return "전북";
  else if (strcmp(region, "Jeollanam-do") == 0) return "전남";
  else return "서울";  // 기본값으로 서울 반환
}


void loop() {
    unsigned long currentMillis = millis();


    // 2초마다 센서 데이터 퍼블리시
    if (currentMillis - lastSensorTime >= sensorInterval) {
        lastSensorTime = currentMillis;
       
        publishSensorData();


        // CO2 데이터 읽기 및 경보 확인
        int co2 = readMHZ19B();
        checkCO2Level(co2); // CO2 경보 확인
    }


    // 2분마다 공기질 데이터 퍼블리시
    if (currentMillis - lastAirQualityTime >= airQualityInterval) {
        lastAirQualityTime = currentMillis;
        publishAirQualityData();
    }
}




// 센서 데이터 읽기 및 퍼블리시
void publishSensorData() {
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;


  // CO2 데이터 읽기 (MH-Z19B)
  int co2 = readMHZ19B();


  // MQ-135 데이터 읽기
  int mq135Value = analogRead(MQ135_PIN);


  // 센서 데이터 AWS IoT로 퍼블리시
  snprintf(payload, sizeof(payload),
           "{ \"state\": { \"reported\": { \"temp\": %.2f, \"humid\": %.2f, \"press\": %.2f, \"co2\": %d, \"mq135\": %d } } }",
           temperature, humidity, pressure, co2, mq135Value);


  if (awsIot.publish(pTOPIC_NAME, payload) == 0) {
    Serial.println("Published Sensor Data to AWS IoT:");
    Serial.println(payload);
  } else {
    Serial.println("Publish Sensor Data to AWS IoT failed");
  }
}


// 공기질 데이터 읽기 및 퍼블리시
void publishAirQualityData() {
  const char* region = getCurrentRegion();
  if (region != nullptr) {
    Serial.print("현재 지역 (영문): ");
    Serial.println(region);


    const char* koreanRegion = convertRegionToKorean(region);
    Serial.print("현재 지역 (한글): ");
    Serial.println(koreanRegion);


    const char* airQuality = getAirQuality(koreanRegion);


    if (airQuality != nullptr) {
      snprintf(payload, sizeof(payload),
               "{ \"state\": { \"reported\": { \"region\": \"%s\", \"airQuality\": %s } } }",
               koreanRegion, airQuality);


      if (awsIot.publish(pTOPIC_NAME, payload) == 0) {
        Serial.println("Published Air Quality Data to AWS IoT:");
        Serial.println(payload);
      } else {
        Serial.println("Publish Air Quality Data to AWS IoT failed");
      }
    }
  } else {
    Serial.println("지역 정보를 가져오지 못했습니다.");
  }
}


// 현재 지역 추정 (IP 기반)
const char* getCurrentRegion() {
  static char region[64];
  HTTPClient http;
  http.begin(ipLocationApiUrl);
  int httpResponseCode = http.GET();


  if (httpResponseCode > 0) {
    String payload = http.getString();
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.println("IP API JSON 파싱 실패");
      return nullptr;
    }
    strncpy(region, doc["regionName"], sizeof(region));
    region[sizeof(region) - 1] = '\0';
    http.end();
    return region;
  } else {
    Serial.printf("IP API 요청 실패, 코드: %d\n", httpResponseCode);
    http.end();
    return nullptr;
  }
}


// MH-Z19B CO2 데이터 읽기 함수
int readMHZ19B() {
  byte request[9] = { 0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79 };
  byte response[9];


  mySerial.write(request, 9);  // 데이터 요청
  delay(100);


  if (mySerial.available() > 0) {
    mySerial.readBytes(response, 9);


    if (response[0] == 0xFF && response[1] == 0x86) {
      int high = response[2];
      int low = response[3];
      return (high << 8) + low;  // CO2 농도 계산
    }
  }


  return -1;  // 오류 발생 시 -1 반환
}


String urlEncode(const char* str) {
  String encodedString = "";
  char c;
  while ((c = *str++)) {
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encodedString += c;
    } else {
      char buffer[4];
      snprintf(buffer, sizeof(buffer), "%%%02X", c);
      encodedString += buffer;
    }
  }
  return encodedString;
}


// CO2 농도에 따른 위험 단계 확인 및 부저 작동
void checkCO2Level(int co2) {
    if (co2 >= 5000) { // 3단계
        Serial.println("위험: 3단계 (5,000 ppm 이상)");
        for (int i = 0; i < 3; i++) { // 높은 음 반복
            playBeep(5, 300); // A4
            delay(200);
        }
    } else if (co2 > 2000) { // 2단계
        Serial.println("주의: 2단계 (2,000~5,000 ppm)");
        for (int i = 0; i < 2; i++) { // 중간 음 두 번
            playBeep(3, 300); // F4
            delay(200);
        }
    } else if (co2 > 1000) { // 1단계
        Serial.println("경고: 1단계 (1,000~2,000 ppm)");
        playBeep(1, 300); // D4
    } else {
        Serial.println("안전: CO2 농도 정상 범위.");
        ledcWrite(0, 0); // 부저 끔
    }
}


// 특정 음 재생 함수
void playBeep(int note, int duration) {
    int frequencies[] = {262, 294, 330, 349, 392, 440}; // C4 ~ A4 음계
    ledcWriteTone(0, frequencies[note]); // PWM 채널에 음 주파수 설정
    ledcWrite(0, 128); // 음 재생 (볼륨 설정)
    delay(duration);
    ledcWrite(0, 0); // 음 끔
}




// 한국환경공단 API로 공기질 정보 가져오기
const char* getAirQuality(const char* region) {
  static char airQuality[512];
  HTTPClient http;
  char url[512];


  // URL 인코딩 적용
  String encodedRegion = urlEncode(region);  // region을 URL 인코딩
  snprintf(url, sizeof(url), "%s?serviceKey=%s&returnType=json&sidoName=%s&ver=1.3&numOfRows=1",
           airQualityApiUrl, airQualityApiKey, encodedRegion.c_str());


  http.begin(url);
  http.addHeader("User-Agent", "ESP32");


  int httpResponseCode = http.GET();


  if (httpResponseCode > 0) {
    String payload = http.getString();


    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.print("JSON 파싱 실패: ");
      Serial.println(error.c_str());
      return nullptr;
    }


    const char* pm10 = doc["response"]["body"]["items"][0]["pm10Value"];
    const char* pm25 = doc["response"]["body"]["items"][0]["pm25Value"];
    const char* o3 = doc["response"]["body"]["items"][0]["o3Value"];


    snprintf(airQuality, sizeof(airQuality),
             "{ \"pm10\": \"%s\", \"pm25\": \"%s\", \"o3\": \"%s\" }",
             pm10, pm25, o3);
    http.end();
    return airQuality;
  } else {
    Serial.printf("HTTP 요청 실패, 코드: %d\n", httpResponseCode);
    http.end();
    return nullptr;
  }
}