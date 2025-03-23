#include <SPI.h>
#include <WiFiNINA.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

// 🔹 Sensor Definitions
#define DHTPIN 2             // DHT11 sensor connected to D2
#define DHTTYPE DHT11 
#define FLAME_SENSOR_PIN 9 

DHT dht(DHTPIN, DHTTYPE);

unsigned long previousMillis = 0;
unsigned long lastTimeUpdate  = 0;
unsigned long lastSendData = 0;  // ⏳ משתנה שישמור את זמן השליחה האחרון


// פרטי חיבור לרשת
char ssid[] = "shalev_2.4";
char pass[] = "314833625";
char server_ip[] = "10.100.102.3";

WiFiClient client;
WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org", 7200, 30000);

// משתנים גלובליים
float temperature_outside = 0.0;
float temperature_inside = 0.0;  
float timeAsFloat = 0.0;  

bool acTurnedOnToday = false;  
bool acTurnedOffToday = false;
bool update = false;
bool fire_detected = false;
bool fireDetectedOnce = false;

float fireDetectionTime = 0.0;

int newTempToSend = 0;
int tempClick = 0;
int lastSentTemp = 0;


String timeNow;  // משתנה גלובלי לשמירת השעה
String currentTime;
void sendRequest(const char* endpoint) {
    if (client.connect(server_ip, 8000)) {
        client.print("GET ");
        client.print(endpoint);
        client.println(" HTTP/1.1");
        client.println("Host: 10.100.102.3");
        client.println("Connection: close");
        client.println();
        client.stop();

    }

}

void turnOnAC() {  sendRequest("/turn_on_ac");}
void turnOffAC() { sendRequest("/turn_off_ac");}
void set_temp_30() { sendRequest("/set_temp_30"); }
void set_temp_28() { sendRequest("/set_temp_28"); }
void set_temp_25() { sendRequest("/set_temp_25"); }
void set_temp_22() { sendRequest("/set_temp_22"); }
void set_temp_18() { sendRequest("/set_temp_18"); }
void set_temp_16() { sendRequest("/set_temp_16"); }



void connectToWiFi() {
   Serial.println("try to connect to wifi");

    while (WiFi.status() != WL_CONNECTED) {
        WiFi.begin(ssid, pass);
        delay(5000);
    }
}

void updateTime() {
    timeClient.update();
    currentTime = timeClient.getFormattedTime();
    timeNow = currentTime;
    int hours = currentTime.substring(0, 2).toInt();
    int minutes = currentTime.substring(3, 5).toInt();
    int seconds = currentTime.substring(6, 8).toInt();

    timeAsFloat = hours + (minutes / 60.0) + (seconds / 3600.0);
   delay(5000);
}

void updateTemperatureOutside() {

    
    if (client.connect("api.openweathermap.org", 80)) {
        client.print(String("GET /data/2.5/weather?q=Tel%20Aviv,IL&appid=f979801612f305a47752921fa4786425&units=metric HTTP/1.1\r\n") +
                     "Host: api.openweathermap.org\r\n" +
                     "Connection: close\r\n\r\n");

        delay(1000);
        String response = "";
        while (client.available()) {
            response += client.readString();
        }

        int jsonIndex = response.indexOf('{');
        if (jsonIndex != -1) {
            String jsonData = response.substring(jsonIndex);
            int tempIndex = jsonData.indexOf("\"temp\":");
            if (tempIndex != -1) {
                int tempStart = tempIndex + 7;
                int tempEnd = jsonData.indexOf(",", tempStart);
                temperature_outside = jsonData.substring(tempStart, tempEnd).toFloat();
            }
        }


        client.stop();
    }
    Serial.println("its ok");
}

void sendTemperatureData(String time, float temperature_outside, float temperature_inside, bool fire_detected, int temp_AC) {
    if (client.connect(server_ip, 8001)) {
        Serial.println("🔗 Connected to Flask Server");

        // ✅ Proper JSON formatting
       String postData = "{\"time\": \"" + time + "\", \"temperature_outside\": " + String(temperature_outside, 2) + 
                  ", \"temperature_inside\": " + String(temperature_inside, 2) + 
                  ", \"fire_detected\": \"" +  String(fire_detected ? "Yes" : "No") + "\", "   // 🔥 תיקון גם כאן
                   "\"air_conditioner_temp\": " + String(temp_AC) + "}";

        Serial.println("📤 Sending Data: " + postData);

        // Sending HTTP POST request
        client.println("POST /upload_data HTTP/1.1");
        client.println("Host: " + String(server_ip));
        client.println("Content-Type: application/json");
        client.print("Content-Length: ");
        client.println(postData.length());
        client.println();
        client.print(postData);

        delay(100);

        while (client.available()) {
            String response = client.readString();
            Serial.println(response);
        }

        client.stop();
    } else {
        Serial.println("❌ Connection to server failed");
    }
}



void updateFlameStatus() {
    int flameState = digitalRead(FLAME_SENSOR_PIN);

    if (flameState == HIGH) {  // Sensor detects fire when LOW
        fire_detected = true;
      
    } else {
        fire_detected = false;
      
    }
}


void updateTemperatureInside() {
    temperature_inside = dht.readTemperature();
    if (isnan(temperature_inside)) {
        Serial.println("❌ ERROR: Failed to read from DHT sensor!");
        temperature_inside = -1;  
    } else {
        Serial.print("✅ Indoor Temperature: ");
        Serial.print(temperature_inside);
        Serial.println(" °C");
    }
}


void logTemperatureData() {
    Serial.print("🌡️ Outdoor Temperature: ");
    Serial.println(temperature_outside);
    Serial.print("🏠 Indoor Temperature: ");
    Serial.println(temperature_inside);
    Serial.print("⏰ Time: ");
    Serial.println(timeNow);
    Serial.print("🔥 Fire Status: ");
    Serial.println(fire_detected);
    Serial.println("------------------------------------------------");
}


void adjustTemperature() {
    updateTemperatureOutside();
    updateTemperatureInside();

    int newTempClick = tempClick;

    if (timeAsFloat >= 8.5 && timeAsFloat < 21.50) {
        if (temperature_outside >= 27) {
            if (temperature_inside >= 26) newTempClick = -9;
            else if (temperature_inside >= 23) newTempClick = -7;
            else newTempClick = -3;
        } else if (temperature_outside >= 18) {
            if (temperature_inside >= 25) newTempClick = -3;
            else if (temperature_inside >= 21) newTempClick = 0;
            else newTempClick = 3;
        } else {
            if (temperature_inside <= 19) newTempClick = 5;
            else newTempClick = 3;
        }

        int newTempToSend = newTempClick + 25;  

        if (newTempToSend != lastSentTemp) {  
            lastSentTemp = newTempToSend;
            tempClick = newTempClick;

            if (newTempToSend == 30) set_temp_30();
            else if (newTempToSend == 28) set_temp_28();
            else if (newTempToSend == 25) set_temp_25();
            else if (newTempToSend == 22) set_temp_22();
            else if (newTempToSend == 18) set_temp_18();
            else if (newTempToSend == 16) set_temp_16();

            delay(2000);
        }
    }
}

void setup() {
    Serial.begin(9600);
    while (!Serial) {}

    if (WiFi.status() == WL_NO_MODULE) {
        while (true);
    }

    lastSendData = millis();



    pinMode(FLAME_SENSOR_PIN, INPUT);
    connectToWiFi();
    timeClient.begin();
    dht.begin();
    updateTime();
    updateTemperatureOutside();
    updateTemperatureInside();

    delay(2000);
}


void loop() {
  updateFlameStatus();

  unsigned long currentMillis = millis();

  if (currentMillis - lastTimeUpdate >= 10000) {  
        updateTime();  // ⏳ עכשיו הזמן יתעדכן רק כל 10 שניות
    }


   

    if (timeAsFloat >= 8.00 && timeAsFloat < 22.00&& !acTurnedOnToday) {
    turnOnAC();
    acTurnedOffToday = false;
    acTurnedOnToday = true;
    if (temperature_outside < 10.00) {
        set_temp_30(); // קר מאוד – 30 מעלות
        tempClick = 5;
    } 
    else if (temperature_outside < 18.00) {
        set_temp_28(); // קריר – 28 מעלות
        tempClick = 3;
    } 
    else if (temperature_outside < 25.00) {
        // לא משנים כלום - נשארים על 25°C כי המזגן נדלק ככה כברירת מחדל
    } 
    else if (temperature_outside < 30.00) {
        set_temp_22(); // מתחיל להיות חם – 22 מעלות
        tempClick = -3;
    } 
    else if (temperature_outside < 35.00) {
        set_temp_18(); // חם מאוד – 18 מעלות
        tempClick = -7;
    } 
    else {
        set_temp_16(); // חום קיצוני – 16 מעלות
        tempClick = -9;
    }
      sendTemperatureData(timeNow, temperature_outside, temperature_inside, fire_detected,tempClick+25);

        delay(2000);
    }



    if (timeAsFloat >= 22.00 && !acTurnedOffToday) {
        turnOffAC();
        acTurnedOffToday = true;
        delay(2000);
        update = false;
    }

    if (fire_detected && !fireDetectedOnce) {  
        turnOffAC();
        acTurnedOffToday = true;
        updateTemperatureOutside();
        updateTemperatureInside();
        fireDetectedOnce = true;  // נועלים את החיישן כך שלא ישלח שוב פקודות
        fireDetectionTime = timeAsFloat; 
        lastSendData = millis(); // שמירת זמן גילוי השרפה
        Serial.println("🔥 שרפה זוהתה! המזגן כובה ונעול ל-30 דקות.");
        sendTemperatureData(timeNow, temperature_outside, temperature_inside, fire_detected,tempClick+25);
    }

    // ⏳ אחרי 30 דקות -> אם אין להבה, הפעל מחדש
    if (fireDetectedOnce && (timeAsFloat - fireDetectionTime >= 0.016)) {  
        if (!fire_detected && timeAsFloat >= 8.00 && timeAsFloat < 22.00) {   
            turnOnAC();
            updateTemperatureOutside();
            updateTemperatureInside();
            fireDetectedOnce = false;  // הסרת הנעילה
            Serial.println("✅ עברו 30 דקות. המזגן הופעל מחדש.");
        } else if (fire_detected) {  // אם עדיין יש להבה, ממשיכים לחכות
            fireDetectionTime = timeAsFloat;  // עדכון זמן כדי להמשיך להמתין
            Serial.println("🔥 עדיין יש שרפה! המזגן נשאר כבוי.");
            lastSendData = millis();
        }
    }

    if (!fire_detected && currentMillis - lastSendData >= 1800000 ) {  // 30 דקות = 1,800,000 מילישניות
        lastSendData = currentMillis;
        adjustTemperature();
        logTemperatureData();
        sendTemperatureData(timeNow, temperature_outside, temperature_inside, fire_detected, tempClick + 25);
    }



    if (timeAsFloat < 1 && !update) { 
       lastSendData = millis();
        acTurnedOnToday = false;
        fireDetectedOnce = false;
        update = true;
        Serial.println("✅ איפוס יומי בוצע בהצלחה!");

    }

delay(10000);
}

    

