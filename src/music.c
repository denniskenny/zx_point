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

/* The three sea shanties now play as 3-channel Tritone arrangements
 * (see oro_play/lowlands_play/spanish_play).  The single-voice lead
 * versions were removed; only the short level-complete fanfare still
 * uses this frame-paced beeper player. */

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
