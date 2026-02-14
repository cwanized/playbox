#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h> // für sinf()

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/adc.h"
#include "driver/dac.h"
#include "esp_log.h"

#include "songs.h"

/* ===================== GPIO DEFINES ===================== */

/* PWM / LED / Buzzer */
#define OUT_LED_PIN GPIO_NUM_14
#define IN_LED_PIN GPIO_NUM_27 // Steuerung / Taster
#define BUZZER_PIN GPIO_NUM_12

#define OUT_P1_R_PIN GPIO_NUM_21
#define OUT_P1_G_PIN GPIO_NUM_1 // UART ungenutzt
#define OUT_P1_B_PIN GPIO_NUM_3 // UART ungenutzt
#define OUT_P2_R_PIN GPIO_NUM_4
#define OUT_P2_G_PIN GPIO_NUM_17
#define OUT_P2_B_PIN GPIO_NUM_16

/* Joystick / Buttons P1 */
#define IN_P1_TOP_PIN GPIO_NUM_22
#define IN_P1_DOWN_PIN GPIO_NUM_19
#define IN_P1_LEFT_PIN GPIO_NUM_23
#define IN_P1_RIGHT_PIN GPIO_NUM_18
#define IN_P1_FIRE_PIN GPIO_NUM_5

/* Joystick / Buttons P2 */
#define IN_P2_TOP_PIN GPIO_NUM_32
#define IN_P2_DOWN_PIN GPIO_NUM_33
#define IN_P2_LEFT_PIN GPIO_NUM_34
#define IN_P2_RIGHT_PIN GPIO_NUM_35
#define IN_P2_FIRE_PIN GPIO_NUM_13

/* ADC Pins */
#define ADC0_PIN GPIO_NUM_36
#define ADC1_PIN GPIO_NUM_37
#define ADC2_PIN GPIO_NUM_38
#define ADC3_PIN GPIO_NUM_39

/* DAC Pins */
#define DAC1_PIN GPIO_NUM_25
#define DAC2_PIN GPIO_NUM_26

/* ===================== LEDC CONFIG ===================== */
#define LED_PWM_FREQ_HZ 80
#define LED_PWM_RES LEDC_TIMER_13_BIT

#define BUZZER_PWM_FREQ_HZ 2000
#define BUZZER_PWM_RES LEDC_TIMER_10_BIT
#define BUZZER_LEDC_CHANNEL LEDC_CHANNEL_0
#define BUZZER_LEDC_TIMER LEDC_TIMER_0

#define MAX_TONE_SEQUENCE 16

#define ORB_PWM_MAX 7000
#define ORB_PWM_MIN 800
#define ORB_FADE_STEP 20
#define ORB_FADE_MS 20
#define ORB_BLINK_MS 250
#define ORB_BEEP_BLINK_MS 100

// Globale Variable für den Sinuswinkel
static float orb_breath_angle = 0.0f;

static bool quizmaster_triggered = false;
static int last_p1_state = 1;
static int last_p2_state = 1;

/* ===================== MODES ===================== */
typedef enum
{
    IDLE_MODE = 0,
    BEEP_MODE,
    MIDI_MODE,
    QUIZMASTER_MODE,
    TONLEITER_MODE,
    MODE_COUNT
} Mode;

volatile Mode currentMode = IDLE_MODE;

// Array mit Flags pro Mode, um Pieps/Sequenz nur einmal auszulösen
static bool mode_done_flags[MODE_COUNT] = {0};


/* ===================== BUZZER STATE ===================== */
typedef struct
{
    int remaining_steps;
    TickType_t last_tick;
} buzzer_state_t;

static buzzer_state_t buzzer = {0};

/* ===================== BUZZER / TONE ===================== */
typedef struct
{
    int remaining_ticks;
    TickType_t last_tick;
    uint32_t frequency;
} buzzer_tone_t;

static buzzer_tone_t tone = {0};

typedef struct
{
    tone_step_t steps[MAX_TONE_SEQUENCE];
    int count;         // Anzahl der Töne in der Sequenz
    int current_index; // aktuell gespielter Ton
    bool active;       // ob Sequenz läuft
} tone_sequence_t;

static tone_sequence_t sequence = {0};

typedef enum
{
    SONG_ODE = 0,
    SONG_HAPPY,
    SONG_HALLELUJAH,
    SONG_ENTCHEN,
    SONG_COUNT
} midi_song_t;

static midi_song_t currentSong = SONG_ODE;

typedef enum
{
    MIDI_WAIT, // Warten auf Button-Eingabe oder Liedstart
    MIDI_PLAY, // Lied gerade abspielen
    MIDI_NEXT, // Nächstes Lied
    MIDI_PREV, // Vorheriges Lied
} midi_state_t;

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

    if (tone.remaining_ticks > 0)
    {
        if (now - tone.last_tick >= 1)
        {
            tone.remaining_ticks--;
            tone.last_tick = now;

            if (tone.remaining_ticks <= 0)
            {
                // Ton beenden
                ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL, 0);
                ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL);
            }
        }
    }
}

void PlayToneSequence(const tone_step_t *steps, int count)
{
    if (count > MAX_TONE_SEQUENCE)
        count = MAX_TONE_SEQUENCE;

    for (int i = 0; i < count; i++)
    {
        sequence.steps[i] = steps[i];
    }

    sequence.count = count;
    sequence.current_index = 0;
    sequence.active = true;

    // ersten Ton sofort starten
    if (count > 0)
    {
        PlayTone(sequence.steps[0].freq_hz, sequence.steps[0].duration_ms);
    }
}

/* Muss im Loop oder Task regelmäßig aufgerufen werden */
void ToneSequence_Update(void)
{
    if (!sequence.active)
        return;

    // Wenn der aktuelle Ton vorbei ist, nächsten Ton starten
    if (tone.remaining_ticks <= 0)
    {
        sequence.current_index++;
        if (sequence.current_index < sequence.count)
        {
            PlayTone(sequence.steps[sequence.current_index].freq_hz,
                     sequence.steps[sequence.current_index].duration_ms);
        }
        else
        {
            // Sequenz fertig
            sequence.active = false;
        }
    }
}

void Play_current_song(void)
{
    switch (currentSong)
    {
    case SONG_ODE:
        PlayToneSequence(
            ode_an_die_freude,
            ode_an_die_freude_len);
        break;

    case SONG_HAPPY:
        PlayToneSequence(
            melody_happy_birthday,
            melody_happy_birthday_len);
        break;

    case SONG_HALLELUJAH:
        PlayToneSequence(
            melody_hallelujah_motif,
            melody_hallelujah_motif_len);
        break;
    case SONG_ENTCHEN:
        PlayToneSequence(
            alle_meine_entchen,
            alle_meine_entchen_len);
        break;

    default:
        break;
    }
}

void StopToneSequence(void)
{
    sequence.active = false;
    tone.remaining_ticks = 0;

    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL);
}

/* ===================== MIDI SONG MODE ===================== */
void MidiSongMode(void)
{
    // Song wechseln
    currentSong = (currentSong + 1) % SONG_COUNT;

    // Log
    ESP_LOGI("SONG", "Changed to %d", currentSong);

    // Song abspielen
    Play_current_song();
}

/* ===================== ORB LED STATE ===================== */
typedef struct
{
    int target_blinks;    // Anzahl der Blinks bei Mode-Effekt
    int blinks_done;      // bisher erledigt
    bool led_on;          // aktueller Zustand
    TickType_t last_tick; // letztes Update
    uint16_t duty;        // aktueller PWM Duty
    int fade_dir;         // +1 = heller, -1 = dunkler
} orb_state_t;

static orb_state_t orb = {0};

/* ===================== ORB LED ===================== */

void orb_update(void)
{
    TickType_t now = xTaskGetTickCount();

    // === Smooth Breath Effekt im Idle ===
    if (currentMode == IDLE_MODE)
    {
        if (now - orb.last_tick >= pdMS_TO_TICKS(ORB_FADE_MS))
        {
            orb.last_tick = now;

            float sin_val = (sinf(orb_breath_angle) + 1.0f) / 2.0f;
            orb.duty = ORB_PWM_MIN + (uint16_t)(sin_val * (ORB_PWM_MAX - ORB_PWM_MIN));

            ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_7, orb.duty);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_7);

            orb_breath_angle += 0.05f;
            if (orb_breath_angle > 2 * M_PI)
                orb_breath_angle -= 2 * M_PI;
        }

        return; // kein weiterer Blinkcode
    }

    // Andere Modes außer BEEP_MODE: Blinkverhalten
    if (currentMode != BEEP_MODE)
    {
        if (orb.blinks_done >= orb.target_blinks)
            return;

        if (now - orb.last_tick >= pdMS_TO_TICKS(ORB_BLINK_MS))
        {
            orb.last_tick = now;
            orb.led_on = !orb.led_on;
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_7, orb.led_on ? ORB_PWM_MAX : 0);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_7);

            if (!orb.led_on)
                orb.blinks_done++;
        }
    }
}



// === Mode-Effekt Trigger anpassen ===
void mode_effects_trigger(void)
{
    orb.blinks_done = 0;
    orb.last_tick = xTaskGetTickCount();

    // Alle Mode-Flags zurücksetzen
    for (int i = 0; i < MODE_COUNT; i++)
        mode_done_flags[i] = false;

    switch (currentMode)
    {
        case IDLE_MODE:        orb.target_blinks = 0; break;
        case BEEP_MODE:        orb.target_blinks = 1; break;
        case MIDI_MODE:        orb.target_blinks = 2; break;
        case QUIZMASTER_MODE:  orb.target_blinks = 3; break;
        case TONLEITER_MODE:   orb.target_blinks = 4; break;
        default:               orb.target_blinks = 0; break;
    }
}

/* ===================== MODI Implemenation ===================== */

void BeepMode(void)
{
    // Array mit Pins für P1 und P2
    const gpio_num_t pins[10] = {
        IN_P1_TOP_PIN, IN_P1_DOWN_PIN, IN_P1_LEFT_PIN, IN_P1_RIGHT_PIN, IN_P1_FIRE_PIN,
        IN_P2_TOP_PIN, IN_P2_DOWN_PIN, IN_P2_LEFT_PIN, IN_P2_RIGHT_PIN, IN_P2_FIRE_PIN
    };

    // Frequenzen für jeden Button
    const uint32_t freqs[10] = {
        262, 294, 330, 349, 392,   // P1: C4, D4, E4, F4, G4
        440, 494, 523, 587, 659    // P2: A4, B4, C5, D5, E5
    };

    static int last_state[10] = {1,1,1,1,1,1,1,1,1,1};
    static TickType_t beep_orb_end_tick = 0;

    TickType_t now = xTaskGetTickCount();

    for (int i = 0; i < 10; i++)
    {
        int state = gpio_get_level(pins[i]);

        // Edge Detect: Button gedrückt
        if (state == 0 && last_state[i] == 1)
        {
            PlayTone(freqs[i], 200); // Ton abspielen

            // ORB LED direkt einschalten
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_7, ORB_PWM_MAX);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_7);

            // Timer für LED ausschalten setzen
            beep_orb_end_tick = now + pdMS_TO_TICKS(ORB_BEEP_BLINK_MS);
        }

        last_state[i] = state;
    }

    // LED nach Ablauf ausschalten
    if (beep_orb_end_tick != 0 && now >= beep_orb_end_tick)
    {
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_7, 0);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_7);
        beep_orb_end_tick = 0;
    }
}

void QuizmasterMode(void)
{
    static TickType_t quiz_orb_end_tick = 0;
    static uint16_t current_duty = 0;  // aktueller PWM-Wert
    TickType_t now = xTaskGetTickCount();

    int p1_state = gpio_get_level(IN_P1_TOP_PIN);
    int p2_state = gpio_get_level(IN_P2_TOP_PIN);

    // --- Edge Detect Player 1 ---
    if (p1_state == 0 && last_p1_state == 1 && !quizmaster_triggered)
    {
        PlayTone(523, 300);
        quizmaster_triggered = true;
        current_duty = ORB_PWM_MAX;
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_7, current_duty);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_7);
        quiz_orb_end_tick = now + pdMS_TO_TICKS(ORB_BEEP_BLINK_MS);
    }

    // --- Edge Detect Player 2 ---
    if (p2_state == 0 && last_p2_state == 1 && !quizmaster_triggered)
    {
        PlayTone(659, 300);
        quizmaster_triggered = true;
        current_duty = ORB_PWM_MAX;
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_7, current_duty);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_7);
        quiz_orb_end_tick = now + pdMS_TO_TICKS(ORB_BEEP_BLINK_MS);
    }

    // --- LED sanft ausfaden ---
    if (quiz_orb_end_tick != 0 && now >= quiz_orb_end_tick)
    {
        if (current_duty > ORB_FADE_STEP)
            current_duty -= ORB_FADE_STEP;
        else
            current_duty = 0;

        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_7, current_duty);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_7);

        if (current_duty == 0)
        {
            quiz_orb_end_tick = 0;
            quizmaster_triggered = false; // Reset für nächste Frage
        }
    }

    last_p1_state = p1_state;
    last_p2_state = p2_state;
}






/* ===================== MODI HANDLER ===================== */

void HandleIdleMode(void)
{
    static bool initialized = false;

    if (!initialized)
    {
        orb.fade_dir = 1;
        orb.duty = ORB_PWM_MIN;
        orb.last_tick = xTaskGetTickCount();
        orb.target_blinks = 0;
        orb.blinks_done = 0;
        initialized = true;
    }

    if (currentMode != IDLE_MODE)
        initialized = false;
}

void HandleBeepMode(void)
{
    if (!mode_done_flags[BEEP_MODE])
    {
        PlayToneSequence(beep_mode_tones, beep_mode_len);
        orb.target_blinks = 1;
        orb.blinks_done = 0;
        mode_done_flags[BEEP_MODE] = true;
    }

    BeepMode();
}

void HandleMidiMode(void)
{
    static int last_left_state = 1;
    static int last_right_state = 1;

    if (currentMode != MIDI_MODE)
    {
        last_left_state = 1;
        last_right_state = 1;
        StopToneSequence();
        orb.blinks_done = 0;
        orb.target_blinks = 0;
        return;
    }

    if (!mode_done_flags[MIDI_MODE])
    {
        PlayToneSequence(midi_mode_tones, midi_mode_len);
        orb.target_blinks = 2;
        orb.blinks_done = 0;
        mode_done_flags[MIDI_MODE] = true;
    }

    int left_state = gpio_get_level(IN_P1_LEFT_PIN);
    int right_state = gpio_get_level(IN_P1_RIGHT_PIN);

    if (left_state == 0 && last_left_state == 1)
    {
        StopToneSequence();
        currentSong = (currentSong - 1 + SONG_COUNT) % SONG_COUNT;
        Play_current_song();
    }

    if (right_state == 0 && last_right_state == 1)
    {
        StopToneSequence();
        currentSong = (currentSong + 1) % SONG_COUNT;
        Play_current_song();
    }

    last_left_state = left_state;
    last_right_state = right_state;
}

void HandleQuizmasterMode(void)
{
    if (!mode_done_flags[QUIZMASTER_MODE])
    {
        PlayToneSequence(quizmaster_mode_tones, quizmaster_mode_len); // Intro
        orb.target_blinks = 3;
        orb.blinks_done = 0;
        mode_done_flags[QUIZMASTER_MODE] = true;

        quizmaster_triggered = false; // Reset für neue Frage
        last_p1_state = 1;
        last_p2_state = 1;
    }

    QuizmasterMode();
}

void HandleTonleiterMode(void)
{
    if (!mode_done_flags[TONLEITER_MODE])
    {
        PlayToneSequence(tonleiter_mode_tones, tonleiter_mode_len);
        orb.target_blinks = 4;
        orb.blinks_done = 0;
        mode_done_flags[TONLEITER_MODE] = true;
    }
}


/* ===================== PIN INIT ===================== */
void init_pins(void)
{
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
                           //(1ULL << OUT_P1_G_PIN) |
                           //(1ULL << OUT_P1_B_PIN) |
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
        .clk_cfg = LEDC_AUTO_CLK};
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

    // ledc_channel.gpio_num = OUT_P2_G_PIN;
    // ledc_channel.channel = LEDC_CHANNEL_5;
    // ledc_channel_config(&ledc_channel);

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
        .clk_cfg = LEDC_AUTO_CLK};
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
    dac_output_enable(DAC_CHANNEL_1); // GPIO25
    dac_output_enable(DAC_CHANNEL_2); // GPIO26
}

void app_main(void)
{
    ESP_LOGI("APP", "BOOT");

    init_pins();

    ESP_LOGI("APP", "AFTER_INIT");

    int last_switch = 1;

    /* ===== Startsong ===== */
    PlayToneSequence(
        win95_true_boot,
        win95_true_boot_len);

    TickType_t last_log_tick = xTaskGetTickCount();

    while (true)
    {
        TickType_t now = xTaskGetTickCount();

        // Mode-Button Edge Detect
        int state = gpio_get_level(IN_LED_PIN);
        if (state == 0 && last_switch == 1)
        {
            currentMode = (currentMode + 1) % MODE_COUNT;
            ESP_LOGI("MODE", "Changed to %d", currentMode);
            mode_effects_trigger();   /* <-- FIX */
        }

        last_switch = state; /* <-- FIX: immer updaten */
        // Handler einmalig aufrufen
        switch (currentMode)
        {
        case IDLE_MODE:
            HandleIdleMode();
            break;
        case BEEP_MODE:
            HandleBeepMode();
            break;
        case MIDI_MODE:
            HandleMidiMode();
            break;
        case QUIZMASTER_MODE:
            HandleQuizmasterMode();
            break;
        case TONLEITER_MODE:
            HandleTonleiterMode();
            break;
        default:
            break;
        }

        //last_switch = state;

        // ===== Runtime Updates zentral =====
        buzzer_update();
        ToneSequence_Update();
        orb_update();

        // ===== Optional: periodisches Loggen =====
        if (now - last_log_tick >= pdMS_TO_TICKS(1000))
        {
            ESP_LOGI("APP", "Running... Mode=%d Song=%d", currentMode, currentSong);
            last_log_tick = now;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}