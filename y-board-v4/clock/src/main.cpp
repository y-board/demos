#include "ImprovWiFiLibrary.h"
#include <Preferences.h>
#include <WiFi.h>
#include <time.h>
#include <yboard.h>

#define NTP_SERVER "pool.ntp.org"
#define TZ "MST7MDT,M3.2.0,M11.1.0"

// 'bell-solid', 9x10px
const unsigned char epd_bitmap_bell_solid[] PROGMEM = {0x08, 0x00, 0x1c, 0x00, 0x3e, 0x00, 0x7f,
                                                       0x00, 0x7f, 0x00, 0x7f, 0x00, 0x7f, 0x00,
                                                       0xff, 0x80, 0x08, 0x00, 0x0c, 0x00};
// 'volume-solid', 10x9px
const unsigned char epd_bitmap_volume_solid[] PROGMEM = {0x00, 0x00, 0x0c, 0x00, 0x1c, 0x80,
                                                         0x7d, 0x40, 0xfd, 0x40, 0x7d, 0x40,
                                                         0x1c, 0x80, 0x0c, 0x00, 0x00, 0x00};

// Array of all bitmaps for convenience. (Total bytes used to store images in PROGMEM = 96)
const int epd_bitmap_allArray_LEN = 2;
const unsigned char *epd_bitmap_allArray[2] = {epd_bitmap_bell_solid, epd_bitmap_volume_solid};

time_t now;
tm tm;
int old_sec = 0;
bool old_switch_1;
bool old_switch_2;

ImprovWiFi improvSerial(&Serial);
Preferences preferences;

void update_display();

void onImprovWiFiErrorCb(ImprovTypes::Error err) {
    Yboard.display.setCursor(0, 0);
    Yboard.display.println("Error setting up WiFi!");
    Yboard.display.display();
}

void onImprovWiFiConnectedCb(const char *ssid, const char *password) {
    // Save ssid and password here
    preferences.begin("wifi", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();

    Yboard.display.setCursor(0, 0);
    Yboard.display.println("Connected to WiFi!");
    Yboard.display.display();

    delay(500);

    Yboard.display.setTextSize(4);
    update_display();
    old_switch_1 = Yboard.get_switch(1);
}

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

void update_display() {
    int hour = tm.tm_hour == 12 ? 12 : tm.tm_hour % 12;
    bool pm = tm.tm_hour >= 12;

    Yboard.display.clearDisplay();

    // Draw clock
    Yboard.display.setCursor(0, 0);
    Yboard.display.setTextSize(4);
    Yboard.display.printf("%.2d:%.2d", hour, tm.tm_min);

    // Draw AM/PM indicator
    Yboard.display.setTextSize(1);
    Yboard.display.setCursor(Yboard.display.width() - 11, 0);
    Yboard.display.printf(pm ? "PM" : "AM");

    if (Yboard.get_switch(1)) {
        Yboard.display.drawBitmap(Yboard.display.width() - 24, Yboard.display.height() - 10,
                                  epd_bitmap_allArray[1], 10, 9, 1);
    }

    if (Yboard.get_switch(2)) {
        Yboard.display.drawBitmap(Yboard.display.width() - 9, Yboard.display.height() - 10,
                                  epd_bitmap_allArray[0], 9, 10, 1);
    }

    Yboard.display.display();
}

void update_leds() {
    int num_leds_on = (tm.tm_sec * Yboard.num_leds) / 60 + 1;
    bool even_minute = tm.tm_min % 2 == 0;

    // Calculate hue based on the current minute to create a rainbow effect
    int hue = (tm.tm_min * 255) / 59; // Map minutes (0-59) to hue (0-255)

    // Convert HSV to RGB values
    int red, green, blue;
    hsvToRgb(hue, 255, 255, red, green, blue);

    // Calculate the brightness of the last LED based on the current second
    int last_red, last_green, last_blue;
    int brightness;

    if (even_minute) {
        brightness = map((tm.tm_sec % (60 / Yboard.num_leds)), 0, 2, 50, 255);
    } else {
        brightness = map((tm.tm_sec % (60 / Yboard.num_leds)), 0, 2, 150, 0);
    }

    hsvToRgb(hue, 255, brightness, last_red, last_green, last_blue);

    // Loop through each LED that needs to be turned on
    for (int i = 0; i < num_leds_on; i++) {
        if (even_minute) {
            Yboard.set_led_color(i + 1, red, green, blue);
        } else {
            Yboard.set_led_color(i + 1, 0, 0, 0);
        }
    }

    // Turn on the last LED with a certain amount of brightness
    Yboard.set_led_color(num_leds_on, last_red, last_green, last_blue);

    // Turn off the rest of the LEDs
    for (int i = num_leds_on + 1; i < Yboard.num_leds; i++) {
        if (even_minute) {
            Yboard.set_led_color(i + 1, 0, 0, 0);
        } else {
            Yboard.set_led_color(i + 1, red, green, blue);
        }
    }
}

void play_sound() {
    if (Yboard.get_switch(1)) {
        // This gets played every second
        Yboard.set_sound_file_volume(3);
        Yboard.play_sound_file("/tick-quiet.wav");

        // This gets played every hour
        if (tm.tm_min == 0 && tm.tm_sec == 0) {
            Yboard.set_sound_file_volume(3);
            Yboard.play_sound_file("/beep.wav");
        }
    }
}

void setup() {
    Serial.begin(115200);
    Yboard.setup();
    Yboard.display.setTextSize(1);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(500);

    configTime(0, 0, NTP_SERVER); // 0, 0 because we will use TZ in the next line
    setenv("TZ", TZ, 1);          // Set environment variable with your time zone
    tzset();

    // Set up improv
    improvSerial.setDeviceInfo(ImprovTypes::ChipFamily::CF_ESP32_S3, "Clock", "1.0.0", "Clock");
    improvSerial.onImprovError(onImprovWiFiErrorCb);
    improvSerial.onImprovConnected(onImprovWiFiConnectedCb);

    // See if there are previous credentials saved
    preferences.begin("wifi", true); // Open in read-only mode
    String savedSSID = preferences.getString("ssid", "");
    String savedPassword = preferences.getString("password", "");
    preferences.end();
    if (savedSSID.isEmpty()) {
        Serial.println("No WiFi credentials saved.");
        Yboard.display.setCursor(0, 0);
        Yboard.display.println("No WiFi credentials.");
        Yboard.display.println("Connect to serial.");
        Yboard.display.display();
    } else {
        Serial.println("Found saved WiFi credentials.");
        Yboard.display.setCursor(0, 0);
        Yboard.display.printf("Connecting to \n%s...", savedSSID.c_str());
        Yboard.display.display();
        improvSerial.tryConnectToWifi(savedSSID.c_str(), savedPassword.c_str());
    }
}

void loop() {
    improvSerial.handleSerial();

    if (!improvSerial.isConnected()) {
        return;
    }

    time(&now);
    localtime_r(&now, &tm);

    // If switch has changed, update the display
    if (Yboard.get_switch(1) != old_switch_1) {
        update_display();
        old_switch_1 = Yboard.get_switch(1);
    }
    if (Yboard.get_switch(2) != old_switch_2) {
        update_display();
        old_switch_2 = Yboard.get_switch(2);
    }

    // Skip this loop iteration if the second hasn't changed
    if (tm.tm_sec == old_sec) {
        delay(50);
        return;
    }

    old_sec = tm.tm_sec;

    update_display();
    update_leds();
    play_sound();
}
