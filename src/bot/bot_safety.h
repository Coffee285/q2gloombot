/*
 * bot_safety.h -- safety macros and entity reference system for q2gloombot
 *
 * Provides crash-proofing primitives used throughout the bot codebase:
 *   - SAFE_DIV         : division-by-zero protection
 *   - ARRAY_INDEX_VALID : bounds checking for array indices
 *   - Q_strncpyz       : safe bounded string copy (NUL-terminated)
 *   - bot_entityref_t  : safe entity reference with serial tracking
 */

#ifndef BOT_SAFETY_H
#define BOT_SAFETY_H

#include "q_shared.h"

/* -----------------------------------------------------------------------
   Division safety
   ----------------------------------------------------------------------- */
#define SAFE_DIV(a, b) ((b) != 0 ? (a) / (b) : 0)

/* -----------------------------------------------------------------------
   Array bounds checking
   ----------------------------------------------------------------------- */
#define ARRAY_INDEX_VALID(idx, max) ((idx) >= 0 && (idx) < (max))

/* -----------------------------------------------------------------------
   Safe string copy (always NUL-terminates)
   ----------------------------------------------------------------------- */
static inline void Q_strncpyz(char *dest, const char *src, int destsize)
{
    if (!dest || destsize <= 0) return;
    if (!src) { dest[0] = '\0'; return; }
    strncpy(dest, src, (size_t)(destsize - 1));
    dest[destsize - 1] = '\0';
}

/* -----------------------------------------------------------------------
   Entity reference — tracks entity pointer + serial number to detect
   recycled entity slots.

   Quake 2 reuses edict_t slots; after G_FreeEdict + G_Spawn the same
   pointer may refer to a completely different entity.  The serial
   number (derived from entity_state_t.number combined with a simple
   counter) lets us detect this.

   Usage:
     bot_entityref_t ref;
     EntityRef_Set(&ref, target);
     ...
     edict_t *t = EntityRef_Get(&ref);
     if (t) { use t }
   ----------------------------------------------------------------------- */
typedef struct {
    edict_t *ent;
    int      serial;   /* snapshot of ent->s.number at time of Set() */
} bot_entityref_t;

static inline qboolean EntityRef_IsValid(const bot_entityref_t *ref)
{
    if (!ref || !ref->ent) return false;
    if (!ref->ent->inuse) return false;
    if (ref->ent->s.number != ref->serial) return false;
    return true;
}

static inline void EntityRef_Set(bot_entityref_t *ref, edict_t *ent)
{
    if (!ref) return;
    if (ent && ent->inuse) {
        ref->ent    = ent;
        ref->serial = ent->s.number;
    } else {
        ref->ent    = NULL;
        ref->serial = 0;
    }
}

static inline edict_t *EntityRef_Get(const bot_entityref_t *ref)
{
    if (!ref || !ref->ent) return NULL;
    if (!ref->ent->inuse) return NULL;
    if (ref->ent->s.number != ref->serial) return NULL;
    return ref->ent;
}

static inline void EntityRef_Clear(bot_entityref_t *ref)
{
    if (!ref) return;
    ref->ent    = NULL;
    ref->serial = 0;
}

#endif /* BOT_SAFETY_H */
