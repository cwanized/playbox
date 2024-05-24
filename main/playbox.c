#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/ledc.h"
#include "esp_log.h" // Header für ESP-IDF-Logging


#define BUZZER_PIN 22
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_TIMER LEDC_TIMER_0

#define BUTTON_PIN 0 // Beispiel-Pin für den Taster
#define ESP_INTR_FLAG_DEFAULT 0



enum Mode
{
    QUIZMASTER_MODE,
    MIDI_MODE,
    TONLEITER_MODE
};

volatile enum Mode currentMode = QUIZMASTER_MODE;

// Interrupt-Handler für den Modus-Button
// Interrupt-Handler-Funktion
static void IRAM_ATTR button_isr_handler(void *arg)
{
    // Hier wird der Code ausgeführt, der auf den Interrupt reagieren soll
    currentMode = (currentMode + 1) % 3; // Wechsle zum nächsten Modus
    ESP_LOGI("MODE", "Mode changed to: %d", currentMode); // Debug-Info ausgeben mit esp_logx

}

// Funktionen für verschiedene Modi
void quizmasterMode()
{

    // if(gpio_get_level(GPIO_NUM_2) == 0){
    gpio_set_level(GPIO_NUM_4, 1);
    //}else{
    //    gpio_set_level(GPIO_NUM_4, 0);
    //}

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_NUM_4, 0);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void midiMode()
{
    ESP_LOGI("MODE", "MIDI mode"); // Debug-Info ausgeben mit esp_logx
    // Code für den MIDI-Modus
    // Set duty cycle to 50%
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL, 255 / 2);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL);
    vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 second delay

    // Turn off the buzzer
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL);
    vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 second delay
}

void tonleiterMode()
{
    // Code für den Tonleiter-Modus
    ESP_LOGI("MODE", "Tonleiter mode"); // Debug-Info ausgeben mit esp_logx

}




void app_main(void)
{
    //init_uart();
    ESP_LOGI("MODE", "BOOT"); // Debug-Info ausgeben mit esp_logx
     
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_NUM_2, GPIO_PULLUP_ONLY);

    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);

    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_8_BIT, // 8-bit duty resolution
        .freq_hz = 1000,                     // 1 kHz frequency
        .speed_mode = LEDC_LOW_SPEED_MODE,   // High speed mode
        .timer_num = LEDC_TIMER              // Timer 0
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .channel = LEDC_CHANNEL,
        .duty = 0,
        .gpio_num = BUZZER_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = LEDC_TIMER};
    ledc_channel_config(&ledc_channel);

    // GPIO-Konfiguration für den Button-Pin
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_NEGEDGE; // Interrupt bei steigenden und fallenden Flanken
    io_conf.pin_bit_mask = (1ULL << BUTTON_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);

    // Interrupt-Service installieren und Interrupt-Handler hinzufügen
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);

    while (true)
    {
        ESP_LOGI("MODE", "MAIN"); // Debug-Info ausgeben mit esp_logx
        // Je nach aktuellem Modus wird die entsprechende Funktion aufgerufen
        switch (currentMode)
        {
        case QUIZMASTER_MODE:
            quizmasterMode();
            break;
        case MIDI_MODE:
            midiMode();
            break;
        case TONLEITER_MODE:
            tonleiterMode();
            break;
        }
    }
}
