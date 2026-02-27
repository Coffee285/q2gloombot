/*
 * gloom_structs.h -- Gloom structure types, build priorities, and target priorities
 *
 * Defines the buildable structure types for both teams, the priority
 * ordering used by builder bots, and the target-priority system used
 * by combat bots.
 */

#ifndef GLOOM_STRUCTS_H
#define GLOOM_STRUCTS_H

/* -----------------------------------------------------------------------
   Buildable structure types
   ----------------------------------------------------------------------- */
typedef enum {
    STRUCT_NONE = 0,

    /* ---- Human structures ---- */
    STRUCT_TELEPORTER,    /* human spawn point                              */
    STRUCT_TURRET_MG,     /* automated machinegun turret                    */
    STRUCT_TURRET_ROCKET, /* automated rocket turret                        */
    STRUCT_AMMO_DEPOT,    /* ammunition resupply station                    */
    STRUCT_CAMERA,        /* motion-sensor early warning                    */
    STRUCT_REACTOR,       /* human primary structure (power source)         */

    /* ---- Alien structures ---- */
    STRUCT_EGG,           /* alien spawn point                              */
    STRUCT_SPIKER,        /* auto-attack acid-spit turret                   */
    STRUCT_COCOON,        /* heals nearby aliens over time                  */
    STRUCT_OBSTACLE,      /* living wall — blocks corridors                 */
    STRUCT_ACID_TUBE,     /* legacy alias for STRUCT_SPIKER                 */
    STRUCT_OVERMIND,      /* alien primary structure                        */

    STRUCT_MAX
} gloom_struct_type_t;

/* -----------------------------------------------------------------------
   Build priorities
   Lower numeric value = higher urgency.
   ----------------------------------------------------------------------- */
typedef enum {
    BUILD_PRIORITY_CRITICAL = 0,  /* primary structure is gone — rebuild NOW */
    BUILD_PRIORITY_SPAWNS   = 1,  /* no active spawn points                  */
    BUILD_PRIORITY_DEFENSE  = 2,  /* insufficient defensive structures       */
    BUILD_PRIORITY_UTILITY  = 3,  /* utility structures (depot, cocoon, cam) */
    BUILD_PRIORITY_REPAIR   = 4,  /* repair damaged structures               */
    BUILD_PRIORITY_EXPAND   = 5,  /* forward base expansion                  */
    BUILD_PRIORITY_NONE     = 6   /* nothing to build right now              */
} build_priority_t;

/* -----------------------------------------------------------------------
   Target priorities for combat bots
   Lower numeric value = higher priority.
   ----------------------------------------------------------------------- */
typedef enum {
    TARGET_PRIORITY_PRIMARY_STRUCT = 0, /* Reactor / Overmind              */
    TARGET_PRIORITY_BUILDER        = 1, /* Engineer / Breeder              */
    TARGET_PRIORITY_SPAWN          = 2, /* Teleporter / Egg                */
    TARGET_PRIORITY_DEFENSE        = 3, /* Turret / Spiker                 */
    TARGET_PRIORITY_HIGH_VALUE     = 4, /* Mech / Stalker / Guardian       */
    TARGET_PRIORITY_NORMAL         = 5, /* standard combat class           */
    TARGET_PRIORITY_LOW            = 6, /* Hatchling / Grunt               */
    TARGET_PRIORITY_NONE           = 7  /* no valid target                 */
} target_priority_t;

/* -----------------------------------------------------------------------
   Helper functions — team primary and spawn structures
   ----------------------------------------------------------------------- */
static inline gloom_struct_type_t Gloom_PrimaryStruct(int team)
{
    return (team == 1 /*TEAM_HUMAN*/) ? STRUCT_REACTOR : STRUCT_OVERMIND;
}

static inline gloom_struct_type_t Gloom_SpawnStruct(int team)
{
    return (team == 1 /*TEAM_HUMAN*/) ? STRUCT_TELEPORTER : STRUCT_EGG;
}

#endif /* GLOOM_STRUCTS_H */
