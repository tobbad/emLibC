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
    EXPECT_EQ(res, EM_ERR);
    res = state_init(&state);
    key_state_e key;
    uint32_t stat;
    EXPECT_EQ(res, EM_OK);
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

TEST_F(StateTest, TestSetAllStatesPropagadeIdx){
     state_t state;
     state_init(&state);
     //state_print(&state, "Initial state"NL);
     key_state_e key;
     uint32_t u32State=0;
     em_msg res;
     for (uint8_t i=0;i<MAX_STATE_CNT;i++){
    	key =state_get_key_by_idx(&state, i);
        EXPECT_EQ(key, (uint8_t) OFF);
        res= state_propagate_by_idx(&state, i);
        EXPECT_EQ(res, 1);
        key =state_get_key_by_idx(&state, i);
        EXPECT_EQ(key,  BLINKING);
        res= state_propagate_by_idx(&state, i);
		key = state_get_key_by_idx(&state, i);
        EXPECT_EQ(key,  ON);
    }
    u32State = state_get_u32(&state);
    //printf("Final u32 state %08x"NL, u32State);
    //state_print(&state, "Final state"NL);
    EXPECT_EQ(u32State, 0xaaaaaaaa);
}

TEST_F(StateTest, TestSetAllStatesPropagateLbl){
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


TEST_F(StateTest, TestCopyMergeAndCompare){
    state_t state1;
    state_t state2;
    state_t state3;
    state_t merge;
    state_init(&state1);
    state_init(&state2);
    state_init(&state3);
    state_init(&state3);
    em_msg res;
    // Setup test state
    for (uint8_t i=0;i<9;i++){
        key_state_e s1, s2;
        state_set_key_by_idx(&state1,i ,(key_state_e)(i/3));
        state_set_key_by_idx(&state2,i ,(key_state_e)(i%3));
    }
    state_copy(&state2, &merge);
    state_merge(&state1, &merge);
    state_set_key_by_idx(&state3, 0, OFF);
    state_set_key_by_idx(&state3, 1, BLINKING);
    state_set_key_by_idx(&state3, 2, ON);
    state_set_key_by_idx(&state3, 3, BLINKING);
    state_set_key_by_idx(&state3, 4, ON);
    state_set_key_by_idx(&state3, 5, OFF);
    state_set_key_by_idx(&state3, 6, OFF);
    state_set_key_by_idx(&state3, 7, BLINKING);
    state_set_key_by_idx(&state3, 8, ON);
    res = state_is_same(&merge, &state3);
	EXPECT_EQ(res, EM_OK);


}

TEST_F(StateTest, TestCopyMergeAndCompareOneOff){
    const uint8_t cnt = 8;
    state_t state1;
    state_t state2;
    state_t state3;
    state_t merge;
    state_init(&state1);
    state1.first = 2;
    state1.cnt = cnt;
    state_init(&state2);
    state_init(&state3);
    em_msg res;
    // Setup test state
    for (uint8_t i=0;i<cnt;i++){
        key_state_e s1, s2;
        state_set_key_by_idx(&state1,i ,(key_state_e)(i/3));
        state_set_key_by_idx(&state2,i ,(key_state_e)(i%3));
    }
    state_copy(&state2, &merge);
    state_print(&state1, "State1");
    state_print(&state2, "State2");
    state_merge(&state1, &merge);
    state_print(&merge, "Merge1");
    state_set_key_by_idx(&state3, 0, OFF);
    state_set_key_by_idx(&state3, 1, BLINKING);
    state_set_key_by_idx(&state3, 2, ON);
    state_set_key_by_idx(&state3, 3, BLINKING);
    state_set_key_by_idx(&state3, 4, ON);
    state_set_key_by_idx(&state3, 5, OFF);
    state_set_key_by_idx(&state3, 6, OFF);
    state_set_key_by_idx(&state3, 7, BLINKING);
    state_set_key_by_idx(&state3, 8, ON);
    state_print(&state3, "State3");
    res = state_is_same(&merge, &state3);
    EXPECT_EQ(res, EM_OK);


}


TEST_F(StateTest, TestSubKeyState){
    key_state_e res;
    res = state_key_diff(OFF, OFF);
    EXPECT_EQ(res, OFF);
    res = state_key_diff(OFF, BLINKING);
    EXPECT_EQ(res, BLINKING);
    res = state_key_diff(OFF, ON);
    EXPECT_EQ(res, ON);

    res = state_key_diff(BLINKING, OFF);
    EXPECT_EQ(res, ON);
    res = state_key_diff(BLINKING, BLINKING);
    EXPECT_EQ(res, OFF);
    res = state_key_diff(BLINKING, ON);
    EXPECT_EQ(res, BLINKING);

    res = state_key_diff(ON, OFF);
    EXPECT_EQ(res, BLINKING);
    res = state_key_diff(ON, BLINKING);
    EXPECT_EQ(res, ON);
    res = state_key_diff(ON, ON);
    EXPECT_EQ(res, OFF);


}
TEST_F(StateTest, TestSubKeyStateVec){
    state_t state1;
    state_t state2;
    state_t diff;
    state_t rdiff;
    uint8_t i = 0;
    state_init(&state1);
    state_init(&state2);
    state_init(&diff);
    state_init(&rdiff);
    state_set_key_by_idx(&rdiff, i++, OFF);
    state_set_key_by_idx(&rdiff, i++, BLINKING);
    state_set_key_by_idx(&rdiff, i++, ON);
    state_set_key_by_idx(&rdiff, i++, ON);
    state_set_key_by_idx(&rdiff, i++, OFF);
    state_set_key_by_idx(&rdiff, i++, BLINKING);
    state_set_key_by_idx(&rdiff, i++, BLINKING);
    state_set_key_by_idx(&rdiff, i++, ON);
    state_set_key_by_idx(&rdiff, i++, OFF);
    em_msg res = EM_OK;
    // Setup test state
    for (uint8_t i=0;i<9;i++){
        key_state_e s1, s2;
        state_set_key_by_idx(&state1,i ,(key_state_e)(i/3));
        state_set_key_by_idx(&state2,i ,(key_state_e)(i%3));
    }
    state_diff(&state1, &state2, &diff);
    //state_print(&state1, (char*)"state1");
    //state_print(&state2, (char*)"state2");
    //state_print(&diff, (char*)"diff");
    res= state_is_same(&rdiff, &diff);
    EXPECT_EQ(res, true);
}

TEST_F(StateTest, TestKeyAdd){
    state_t state1;
    state_t state2;
    state_t diff;
    state_init(&state1);
    state_init(&state2);
    state_init(&diff);
    em_msg res = EM_OK;
    //printf("Setup test state"NL);
    for (uint8_t i=0;i<9;i++){
        key_state_e s1, s2;
        state_set_key_by_idx(&state1,i ,(key_state_e)(i/3));
        state_set_key_by_idx(&state2,i ,(key_state_e)(i%3));
        //printf("%d"NL, i);
    }
    //state_print(&state1, (char*)"state 1");
    //state_print(&state2, (char*)"state 2");
    state_diff(&state1, &state2, &diff);
    //state_print(&diff, (char*)"state1 -state2");
    state_add(&state1, &diff);
    //printf("state 1 +diff"NL);
    //state_print(&state1, (char*)"state1 + diff");

    res= state_is_same(&state1, &state2);
    EXPECT_EQ(res, true);
}

