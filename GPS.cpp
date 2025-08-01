#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

HardwareSerial gpsSerial(1);
TinyGPSPlus gps;

unsigned long lastReport = 0;

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);
  Serial.println("开始GPS数据解析...");
}

void loop() {
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  unsigned long now = millis();
  if (now - lastReport >= 3000) {
    lastReport = now;

    if (gps.location.isValid()) {
      Serial.printf("定位有效: 纬度=%.6f, 经度=%.6f\n",
                    gps.location.lat(), gps.location.lng());
    } else {
      Serial.println("定位无效");
    }

    if (gps.date.isValid()) {
      Serial.printf("日期: %02d/%02d/%04d\n",
                    gps.date.day(), gps.date.month(), gps.date.year());
    } else {
      Serial.println("日期无效");
    }

    if (gps.time.isValid()) {
      Serial.printf("UTC时间: %02d:%02d:%02d\n",
                    gps.time.hour(), gps.time.minute(), gps.time.second());
    } else {
      Serial.println("时间无效");
    }

    Serial.printf("卫星数: %d\n", gps.satellites.value());
    Serial.printf("HDOP: %.1f\n", gps.hdop.hdop());

    Serial.println("----------------------");
  }
}
