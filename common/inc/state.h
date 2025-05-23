/*
 * state.h
 *
 *  Created on: Jan 22, 2025
 *      Author: TBA
 */

#ifndef INC_STATE_H_
#define INC_STATE_H_
#include "common.h"

#define CMD_LEN 4
typedef enum{
    OFF,
    BLINKING,
    ON,
    KEY_STAT_CNT
}key_state_e;

typedef struct state_s{
    uint8_t first;
    uint8_t cnt;
    bool dirty;
    char clabel; // Control label or 0
    key_state_e state[MAX_BUTTON_CNT];
    char  label[MAX_BUTTON_CNT];
} state_t; // Size is 2*MAX_BUTTON_CNT + 4=  36 Byte (MAX_BUTTON_CNT = 16)

typedef struct statea_s{
    uint8_t range;  // Unteres Nibble: tiefstes gültiges Labele (0-15);Oberes Nibble: letzes gültiges Label (0-15)
                    // und daher hat ein Label nur 2 Bit in den 16 Bit state Variable
    bool dirty;     // Das zwei Topbit indizieren die Art der Payload (01: *cmd, 11; cmdStr[CMD_LEN])
    uint16_t state; // Jedes label hat eine  state: 00=OFF, 01: BLINKING, 11 ON
    union cmd_u{
        char *cmd;  // Kann ein pointer zu einem Pointer enthalten, das den anderen Geräten mitgeteilt wird oder NULL
        char cmdStr[CMD_LEN]; //CMD_LEN ist 4 ist leer (nur Spaces) oder ein command
    } cmd;
    char  label[MAX_BUTTON_CNT];
} statea_t; // Size is MAX_BUTTON_CNT+8 = 24 Bytes

int8_t state_ch2idx(state_t *state, char ch);
void state_init(state_t *state);
void state_clear_state(state_t * state, char ch);
key_state_e state_get_state(state_t * state, char ch);
void state_set_label(state_t * state, char ch);
void state_set_u16(state_t * state, uint16_t u16);
uint16_t state_get_u16(state_t * state);
bool state_propagate(state_t *state, char ch);
bool state_is_same(state_t *last, state_t *this);
bool state_merge(state_t *inState, state_t *outState);
void state_print(state_t *state,  char *title );
void state_copy(state_t *from, state_t *to );

#endif /* INC_STATE_H_ */
