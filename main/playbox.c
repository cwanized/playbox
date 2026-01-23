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
#define IN_LED_PIN       GPIO_NUM_27   // Steuerung / Taster
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
#define LED_PWM_FREQ_HZ       80
#define LED_PWM_RES           LEDC_TIMER_13_BIT

#define BUZZER_PWM_FREQ_HZ    2000
#define BUZZER_PWM_RES        LEDC_TIMER_10_BIT
#define BUZZER_LEDC_CHANNEL   LEDC_CHANNEL_0
#define BUZZER_LEDC_TIMER     LEDC_TIMER_0

#define MAX_TONE_SEQUENCE 16


#define ORB_PWM_MAX 1023
#define ORB_PWM_MIN 300
#define ORB_FADE_STEP 3
#define ORB_FADE_MS 20
#define ORB_BLINK_MS 250

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

/* ===================== BUZZER / TONE ===================== */
typedef struct {
    int remaining_ticks;
    TickType_t last_tick;
    uint32_t frequency;
} buzzer_tone_t;

static buzzer_tone_t tone = {0};

typedef struct {
    uint32_t freq_hz;
    uint32_t duration_ms;
} tone_step_t;

typedef struct {
    tone_step_t steps[MAX_TONE_SEQUENCE];
    int count;          // Anzahl der Töne in der Sequenz
    int current_index;  // aktuell gespielter Ton
    bool active;        // ob Sequenz läuft
} tone_sequence_t;

static tone_sequence_t sequence = {0};

tone_step_t ode_an_die_freude[] = {
    { 330, 400 }, // E4
    { 330, 400 }, // E4
    { 349, 400 }, // F4
    { 392, 400 }, // G4
    { 392, 400 }, // G4
    { 349, 400 }, // F4
    { 330, 400 }, // E4
    { 294, 400 }, // D4
    { 262, 400 }, // C4
    { 262, 400 }, // C4
    { 294, 400 }, // D4
    { 330, 400 }, // E4
    { 330, 600 }, // E4 (längere Note)
    { 294, 200 }, // D4
    { 294, 600 }  // D4 (längere Note)
};

tone_step_t melody_happy_birthday[] = {
    { 264, 300 }, // C4
    { 264, 200 }, // C4
    { 297, 500 }, // D4
    { 264, 500 }, // C4
    { 352, 500 }, // F4
    { 330, 800 }, // E4

    { 264, 300 }, // C4
    { 264, 200 }, // C4
    { 297, 500 }, // D4
    { 264, 500 }, // C4
    { 396, 500 }, // G4
    { 352, 800 }, // F4

    { 264, 300 }, // C4
    { 264, 200 }, // C4
    { 528, 500 }, // C5
    { 440, 500 }, // A4
    { 352, 500 }, // F4
    { 330, 500 }, // E4
    { 297, 800 }, // D4

    { 466, 300 }, // A#4
    { 466, 200 }, // A#4
    { 440, 500 }, // A4
    { 352, 500 }, // F4
    { 396, 500 }, // G4
    { 352, 1000 } // F4
};

tone_step_t melody_hallelujah_motif[] = {
    { 262, 400 }, // C4
    { 262, 400 }, // C4
    { 330, 600 }, // E4
    { 330, 300 }, // E4
    { 349, 400 }, // F4
    { 330, 800 }, // E4

    { 262, 400 }, // C4
    { 294, 400 }, // D4
    { 330, 1000 } // E4
};



/**
 * PlayTone - spielt einen Ton auf dem passiven Buzzer
 * @freq_hz: Frequenz in Hz
 * @duration_ms: Dauer in Millisekunden
 * 
 * Non-blocking: Ton wird in buzzer_update() gestoppt
 */
void PlayTone(uint32_t freq_hz, uint32_t duration_ms)
{
    // LEDC Timer auf gewünschte Frequenz setzen
    ledc_set_freq(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_TIMER, freq_hz);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL, 128); // 50% Duty
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL);

    tone.remaining_ticks = pdMS_TO_TICKS(duration_ms);
    tone.last_tick = xTaskGetTickCount();
    tone.frequency = freq_hz;
}

/**
 * buzzer_update - muss regelmäßig aufgerufen werden, stoppt Ton wenn fertig
 */
void buzzer_update(void)
{
    TickType_t now = xTaskGetTickCount();

    if (tone.remaining_ticks > 0) {
        if (now - tone.last_tick >= 1) {
            tone.remaining_ticks--;
            tone.last_tick = now;

            if (tone.remaining_ticks <= 0) {
                // Ton beenden
                ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL, 0);
                ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL);
            }
        }
    }
}

void PlayToneSequence(tone_step_t *steps, int count)
{
    if (count > MAX_TONE_SEQUENCE) count = MAX_TONE_SEQUENCE;

    for (int i = 0; i < count; i++) {
        sequence.steps[i] = steps[i];
    }

    sequence.count = count;
    sequence.current_index = 0;
    sequence.active = true;

    // ersten Ton sofort starten
    if (count > 0) {
        PlayTone(sequence.steps[0].freq_hz, sequence.steps[0].duration_ms);
    }
}

/* Muss im Loop oder Task regelmäßig aufgerufen werden */
void ToneSequence_Update(void)
{
    if (!sequence.active) return;

    // Wenn der aktuelle Ton vorbei ist, nächsten Ton starten
    if (tone.remaining_ticks <= 0) {
        sequence.current_index++;
        if (sequence.current_index < sequence.count) {
            PlayTone(sequence.steps[sequence.current_index].freq_hz,
                     sequence.steps[sequence.current_index].duration_ms);
        } else {
            // Sequenz fertig
            sequence.active = false;
        }
    }
}

/* ===================== ORB LED STATE ===================== */
typedef struct {
    int target_blinks;      // Anzahl der Blinks bei Mode-Effekt
    int blinks_done;        // bisher erledigt
    bool led_on;            // aktueller Zustand
    TickType_t last_tick;   // letztes Update
    uint8_t duty;           // aktueller PWM Duty
    int fade_dir;           // +1 = heller, -1 = dunkler
} orb_state_t;

static orb_state_t orb = {0};

/* ===================== ORB LED ===================== */
void orb_update(void)
{
    TickType_t now = xTaskGetTickCount();

    // === Glow im Idle ===
    if (currentMode == IDLE_MODE) {
        if (now - orb.last_tick >= pdMS_TO_TICKS(ORB_FADE_MS)) {
            orb.last_tick = now;

            orb.duty += orb.fade_dir * ORB_FADE_STEP;
            if (orb.duty >= ORB_PWM_MAX) {
                orb.duty = ORB_PWM_MAX;
                orb.fade_dir = -1;
            } else if (orb.duty <= ORB_PWM_MIN) {
                orb.duty = ORB_PWM_MIN;
                orb.fade_dir = 1;
            }

            ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_7, orb.duty);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_7);
        }
        return;
    }

    // === Blink bei Mode-Effekt ===
    if (orb.blinks_done >= orb.target_blinks)
        return;

    if (now - orb.last_tick >= pdMS_TO_TICKS(ORB_BLINK_MS)) {
        orb.last_tick = now;
        orb.led_on = !orb.led_on;
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_7, orb.led_on ? ORB_PWM_MAX : 0);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_7);

        if (!orb.led_on) orb.blinks_done++;
    }
}

// === Mode-Effekt Trigger anpassen ===
void mode_effects_trigger(void)
{
    switch (currentMode)
    {
        case IDLE_MODE:
            orb.target_blinks = 0;
            orb.blinks_done = 0;
            orb.fade_dir = 1;
            orb.duty = ORB_PWM_MIN;
            break;

        case BEEP_MODE:
            PlayTone(440, 150);   // kurzer Piepton
            orb.target_blinks = 1;
            orb.blinks_done = 0;
            break;

        case QUIZMASTER_MODE:
            PlayTone(880, 200);   // höherer Ton
            orb.target_blinks = 3;
            orb.blinks_done = 0;
            break;

        case MIDI_MODE:
            PlayTone(660, 150);
            orb.target_blinks = 2;
            orb.blinks_done = 0;
            break;

        case TONLEITER_MODE:
            PlayTone(523, 300);
            orb.target_blinks = 4;
            orb.blinks_done = 0;
            break;

        default:
            break;
    }
}

/* ===================== PIN INIT ===================== */
void init_pins(void) {
    gpio_config_t io_conf = {0};

    /* ---------------- Inputs ---------------- */
    // P1 + P2 Joysticks / Buttons + IN_LED_PIN
    io_conf.pin_bit_mask = (1ULL << IN_P1_TOP_PIN) |
                           (1ULL << IN_P1_DOWN_PIN) |
                           (1ULL << IN_P1_LEFT_PIN) |
                           (1ULL << IN_P1_RIGHT_PIN) |
                           (1ULL << IN_P1_FIRE_PIN) |
                           (1ULL << IN_P2_TOP_PIN) |
                           (1ULL << IN_P2_DOWN_PIN) |
                           (1ULL << IN_P2_LEFT_PIN) |
                           (1ULL << IN_P2_RIGHT_PIN) |
                           (1ULL << IN_P2_FIRE_PIN) |
                           (1ULL << IN_LED_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    /* ---------------- Outputs ---------------- */
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

    /* ---------------- LEDC PWM ---------------- */
    // Timer für RGB LEDs
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = LED_PWM_RES,
        .timer_num = LEDC_TIMER_1,
        .freq_hz = LED_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {0};

    // P1 RGB
    ledc_channel.gpio_num = OUT_P1_R_PIN;
    ledc_channel.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_channel.channel = LEDC_CHANNEL_1;
    ledc_channel.timer_sel = LEDC_TIMER_1;
    ledc_channel.intr_type = LEDC_INTR_DISABLE;
    ledc_channel.duty = 0;
    ledc_channel_config(&ledc_channel);

    ledc_channel.gpio_num = OUT_P1_G_PIN;
    ledc_channel.channel = LEDC_CHANNEL_2;
    ledc_channel_config(&ledc_channel);

    ledc_channel.gpio_num = OUT_P1_B_PIN;
    ledc_channel.channel = LEDC_CHANNEL_3;
    ledc_channel_config(&ledc_channel);

    // P2 RGB
    ledc_channel.gpio_num = OUT_P2_R_PIN;
    ledc_channel.channel = LEDC_CHANNEL_4;
    ledc_channel_config(&ledc_channel);

    ledc_channel.gpio_num = OUT_P2_G_PIN;
    ledc_channel.channel = LEDC_CHANNEL_5;
    ledc_channel_config(&ledc_channel);

    ledc_channel.gpio_num = OUT_P2_B_PIN;
    ledc_channel.channel = LEDC_CHANNEL_6;
    ledc_channel_config(&ledc_channel);

    // OUT_LED / ORB LED
    ledc_channel.gpio_num = OUT_LED_PIN;
    ledc_channel.channel = LEDC_CHANNEL_7;
    ledc_channel_config(&ledc_channel);

    // Buzzer PWM (LOW SPEED)
    ledc_timer_config_t buzzer_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = BUZZER_PWM_RES,
        .timer_num = BUZZER_LEDC_TIMER,
        .freq_hz = BUZZER_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&buzzer_timer);

    ledc_channel.gpio_num = BUZZER_PIN;
    ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
    ledc_channel.channel = BUZZER_LEDC_CHANNEL;
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


void app_main(void)
{
    ESP_LOGI("APP", "BOOT");

    // Pins initialisieren
    init_pins();

        int last_switch = 1;

    // ORB-LED Initialisierung
    orb.fade_dir = 1;
    orb.duty = ORB_PWM_MIN;
    orb.last_tick = xTaskGetTickCount();

    // Beispiel-Sequenz: 4 Töne
    tone_step_t melody[] = {
        {440, 200},  // A4
        {494, 200},  // B4
        {523, 200},  // C5
        {587, 200}   // D5
    };

    // Starte Sequenz
    PlayToneSequence(ode_an_die_freude, sizeof(ode_an_die_freude)/sizeof(tone_step_t));

    while (true)
    {
        /* ===== Switch polling (edge detect) ===== */
        int state = gpio_get_level(IN_LED_PIN);
        if (state == 0 && last_switch == 1) {
            // Mode wechseln
            currentMode = (currentMode + 1) % MODE_COUNT;
            ESP_LOGI("MODE", "Changed to %d", currentMode);

            // Mode-Effekt (Ton + LED)
            mode_effects_trigger();
        }
        last_switch = state;

        /* ===== Switch polling (edge detect) ===== */
        buzzer_update();
        ToneSequence_Update();
        orb_update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}