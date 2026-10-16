/*
 * string_hash.h
 *
 *  Created on: 14.10.2026
 *      Author: badi
 *
 * Statische Hash-Map  char[TIME_MEAS_CHAR_PER_LINE + 1] -> int8_t.
 * Der Schluessel entspricht time_meas_t.line aus _time.c.
 * Kein malloc: alle Knoten liegen im Pool innerhalb von sh_hash_map_t.
 * Kollisionen werden ueber verkettete Listen (separate chaining) geloest.
 */

#ifndef EMLIBC_COMMON_INC_STRING_HASH_H_
#define EMLIBC_COMMON_INC_STRING_HASH_H_
#include "_time.h"
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// Maximale Anzahl gespeicherter Eintraege
#ifndef SH_SIZE
#define SH_SIZE 100
#endif
// Anzahl Buckets (moeglichst eine Primzahl, aber nicht 31 = Hash-Multiplikator)
#ifndef SH_BUCKET_CNT
#define SH_BUCKET_CNT 37
#endif
// Schluessel inkl. abschliessendem '\0' (wie time_meas_t.line)
#define SH_KEY_LEN (TIME_MEAS_CHAR_PER_LINE + 1)

typedef struct sh_node_s {
    char key[SH_KEY_LEN];
    int8_t value;
    bool used;
    struct sh_node_s *next;
} sh_node_t;

typedef struct sh_hash_map_s {
    int16_t numOfElements;
    sh_node_t *bucket[SH_BUCKET_CNT]; // Kopf der Liste je Bucket
    sh_node_t node[SH_SIZE];          // Knoten-Pool
} sh_hash_map_t;

em_msg sh_init(sh_hash_map_t *mp);
uint16_t sh_hash_function(const char *key);
// Fuegt key ein oder ueberschreibt den Wert eines vorhandenen key.
// EM_ERR: key zu lang (>= SH_KEY_LEN) oder Pool voll.
em_msg sh_insert(sh_hash_map_t *mp, const char *key, int8_t value);
// EM_ERR, wenn key nicht vorhanden ist.
em_msg sh_delete(sh_hash_map_t *mp, const char *key);
// Schreibt den Wert nach *value. EM_ERR, wenn key nicht vorhanden ist.
em_msg sh_search(const sh_hash_map_t *mp, const char *key, int8_t *value);

#ifdef __cplusplus
}
#endif

#endif /* EMLIBC_COMMON_INC_STRING_HASH_H_ */
