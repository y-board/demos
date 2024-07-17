#include <Arduino.h>
#include <yboard.h>

QueueHandle_t display_queue;
char buffer[100];

void read_serial(void *pvParameters) {
    while (1) {
        if (Serial.available() > 0) {
            int chars_read = Serial.readBytesUntil('\n', buffer, 100);
            buffer[chars_read] = '\0';

            if (chars_read != 0) {
                int value = atoi(buffer);
                xQueueSend(display_queue, &value, portMAX_DELAY);
            }
        }
        delay(100);
    }
}

void setup() {
    Serial.begin(115200);
    Yboard.setup();

    // Create queue
    display_queue = xQueueCreate(10, sizeof(int));

    // Create a task
    xTaskCreate(read_serial, "Read Serial", 1024, NULL, 1, NULL);
}

void loop() {
    int value;
    if (xQueueReceive(display_queue, &value, portMAX_DELAY)) {
        // Serial.println(value);
        for (int i = 1; i < 21; i++) {
            if (i <= value) {
                Yboard.set_led_color(i, 0, 255, 0);
            } else {
                Yboard.set_led_color(i, 0, 0, 0);
            }
        }
    }
}
