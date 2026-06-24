/*
 * music.c — Frame-paced single-voice beeper melody player
 *
 * Renders the Channel A (lead) lines of the design-doc sea shanties on
 * the 48K beeper. Each music_tick() emits a short tone burst at the
 * current note's pitch, articulating notes with one silent frame at the
 * end so repeated pitches stay distinct. Pacing is one tick per frame,
 * so the caller must invoke music_tick() once per vsync from a static
 * screen loop. See music.h for scope/limitations.
 */

#include <stdint.h>
#include "../include/music.h"
#include "../include/sound.h"
#include "../include/vsync.h"

/* --- Pitch table (sfx_play_note delay values; higher = lower pitch) ---
 * delay(freq) ~= 108200 / freq.  Capitals = octave 5, lowercase = oct 6.
 * Max playable delay is 255 (~A4); all melody leads stay above that.    */
#define R    0     /* rest */
#define A4 246
#define B4 219
#define C5 207
#define D5 184
#define E5 164
#define FS 146    /* F#5 */
#define F5 155
#define G5 138
#define A5 123
#define B5 110
#define c6 103
#define d6  92
#define e6  82

/* Frames-per-eighth tempo unit for each tune (50 Hz). */
/* Oro: rousing march.  Lowlands: haunting.  Spanish: slow dirge. */

/* --- Oro Se do Bheatha 'Bhaile (Lead, transposed +1 octave) --- */
const music_note_t music_oro[] = {
    {A5,32},{A5,16},{G5, 8},{A5, 8},
    {B5,16},{A5,16},{G5,16},{E5,16},
    {G5,32},{G5,16},{G5,12},{A5, 4},
    {G5,16},{D5,16},{E5,16},{G5,16},
    {A5,24},{A5, 8},{A5,16},{G5,12},{A5, 4},
    {B5,16},{A5,16},{B5,16},
    {d6,64},{d6,16},
    {e6,16},{B5,16},{d6,16},{B5,16},
    {A5,28},{B5, 4},{A5,32},
    {R,  24},   /* breath before the loop */
};
const uint8_t music_oro_len = sizeof(music_oro) / sizeof(music_oro[0]);

/* --- Lowlands Away (Lead) --- */
const music_note_t music_lowlands[] = {
    {c6,56},{G5,56},
    {c6,14},{d6,14},{e6,14},{d6,14},{c6,28},{B5,28},
    {G5,84},{F5,28},
    {B5,42},{c6,14},{d6,28},{B5,28},
    {c6,28},{B5,28},{F5,28},{B5,14},{A5,14},
    {G5,56},{F5,28},{E5,28},
    {C5,84},
    {C5,28},
    {E5,14},{F5,14},{G5,14},{E5,14},{F5,14},{E5,14},{C5,28},
    {c6,56},{G5,56},
    {c6,14},{d6,14},{e6,14},{d6,14},{c6,28},{B5,28},
    {G5,84},{F5,28},
    {B5,42},{c6,14},{d6,28},{B5,28},
    {c6,28},{B5,28},{F5,28},{B5,14},{A5,14},
    {G5,56},{F5,28},{E5,28},
    {C5,84},
    {R, 28},
};
const uint8_t music_lowlands_len = sizeof(music_lowlands) / sizeof(music_lowlands[0]);

/* --- Spanish Ladies / Farewell To Spain (Lead, slow dirge) --- */
const music_note_t music_spanish[] = {
    {B4,50},
    {E5,50},{E5,50},{FS,50},
    {E5,100},{E5,25},{FS,25},
    {G5,50},{FS,50},{E5,50},
    {E5,25},{D5,25},{B4,50},{B4,50},
    {E5,50},{E5,50},{FS,50},
    {E5,100},{FS,50},
    {G5,50},{A5,50},{G5,50},
    {FS,100},{FS,50},
    {G5,50},{FS,50},{G5,50},
    {A5,50},{G5,50},{A5,25},{A5,25},
    {B5,25},{A5,25},{G5,50},{E5,50},
    {E5,25},{D5,25},{B4,50},{B5,25},{A5,25},
    {G5,50},{E5,50},{E5,50},
    {E5,25},{D5,25},{B4,50},{A4,50},
    {B4,50},{G5,50},{FS,50},
    {E5,100},
};
const uint8_t music_spanish_len = sizeof(music_spanish) / sizeof(music_spanish[0]);

/* --- Trumpet fanfare: C-E-G-C' --- */
const music_note_t music_fanfare[] = {
    {C5,12},{E5,12},{G5,12},{c6,24},
};
const uint8_t music_fanfare_len = sizeof(music_fanfare) / sizeof(music_fanfare[0]);

/* --- Player state --- */
static const music_note_t *m_song;
static uint8_t m_len;
static uint8_t m_idx;
static uint8_t m_loop;
static uint8_t m_active;
static uint8_t m_rem;      /* frames remaining for current note */
static uint8_t m_delay;    /* current note pitch */
static uint8_t m_burst;    /* half-cycles to emit per audible frame */

/* T-state budget for one note's per-frame tone burst (~0.4 frame). */
#define SUSTAIN_T 30000

static void load_note(void)
{
    const music_note_t *n = &m_song[m_idx];
    uint16_t per;

    m_delay = n->delay;
    m_rem   = n->frames;

    if (m_delay == 0) {
        m_burst = 0;            /* rest */
    } else {
        per = 43 + ((uint16_t)m_delay << 5);   /* T-states per half-cycle */
        m_burst = (uint8_t)(SUSTAIN_T / per);
        if (m_burst == 0) m_burst = 1;
    }
}

void music_start(const music_note_t *song, uint8_t len, uint8_t loop)
{
    m_song   = song;
    m_len    = len;
    m_loop   = loop;
    m_idx    = 0;
    m_active = 1;
    load_note();
}

void music_stop(void)
{
    m_active = 0;
}

void music_tick(void)
{
    if (!m_active) return;

    if (m_rem == 0) {
        if (++m_idx >= m_len) {
            if (m_loop) {
                m_idx = 0;
            } else {
                m_active = 0;
                return;
            }
        }
        load_note();
    }

    /* Last frame of a note stays silent for articulation. */
    if (m_rem > 1 && m_burst)
        sfx_play_note(m_burst, m_delay);

    m_rem--;
}

void music_play_blocking(const music_note_t *song, uint8_t len)
{
    music_start(song, len, 0);
    while (m_active) {
        vsync_wait();
        music_tick();
    }
}
