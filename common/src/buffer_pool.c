/*
 * buffer_pool.c
 *
 *  Created on: Aug 1, 2025
 *      Author: badi
 */
#include "buffer_pool.h"
#include "buffer.h"
#include "common.h"
#include <assert.h>


buffer_pool_t *buffer_pool_new(uint8_t lcnt, uint8_t charCnt, bp_type_e type) {
    buffer_pool_t *bp = calloc(1, sizeof(buffer_pool_t));
    if (!bp) {
        printf("Can not allocate buffer_pool_t" NL);
        return NULL;
    }
    bp->type = type;
    if (bp->type == LINEAR){
        bp->next =-1;
    }
    bp->buffer_cnt = lcnt;
    bp->buffer = calloc(bp->buffer_cnt,  sizeof(buffer_t *));
    if (!bp->buffer) {
        free(bp);
        return NULL;
    }
    for (uint8_t i = 0; i < bp->buffer_cnt; i++) {
        bp->buffer[i] = buffer_new(charCnt);
        if (!bp->buffer[i]) {
             /* cleanup bei Fehler */
             for (uint8_t j = 0; j < i; j++){
                 buffer_free(bp->buffer[j]);
             }
             free(bp->buffer);
             free(bp);
             return NULL;
        } 
        bp->buffer[i]->id = i;
   }
    return bp;
}

void buffer_pool_free(buffer_pool_t *pool){
    //printf("buffer_pool_free(%p)\n", pool);
    
    if (!pool) return;

    for (uint8_t i = 0; i < pool->buffer_cnt; i++) {
//        printf("  free buffer[%d]=%p\n", i, pool->buffer[i]);
        buffer_free(pool->buffer[i]);
    }

    free(pool->buffer);
    free(pool);
}

buffer_t *buffer_pool_get(buffer_pool_t *bp) {
    if (bp == NULL) return NULL;
    buffer_t *buffer = NULL;
    for (uint8_t i = 0; i < bp->buffer_cnt; i++) {
        if (!buffer_is_used(bp->buffer[i])) {
            buffer = bp->buffer[i];
            buffer->id = i;
            buffer->state = BUFFER_USED;
            return buffer;
        }
    }
    return NULL;
}
em_msg buffer_pool_return(buffer_pool_t *bp, buffer_t *buffer) {
    em_msg res = EM_ERR;
    if (!bp)  {
        printf("bp  is NULL" NL);
        return res;
    }
    if (!buffer){
        printf("buffer is  NULL" NL);
        return res;
    }
    if (buffer->id > bp->buffer_cnt-1){
        printf("buffer is  larger allowed" NL);
        return EM_ERR;
    }

   /* Pointer prüfen (gehört der Buffer wirklich zum Pool?) */
   if (bp->buffer[buffer->id] != buffer) {
       printf("buffer id do not match"NL);
       printf("bp->buffer[buffer->id]: %p"NL);
       printf("buffer                : %p"NL);
       return EM_ERR;
   }
   if (buffer->state != BUFFER_USED){
       printf("Buffer is not used"NL);
       return EM_ERR;
   }
   return buffer_reset(buffer);
}

em_msg buffer_pool_print(buffer_pool_t *bp, char * title) {
    em_msg res = EM_ERR;
    if (title) printf("%s"NL , title);
    if (bp == NULL) {
        printf("Buffer pool is NULL" NL);
        return res;
    }
    //printf("Buffer pool %p with %d buffers and type %d:" NL, bp, bp->buffer_cnt, bp->type);
    for (uint8_t i = 0; i < bp->buffer_cnt; i++) {
        buffer_print(bp->buffer[i], "bp");
        return EM_OK;
    }
    res = EM_OK;
    return res;
}
