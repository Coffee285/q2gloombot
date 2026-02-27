/*
 * bot_build.h -- building AI public API for q2gloombot
 *
 * Provides the interface for Engineer (human) and Breeder (alien) bots
 * to manage construction, placement, and repair of structures.
 *
 * BUILD ORDER
 * -----------
 * Human Engineer: Teleporter -> 2 Turrets -> Ammo Depot -> Camera ->
 *                 forward Teleporter -> more Turrets
 *
 * Alien Breeder:  Egg -> Spiker -> Cocoon -> 2nd Egg -> Obstacle ->
 *                 forward Egg -> more Spikers
 *
 * Both orders adapt to game state:
 *   - Winning (pushing)  → build more forward spawn points
 *   - Losing (defending) → build more defenses near existing spawns
 */

#ifndef BOT_BUILD_H
#define BOT_BUILD_H

#include "bot.h"

/* -----------------------------------------------------------------------
   Structure health threshold for triggering a repair
   ----------------------------------------------------------------------- */
#define BOT_BUILD_REPAIR_THRESHOLD  0.50f  /* repair when below 50% HP     */

/* -----------------------------------------------------------------------
   Maximum number of tracked friendly structures per team
   ----------------------------------------------------------------------- */
#define BOT_BUILD_MAX_STRUCTS  64

/* -----------------------------------------------------------------------
   A lightweight record of a friendly structure the builder is aware of
   ----------------------------------------------------------------------- */
typedef struct {
    edict_t             *ent;          /* pointer to the structure edict    */
    gloom_struct_type_t  type;         /* what kind of structure it is      */
    vec3_t               origin;       /* cached world position             */
    int                  hp;           /* cached current health             */
    int                  max_hp;       /* cached maximum health             */
    qboolean             in_use;       /* slot is occupied                  */
} bot_struct_record_t;

/* -----------------------------------------------------------------------
   Builder memory — tracks all known friendly structures for a team
   ----------------------------------------------------------------------- */
typedef struct {
    bot_struct_record_t structs[BOT_BUILD_MAX_STRUCTS];
    int                 count;
    int                 spawn_count;   /* number of active spawn structs    */
} bot_build_memory_t;

/* -----------------------------------------------------------------------
   Public API
   ----------------------------------------------------------------------- */

/* Initialise the build subsystem (called from Bot_Init). */
void BotBuild_Init(void);

/* Per-frame update: refresh structure records for a given team. */
void BotBuild_UpdateStructures(int team);

/*
 * Choose the highest-priority structure to build next.
 * Updates bs->build.priority and bs->build.what_to_build.
 */
void BotBuild_ChooseNext(bot_state_t *bs);

/*
 * Find the nearest friendly structure that needs repair (below threshold).
 * Returns a pointer to the structure edict, or NULL if none needs repair.
 */
edict_t *BotBuild_FindRepairTarget(bot_state_t *bs);

/*
 * Delegate to bot_build_placement.c:
 * Find a good world position to place the given structure type.
 * Returns true and fills *out_origin on success.
 */
qboolean BotBuild_FindPlacement(bot_state_t *bs, gloom_struct_type_t type,
                                 vec3_t out_origin);

/* -----------------------------------------------------------------------
   Placement sub-module (bot_build_placement.c)
   ----------------------------------------------------------------------- */
qboolean BotBuildPlacement_ForSpawn(bot_state_t *bs, vec3_t out_origin);
qboolean BotBuildPlacement_ForDefense(bot_state_t *bs,
                                       gloom_struct_type_t type,
                                       vec3_t out_origin);
qboolean BotBuildPlacement_ForUtility(bot_state_t *bs,
                                       gloom_struct_type_t type,
                                       vec3_t out_origin);

#endif /* BOT_BUILD_H */
