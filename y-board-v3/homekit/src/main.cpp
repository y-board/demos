
#include <HomeSpan.h>
#include <yboard.h>

struct RGB {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct YboardLeds : Service::LightBulb {

    SpanCharacteristic *power;
    SpanCharacteristic *H;
    SpanCharacteristic *S;
    SpanCharacteristic *V;

    YboardLeds() : Service::LightBulb() {
        power = new Characteristic::On();
        H = new Characteristic::Hue(0);
        S = new Characteristic::Saturation(0);
        V = new Characteristic::Brightness(100);

        new SpanButton(Yboard.button1_pin);
    }

    bool update() {
        bool p;
        float v, h, s, r, g, b;

        h = H->getVal<float>();
        s = S->getVal<float>();
        v = V->getVal<float>();

        p = power->getNewVal();

        Serial.printf("Power: %d\n", p);

        // Turn off the LEDs if the power is off
        if (!p) {
            Yboard.set_all_leds_color(0, 0, 0);
            return true;
        }

        if (H->updated()) {
            h = H->getNewVal<float>();
        }

        if (S->updated()) {
            s = S->getNewVal<float>();
        }

        if (V->updated()) {
            v = V->getNewVal<float>();
        }

        LedPin::HSVtoRGB(h, s / 100.0, v / 100.0, &r, &g, &b);

        int R = map(r * 100, 0, 100, 0, 255);
        int G = map(g * 100, 0, 100, 0, 255);
        int B = map(b * 100, 0, 100, 0, 255);

        Serial.printf("RGB: %d %d %d\n", R, G, B);
        Yboard.set_all_leds_color(R, G, B);
        return true;
    }

    void button(int pin, int pressType) override {
        Serial.printf("Button %d %d\n", pin, pressType);

        if (pin == Yboard.button1_pin) {
            if (pressType == SpanButton::SINGLE) {
                if (power->getVal() == 0) {
                    float h, s, v, r, g, b;
                    h = H->getVal<float>();
                    s = S->getVal<float>();
                    v = V->getVal<float>();
                    LedPin::HSVtoRGB(h, s / 100.0, v / 100.0, &r, &g, &b);
                    int R = map(r * 100, 0, 100, 0, 255);
                    int G = map(g * 100, 0, 100, 0, 255);
                    int B = map(b * 100, 0, 100, 0, 255);

                    power->setVal(1);
                    Yboard.set_all_leds_color(R, G, B);
                } else {
                    power->setVal(0);
                    Yboard.set_all_leds_color(0, 0, 0);
                }
            }
        }
    }
};

struct YboardButtons : Service::StatelessProgrammableSwitch {

    SpanCharacteristic *switchEvent;

    YboardButtons(int buttonPin, int index) : Service::StatelessProgrammableSwitch() {
        switchEvent = new Characteristic::ProgrammableSwitchEvent();
        new Characteristic::ServiceLabelIndex(index);
        new SpanButton(buttonPin);
    }

    void button(int pin, int pressType) override {
        Serial.printf("Button %d %d\n", pin, pressType);
        switchEvent->setVal(pressType);
    }
};

void setup() {
    Serial.begin(9600);
    Yboard.setup();

    homeSpan.setControlPin(Yboard.button1_pin);
    homeSpan.begin(Category::Lighting, "Y-Board");

    new SpanAccessory();
    new Service::AccessoryInformation();
    new Characteristic::Identify();
    new Characteristic::Name("Lights");
    new YboardLeds();

    new SpanAccessory();
    new Service::AccessoryInformation();
    new Characteristic::Identify();
    new Characteristic::Name("Push Button Switches");
    new YboardButtons(Yboard.button2_pin, 1);
    new YboardButtons(Yboard.button3_pin, 2);

    // new SpanAccessory();
    // new Service::AccessoryInformation();
    // new Characteristic::Identify();
    // new Characteristic::Name("Temperature Sensor");
    // new DEV_TempSensor();
}

void loop() { homeSpan.poll(); }
