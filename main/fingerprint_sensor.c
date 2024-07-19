#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_system.h"
#include "esp_log.h"

#define INTERRUPT_PIN (GPIO_NUM_21)
#define TXD_PIN (GPIO_NUM_17)  // Yellow wire
#define RXD_PIN (GPIO_NUM_16)  // Green wire
#define BUF_SIZE (128)
#define IMAGE_DELAY_MS (2000) // Delay between collecting images

static const int UART_NUM = UART_NUM_2;
static volatile bool finger_detected = false;

void init_uart() {
    const uart_config_t uart_config = {
    .baud_rate = 57600,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
};

    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
}

void send_command(const uint8_t* command, size_t len) {
    uart_write_bytes(UART_NUM, (const char*)command, len);
}

int read_response(uint8_t* response, size_t len) {
    int read_len = uart_read_bytes(UART_NUM, response, len, pdMS_TO_TICKS(2000));
    if (read_len < 0) {
        printf("UART read error\n");
    } else if (read_len == 0) {
        printf("UART timeout\n");
    }
    return read_len;
}


void collect_fingerprint_image() {
   uint8_t collect_image_cmd[] = {
    0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, // Header and Module Address
    0x01,                               // Package Identifier (Command)
    0x03,                               // Package Length
    0x01,                               // Instruction Code (GenImg)
    0x00, 0x00                          // Checksum Placeholder
    };

    uint16_t checksum = 0;
    for (int i = 2; i < sizeof(collect_image_cmd) - 2; i++) {
        checksum += collect_image_cmd[i];
    }

    printf("Sending command: ");
        for (int i = 0; i < sizeof(collect_image_cmd); i++) {
            printf("%02X ", collect_image_cmd[i]);
        }
        printf("\n");

    collect_image_cmd[sizeof(collect_image_cmd) - 2] = (checksum >> 8) & 0xFF; // High byte
    collect_image_cmd[sizeof(collect_image_cmd) - 1] = checksum & 0xFF;      // Low byte


    uint8_t response[BUF_SIZE];
    int length = read_response(response, BUF_SIZE);

    if (length > 0) {
        printf("Response received: ");
        for (int i = 0; i < length; i++) {
            printf("%02X ", response[i]);
        }
        printf("\n");

        // Check confirmation code
        if (response[9] == 0x00) {
            printf("Fingerprint image collected successfully.\n");
        } else if (response[9] == 0x02) {
            printf("Can't detect finger.\n");
        } else if (response[9] == 0x03) {
            printf("Failed to collect fingerprint image.\n");
        } else {
            printf("Error collecting fingerprint image. Error code: %02X.\n", response[9]);
        }
    } else {
        printf("No response received.\n");
    }

}


bool generate_character_file(uint8_t bufferID) {
    if (bufferID != 0x01 && bufferID != 0x02) {
        bufferID = 0x02; // Default to CharBuffer2 if invalid BufferID is provided
    }

    uint8_t gen_char_file_cmd[] = {
        0xEF, 0x01,                      // Header
        0xFF, 0xFF, 0xFF, 0xFF,          // Module Address (default: 0xFFFFFFFF)
        0x05,                            // Package Length
        0x02,                            // Instruction Code
        bufferID,                         // Buffer ID
        0x00, 0x00                       // Placeholder for checksum
    };

    uint16_t checksum = 0;
    for (int i = 2; i < sizeof(gen_char_file_cmd) - 2; i++) {
        checksum += gen_char_file_cmd[i];
    }
    gen_char_file_cmd[sizeof(gen_char_file_cmd) - 2] = (checksum >> 8) & 0xFF; // High byte
    gen_char_file_cmd[sizeof(gen_char_file_cmd) - 1] = checksum & 0xFF;      // Low byte

    printf("Sending command to generate character file...\n");
    send_command(gen_char_file_cmd, sizeof(gen_char_file_cmd));

    uint8_t response[BUF_SIZE];
    int length = read_response(response, BUF_SIZE);

    if (length > 0) {
        printf("Response received: ");
        for (int i = 0; i < length; i++) {
            printf("%02X ", response[i]);
        }
        printf("\n");

        if (response[9] == 0x00) {
            printf("Character file generated successfully for buffer ID %d.\n", bufferID);
            return true;
        } else {
            printf("Failed to generate character file. Error code: %02X\n", response[9]);
        }
    } else {
        printf("No response received for generating character file.\n");
    }

    return false;
}

void store_character_file(uint8_t id) {
    uint8_t store_template_cmd[] = {
        0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x06, 0x06, 0x01, 0x00, id, 0x00, 0x00, (0x06 + 0x06 + id)
    };

    uint16_t checksum = 0;
    for (int i = 6; i < sizeof(store_template_cmd) - 2; i++) {
        checksum += store_template_cmd[i];
    }
    store_template_cmd[14] = (checksum >> 8) & 0xFF; // High byte
    store_template_cmd[15] = checksum & 0xFF; // Low byte

    send_command(store_template_cmd, sizeof(store_template_cmd));
    printf("Storing template with ID %d.\n", id);

    uint8_t response[BUF_SIZE];
    int length = read_response(response, BUF_SIZE);

    if (length > 0 && response[9] == 0x00) {
        printf("Template stored successfully with ID %d.\n", id);
    } else {
        printf("Failed to store template. Error code: %02X\n", response[9]);
    }
}

void generate_template(uint8_t id) {
    uint8_t generate_template_cmd[] = {
        0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x05, 0x05, 0x01, 0x00, 0x00, (0x05 + 0x05 + 0x01)
    };

    send_command(generate_template_cmd, sizeof(generate_template_cmd));
    printf("Generating template from character files.\n");

    uint8_t response[BUF_SIZE];
    int length = read_response(response, BUF_SIZE);

    if (length > 0 && response[9] == 0x00) {
        printf("Template generated successfully.\n");
        store_character_file(id); // Store the template in flash
    } else {
        printf("Failed to generate template. Error code: %02X.\n", response[9]);
    }
}

void enroll_fingerprint(uint8_t id) {
    // Collect first image
    collect_fingerprint_image();
    vTaskDelay(pdMS_TO_TICKS(IMAGE_DELAY_MS)); // Wait before collecting the second image

    // Generate character file for first image
    if (generate_character_file(1)) {
        // Collect second image
        collect_fingerprint_image();

        // Generate character file for second image
        if (generate_character_file(2)) {
            // Generate template from the two character files
            generate_template(id);
        } else {
            printf("Failed to generate character file for second image.\n");
        }
    } else {
        printf("Failed to generate character file for first image.\n");
    }
}

void init_gpio() {
    esp_rom_gpio_pad_select_gpio(INTERRUPT_PIN);
    gpio_set_direction(INTERRUPT_PIN, GPIO_MODE_INPUT);
}

void app_main() {
    printf("Starting up...\n");
    init_gpio();
    init_uart();

    uint8_t id = 1;

    while (1) {
        bool interrupt_level = gpio_get_level(INTERRUPT_PIN);
        bool finger_present = !interrupt_level;

        if (finger_present) {
            printf("Finger detected.\n");
            enroll_fingerprint(id); // Enroll a fingerprint
            printf("Fingerprint saved with ID %d.\n", id);
        } else {
            printf("No finger detected.\n");
        }

        vTaskDelay(pdMS_TO_TICKS(500));  // Check every 500ms
    }
}
