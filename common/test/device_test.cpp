

#include <iostream>
#include <cstdint>
#include "device.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

dev_handle dummy_open(void){ return 0; };
elres_t dummy_read(uint8_t *buffer, uint16_t cnt){ return EMLIB_ERROR;};
elres_t dummy_write(uint8_t *buffer, uint16_t cnt){ return EMLIB_ERROR;};
elres_t dummy_ioctrl(dev_command_t cmd, uint16_t value){ return EMLIB_ERROR;};
elres_t dummy_close(dev_handle hdl){ return EMLIB_ERROR;};


class DeviceTest : public ::testing::Test {

    protected:

    device_t dev;

    void SetUp()
    {


    }

    void TearDown()
    {

    }

    void init_struct(uint8_t select){
        uint8_t idx = 0;
        if (select & (1<<idx)) {
            dev.open = dummy_open;
        } else {
            dev.open = NULL;
        }
        idx++;
        if (select & (1<<idx)) {
            dev.read = dummy_read;
        } else {
            dev.read = NULL;
        }
        idx++;
        if (select & (1<<idx)) {
            dev.write = dummy_write;
        } else {
            dev.write = NULL;
        }
        idx++;
        if (select & (1<<idx)) {
            dev.ioctrl = dummy_ioctrl;
        } else {
            dev.ioctrl = NULL;
        }
        idx++;
        if (select & (1<<idx)) {
            dev.close = dummy_close;
        } else {
            dev.close = NULL;
        }
    }
};
/*
 * Check that the internal used function(s) work correct
 */
TEST_F(DeviceTest, internal_init_struct){
    for (uint8_t i=0;i<=DEV_ALL;++i)
    {
        init_struct(i);
        if (i & DEV_OPEN  ){
            EXPECT_EQ(dev.open, dummy_open);
        } else {
            EXPECT_TRUE(dev.open==NULL);

        }
        if (i & DEV_READ  ){
            EXPECT_EQ(dev.read  , dummy_read);
        } else {
            EXPECT_TRUE(dev.read==NULL);

        }
        if (i & DEV_WRITE ){
            EXPECT_EQ(dev.write , dummy_write);
        } else {
            EXPECT_TRUE(dev.write==NULL);

        }
        if (i & DEV_IOCTRL){
            EXPECT_EQ(dev.ioctrl, dummy_ioctrl);
        } else {
            EXPECT_TRUE(dev.ioctrl==NULL);

        }
        if (i & DEV_CLOSE ){
            EXPECT_EQ(dev.close , dummy_close);
        } else {
            EXPECT_TRUE(dev.close==NULL);

        }
    }
}

/*
 * Check reset of structure (setting is checked above)
 */
TEST_F(DeviceTest, Device_reset){

    init_struct(DEV_ALL);

    device_free(&dev);

    EXPECT_TRUE(NULL==dev.open);
    EXPECT_TRUE(NULL==dev.read);
    EXPECT_TRUE(NULL==dev.write);
    EXPECT_TRUE(NULL==dev.ioctrl);
    EXPECT_TRUE(NULL==dev.close);

}

TEST_F(DeviceTest, Check_with_Null_Pointer) {
    dev_handle act, exp = EMLIB_ERROR;
    act = device_check(NULL, DEV_NONE);
    EXPECT_EQ(exp, act);
}
/*
 * Test if a device structure with the function set is only
 * accepted as correct in the device_check if related functions
 * exist.
 */
TEST_F(DeviceTest, Create_with_missing_function){
    uint8_t i,j;
    for ( i=0;i<(1<<5);i++) {
        init_struct(i);
        for ( j=0;j<(1<<5);j++) {
            dev_handle act;
            /* If more than needed function are defined - that's OK! */
            dev_handle exp = ((i&j)==j)?EMLIB_OK:EMLIB_ERROR;
            act = device_check(&dev,(dev_func_t)j);
            EXPECT_EQ(exp, act);
        }
   }
}
