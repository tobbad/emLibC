

#include <iostream>
#include <cstdint>
#include "gtest/gtest.h"
#include "device.h"


dev_handle_t dummy_open(dev_handle_t dev, void *){ return 0; };
em_msg dummy_read(dev_handle_t hdl, uint8_t *buffer, uint16_t *cnt){ return EM_ERR;};
em_msg dummy_write(dev_handle_t hdl, const uint8_t *buffer, uint16_t cnt){ return EM_ERR;};
em_msg dummy_ioctrl(dev_handle_t hdl, dev_command_t cmd, uint16_t value){ return EM_ERR;};
em_msg dummy_close(dev_handle_t hdl){ return EM_ERR;};
device_t dev = {
	.open     = &dummy_open,
	.read     = &dummy_read,
	.write    = &dummy_write,
	.ioctrl   = &dummy_ioctrl,
	.close    = &dummy_close,
	.ready_cb = NULL,
	.dev_type=0
};


class DeviceTest : public ::testing::Test {

    protected:


    void SetUp()
    {


    }

    void TearDown()
    {

    }

    void init_struct(uint8_t select){
        uint8_t idx = 0;
        if (select & (1<<idx)) {
            dev.open = &dummy_open;
        } else {
            dev.open = NULL;
        }
        idx++;
        if (select & (1<<idx)) {
            dev.read = &dummy_read;
        } else {
            dev.read = NULL;
        }
        idx++;
        if (select & (1<<idx)) {
            dev.write = &dummy_write;
        } else {
            dev.write = NULL;
        }
        idx++;
        if (select & (1<<idx)) {
            dev.ioctrl = &dummy_ioctrl;
        } else {
            dev.ioctrl = NULL;
        }
        idx++;
        if (select & (1<<idx)) {
            dev.close = &dummy_close;
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
        if (i & DEV_LREAD ){
            EXPECT_EQ(dev.write , dummy_write);
        } else {
            EXPECT_TRUE(dev.write==NULL);

        }
        if (i & DEV_WRITE ){
            EXPECT_EQ(dev.write , dummy_write);
        } else {
            EXPECT_TRUE(dev.write==NULL);

        }
        if (i & DEV_LWRITE ){
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

	dev_handle_t hdl = device_init(&dev, NULL);
    device_reset(hdl);
    EXPECT_TRUE(NULL==dev.open);
    EXPECT_TRUE(NULL==dev.read);
    EXPECT_TRUE(NULL==dev.write);
    EXPECT_TRUE(NULL==dev.ioctrl);
    EXPECT_TRUE(NULL==dev.close);

}
