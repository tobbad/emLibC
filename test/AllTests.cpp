/*
/* AllTests.cpp
 *
 *  Created on: 18.01.2015
 *      Author: badi
 */

#include "gtest/gtest.h"
#include "gmock/gmock.h"

int main(int argc, char** argv)
{
    testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

