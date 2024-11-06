#include <WiFi.h>
#include <time.h>
#include <yboard.h>

// const char *ssid = "EB-IOT-LAB";
// const char *password = "enterthen3tl@b";
const char *ssid = "BYU-WiFi";

#define NTP_SERVER "pool.ntp.org"
#define TZ "MST7MDT,M3.2.0,M11.1.0"

time_t now;
tm tm;

void setup() {
    Serial.begin(115200);
    Yboard.setup();
    Yboard.display.setTextSize(1);

    configTime(0, 0, NTP_SERVER); // 0, 0 because we will use TZ in the next line
    setenv("TZ", TZ, 1);          // Set environment variable with your time zone
    tzset();

    // Connect to Wi-Fi
    // WiFi.begin(ssid, password);
    WiFi.begin(ssid);
    while (WiFi.status() != WL_CONNECTED) {
        Yboard.display.setCursor(0, 0);
        Yboard.display.println("Connecting...");
        Yboard.display.display();
        Serial.println("Connecting to WiFi...");
        delay(1000);
    }

    Yboard.display.clearDisplay();
    Yboard.display.setCursor(0, 0);
    Yboard.display.println("Connected!");
    Yboard.display.display();
    Serial.println("Connected to WiFi");
    delay(500);

    Yboard.display.setTextSize(4);
}

void loop() {
    time(&now);
    localtime_r(&now, &tm);

    // Display the time
    Yboard.display.clearDisplay();
    Yboard.display.setCursor(0, 0);
    Yboard.display.printf("%.2d:%.2d", tm.tm_hour, tm.tm_min, tm.tm_sec);
    Yboard.display.display();

    // Update the LEDS with the seconds
    for (int i = 0; i < Yboard.led_count; i++) {
        if (i < ((tm.tm_sec * Yboard.led_count) / 60) + 1) {
            if (tm.tm_min % 2 == 0) {
                Yboard.set_led_color(i + 1, 255, 255, 255);
            } else {
                Yboard.set_led_color(i + 1, 0, 0, 0);
            }
        } else {
            if (tm.tm_min % 2 == 0) {
                Yboard.set_led_color(i + 1, 0, 0, 0);
            } else {
                Yboard.set_led_color(i + 1, 255, 255, 255);
            }
        }
    }

    delay(1000);
}
