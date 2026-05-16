#pragma once

#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cwisstable.h"
typedef const char *icstr_t;

struct consed_cstr_s {
    size_t hash;
    size_t len_w_nul;
    icstr_t cstr;
};

typedef struct consed_cstr_s consed_cstr_t;

static inline void kConsedCstrPolicy_copy(void *dst, const void *src) {
    consed_cstr_t **dccp = (consed_cstr_t **)dst;
    consed_cstr_t **sccp = (consed_cstr_t **)src;
    const size_t src_hash      = (*sccp)->hash;
    const size_t src_len_w_nul = (*sccp)->len_w_nul;
    const char *src_cstr       = (*sccp)->cstr;

    consed_cstr_t *new_ccstr = malloc(sizeof(consed_cstr_t) + src_len_w_nul);
    if (!new_ccstr) {
        assert(!"malloc failure");
    }
    new_ccstr->hash      = src_hash;
    new_ccstr->len_w_nul = src_len_w_nul;
    char *new_cstr       = (char *)((uintptr_t)new_ccstr + sizeof(consed_cstr_t));
    memcpy(new_cstr, src_cstr, src_len_w_nul);
    new_ccstr->cstr = new_cstr;
    *dccp           = new_ccstr;
}

static inline void kConsedCstrPolicy_dtor(void *val) {
    consed_cstr_t *ccstr = *(consed_cstr_t **)val;
    free(ccstr);
}

static inline size_t kConsedCstrPolicy_hash(const void *val) {
    consed_cstr_t *ccstr = *(consed_cstr_t **)val;
    if (ccstr->hash != CWISS_AbslHash_kInit) {
        return ccstr->hash;
    }
    CWISS_FxHash_State state = CWISS_AbslHash_kInit;
    CWISS_FxHash_Write(&state, &ccstr->len_w_nul, sizeof(ccstr->len_w_nul));
    CWISS_FxHash_Write(&state, ccstr->cstr, ccstr->len_w_nul);
    CWISS_FxHash_Finish(&state);
    ccstr->hash = CWISS_FxHash_Finish(&state);;
    return ccstr->hash;
}

static inline bool kConsedCstrPolicy_eq(const void *a, const void *b) {
    consed_cstr_t **acc = (consed_cstr_t **)a;
    consed_cstr_t **bcc = (consed_cstr_t **)b;
    if (*acc == *bcc) {
        return true;
    }
    if ((*acc)->hash != (*bcc)->hash) {
        return false;
    }
    if ((*acc)->len_w_nul != (*bcc)->len_w_nul) {
        return false;
    }
    int memcmp_res = memcmp((*acc)->cstr, (*bcc)->cstr, (*acc)->len_w_nul);
    if (!memcmp_res) {
        return true;
    } else {
        return false;
    }
}

CWISS_DECLARE_NODE_SET_POLICY(kConsedCstrPolicy, consed_cstr_t *,
                              (obj_copy, kConsedCstrPolicy_copy),
                              (obj_dtor, kConsedCstrPolicy_dtor),
                              (key_hash, kConsedCstrPolicy_hash), (key_eq, kConsedCstrPolicy_eq));

CWISS_DECLARE_HASHSET_WITH(ConsedCstrSet, consed_cstr_t *, kConsedCstrPolicy);

static inline size_t ConsedCstrSet_cstr_hash(const char *self) {
    CWISS_FxHash_State state = CWISS_AbslHash_kInit;
    const size_t len_w_nul   = strlen(self) + 1;
    CWISS_FxHash_Write(&state, &len_w_nul, sizeof(len_w_nul));
    CWISS_FxHash_Write(&state, self, len_w_nul - 1);
    return CWISS_FxHash_Finish(&state);
}

static inline bool ConsedCstrSet_cstr_eq(const char *self, consed_cstr_t *const *that) {
    return !strcmp(self, (*that)->cstr);
}

CWISS_DECLARE_LOOKUP_NAMED(ConsedCstrSet, cstr, char);

static inline consed_cstr_t *make_consd_cstr(const char *cstr) {
    const size_t len      = strlen(cstr);
    consed_cstr_t *ccstrp = malloc(sizeof(consed_cstr_t) + len + 1);
    ccstrp->len_w_nul = len + 1;
    char *cstr_copy   = (char *)((uintptr_t)ccstrp + sizeof(consed_cstr_t));
    memcpy(cstr_copy, cstr, len + 1);
    ccstrp->cstr = cstr_copy;
    ccstrp->hash = CWISS_AbslHash_kInit;
    kConsedCstrPolicy_hash(&ccstrp);
    return ccstrp;
}

static inline icstr_t inter_string_to_set(ConsedCstrSet *set, const char *cstr) {
    const char *interned_cstr = NULL;
    consed_cstr_t *ccstr      = NULL;
    ConsedCstrSet_Insert ins  = ConsedCstrSet_deferred_insert_by_cstr(set, cstr);
    consed_cstr_t **ccstrp    = ConsedCstrSet_Iter_get(&ins.iter);
    if (ins.inserted) {
        ccstr                  = make_consd_cstr(cstr);
        const size_t len_w_nul = strlen(cstr) + 1;
        memcpy((char *)ccstr->cstr, cstr, len_w_nul);
        ccstr->hash = ConsedCstrSet_cstr_hash(ccstr->cstr);
        *ccstrp     = ccstr;
    } else {
        ccstr = *ccstrp;
    }
    return ccstr->cstr;
}

extern ConsedCstrSet global_string_interning_set;

__attribute__((constructor)) static void init_string_interning_set(void) {
    global_string_interning_set = ConsedCstrSet_new(0);
}

__attribute__((destructor)) static void deinit_string_interning_set(void) {
    ConsedCstrSet_destroy(&global_string_interning_set);
}

static inline icstr_t inter_string(const char *cstr) {
    return inter_string_to_set(&global_string_interning_set, cstr);
}

struct funcstat_s {
    // layout struct so hashed contents are contiguous
    const icstr_t name;
    const icstr_t filename;
    const icstr_t caller;
    const zlong flineno;
    const zlong lineno;
    const int tp;
    zlong num_execs;
    double ns;
};

typedef struct funcstat_s funcstat_t;
