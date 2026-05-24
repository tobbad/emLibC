/*
 * state.h
 *
 *  Created on: Jan 22, 2025
 *      Author: TBA
 */

#ifndef INC_STATE_H_
#define INC_STATE_H_
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum __attribute__((packed)) { OFF, BLINKING, ON, STATE_CNT } key_state_e;

extern char key2char[][4];

#define MAX_STATE_CNT MAX_BUTTON_CNT
#define STATE_MASK   0x03

// #define MODDIFF( ref_i, state_i)
// {ref_i==state_i?0?ref_i>state_i?ref_i-state_i:state_i+STATE_CNT-ref_i};
#if REDUCED_PAYLOAD == 0
typedef struct state_s {
    uint8_t first;
    uint8_t cnt;
    uint8_t dirty;
    uint8_t id;                       // 8 Bitcounter, der bei jedem Senden in diesem Slots
                                      // inkrementiert mitgesendet wird
    clabel_u clabel;                  // is 4 bytes
    key_state_e state[MAX_STATE_CNT]; // 16 bytes
    char label[MAX_STATE_CNT];        // 16 bytes
} state_t;                            // Size is 2*MAX_BUTTON_CNT + 8=  40 Byte (MAX_BUTTON_CNT = 16)

#define STATE_SET_RANGE(_s, _first, _cnt)                  \
    do {                                                    \
        (_s).first = _first) & 0x0F;   \
        (_s).cnt   = _cnt;              \
    } while (0)
#define STATE_SET_RANGEP(_s, _first, _cnt)                  \
    do {                                                    \
        (_s)->first = _first;            \
        (_s)->cnt   = _cnt;              \
    } while (0)
#define STATE_SET_FIRSTP(_s, _first)                  \
    do {                                                    \
        (_s)->first = _first;            \
    } while (0)

#define STATE_SET_CNTP(_s, cnt)                  \
    do {                                                    \
        (_s)->cnt = cnt;            \
    } while (0)

#else
typedef struct state_s {
uint8_t range;   // Unteres Nibble: tiefstes gültiges Labele (0-15):
                 // Oberes Nibble: letzes gültiges Label (0-15)
                 // und daher hat ein Label nur 2 Bit in den 16 Bit state Variable
bool dirty;      // Das zwei Topbit indizieren die Art der clabel (01: *cmd, 11;
                 // str[CMD_LEN])
uint8_t id;      // 8 Bitcounter, der bei jedem Senden in diesem Slots
uint32_t state;  // Jedes label hat eine  state: 00=OFF, 01: BLINKING, 11 ON
                 // 16*2 =32 Bit
clabel_u clabel; // 4 Bytes
} state_t;          // Size is 10 Bytes oder mit id 11 Bytes, Label gibt es nicht da es sowiso MAX_BUTTON_CNT
                 // (0..MAX_BUTTON_CNT-1) Labels gibt

#define STATE_SET_RANGE(_s, _first, _cnt)                  \
    do {                                                    \
        (_s).range = (((_first) & 0x0F) |                  \
                     (((_cnt) & 0x0F) << 4));              \
    } while (0)
#define STATE_SET_RANGEP(_s, _first, _cnt)                  \
    do {                                                    \
        (_s)->range = (((_first) & 0x0F) |                  \
                      (((_cnt) & 0x0F) << 4));              \
    } while (0)

#define STATE_SET_FIRST(_s, _first)                  \
    do {                                                    \
        (_s)->range = ((s->range&0xF0|(_first) & 0x0F);              \
    } while (0)

#define STATE_FIRSTP(_s)                  \
    do {                                                    \
        (_s)->range& 0x0F ;              \
    } while (0)
#define STATE_FIRST(_s)                  \
    do {                                                    \
        (_s).range& 0x0F;              \
    } while (0)

#define STATE_CNTP(_s)                  \
    do {                                                    \
        ((_s)->range& 0xF0)>>4;              \
    } while (0)
#define STATE_CNT(_s)                   \
    do {                                                    \
        ((_s).range& 0xF0)>>4;              \
    } while (0)
#endif
int8_t state_ch2idx(const state_t *state, char ch);
int8_t state_nr2idx(state_t *state, uint8_t nr);
em_msg state_init(state_t *state);
em_msg state_reset(state_t *state);
em_msg state_set_first(state_t *state, uint8_t cnt);
em_msg state_set(state_t *state, uint8_t nr, key_state_e);
key_state_e state_get(const state_t *state, uint8_t nr);
em_msg state_set_state(const state_t *from, state_t *to);
em_msg state_check(const state_t *state);
em_msg state_undirty(state_t *state);
key_state_e state_key_diff(key_state_e state1, key_state_e state2);
key_state_e state_get_key_by_lbl(const state_t *state, char ch);
key_state_e state_get_key_by_idx(const state_t *state, uint8_t idx);
em_msg state_propagate_by_state(const state_t *inState, state_t *outState);
em_msg state_set_key_by_idx(state_t *state, uint8_t nr, key_state_e new_state);
em_msg state_set_key_by_lbl(state_t *state, char ch, key_state_e new_state);
em_msg state_propagate_by_lbl(state_t *state, char ch);
em_msg state_propagate_by_idx(state_t *state, uint8_t nr);
em_msg state_propagate(state_t *inState, uint8_t idx);
em_msg state_set_u32(state_t *state, uint32_t u32);
uint32_t state_get_u32(state_t *state);
em_msg state_copy(const state_t *from, state_t *to);
em_msg state_is_same(state_t *last, state_t *cur);
em_msg state_diff(state_t *ref, state_t *state, state_t *diff);
em_msg state_add(state_t *inState, state_t *add);
bool   state_merge(state_t *inState, state_t *outState);
em_msg state_check(const state_t *state);
em_msg state_print(const state_t *state, const char *title, bool doLong);
em_msg state_get_dirty(state_t *state);
em_msg state_set_dirty(state_t *state);
em_msg state_set_undirty(state_t *state);
uint8_t state_get_cnt(state_t *state);
uint8_t state_get_first(state_t *state);
em_msg state_set_cnt(state_t *state, uint8_t nr);
// only for Debug
#ifdef UNIT_TEST
em_msg state_set_key_by_idx_unchecked(state_t *state, uint8_t nr, uint8_t new_state);
#endif

#ifdef __cplusplus
}
#endif

#endif /* INC_STATE_H_ */
