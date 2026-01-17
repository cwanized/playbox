#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/adc.h"
#include "driver/dac.h"
#include "esp_log.h"

/* ===================== GPIO DEFINES ===================== */

/* PWM / LED / Buzzer */
#define OUT_LED_PIN      GPIO_NUM_14
#define IN_LED_PIN      GPIO_NUM_27
#define BUZZER_PIN       GPIO_NUM_12

#define OUT_P1_R_PIN     GPIO_NUM_21
#define OUT_P1_G_PIN     GPIO_NUM_1    // UART ungenutzt
#define OUT_P1_B_PIN     GPIO_NUM_3    // UART ungenutzt
#define OUT_P2_R_PIN     GPIO_NUM_4
#define OUT_P2_G_PIN     GPIO_NUM_17
#define OUT_P2_B_PIN     GPIO_NUM_16

/* Joystick / Buttons P1 */
#define IN_P1_TOP_PIN    GPIO_NUM_22
#define IN_P1_DOWN_PIN   GPIO_NUM_19
#define IN_P1_LEFT_PIN   GPIO_NUM_23
#define IN_P1_RIGHT_PIN  GPIO_NUM_18
#define IN_P1_FIRE_PIN   GPIO_NUM_5

/* Joystick / Buttons P2 */
#define IN_P2_TOP_PIN    GPIO_NUM_32
#define IN_P2_DOWN_PIN   GPIO_NUM_33
#define IN_P2_LEFT_PIN   GPIO_NUM_34
#define IN_P2_RIGHT_PIN  GPIO_NUM_35
#define IN_P2_FIRE_PIN   GPIO_NUM_13

/* ADC Pins */
#define ADC0_PIN         GPIO_NUM_36
#define ADC1_PIN         GPIO_NUM_37
#define ADC2_PIN         GPIO_NUM_38
#define ADC3_PIN         GPIO_NUM_39

/* DAC Pins */
#define DAC1_PIN         GPIO_NUM_25
#define DAC2_PIN         GPIO_NUM_26

/* ===================== LEDC CONFIG ===================== */
#define BUZZER_LEDC_CHANNEL   LEDC_CHANNEL_0
#define BUZZER_LEDC_TIMER     LEDC_TIMER_0

#define LED_PWM_FREQ_HZ       5000
#define LED_PWM_RES           LEDC_TIMER_13_BIT

#define BUZZER_PWM_FREQ_HZ    2000
#define BUZZER_PWM_RES        LEDC_TIMER_10_BIT

/* ===================== MODES ===================== */

typedef enum {
    IDLE_MODE = 0,
    BEEP_MODE,
    QUIZMASTER_MODE,
    MIDI_MODE,
    TONLEITER_MODE,
    MODE_COUNT
} Mode;

volatile Mode currentMode = IDLE_MODE;

/* ===================== BUZZER STATE ===================== */

typedef struct {
    int remaining_steps;
    TickType_t last_tick;
} buzzer_state_t;

static buzzer_state_t buzzer = {0};

/* ===================== ORB LED STATE ===================== */

typedef struct {
    int blink_target;
    int blink_done;
    bool led_on;
    TickType_t last_tick;
} orb_state_t;

static orb_state_t orb = {0};

/* ===================== BUZZER ===================== */

void init_pins(void) {
    /* ---------------- Inputs ---------------- */
    gpio_config_t io_conf = {0};

    // P1 Joystick / Buttons
    io_conf.pin_bit_mask = (1ULL << IN_P1_TOP_PIN) |
                           (1ULL << IN_P1_DOWN_PIN) |
                           (1ULL << IN_P1_LEFT_PIN) |
                           (1ULL << IN_P1_RIGHT_PIN) |
                           (1ULL << IN_P1_FIRE_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;   // Taster gegen GND
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    // P2 Joystick / Buttons
    io_conf.pin_bit_mask = (1ULL << IN_P2_TOP_PIN) |
                           (1ULL << IN_P2_DOWN_PIN) |
                           (1ULL << IN_P2_LEFT_PIN) |
                           (1ULL << IN_P2_RIGHT_PIN) |
                           (1ULL << IN_P2_FIRE_PIN);
    gpio_config(&io_conf);

    /* ---------------- Outputs ---------------- */
    // LEDs / Buzzer
    io_conf.pin_bit_mask = (1ULL << OUT_LED_PIN) |
                           (1ULL << BUZZER_PIN) |
                           (1ULL << OUT_P1_R_PIN) |
                           (1ULL << OUT_P1_G_PIN) |
                           (1ULL << OUT_P1_B_PIN) |
                           (1ULL << OUT_P2_R_PIN) |
                           (1ULL << OUT_P2_G_PIN) |
                           (1ULL << OUT_P2_B_PIN);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    /* ---------------- LEDC / PWM ---------------- */
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = LED_PWM_RES,
        .timer_num = LEDC_TIMER_1,
        .freq_hz = LED_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    // ORB_LED / OUT_LED
    ledc_channel_config_t ledc_channel = {
        .gpio_num = OUT_LED_PIN,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_1,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&ledc_channel);

    ledc_channel.gpio_num = OUT_LED_PIN;
    ledc_channel.channel = LEDC_CHANNEL_2;
    ledc_channel_config(&ledc_channel);

    // Buzzer PWM
    ledc_timer_config_t buzzer_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = BUZZER_PWM_RES,
        .timer_num = BUZZER_LEDC_TIMER,
        .freq_hz = BUZZER_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&buzzer_timer);

    ledc_channel.channel = BUZZER_LEDC_CHANNEL;
    ledc_channel.gpio_num = BUZZER_PIN;
    ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
    ledc_channel.timer_sel = BUZZER_LEDC_TIMER;
    ledc_channel.duty = 0;
    ledc_channel_config(&ledc_channel);

    /* ---------------- ADC ---------------- */
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_2, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_11);

    /* ---------------- DAC ---------------- */
dac_output_enable(DAC_CHANNEL_1);  // GPIO25
dac_output_enable(DAC_CHANNEL_2);  // GPIO26
}

void buzzer_start(int beeps)
{
    buzzer.remaining_steps = beeps * 2; // on/off
}

void buzzer_update(void)
{
    if (buzzer.remaining_steps <= 0)
        return;

    TickType_t now = xTaskGetTickCount();
    if (now - buzzer.last_tick < pdMS_TO_TICKS(120))
        return;

    buzzer.last_tick = now;

    bool on = buzzer.remaining_steps % 2;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL, on ? 128 : 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL);

    buzzer.remaining_steps--;
}

/* ===================== ORB LED ===================== */

void orb_start(int blinks)
{
    orb.blink_target = blinks;
    orb.blink_done = 0;
    orb.led_on = false;
}

void orb_update(void)
{
    TickType_t now = xTaskGetTickCount();

    /* Idle glow (langsames Atmen simuliert) */
    if (currentMode == IDLE_MODE) {
        if (now - orb.last_tick > pdMS_TO_TICKS(500)) {
            orb.last_tick = now;
            orb.led_on = !orb.led_on;
            gpio_set_level(OUT_LED_PIN, orb.led_on);
        }
        return;
    }

    /* Blink-Modi */
    if (orb.blink_done >= orb.blink_target)
        return;

    if (now - orb.last_tick < pdMS_TO_TICKS(200))
        return;

    orb.last_tick = now;
    orb.led_on = !orb.led_on;
    gpio_set_level(OUT_LED_PIN, orb.led_on);

    if (!orb.led_on)
        orb.blink_done++;
}

/* ===================== MODE EFFECT TRIGGER ===================== */

void mode_effects_trigger(void)
{
    switch (currentMode)
    {
        case IDLE_MODE:
            break;

        case BEEP_MODE:
            buzzer_start(1);
            orb_start(1);
            break;

        case QUIZMASTER_MODE:
            buzzer_start(3);
            orb_start(3);
            break;

        case MIDI_MODE:
            buzzer_start(2);
            orb_start(2);
            break;

        case TONLEITER_MODE:
            buzzer_start(4);
            orb_start(4);
            break;

        default:
            break;
    }
}

/* ===================== APP MAIN ===================== */

void app_main(void)
{
    ESP_LOGI("APP", "BOOT");

    /* GPIO Setup */
    //gpio_set_direction(IN_LED_PIN, GPIO_MODE_INPUT);
    //gpio_set_pull_mode(IN_LED_PIN, GPIO_PULLUP_ONLY);

    //gpio_set_direction(OUT_LED_PIN, GPIO_MODE_OUTPUT);

    /* LEDC Setup (Buzzer) */
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = BUZZER_LEDC_TIMER,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 2000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t channel = {
        .gpio_num = BUZZER_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = BUZZER_LEDC_CHANNEL,
        .timer_sel = BUZZER_LEDC_TIMER,
        .duty = 0
    };
    ledc_channel_config(&channel);

    int last_switch = 1;


    while (true)
    {
        /* ===== Switch polling (edge detect) ===== */
        int state = gpio_get_level(IN_LED_PIN      );
        if (state == 0 && last_switch == 1) {
            currentMode = (currentMode + 1) % MODE_COUNT;
            ESP_LOGI("MODE", "Changed to %d", currentMode);
            mode_effects_trigger();
        }
        last_switch = state;

        /* ===== Update Effects ===== */
        buzzer_update();
        orb_update();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
