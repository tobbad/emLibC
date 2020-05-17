/*
 * slip.c
 *
 *  Created on: 16.05.2020
 *      Author: badi
 */
#include <string.h>
#include "common.h"
#include "slip.h"

//#define DEBUG

#if defined(DEBUG)
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
    slip_escape_set_t set;
    slip_codec_function_t function;
    slip_codec_state_t state;
    uint16_t size;
    // use this function for writting encoded or decoded data
    elres_t (*write)(uint8_t value);
} slip_codec_t;

static slip_codec_t codec[SLIP_HANDLE_CNT];

static const slip_codec_t reset_codec={
    .set = 0,
    .function = 0,
    .state = 0,
    .size = 0,
    .write = 0,
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


slip_handle_e slip_start(elres_t (*write)(uint8_t value), slip_function_t state)
{
    slip_handle_e hdl = SLIP_HANDLE_ERROR;
    if ((NULL != write) && (state<SLIP_STATE_CNT))
    {
        for (uint8_t index=0;index<SLIP_HANDLE_CNT;index++)
        {
            if (NULL == codec[index].write)
            {
                DEB_PRINTF("Return handle %d\n", index);
                codec[index].write = write;
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
    return hdl;
}

elres_t slip_write(slip_handle_e hdl, const uint8_t * buffer, uint16_t length)
{
    elres_t res = EMLIB_ERROR;
    if ((NULL != codec[hdl].write) && (hdl>=0) && (hdl<SLIP_HANDLE_CNT))
    {
        uint8_t map_size = codec[hdl].set==SLIP_ESC_SIMPLE_SET?SLIP_SIMPLE_MAP_SIZE:SLIP_MAP_SIZE;

        if (codec[hdl].state == SLIP_STATE_ENCODE_STARTED)
        {
            codec[hdl].write(SLIP_PKT_LIMIT);
            codec[hdl].state = SLIP_STATE_NORMAL;
        }
        res = EMLIB_OK;
        for (uint16_t idx = 0;idx < length && (res == EMLIB_OK); idx++)
        {
            uint8_t value = buffer[idx];
            //printf("Process value[%d] = 0x%02x\n", idx, value);
            if (codec[hdl].function == SLIP_ENCODE)
            {
                uint8_t mapIdx = 0;
                for (mapIdx=0; mapIdx<map_size;mapIdx++)
                {
                    DEB_ENCODE("Value %d == %d\n", value, slip_map[mapIdx][0]);
                    if (value == slip_map[mapIdx][0])
                    {
                        codec[hdl].write(SLIP_ESCAPE);
                        value = slip_map[mapIdx][1];
                        DEB_ENCODE("New value 0x%02x\n", value);
                        break;
                    }
                }
                DEB_ENCODE("Write Value %d\n", value);
                res = codec[hdl].write(value);
            } else {
                DEB_DECODE("Decode %d\n", value);
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
                            // Return decoded size
                            res = codec[hdl].size;
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
                            DEB_DECODE("DE map 0x%02x -> 0x%02x\n",value, slip_map[mapIdx][0]);
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
                res = codec[hdl].write(value);
                codec[hdl].size++;
            }
        }
    }
    return res;
}

slip_handle_e slip_end(slip_handle_e hdl)
{
    slip_handle_e res = hdl;
    if ((NULL != codec[hdl].write) && (hdl>=0) && (hdl<SLIP_HANDLE_CNT))
    {
        res = codec[hdl].write(SLIP_PKT_LIMIT);
        codec[hdl] = reset_codec;
        res = SLIP_HANDLE_ERROR;
    }
    return res;
}



