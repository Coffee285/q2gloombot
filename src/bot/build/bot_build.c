/*
 * bot_build.c -- building AI core logic for q2gloombot
 *
 * Implements structure tracking, build-order selection, and repair logic
 * for both the Engineer (human) and Breeder (alien) builder classes.
 *
 * BUILD ORDER REFERENCE
 * ----------------------
 * Human Engineer:
 *   1. Teleporter (spawn)
 *   2. 2× Turret_MG (defense)
 *   3. Ammo Depot (utility)
 *   4. Camera (utility)
 *   5. Forward Teleporter (expand)
 *   6. More Turrets (defense)
 *
 * Alien Breeder:
 *   1. Egg (spawn)
 *   2. Spiker (defense)
 *   3. Cocoon (utility / heal)
 *   4. 2nd Egg (spawn)
 *   5. Obstacle (defense / block)
 *   6. Forward Egg (expand)
 *   7. More Spikers (defense)
 */

#include "bot_build.h"

/* -----------------------------------------------------------------------
   Module state — one build memory block per team
   ----------------------------------------------------------------------- */
static bot_build_memory_t s_build_mem[3]; /* indices: TEAM_HUMAN, TEAM_ALIEN */

/* -----------------------------------------------------------------------
   Human build order table
   Entries are tried in order; the first unsatisfied one is chosen.
   ----------------------------------------------------------------------- */
typedef struct {
    gloom_struct_type_t type;
    int                 min_count; /* build until we have this many         */
    build_priority_t    priority;
} build_order_entry_t;

static const build_order_entry_t s_human_build_order[] = {
    { STRUCT_TELEPORTER,    1, BUILD_PRIORITY_SPAWNS  },
    { STRUCT_TURRET_MG,     2, BUILD_PRIORITY_DEFENSE },
    { STRUCT_AMMO_DEPOT,    1, BUILD_PRIORITY_UTILITY },
    { STRUCT_CAMERA,        1, BUILD_PRIORITY_UTILITY },
    { STRUCT_TELEPORTER,    2, BUILD_PRIORITY_EXPAND  },
    { STRUCT_TURRET_MG,     4, BUILD_PRIORITY_DEFENSE },
};

static const build_order_entry_t s_alien_build_order[] = {
    { STRUCT_EGG,           1, BUILD_PRIORITY_SPAWNS  },
    { STRUCT_SPIKER,        1, BUILD_PRIORITY_DEFENSE },
    { STRUCT_COCOON,        1, BUILD_PRIORITY_UTILITY },
    { STRUCT_EGG,           2, BUILD_PRIORITY_SPAWNS  },
    { STRUCT_OBSTACLE,      1, BUILD_PRIORITY_DEFENSE },
    { STRUCT_EGG,           3, BUILD_PRIORITY_EXPAND  },
    { STRUCT_SPIKER,        3, BUILD_PRIORITY_DEFENSE },
};

/* -----------------------------------------------------------------------
   BotBuild_Init
   Called once during Bot_Init().
   ----------------------------------------------------------------------- */
void BotBuild_Init(void)
{
    int i;
    for (i = 0; i < 3; i++) {
        memset(&s_build_mem[i], 0, sizeof(bot_build_memory_t));
    }
}

/* -----------------------------------------------------------------------
   BotBuild_UpdateStructures
   Scan all in-use edicts for structures belonging to the given team and
   refresh the build memory.  Called every few seconds from Bot_Frame.
   ----------------------------------------------------------------------- */
void BotBuild_UpdateStructures(int team)
{
    int i;
    bot_build_memory_t *mem;

    if (team < 1 || team > 2) return;
    mem = &s_build_mem[team];

    /* Reset slot usage flags */
    for (i = 0; i < BOT_BUILD_MAX_STRUCTS; i++)
        mem->structs[i].in_use = false;
    mem->count       = 0;
    mem->spawn_count = 0;

    /*
     * In a real implementation this would iterate over all world entities,
     * identify those tagged as Gloom structures belonging to `team`, and
     * populate mem->structs[].  That requires game-specific entity tags
     * not present in this framework stub, so we leave the loop body as a
     * documented extension point.
     */
    (void)i; /* suppress unused-variable warning when loop body is empty */
}

/* -----------------------------------------------------------------------
   Helper: count how many structures of a given type the team has.
   ----------------------------------------------------------------------- */
static int CountStructType(int team, gloom_struct_type_t type)
{
    int i, count = 0;
    bot_build_memory_t *mem;

    if (team < 1 || team > 2) return 0;
    mem = &s_build_mem[team];

    for (i = 0; i < BOT_BUILD_MAX_STRUCTS; i++) {
        if (mem->structs[i].in_use && mem->structs[i].type == type)
            count++;
    }
    return count;
}

/* -----------------------------------------------------------------------
   BotBuild_ChooseNext
   Walk the build order table and choose the first unsatisfied entry.
   Updates bs->build.priority and bs->build.what_to_build.
   ----------------------------------------------------------------------- */
void BotBuild_ChooseNext(bot_state_t *bs)
{
    const build_order_entry_t *order;
    int                        order_len;
    int                        i;

    if (bs->team == TEAM_HUMAN) {
        order     = s_human_build_order;
        order_len = (int)(sizeof(s_human_build_order) /
                          sizeof(s_human_build_order[0]));
    } else {
        order     = s_alien_build_order;
        order_len = (int)(sizeof(s_alien_build_order) /
                          sizeof(s_alien_build_order[0]));
    }

    for (i = 0; i < order_len; i++) {
        if (CountStructType(bs->team, order[i].type) < order[i].min_count) {
            bs->build.priority      = order[i].priority;
            bs->build.what_to_build = order[i].type;
            return;
        }
    }

    /* All build order requirements met — check for repair needs */
    if (BotBuild_FindRepairTarget(bs) != NULL) {
        bs->build.priority      = BUILD_PRIORITY_REPAIR;
        bs->build.what_to_build = STRUCT_NONE;
        return;
    }

    bs->build.priority      = BUILD_PRIORITY_NONE;
    bs->build.what_to_build = STRUCT_NONE;
}

/* -----------------------------------------------------------------------
   BotBuild_FindRepairTarget
   Find the most-damaged friendly structure below the repair threshold.
   Returns the structure edict, or NULL if none need repair.
   ----------------------------------------------------------------------- */
edict_t *BotBuild_FindRepairTarget(bot_state_t *bs)
{
    int                 i;
    bot_build_memory_t *mem;
    edict_t            *best      = NULL;
    float               best_pct  = BOT_BUILD_REPAIR_THRESHOLD;

    if (bs->team < 1 || bs->team > 2) return NULL;
    mem = &s_build_mem[bs->team];

    for (i = 0; i < BOT_BUILD_MAX_STRUCTS; i++) {
        float pct;

        if (!mem->structs[i].in_use) continue;
        if (!mem->structs[i].ent || !mem->structs[i].ent->inuse) continue;
        if (mem->structs[i].max_hp <= 0) continue;

        pct = (float)mem->structs[i].hp / (float)mem->structs[i].max_hp;
        if (pct < best_pct) {
            best_pct = pct;
            best     = mem->structs[i].ent;
        }
    }

    return best;
}

/* -----------------------------------------------------------------------
   BotBuild_FindPlacement
   Delegate to the appropriate placement function based on structure type.
   ----------------------------------------------------------------------- */
qboolean BotBuild_FindPlacement(bot_state_t *bs, gloom_struct_type_t type,
                                 vec3_t out_origin)
{
    switch (type) {
    case STRUCT_TELEPORTER:
    case STRUCT_EGG:
        return BotBuildPlacement_ForSpawn(bs, out_origin);

    case STRUCT_TURRET_MG:
    case STRUCT_TURRET_ROCKET:
    case STRUCT_SPIKER:
    case STRUCT_OBSTACLE:
        return BotBuildPlacement_ForDefense(bs, type, out_origin);

    case STRUCT_AMMO_DEPOT:
    case STRUCT_CAMERA:
    case STRUCT_COCOON:
        return BotBuildPlacement_ForUtility(bs, type, out_origin);

    default:
        return false;
    }
}
