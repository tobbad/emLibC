/*
 * state.h
 *
 *  Created on: Jan 22, 2025
 *      Author: TBA
 */

#ifndef INC_STATE_H_
#define INC_STATE_H_
#include "common.h"


typedef enum{
    OFF,
    BLINKING,
    ON,
    STATE_CNT
}key_state_e;


typedef struct state_s{
    uint8_t first;
    uint8_t cnt;
    uint8_t dirty; //bitfield??
					// Evtl. können in den obersten 2 bit der Inhalt des clabel fields
                    // encodiert werden (01: cmd, 11:str))
    clabel_u clabel;
    key_state_e state[MAX_BUTTON_CNT];
    char  label[MAX_BUTTON_CNT];
} state_t; // Size is 2*MAX_BUTTON_CNT + 4=  36 Byte (MAX_BUTTON_CNT = 16)


typedef struct statea_s{
    uint8_t range;  // Unteres Nibble: tiefstes gültiges Labele (0-15):
                    // Oberes Nibble: letzes gültiges Label (0-15)
                    // und daher hat ein Label nur 2 Bit in den 16 Bit state Variable
    bool dirty;     // Das zwei Topbit indizieren die Art der clabel (01: *cmd, 11; str[CMD_LEN])
    uint32_t state; // Jedes label hat eine  state: 00=OFF, 01: BLINKING, 11 ON 16*2 =32 Bit
    clabel_u clabel;// 4 Bytes
} statea_t; // Size is 10 Bytes, Label gibt es nicht da es owiso MAX_BUTTON_CNT (0..MAX_BUTTON_CNT-1) Labels gibt

uint8_t clable2type(clabel_u *lbl);
int8_t state_ch2idx(state_t *state, char ch);
int8_t state_nr2idx(state_t *state, uint8_t nr);
void state_init(state_t *state);
void state_clear(state_t * state);
void state_undirty(state_t * state);
key_state_e state_get_state(state_t * state, char ch);
void state_set_value(state_t * state, uint8_t nr, key_state_e new_state);
void state_set_label(state_t * state, char ch, key_state_e new_state);
void state_set_index(state_t * state, uint8_t  nr, key_state_e new_state);
void state_set_u32(state_t * state, uint32_t u32);
uint32_t state_get_u32(state_t * state);
bool state_propagate(state_t *state, char ch);
bool state_propagate_index(state_t *state, uint8_t nr);
bool state_is_same(state_t *last, state_t *this);
bool state_merge(state_t *inState, state_t *outState);
void state_print(state_t *state,  char *title );
uint8_t state_get_cnt(state_t *state);
uint8_t state_get_first(state_t *state);
void state_set_cnt(state_t *state, uint8_t nr);
void state_set_first(state_t *state, uint8_t nr);

#endif /* INC_STATE_H_ */
