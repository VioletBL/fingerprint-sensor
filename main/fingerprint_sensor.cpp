#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_system.h"
#include "esp_log.h"
#include <Arduino.h>
#include <Adafruit_Fingerprint.h>

// Pin Definitions
#define INTERRUPT_PIN 21
#define TXD_PIN 17  // Yellow wire
#define RXD_PIN 16  // Green wire
#define IMAGE_DELAY_MS 2000 // Delay between collecting images

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial2);

void init_uart() {
    Serial2.begin(57600, SERIAL_8N1, RXD_PIN, TXD_PIN);
}

void setup() {
    Serial.begin(115200);
    pinMode(INTERRUPT_PIN, INPUT);
    init_uart();
    finger.begin(57600);

    if (finger.verifyPassword()) {
        Serial.println("Found fingerprint sensor!");
    } else {
        Serial.println("Did not find fingerprint sensor :(");
        while (1);
    }
}

void enroll_fingerprint(uint8_t id) {
    int p = -1;
    Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
    while (p != FINGERPRINT_OK) {
        p = finger.getImage();
        switch (p) {
            case FINGERPRINT_OK:
                Serial.println("Image taken");
                break;
            case FINGERPRINT_NOFINGER:
                Serial.print(".");
                break;
            case FINGERPRINT_PACKETRECIEVEERR:
                Serial.println("Communication error");
                break;
            case FINGERPRINT_IMAGEFAIL:
                Serial.println("Imaging error");
                break;
            default:
                Serial.println("Unknown error");
                break;
        }
        delay(100);
    }

    p = finger.image2Tz(1);
    switch (p) {
        case FINGERPRINT_OK:
            Serial.println("Image converted");
            break;
        case FINGERPRINT_IMAGEMESS:
            Serial.println("Image too messy");
            return;
        case FINGERPRINT_PACKETRECIEVEERR:
            Serial.println("Communication error");
            return;
        case FINGERPRINT_FEATUREFAIL:
            Serial.println("Could not find fingerprint features");
            return;
        case FINGERPRINT_INVALIDIMAGE:
            Serial.println("Could not find fingerprint features");
            return;
        default:
            Serial.println("Unknown error");
            return;
    }

    Serial.println("Remove finger");
    delay(2000);
    p = 0;
    while (p != FINGERPRINT_NOFINGER) {
        p = finger.getImage();
    }
    Serial.print("ID "); Serial.println(id);
    p = -1;
    Serial.println("Place same finger again");
    while (p != FINGERPRINT_OK) {
        p = finger.getImage();
        switch (p) {
            case FINGERPRINT_OK:
                Serial.println("Image taken");
                break;
            case FINGERPRINT_NOFINGER:
                Serial.print(".");
                break;
            case FINGERPRINT_PACKETRECIEVEERR:
                Serial.println("Communication error");
                break;
            case FINGERPRINT_IMAGEFAIL:
                Serial.println("Imaging error");
                break;
            default:
                Serial.println("Unknown error");
                break;
        }
        delay(100);
    }

    p = finger.image2Tz(2);
    switch (p) {
        case FINGERPRINT_OK:
            Serial.println("Image converted");
            break;
        case FINGERPRINT_IMAGEMESS:
            Serial.println("Image too messy");
            return;
        case FINGERPRINT_PACKETRECIEVEERR:
            Serial.println("Communication error");
            return;
        case FINGERPRINT_FEATUREFAIL:
            Serial.println("Could not find fingerprint features");
            return;
        case FINGERPRINT_INVALIDIMAGE:
            Serial.println("Could not find fingerprint features");
            return;
        default:
            Serial.println("Unknown error");
            return;
    }

    Serial.print("Creating model for #"); Serial.println(id);
    p = finger.createModel();
    if (p == FINGERPRINT_OK) {
        Serial.println("Prints matched!");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
        Serial.println("Communication error");
        return;
    } else if (p == FINGERPRINT_ENROLLMISMATCH) {
        Serial.println("Fingerprints did not match");
        return;
    } else {
        Serial.println("Unknown error");
        return;
    }

    Serial.print("ID "); Serial.println(id);
    p = finger.storeModel(id);
    if (p == FINGERPRINT_OK) {
        Serial.println("Stored!");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
        Serial.println("Communication error");
        return;
    } else if (p == FINGERPRINT_BADLOCATION) {
        Serial.println("Could not store in that location");
        return;
    } else if (p == FINGERPRINT_FLASHERR) {
        Serial.println("Error writing to flash");
        return;
    } else {
        Serial.println("Unknown error");
        return;
    }
}

void loop() {
    bool interrupt_level = digitalRead(INTERRUPT_PIN);
    bool finger_present = !interrupt_level;

    if (finger_present) {
        Serial.println("Finger detected.");
        enroll_fingerprint(1); // Enroll a fingerprint
        Serial.println("Fingerprint saved.");
    } else {
        Serial.println("No finger detected.");
    }
    delay(500);  // Check every 500ms
}


extern "C" void app_main() {
    initArduino();
    setup();
    
    while (true) {
        loop();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}



