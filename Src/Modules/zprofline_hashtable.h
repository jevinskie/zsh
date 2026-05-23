#pragma once

#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cwisstable.h"
typedef const char *icstr_t;

struct consed_cstr_s {
    size_t hash;
    size_t sz;
};

typedef struct consed_cstr_s consed_buf_t;

static inline const char *consed_buf_get_buf(const consed_buf_t *consed_str) {
    return (const char *)consed_str + sizeof(*consed_str);
}

static inline size_t consed_buf_get_buf_sz(const consed_buf_t *consed_str) {
    return consed_str->sz;
}

static inline size_t consed_cstr_get_sz(const consed_buf_t *consed_str) {
    return  consed_buf_get_buf_sz(consed_str) + sizeof(*consed_str);
}

static inline void kConsedBufPolicy_copy(void *dst, const void *src) {
    consed_buf_t **dccp = (consed_buf_t **)dst;
    const consed_buf_t **sccp = (consed_buf_t **)src;
    const size_t src_hash      = (*sccp)->hash;
    const size_t src_sz = (*sccp)->sz;
    const char *src_cstr       = consed_cstr_get(*sccp);

    consed_buf_t *new_ccstr = malloc(sizeof(consed_buf_t) + src_sz);
    if (!new_ccstr) {
        assert(!"malloc failed in kConsedBufPolicy_copy");
        abort();
    }
    new_ccstr->hash      = src_hash;
    new_ccstr->sz = src_sz;
    char *new_cstr       = (char *)new_ccstr + sizeof(consed_buf_t);
    memcpy(new_cstr, src_cstr, src_sz);
    *dccp           = new_ccstr;
}

static inline void kConsedBufPolicy_dtor(void *val) {
    consed_buf_t *ccstr = *(consed_buf_t **)val;
    free(ccstr);
}

static inline size_t kConsedBufPolicy_hash(const void *val) {
    consed_buf_t *ccstr = *(consed_buf_t **)val;
    if (ccstr->hash != CWISS_FxHash_kInit) {
        return ccstr->hash;
    }
    CWISS_FxHash_State state = CWISS_FxHash_kInit;
    CWISS_FxHash_Write(&state, &ccstr->sz, sizeof(ccstr->sz));
    CWISS_FxHash_Write(&state, consed_buf_get_buf(ccstr), ccstr->sz);
    ccstr->hash = CWISS_FxHash_Finish(&state);;
    return ccstr->hash;
}

static inline bool kConsedBufPolicy_eq(const void *a, const void *b) {
    consed_buf_t *acc = *(consed_buf_t **)a;
    consed_buf_t *bcc = *(consed_buf_t **)b;
    if (acc->hash != bcc->hash || acc->sz != bcc->sz) {
        return false;
    }
    return !!memcmp(consed_buf_get_buf(acc), consed_buf_get_buf(bcc), acc->sz);
}

CWISS_DECLARE_NODE_SET_POLICY(kConsedBufPolicy, consed_buf_t *,
                              (obj_copy, kConsedBufPolicy_copy),
                              (obj_dtor, kConsedBufPolicy_dtor),
                              (key_hash, kConsedBufPolicy_hash), (key_eq, kConsedBufPolicy_eq));

CWISS_DECLARE_HASHSET_WITH(ConsedBufSet, consed_buf_t *, kConsedBufPolicy);

static inline size_t ConsedBufSet_cstr_hash(const char *self) {
    CWISS_FxHash_State state = CWISS_FxHash_kInit;
    const size_t sz   = strlen(self) + 1;
    CWISS_FxHash_Write(&state, &sz, sizeof(sz));
    CWISS_FxHash_Write(&state, self, sz);
    return CWISS_FxHash_Finish(&state);
}

static inline bool ConsedBufSet_cstr_eq(const char *self, consed_buf_t *const *that) {
    return !strcmp(self, consed_buf_get_buf(*that));
}

CWISS_DECLARE_LOOKUP_NAMED(ConsedBufSet, cstr, char);

static inline consed_buf_t *make_consd_cstr(const char *cstr) {
    const size_t sz      = strlen(cstr) + 1;
    consed_buf_t *cbufp = malloc(sizeof(consed_buf_t) + sz);
    cbufp->sz = sz;
    char *cstr_copy   = (char *)cbufp + sizeof(consed_buf_t);
    memcpy(cstr_copy, cstr, sz);
    cbufp->hash = CWISS_FxHash_kInit;
    kConsedBufPolicy_hash(&cbufp);
    return cbufp;
}

static inline icstr_t inter_string_to_set(ConsedBufSet *set, const char *cstr) {
    const char *interned_cstr = NULL;
    consed_buf_t *cbufp      = NULL;
    ConsedBufSet_Insert ins  = ConsedBufSet_deferred_insert_by_cstr(set, cstr);
    consed_buf_t **cbufpp    = ConsedBufSet_Iter_get(&ins.iter);
    if (ins.inserted) {
        cbufp                  = make_consd_cstr(cstr);
        const size_t sz = strlen(cstr) + 1;
        memcpy(consed_buf_get_buf(cbufp), cstr, sz);
        cbufp->hash = CWISS_FxHash_kInit;
        kConsedBufPolicy_hash(&cbufp);
        *cbufpp     = cbufp;
    } else {
        cbufp = *cbufpp;
    }
    return consed_buf_get_buf(cbufp);
}

extern ConsedBufSet global_string_interning_set;

__attribute__((constructor)) static void init_string_interning_set(void) {
    global_string_interning_set = ConsedBufSet_new(0);
}

__attribute__((destructor)) static void deinit_string_interning_set(void) {
    ConsedBufSet_destroy(&global_string_interning_set);
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
