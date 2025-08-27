/*
 * slip_test.cpp
 *
 *  Created on: 16.05.2020
 *      Author: badi
 */
#include <string.h>
#include <stdint.h>
#include "common.h"

#define BUFFER_SIZE 62
#define HEX_CNT 16
#define OFFSET  5

#include "gtest/gtest.h"
class CommonTest : public ::testing::Test {
    protected:

    void SetUp() override    {
    }

    void TearDown() {
    }
};

TEST_F(CommonTest, TestBufferPrint){
    uint8_t buffer[BUFFER_SIZE];
    memset(buffer,0, BUFFER_SIZE);
    for (uint8_t i=0;i<BUFFER_SIZE;i++){
        buffer[i]= i;
    }
    for (int8_t i=OFFSET;i<BUFFER_SIZE;i++){
        buffer[i]= ' '+i-OFFSET;
    }
    PrintBuffer(buffer, BUFFER_SIZE, NULL);
}

