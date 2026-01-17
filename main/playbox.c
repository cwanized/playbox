#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/ledc.h"
#include "esp_log.h" // Header für ESP-IDF-Logging

#define BUZZER_PIN 12
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_TIMER LEDC_TIMER_0

#define BUZZER_LEDC_CHANNEL LEDC_CHANNEL_0
#define BUZZER_LEDC_TIMER   LEDC_TIMER_0

#define BUTTON_PIN 0 // Beispiel-Pin für den Taster
#define ESP_INTR_FLAG_DEFAULT 0
#define SWITCH_PIN GPIO_NUM_14

#define INTERRUPT_PIN GPIO_NUM_0 // GPIO0 als Interrupt-Pin

enum Mode
{
    IDLE_MODE,
    BEEP_MODE,
    QUIZMASTER_MODE,
    MIDI_MODE,
    TONLEITER_MODE
};

typedef struct {
    int beep_count;
    int phase;
    TickType_t last_tick;
} buzzer_state_t;

static buzzer_state_t buzzer = {
    .beep_count = 0,
    .phase = 0,
    .last_tick = 0
};

volatile enum Mode currentMode = TONLEITER_MODE;

// Interrupt-Handler für den Modus-Button
// Interrupt-Handler-Funktion
static void IRAM_ATTR button_isr_handler(void *arg)
{
    // Hier wird der Code ausgeführt, der auf den Interrupt reagieren soll
    currentMode = (currentMode + 1) % 3;                  // Wechsle zum nächsten Modus
    ESP_LOGI("MODE", "Mode changed to: %d", currentMode); // Debug-Info ausgeben mit esp_logx
}

// Funktionen für verschiedene Modi

void IdleMode()
{

}

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

void buzzer_beep(int count)
{
    if (buzzer.phase == 0) {
        buzzer.beep_count = count * 2; // an + aus
        buzzer.phase = 1;
    }

    if (buzzer.beep_count > 0) {
        int on = (buzzer.beep_count % 2);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL, on ? 128 : 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL);
        buzzer.beep_count--;
    } else {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL);
        buzzer.phase = 0;
    }
}

void buzzer_update()
{
    TickType_t now = xTaskGetTickCount();

    if (now - buzzer.last_tick < pdMS_TO_TICKS(120))
        return;

    buzzer.last_tick = now;

    switch (currentMode)
    {
        case IDLE_MODE:
            break;
        case BEEP_MODE:
            buzzer_beep(1);
            break;
        case QUIZMASTER_MODE:
            buzzer_beep(3);
            break;
        case MIDI_MODE:
            buzzer_beep(2);
            break;
        case TONLEITER_MODE:
            buzzer_beep(4);
            break;
    }
}


void app_main(void)
{
    // init_uart();
    ESP_LOGI("MODE", "BOOT"); // Debug-Info ausgeben mit esp_logx

    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_NUM_2, GPIO_PULLUP_ONLY);

    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    gpio_set_direction(SWITCH_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SWITCH_PIN, GPIO_PULLUP_ONLY);

    gpio_set_direction(GPIO_NUM_27, GPIO_MODE_OUTPUT);

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

    buzzer_update();
vTaskDelay(10 / portTICK_PERIOD_MS);

    // Konfiguriere GPIO4 als Eingang
    gpio_pad_select_gpio(INTERRUPT_PIN);
    gpio_set_direction(INTERRUPT_PIN, GPIO_MODE_INPUT);

    // Aktiviere Interrupt auf steigende Flanke für GPIO4
    gpio_set_intr_type(INTERRUPT_PIN, GPIO_INTR_POSEDGE);

    // Installiere GPIO-ISR-Service
    gpio_install_isr_service(0);

// Füge Interrupt-Handler hinzu
#include "driver/gpio.h" // Add the missing header file

    //////    gpio_isr_handler_add(INTERRUPT_PIN, gpio_isr_handler, (void*) INTERRUPT_PIN);

    while (true)
    {
        ESP_LOGI("MODE", "START");
        ESP_LOGI("MODE", "MAIN");                         // Debug-Info ausgeben mit esp_logx
        ESP_LOGI("MODE", "Current IS : %d", currentMode); // Debug-Info ausgeben mit esp_logx

        // LED an GPIO27 blinken lassen (Test)
        gpio_set_level(GPIO_NUM_27, 1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(GPIO_NUM_27, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);


        static int last_state = 1;
        int state = gpio_get_level(SWITCH_PIN);

        if (state == 0 && last_state == 1) {
            ESP_LOGI("SWITCH", "Button gedrückt!");
        }

        last_state = state;


        // Je nach aktuellem Modus wird die entsprechende Funktion aufgerufen
        switch (currentMode)
        {
        case IDLE_MODE:
            // quizmasterMode();
            break;     
        case BEEP_MODE:
            // quizmasterMode();
            break;                         
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
