/*
 * slip_test.cpp
 *
 *  Created on: 16.05.2020
 *      Author: badi
 */
#include <cstdio>
#include <string.h>
#include <stdint.h>
#include "device.h"
#include "state.h"


#include "gtest/gtest.h"



class StateTest : public ::testing::Test {
    protected:

    void SetUp() override
    {
    }

    void TearDown()
    {
    }
};

TEST_F(StateTest, TestNullTestCorrectLabelsAndInitState){
	state_t state;
    em_msg res = state_init(NULL);
    key_state_e key;
    uint32_t stat;
    EXPECT_EQ(res, EM_ERR);
    res = state_init(&state);
    EXPECT_EQ(state.first, 0);
    EXPECT_EQ(state.cnt , MAX_STATE_CNT);
    EXPECT_EQ(state.dirty, false);
    EXPECT_EQ(state.clabel.cmd, 0);
//    state_set_key_by_lbl(&state, '0', ON);
//    state_print(&state, NULL);
//    key = state_get_key_by_idx(&state, 0);
    //EXPECT_EQ(key, ON);
    EXPECT_EQ(state.clabel.cmd, 0);
    for (uint8_t i=0;i<MAX_STATE_CNT;i++){
         EXPECT_EQ(state.state[i], OFF);
         EXPECT_EQ(state.label[i], int2hchar(i));
    }
    stat = state_get_u32(&state);
    EXPECT_EQ(stat, 0);

}

TEST_F(StateTest, Teststate_ch2idx){
     state_t state;
     state_init(&state);
     char ch;

     for (uint8_t i=0;i<MAX_STATE_CNT;i++){
         ch = state.label[i];
         uint8_t r = state_ch2idx(&state, ch);
         EXPECT_EQ(i, r);
     }

}

TEST_F(StateTest, TestSetAllStatesPropagadeIdxIdx){
     state_t state;
     state_init(&state);
     key_state_e key;
     uint32_t u32State;
     em_msg res;
     for (uint8_t i=0;i<MAX_STATE_CNT;i++){
    	key =state_get_key_by_idx(&state, i);
        EXPECT_EQ(key, (uint8_t) OFF);
        res= state_propagate_by_idx(&state, i);
        key =state_get_key_by_idx(&state, i);
        EXPECT_EQ(key,  BLINKING);
        res= state_propagate_by_idx(&state, i);
		key = state_get_key_by_idx(&state, i);
        EXPECT_EQ(key,  ON);
    }
    u32State = state_get_u32(&state);
    EXPECT_EQ(u32State, 0xAAAAAAAA);
}

TEST_F(StateTest, TestSetAllStatesPropagateLblLbl){
     state_t state;
     state_init(&state);
     key_state_e key;
     uint32_t u32State;
     em_msg res;
     for (uint8_t i=0;i<MAX_STATE_CNT;i++){
    	key =state_get_key_by_lbl(&state, state.label[i]);
        EXPECT_EQ(key, (uint8_t) OFF);
        res= state_propagate_by_lbl(&state, state.label[i]);
        key =state_get_key_by_lbl(&state, state.label[i]);
        EXPECT_EQ(key,  BLINKING);
        res= state_propagate_by_lbl(&state, state.label[i]);
		key = state_get_key_by_lbl(&state, state.label[i]);
        EXPECT_EQ(key,  ON);
    }
    u32State = state_get_u32(&state);
    EXPECT_EQ(u32State, 0xAAAAAAAA);
}

TEST_F(StateTest, TestSetAllStatesPropagadeIdxLbl){
     state_t state;
     state_init(&state);
     key_state_e key;
     uint32_t u32State;
     em_msg res;
     for (uint8_t i=0;i<MAX_STATE_CNT;i++){
    	key =state_get_key_by_idx(&state, i);
        EXPECT_EQ(key, (uint8_t) OFF);
        res= state_propagate_by_lbl(&state, state.label[i]);
        key =state_get_key_by_idx(&state, i);
        EXPECT_EQ(key,  BLINKING);
        res= state_propagate_by_lbl(&state, state.label[i]);
		key = state_get_key_by_idx(&state, i);
        EXPECT_EQ(key,  ON);
    }
    u32State = state_get_u32(&state);
    EXPECT_EQ(u32State, 0xAAAAAAAA);
}

TEST_F(StateTest, TestSetAllStatesPropagadeLblIdx){
     state_t state;
     state_init(&state);
     key_state_e key;
     uint32_t u32State;
     em_msg res;
     for (uint8_t i=0;i<MAX_STATE_CNT;i++){
    	key =state_get_key_by_lbl(&state,state.label[i]);
        EXPECT_EQ(key, (uint8_t) OFF);
        res= state_propagate_by_idx(&state, i);
        key =state_get_key_by_lbl(&state,state.label[i]);
        EXPECT_EQ(key,  BLINKING);
        res= state_propagate_by_idx(&state, i);
		key = state_get_key_by_lbl(&state, state.label[i]);
        EXPECT_EQ(key,  ON);
    }
    u32State = state_get_u32(&state);
    EXPECT_EQ(u32State, 0xAAAAAAAA);
}

TEST_F(StateTest, TestSetLimitedStates){
     state_t state;
     state_init(&state);
     uint8_t low=2,up=6;
     uint8_t test;
     int8_t stat;
     uint32_t u32State;
     em_msg res;

     state_set_first(&state, low);
     test =state_get_first(&state);
     EXPECT_EQ(test,  low);
     state_set_cnt(&state, up);
     test =state_get_cnt(&state);
     EXPECT_EQ(test,  up);

     for (uint8_t i=0;i<MAX_STATE_CNT;i++){
    	stat =state_get_key_by_lbl(&state, state.label[i]);
    	if ((em_msg)stat==EM_ERR) continue;
        EXPECT_EQ(stat, (uint8_t) OFF);
        res= state_propagate_by_lbl(&state, state.label[i]);
      	stat =state_get_key_by_idx(&state, i);
    	if ((em_msg)stat==EM_ERR) continue;
    	if (res>=0){
			EXPECT_EQ(stat,  BLINKING);
		} else{
			printf("i= %d got error", i);
			EXPECT_EQ(stat, (uint8_t) OFF);
		}
    	state_propagate_by_idx(&state, i);
   	    stat =state_get_key_by_lbl(&state, state.label[i]);
   	    if (stat>=0){
			EXPECT_EQ(stat,  ON);
		} else{
			printf("i= %d got error", i);
			EXPECT_EQ(stat, (uint8_t) OFF);
		}
    }
    u32State = state_get_u32(&state);
    EXPECT_EQ(u32State, 0x0000AAA0);

}

