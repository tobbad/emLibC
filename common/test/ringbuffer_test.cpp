
#include <string.h>
#include <stdint.h>
#include "ringbuffer.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using ::testing::Return;

#define RBUF_TEST_CNT 10
#define RBUF_TEST_SIZE 16

static uint8_t buffer[RBUF_TEST_CNT][RBUF_TEST_SIZE];

static rbuf_t rbuf[RBUF_TEST_CNT];

class RingbufferRegisterTest : public ::testing::Test {
    protected:

    void SetUp() override {
        rbuf_init();
        for (uint8_t i=0;i<RBUF_TEST_CNT;i++)
        {
            rbuf[i] = rbuf_clear;
            rbuf[i].buffer = buffer[i];
            rbuf[i].buffer_size = RBUF_TEST_SIZE;
        }
    }

    void TearDown() override
    {
    }
};

/* Specification:
 * # Ringbuffer is a limited count of bits.
 * 
 * ** Register 
 * # On NULL given as rbuf structure retiurn negative Handle
 * # Returns handle >=0 succeeds up to the count of available register places
 * # Return negative handle if null pointer is given in buffer
 * # Returns same handle if known memory is given (base of ringbuffer structure)
 * # Does not check if buffer memory is used twice 
 * 
 * ** Deregister
 * # Returns negative value on used handle
 * # Returns same handle value on unknown handle
 * # Same handle presented twice only first time is known
 * 
 * 
 * ** Size function
 * # Size function return buffer size after initialization
 * # Size function returns buffer size -1 on a single byte write
 * # .. check on any count of bytes written
 * 
 * ** Writting
 * # Writing bytes to a buffer only succeeds if there is enough space.
 * Derived:
 *  # Write of NULL buffer suceeds (bytes written is 0)
 *  # Write of zero bytes always succeeds (bytes written is 0)
 *  # Write buffer size of bytes succeeds on empty buffer  (bytes written is size of buffer)
 *  # Write buffer size + 1 bytes fails 
 *  # Write of blocks succeeds with different blocksizes up to buffer size
 * 
 * ** Reading
 * # Reading from the buffer only succeeds if receiving buffer is not NULL.
 * # Reading from the buffer only uses declared buffer size
 * Derived:
 *  # Check buffer size 0
 *  # Check buffer size 1
 *  # Check maximal buffer size - 1
 *  # Check maximal buffer size 
 *  # Check maximal buffer size + 1
 * # Sequence of read bytes is equal to sequence of written bytes
 * Derived:
 *  # Even if written in several blocks of different sizes
 * 
 * Test to succeed:
 */
TEST_F(RingbufferRegisterTest, RegisterRbufNullreturnsNegativeHandle)
{
    rbuf_hdl_t hdl = rbuf_register(NULL);
    
    EXPECT_GT(0, hdl);
}

TEST_F(RingbufferRegisterTest, RegisterRbufNotNullReturnsPositivOrZeroHandleUpToRegisterSize)
{
    uint8_t idx;
    
    for (idx=0;idx<RBUF_REGISTERS;idx++)
    {
        rbuf_hdl_t hdl = rbuf_register(&rbuf[idx]);

        EXPECT_LE(0, hdl);
    }
    rbuf_hdl_t hdl = rbuf_register(&rbuf[idx]);
    
    EXPECT_GT(0, hdl);
}

TEST_F(RingbufferRegisterTest, RegisterDifferentRbufReturnsDifferentHandle)
{
    rbuf_hdl_t hdl1 = rbuf_register(&rbuf[0]);
    rbuf_hdl_t hdl2 = rbuf_register(&rbuf[1]);

    EXPECT_NE(hdl1, hdl2);

}

TEST_F(RingbufferRegisterTest, RegisterSameRbufReturnsSameHandle)
{
    rbuf_hdl_t hdl1 = rbuf_register(&rbuf[0]);
    rbuf_hdl_t hdl2 = rbuf_register(&rbuf[0]);

    EXPECT_EQ(hdl1, hdl2);

}

TEST_F(RingbufferRegisterTest, RegisterRbufWithNoMemoryReturnsNegativeHandle)
{
    rbuf_t rbuf = rbuf_clear;
    rbuf_hdl_t hdl = rbuf_register(&rbuf);
    
    EXPECT_GT(0, hdl);
}


TEST_F(RingbufferRegisterTest, RegisterRbufWithBufferSizeZeroReturnsNegativeHandle)
{
    uint8_t buffer[15];
    rbuf_t rbuf = rbuf_clear;
    rbuf.buffer = buffer;
    rbuf_hdl_t hdl = rbuf_register(&rbuf);
    
    EXPECT_GT(0, hdl);
}

TEST_F(RingbufferRegisterTest, DeregisterReturnsNegativeHandleOnKnownHandleAndSameOnUnknown)
{
    rbuf_hdl_t hdl1 = rbuf_register(&rbuf[0]);
    rbuf_hdl_t hdl2 = rbuf_deregister(hdl1);
    rbuf_hdl_t hdl3 = rbuf_deregister(hdl1);

    EXPECT_GT(0, hdl2);
    EXPECT_EQ(hdl1, hdl3);
}

TEST_F(RingbufferRegisterTest, DeregisterReturnsGreaterEqualZeroHandleOnUnnownHandleWithinAllowedRange)
{
    rbuf_hdl_t hdl2 = rbuf_deregister(3);

    EXPECT_LT(0, hdl2);
}

TEST_F(RingbufferRegisterTest, DeregisterReturnsGreaterEqualZeroHandleOnUnnownHandle)
{
    rbuf_hdl_t hdl2 = rbuf_deregister(42);

    EXPECT_LT(0, hdl2);
}


class RingbufferReadWriteSizeTest : public ::testing::Test {
    protected:
    
    static const rbuf_hdl_t hdl = RBUF_REGISTERS-1;

        
    void SetUp() override {
        rbuf_init();
        for (uint8_t i=0;i<RBUF_REGISTERS;i++)
        {
            rbuf[i] = rbuf_clear;
            rbuf[i].buffer = buffer[i];
            rbuf[i].buffer_size = RBUF_TEST_SIZE;
            if (-1 == rbuf_register(&rbuf[i]))
            {
                break;
            }
        }
    }

    void TearDown() override
    {
    }
};


TEST_F(RingbufferReadWriteSizeTest, EmptyBufferHasMaximalSize)
{
    uint16_t size = rbuf_size(hdl);
    
    EXPECT_EQ(RBUF_TEST_SIZE, size);
}

TEST_F(RingbufferReadWriteSizeTest, MaximalSizeOfBytesCanBeWrittenAndSizeDecrements)
{
    uint16_t size = rbuf_size(hdl);
    uint8_t i;
    
    EXPECT_EQ(RBUF_TEST_SIZE, size);
    for (i = 0;i<size;i++)
    {
        elres_t res = rbuf_write_byte(hdl, i);
        uint16_t free_expected = RBUF_TEST_SIZE-i-1;
        uint16_t free_obtained = rbuf_size(hdl);
        
        EXPECT_EQ(EMLIB_OK, res);
        EXPECT_EQ(free_expected, free_obtained) << "Pushed " << int(i+1) << " items";
    }
    
    elres_t res = rbuf_write_byte(hdl, i);
    EXPECT_EQ(EMLIB_ERROR, res);
}
TEST_F(RingbufferReadWriteSizeTest, ReadWithNullPointerReturnsError)
{
    elres_t res = rbuf_read_byte(hdl, NULL);
    EXPECT_EQ(EMLIB_ERROR, res);
}

TEST_F(RingbufferReadWriteSizeTest, ReadOnEmptyBufferDoesNotChangeValue)
{
    uint8_t value_obt = 42;
    uint8_t value_exp = value_obt;
    elres_t res = rbuf_read_byte(hdl, &value_obt);
    EXPECT_EQ(EMLIB_ERROR, res);
    EXPECT_EQ(value_exp, value_obt);
}

TEST_F(RingbufferReadWriteSizeTest, WriteValueReadValueNoError)
{
    uint8_t value_obt = 0;
    uint8_t value_exp = 42;
    elres_t res1 = rbuf_write_byte(hdl, value_exp);
    elres_t res2 = rbuf_read_byte(hdl, &value_obt);
    EXPECT_EQ(EMLIB_OK, res1);
    EXPECT_EQ(EMLIB_OK, res2);
    EXPECT_EQ(value_exp, value_obt);
}

TEST_F(RingbufferReadWriteSizeTest, WriteTwoValuesReadValuesNoError)
{
    uint8_t value1_obt = 0;
    uint8_t value2_obt = 0;
    uint8_t value1_exp = 42;
    uint8_t value2_exp = 203;
    elres_t res = rbuf_write_byte(hdl, value1_exp);
    EXPECT_EQ(EMLIB_OK, res);
    res = rbuf_write_byte(hdl, value2_exp);
    EXPECT_EQ(EMLIB_OK, res);
    res = rbuf_read_byte(hdl, &value1_obt);
    EXPECT_EQ(EMLIB_OK, res);
    res = rbuf_read_byte(hdl, &value2_obt);
    EXPECT_EQ(EMLIB_OK, res);
    EXPECT_EQ(value1_exp, value1_obt);
    EXPECT_EQ(value2_exp, value2_obt);
}


TEST_F(RingbufferReadWriteSizeTest, WriteDecrementsSizeReadIncrementsBeyondBoundary)
{
    uint16_t offset = 15;
    uint16_t size = rbuf_size(hdl);
    uint8_t cnt_written;
    uint8_t free_expected = size;


    ASSERT_EQ(RBUF_TEST_SIZE, size);
    for (offset = 1;offset<RBUF_TEST_SIZE;offset++)
    {
        SetUp();
        free_expected = size;
        for (cnt_written = 1;cnt_written<size*2+3;cnt_written++)
        {
            elres_t res = rbuf_write_byte(hdl, cnt_written);
            free_expected--;
            uint16_t free_obtained = rbuf_size(hdl);

            ASSERT_EQ(EMLIB_OK, res);
            ASSERT_EQ(free_expected, free_obtained) << "Pushed " << int(cnt_written) << " items";
            /*
             * Do the read if difference is > offset
             */
            if (cnt_written>offset)
            {
                uint8_t value_obtained;
                uint8_t value_expected = cnt_written-offset;
                elres_t res = rbuf_read_byte(hdl, &value_obtained);
                free_expected++;
                uint16_t free_obtained = rbuf_size(hdl);

                ASSERT_EQ(EMLIB_OK, res);
                ASSERT_EQ(free_expected, free_obtained) << "RD: "<< rbuf[hdl].nxtRdIdx << " WR: " << rbuf[hdl].nxtWrIdx;
                ASSERT_EQ(value_expected, value_obtained) << "Read after pushed " << int(cnt_written+1) << " values";


            }
        }
    }
}

TEST_F(RingbufferReadWriteSizeTest, WriteOnFullFailsAndDoesNotDestroiesValuse)
{
    uint16_t size = rbuf_size(hdl);
    uint8_t value_obt;

    for (uint8_t i=0;i<size;i++)
    {
        elres_t res = rbuf_write_byte(hdl, i);
        ASSERT_EQ(EMLIB_OK, res);
    }
    elres_t res = rbuf_write_byte(hdl, 56);
    for (uint8_t i=0;i<size;i++)
    {
        elres_t res = rbuf_read_byte(hdl, &value_obt);
        ASSERT_EQ(EMLIB_OK, res);
        ASSERT_EQ(i, value_obt);
    }
}

