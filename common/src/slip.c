/*
 * slip.c
 *
 *  Created on: 16.05.2020
 *      Author: badi
 */
#include <string.h>
#include "common.h"
#include "device.h"
#include "slip.h"

//#define HOST_DEBUG

#if defined(HOST_DEBUG)
#define DEB_DECODE(...) printf(__VA_ARGS__)
#define DEB_ENCODE(...) printf(__VA_ARGS__)
#define DEB_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEB_DECODE(...)
#define DEB_ENCODE(...)
#define DEB_PRINTF(...)
#endif

typedef enum {
    SLIP_ESC_SIMPLE_SET = 0,
    SLIP_ESC_EXTENDED_SET = 1
} slip_escape_set_t;

typedef enum {
    SLIP_ENCODE = 0,
    SLIP_DECODE = 1
} slip_codec_function_t;

typedef enum {
    SLIP_STATE_DECODE_WFF, // Wait for frame
    SLIP_STATE_ENCODE_STARTED,
    SLIP_STATE_NORMAL,
    SLIP_STATE_ESCAPE,
    SLIP_STATE_DECODE_END,
} slip_codec_state_t;

typedef struct slip_codec_t_
{
    device_t dev;
    slip_escape_set_t set;
    slip_codec_function_t function;
    slip_codec_state_t state;
    uint16_t size;
} slip_codec_t;

static slip_codec_t codec[SLIP_HANDLE_CNT];

static const slip_codec_t reset_codec={
    .set = 0,
    .function = 0,
    .state = 0,
    .size = 0,
};

const uint8_t slip_map[][2]={
    {SLIP_PKT_LIMIT, SLIP_PKT_LIMIT_ESC},
    {SLIP_ESCAPE, SLIP_ESCAPE_ESC},
    {SLIP_DC1, SLIP_DC1_ESC},
    {SLIP_DC3, SLIP_DC3_ESC},
};
static const uint8_t SLIP_SIMPLE_MAP_SIZE = 2;
static const uint8_t SLIP_MAP_SIZE = ELCNT(slip_map);

void slip_init(void)
{
    DEB_PRINTF("Erased %ld bytes %ld\n", sizeof(codec), sizeof(slip_codec_t));
    memset(codec, 0, sizeof(codec));
}


slip_handle_e slip_start(device_t *dev, slip_function_t state)
{
    slip_handle_e hdl = SLIP_HANDLE_ERROR;
    if ((NULL != dev) && (state<SLIP_STATE_CNT))
    {
        elres_t res = device_check(dev, DEV_WRITE);
        if (EMLIB_OK == res)
        {
            for (uint8_t index=0;index<SLIP_HANDLE_CNT;index++)
            {
                if (NULL == codec[index].dev.write)
                {
                    DEB_PRINTF("Return handle %d\n", index);
                    codec[index].dev = *dev;
                    codec[index].set = state>>1;
                    codec[index].function = state&0x01;
                    codec[index].size = 0;
                    if (SLIP_ENCODE == codec[index].function)
                    {
                        codec[index].state = SLIP_STATE_ENCODE_STARTED;
                    } else {
                        codec[index].state = SLIP_STATE_DECODE_WFF;
                    }
                    hdl = index;
                    break;
                }
            }
        }
    }
    return hdl;
}

elres_t slip_write(slip_handle_e hdl, const uint8_t * buffer, uint16_t length)
{
    elres_t res = EMLIB_ERROR;
    if ((hdl>=0) && (hdl<SLIP_HANDLE_CNT))
    {
        uint8_t value;
        uint8_t map_size = codec[hdl].set==SLIP_ESC_SIMPLE_SET?SLIP_SIMPLE_MAP_SIZE:SLIP_MAP_SIZE;
        if (codec[hdl].state == SLIP_STATE_ENCODE_STARTED)
        {
            value = SLIP_PKT_LIMIT;
            codec[hdl].size=0;
            device_write(&codec[hdl].dev, &value, 1);
            codec[hdl].state = SLIP_STATE_NORMAL;
            codec[hdl].size++;
        }
        res = EMLIB_OK;
        for (uint16_t idx = 0;idx < length && (res == EMLIB_OK); idx++)
        {
            uint8_t value = buffer[idx];
            //printf("Process value[%d] = 0x%02x\n", idx, value);
            if (codec[hdl].function == SLIP_ENCODE)
            {
                uint8_t mapIdx = 0;
                DEB_ENCODE("Write Value 0x%02x => ", value);
                for (mapIdx=0; mapIdx<map_size;mapIdx++)
                {
                    if (value == slip_map[mapIdx][0])
                    {
                        uint8_t esc = SLIP_ESCAPE;
                        device_write(&codec[hdl].dev, &esc, 1);
                        codec[hdl].size++;
                        value = slip_map[mapIdx][1];
                        DEB_ENCODE("ESC ");
                        break;
                    }
                }
                DEB_ENCODE("0x%02x\n", value);
                device_write(&codec[hdl].dev, &value, 1);
                codec[hdl].size++;
            } else {
                DEB_DECODE("Decode %d => ", value);
                if (SLIP_STATE_DECODE_WFF == codec[hdl].state)
                {
                    if (value == SLIP_PKT_LIMIT)
                    {
                        DEB_DECODE("Start of frame detected\n");
                        codec[hdl].state = SLIP_STATE_NORMAL;
                        codec[hdl].size = 0;
                        continue;
                    }
                }
                else if (SLIP_STATE_NORMAL == codec[hdl].state)
                {
                    if (SLIP_PKT_LIMIT == value)
                    {
                        if (0 == codec[hdl].size){
                            // silently discard byte
                            DEB_DECODE("Ignore 2nd Start of frame detected (Zero packet size)\n");
                            continue;
                        } else {
                            DEB_DECODE("EOP size = %d\n", codec[hdl].size);
                            //this is the end of a packet
                            codec[hdl].state = SLIP_STATE_DECODE_END;
                            continue;
                        }
                    }
                    if (SLIP_ESCAPE == value)
                    {
                        DEB_DECODE("DE ESC\n");
                        codec[hdl].state = SLIP_STATE_ESCAPE;
                        continue;
                    }
                }
                else if (SLIP_STATE_ESCAPE == codec[hdl].state)
                {
                    uint8_t mapIdx = 0;
                    for (mapIdx=0; mapIdx<map_size;mapIdx++)
                    {
                        if (value == slip_map[mapIdx][1])
                        {
                            DEB_DECODE("ESC 0x%02x -> ",value, slip_map[mapIdx][0]);
                            value = slip_map[mapIdx][0];
                            codec[hdl].state = SLIP_STATE_NORMAL;
                            break;
                        }
                    }
                }
                else
                {
                    // Ignore it eg in
                    DEB_DECODE("Ignore 0x%02x\n", value);
                    continue;
                }
                DEB_DECODE("0x%02x \n", value);
                device_write(&codec[hdl].dev, &value, 1);
                codec[hdl].size++;
            }
        }
    }
    return res;
}

uint16_t slip_end(slip_handle_e hdl)
{
    uint16_t res = 0;
    if ((NULL != codec[hdl].dev.write) && (hdl>=0) && (hdl<SLIP_HANDLE_CNT))
    {
        if (codec[hdl].function == SLIP_ENCODE)
        {
            uint8_t value = SLIP_PKT_LIMIT;
            codec[hdl].dev.write(codec[hdl].dev.user_data, &value, 1);
            codec[hdl].size++;
            DEB_ENCODE("Wrote terminating 0x%02x to output\n", value);
        }
        res = codec[hdl].size;
        codec[hdl] = reset_codec;
        device_reset(&codec[hdl].dev);
    }
    return res;
}



