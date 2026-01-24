#pragma once

#include <stdint.h>

/* Forward-Deklaration des Typs */
typedef struct {
    uint32_t freq_hz;
    uint32_t duration_ms;
} tone_step_t;

/* ===== Mode Tones ===== */
extern const tone_step_t beep_mode_tones[];
extern const int beep_mode_len;

extern const tone_step_t midi_mode_tones[];
extern const int midi_mode_len;

extern const tone_step_t quizmaster_mode_tones[];
extern const int quizmaster_mode_len;

extern const tone_step_t tonleiter_mode_tones[];
extern const int tonleiter_mode_len;

/* ===== Boot Sequences ===== */
extern const tone_step_t boot_sequence[];
extern const int boot_sequence_len;

extern const tone_step_t boot_sequence_fancy[];
extern const int boot_sequence_fancy_len;

extern const tone_step_t boot_sequence_fancy2[];
extern const int boot_sequence_fancy2_len;

extern const tone_step_t boot_sequence_short[];
extern const int boot_sequence_short_len;

extern const tone_step_t win95_boot[];
extern const int win95_boot_len;

extern const tone_step_t win95_speak_boot[];
extern const int win95_speak_boot_len;

extern const tone_step_t win95_true_boot[];
extern const int win95_true_boot_len;

extern const tone_step_t custom_boot[];
extern const int custom_boot_len;

/* ===== Songs ===== */
extern const tone_step_t ode_an_die_freude[];
extern const int ode_an_die_freude_len;

extern const tone_step_t melody_happy_birthday[];
extern const int melody_happy_birthday_len;

extern const tone_step_t melody_hallelujah_motif[];
extern const int melody_hallelujah_motif_len;

extern const tone_step_t alle_meine_entchen[];
extern const int alle_meine_entchen_len;
