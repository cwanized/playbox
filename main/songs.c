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

const tone_step_t boot_sequence_fancy2[] = {
    { 523, 150 },  // C5
    { 659, 150 },  // E5
    { 784, 200 },  // G5
    { 880, 250 },  // A5
    { 987, 200 },  // B5
    { 1047, 300 }, // C6 (Abschluss)
};
const int boot_sequence_fancy2_len = sizeof(boot_sequence_fancy2) / sizeof(tone_step_t);

/* ================= Windows 95 Boot Sound ================= */
const tone_step_t win95_boot[] = {
    { 800, 130 },  // kräftiger Ton (ungefähr ~G5/Bb5 Intervallanfang)
    { 660, 260 },  // tiefer Ton als Auflösung (~E5‑ish)
    { 0,   150 }   // kurzer Stille‑Puffer zum Ausklingen
};

const int win95_boot_len =
    sizeof(win95_boot) / sizeof(tone_step_t);


const tone_step_t win95_speak_boot[] = {
    { 659, 150 },  // E5 – kurzer Startton ("ta")
    { 784, 150 },  // G5 – kurzer Mittelton ("ta")
    { 987, 500 },  // B5 – langer Abschluss ("taaa")
    { 784, 200 },  // G5 – leichtes Nachklingen
    { 659, 300 },  // E5 – Ruhepunkt / Abschluss
};

const int win95_speak_boot_len =
    sizeof(win95_speak_boot) / sizeof(tone_step_t);

const tone_step_t win95_true_boot[] = {
    { 523, 150 },  // C5 – kurzer Startton "ta"
    { 659, 150 },  // E5 – kurzer Mittelton "ta"
    { 784, 500 },  // G5 – langer Abschluss "taaa"
    { 659, 200 },  // E5 – leichtes Nachklingen
    { 523, 300 },  // C5 – ruhiger Abschluss
};

const int win95_true_boot_len =
    sizeof(win95_true_boot) / sizeof(tone_step_t);    


const tone_step_t custom_boot[] = {
    { 523, 150 },  // C5 – kurzer Startton
    { 659, 200 },  // E5 – zweite Stufe
    { 784, 400 },  // G5 – Akkordabschluss
    { 880, 300 },  // A5 – finaler Aufstieg
    { 784, 250 },  // G5 – leichtes „Abklingen“
    { 659, 250 },  // E5 – sanft zurück
    { 523, 300 }   // C5 – Ruhepunkt / Ende
};
const int custom_boot_len =
    sizeof(custom_boot) / sizeof(tone_step_t);


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

    /* ==================== Alle meine Entchen ==================== */

const tone_step_t alle_meine_entchen[] = {
    { 523, 400 },  // C5 – Alle
    { 523, 400 },  // C5 – mei-
    { 587, 400 },  // D5 – ne
    { 659, 400 },  // E5 – Ent-
    { 659, 400 },  // E5 – chen
    { 587, 400 },  // D5 – schwim-
    { 523, 400 },  // C5 – men
    { 523, 400 },  // C5 – auf
    { 392, 400 },  // G4 – dem
    { 392, 400 },  // G4 – See
    { 440, 400 },  // A4 – Köpf-
    { 392, 400 },  // G4 – fen
    { 440, 400 },  // A4 – und
    { 392, 400 },  // G4 – Schwänz-
    { 349, 400 },  // F4 – chen
    { 330, 600 }   // E4 – weiß
};

const int alle_meine_entchen_len =
    sizeof(alle_meine_entchen) / sizeof(tone_step_t);
