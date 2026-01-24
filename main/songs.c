#include "songs.h"

const tone_step_t beep_mode_tones[] = {
    {440, 150}
};
const int beep_mode_len = sizeof(beep_mode_tones)/sizeof(tone_step_t);

const tone_step_t midi_mode_tones[] = {
    {660, 150}, {760, 150}
};
const int midi_mode_len = sizeof(midi_mode_tones)/sizeof(tone_step_t);

const tone_step_t quizmaster_mode_tones[] = {
    {880, 150}, {950, 150}, {1020, 150}
};
const int quizmaster_mode_len = sizeof(quizmaster_mode_tones)/sizeof(tone_step_t);

const tone_step_t tonleiter_mode_tones[] = {
    {1120, 150}, {1220, 150}, {1320, 150}, {1420, 150}
};
const int tonleiter_mode_len = sizeof(tonleiter_mode_tones)/sizeof(tone_step_t);


/* ================= Boot Sequenz ================= */

const tone_step_t boot_sequence[] = {
    { 523, 150 }, // C5
    { 587, 150 }, // D5
    { 659, 150 }, // E5
    { 698, 150 }, // F5
    { 784, 300 }, // G5
    { 659, 150 }, // E5
    { 784, 300 }, // G5
    { 880, 400 }, // A5
    { 784, 200 }, // G5
    { 659, 200 }, // E5
    { 587, 150 }, // D5
    { 523, 400 }  // C5
};

const int boot_sequence_len =
    sizeof(boot_sequence) / sizeof(tone_step_t);

    /* ================= Futuristischer Boot-Ton ================= */

const tone_step_t boot_sequence_fancy[] = {
    { 523, 120 }, // C5
    { 587, 120 }, // D5
    { 659, 120 }, // E5
    { 698, 120 }, // F5
    { 784, 200 }, // G5
    { 880, 150 }, // A5
    { 784, 150 }, // G5
    { 880, 150 }, // A5 (Triller hoch)
    { 988, 150 }, // B5
    { 1047, 250 }, // C6 (Höhepunkt)
    { 988, 150 },  // B5
    { 880, 150 },  // A5
    { 784, 200 },  // G5
    { 698, 200 },  // F5
    { 659, 300 },  // E5 (sanftes Landen)
    { 523, 400 }   // C5 (Abschluss)
};

const int boot_sequence_fancy_len =
    sizeof(boot_sequence_fancy) / sizeof(tone_step_t);

/* ================= Kurzer Boot-Ton ================= */
const tone_step_t boot_sequence_short[] = {
    { 523, 120 }, // C5
    { 659, 120 }, // E5
    { 784, 150 }, // G5
    { 880, 200 }, // A5 (leicht steigender Höhepunkt)
    { 1047, 250 }, // C6 (Höhepunkt)
    { 880, 150 },  // A5
    { 784, 200 },  // G5
    { 659, 300 },  // E5 (sanftes Landen)
    { 523, 400 }   // C5 (Abschluss)
};
const int boot_sequence_short_len = sizeof(boot_sequence_short) / sizeof(tone_step_t);


/* ================= Windows 95 Boot Sound ================= */
const tone_step_t win95_boot[] = {
    { 523, 150 },  // C5
    { 659, 150 },  // E5
    { 784, 200 },  // G5
    { 880, 250 },  // A5
    { 987, 200 },  // B5
    { 1047, 300 }, // C6 (Abschluss)
};
const int win95_boot_len = sizeof(win95_boot) / sizeof(tone_step_t);



/* ================= Ode an die Freude ================= */

const tone_step_t ode_an_die_freude[] = {
    { 330, 400 }, // E4
    { 330, 400 },
    { 349, 400 },
    { 392, 400 },
    { 392, 400 },
    { 349, 400 },
    { 330, 400 },
    { 294, 400 },
    { 262, 400 },
    { 262, 400 },
    { 294, 400 },
    { 330, 400 },
    { 330, 600 },
    { 294, 200 },
    { 294, 600 }
};

const int ode_an_die_freude_len =
    sizeof(ode_an_die_freude) / sizeof(tone_step_t);

/* ================= Happy Birthday ================= */

const tone_step_t melody_happy_birthday[] = {
    { 264, 300 }, { 264, 200 }, { 297, 500 }, { 264, 500 },
    { 352, 500 }, { 330, 800 },

    { 264, 300 }, { 264, 200 }, { 297, 500 }, { 264, 500 },
    { 396, 500 }, { 352, 800 },

    { 264, 300 }, { 264, 200 }, { 528, 500 }, { 440, 500 },
    { 352, 500 }, { 330, 500 }, { 297, 800 },

    { 466, 300 }, { 466, 200 }, { 440, 500 },
    { 352, 500 }, { 396, 500 }, { 352, 1000 }
};

const int melody_happy_birthday_len =
    sizeof(melody_happy_birthday) / sizeof(tone_step_t);

/* ================= Hallelujah (Motiv) ================= */

const tone_step_t melody_hallelujah_motif[] = {
    { 262, 400 },
    { 262, 400 },
    { 330, 600 },
    { 330, 300 },
    { 349, 400 },
    { 330, 800 },
    { 262, 400 },
    { 294, 400 },
    { 330, 1000 }
};

const int melody_hallelujah_motif_len =
    sizeof(melody_hallelujah_motif) / sizeof(tone_step_t);
