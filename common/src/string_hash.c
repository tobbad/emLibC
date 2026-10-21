/*
 * string_hash.c
 *
 *  Created on: 14.10.2026
 *      Author: badi
 */
#include "string_hash.h"
#include "common.h"
#include <string.h>

#define PRIM_NUMBER 31

em_msg sh_init(sh_hash_map_t *mp) {
    if (mp == NULL) {
        return EM_ERR;
    }
    memset(mp, 0, sizeof(*mp));
    return EM_OK;
}

// Polynom-Hash nach Horner: h = h * 31 + key[i], am Ende mod SH_BUCKET_CNT.
// SH_BUCKET_CNT darf kein Vielfaches von PRIM_NUMBER sein, sonst fallen alle
// Zeichen ausser dem letzten aus dem Hash.
uint16_t sh_hash_function(const char *key) {
    uint32_t sum = 0;
    for (uint8_t i = 0; (i < SH_KEY_LEN) && (key[i] != '\0'); i++) {
        sum = sum * PRIM_NUMBER + (uint8_t)key[i];
    }
    return (uint16_t)(sum % SH_BUCKET_CNT);
}

static bool sh_key_valid(const char *key) { return (key != NULL) && (strnlen(key, SH_KEY_LEN) < SH_KEY_LEN); }

static sh_node_t *sh_find(const sh_hash_map_t *mp, const char *key) {
    sh_node_t *node = mp->bucket[sh_hash_function(key)];
    while (node != NULL) {
        if (strcmp(key, node->key) == 0) {
            return node;
        }
        node = node->next;
    }
    return NULL;
}

static sh_node_t *sh_get_node(sh_hash_map_t *mp) {
    for (uint16_t i = 0; i < SH_SIZE; i++) {
        if (!mp->node[i].used) {
            mp->node[i].used = true;
            return &mp->node[i];
        }
    }
    return NULL;
}

em_msg sh_insert(sh_hash_map_t *mp, const char *key, int8_t value) {
    if ((mp == NULL) || !sh_key_valid(key)) {
        return EM_ERR;
    }
    sh_node_t *node = sh_find(mp, key);
    if (node != NULL) {
        node->value = value;
        return EM_OK;
    }
    node = sh_get_node(mp);
    if (node == NULL) {
        return EM_ERR;
    }
    uint16_t bucketIndex = sh_hash_function(key);
    strncpy(node->key, key, SH_KEY_LEN);
    node->value = value;
    // Am Kopf der Liste einfuegen
    node->next = mp->bucket[bucketIndex];
    mp->bucket[bucketIndex] = node;
    mp->numOfElements++;
    return EM_OK;
}

em_msg sh_delete(sh_hash_map_t *mp, const char *key) {
    if ((mp == NULL) || !sh_key_valid(key)) {
        return EM_ERR;
    }
    sh_node_t **link = &mp->bucket[sh_hash_function(key)];
    while (*link != NULL) {
        sh_node_t *node = *link;
        if (strcmp(key, node->key) == 0) {
            *link = node->next;
            memset(node, 0, sizeof(*node));
            mp->numOfElements--;
            return EM_OK;
        }
        link = &node->next;
    }
    return EM_ERR;
}

em_msg sh_search(const sh_hash_map_t *mp, const char *key, int8_t *value) {
    if ((mp == NULL) || (value == NULL) || !sh_key_valid(key)) {
        return EM_ERR;
    }
    const sh_node_t *node = sh_find(mp, key);
    if (node == NULL) {
        return EM_ERR;
    }
    *value = node->value;
    return EM_OK;
}
