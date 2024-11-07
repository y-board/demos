#include <WiFi.h>
#include <time.h>
#include <yboard.h>

const char *ssid = "BYU-WiFi";

#define NTP_SERVER "pool.ntp.org"
#define TZ "MST7MDT,M3.2.0,M11.1.0"

time_t now;
tm tm;
int old_sec = 0;
bool done = false;

// Helper function to convert HSV to RGB
void hsvToRgb(int hue, int saturation, int value, int &red, int &green, int &blue) {
    float h = hue / 60.0;
    float s = saturation / 255.0;
    float v = value / 255.0;

    int i = (int)h;
    float f = h - i;
    float p = v * (1 - s);
    float q = v * (1 - s * f);
    float t = v * (1 - s * (1 - f));

    switch (i) {
    case 0:
        red = v * 255;
        green = t * 255;
        blue = p * 255;
        break;
    case 1:
        red = q * 255;
        green = v * 255;
        blue = p * 255;
        break;
    case 2:
        red = p * 255;
        green = v * 255;
        blue = t * 255;
        break;
    case 3:
        red = p * 255;
        green = q * 255;
        blue = v * 255;
        break;
    case 4:
        red = t * 255;
        green = p * 255;
        blue = v * 255;
        break;
    case 5:
        red = v * 255;
        green = p * 255;
        blue = q * 255;
        break;
    }
}

void setup() {
    Serial.begin(115200);
    Yboard.setup();
    Yboard.display.setTextSize(1);

    configTime(0, 0, NTP_SERVER); // 0, 0 because we will use TZ in the next line
    setenv("TZ", TZ, 1);          // Set environment variable with your time zone
    tzset();

    // Connect to Wi-Fi
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
    Yboard.display.printf("%.2d:%.2d", tm.tm_hour, tm.tm_min);
    Yboard.display.display();

    // Calculate hue based on the current minute to create a rainbow effect
    int hue = (tm.tm_min * 255) / 59; // Map minutes (0-59) to hue (0-255)

    // Convert HSV to RGB values
    int red, green, blue;
    hsvToRgb(hue, 255, 255, red, green, blue);

    int last_red, last_green, last_blue;
    int brightness = map((tm.tm_sec % (60 / Yboard.led_count)), 0, 2, 50, 255);
    hsvToRgb(hue, 255, brightness, last_red, last_green, last_blue);
    Serial.printf("Brightness: %d\n", brightness);

    for (int i = 0; i < Yboard.led_count; i++) {
        if (i < ((tm.tm_sec * Yboard.led_count) / 60) + 1) {
            if (tm.tm_min % 2 == 0) {
                if (i == ((tm.tm_sec * Yboard.led_count) / 60)) {
                    Yboard.set_led_color(i + 1, last_red, last_green, last_blue);
                    Serial.printf("Using last color: %d %d %d\n", last_red, last_green, last_blue);
                } else {
                    Yboard.set_led_color(i + 1, red, green, blue);
                }
            } else {
                Yboard.set_led_color(i + 1, 0, 0, 0);
            }
        } else {
            if (tm.tm_min % 2 == 0) {
                Yboard.set_led_color(i + 1, 0, 0, 0);
            } else {
                if (i == ((tm.tm_sec * Yboard.led_count) / 60)) {
                    Yboard.set_led_color(i + 1, last_red, last_green, last_blue);
                    Serial.printf("Using last color: %d %d %d\n", last_red, last_green, last_blue);
                } else {
                    Yboard.set_led_color(i + 1, red, green, blue);
                }
            }
        }
    }

    delay(100);
}
