/*
 * bot.h -- master bot header for q2gloombot
 *
 * Defines bot_state_t and all bot-related enumerations for the
 * asymmetric Gloom game mode (Space Marines vs. Alien Spiders).
 *
 * RESOURCE SYSTEMS
 * ----------------
 *  Humans  — earn "credits" via kills/objectives; spend at an Armory to
 *             purchase better weapons, armor, and battlesuits.
 *  Aliens  — earn "evos" (evolution points) via kills/damage; spend at
 *             the Overmind to evolve into higher-tier classes.
 *
 * WIN CONDITION
 * -------------
 *  Destroy the enemy's primary structure (Reactor / Overmind), then
 *  eliminate the remaining players who can no longer respawn.
 */

#ifndef BOT_H
#define BOT_H

#include "g_local.h"
#include "gloom_classes.h"
#include "gloom_structs.h"

/* -----------------------------------------------------------------------
   Limits
   ----------------------------------------------------------------------- */
#define MAX_BOTS         16    /* maximum concurrent bots                   */
#define BOT_THINK_RATE   0.1f  /* seconds between bot think calls           */

/* -----------------------------------------------------------------------
   Navigation constants
   ----------------------------------------------------------------------- */
#define BOT_MAX_PATH_NODES  256   /* maximum path length in nodes           */
#define BOT_INVALID_NODE    -1    /* sentinel for "no node"                 */

/* -----------------------------------------------------------------------
   Memory / awareness constants
   ----------------------------------------------------------------------- */
#define BOT_MAX_REMEMBERED_ENEMIES   8
#define BOT_MAX_REMEMBERED_TEAMMATES 8
#define BOT_ENEMY_MEMORY_TIME        10.0f /* seconds until enemy is forgotten */

/* -----------------------------------------------------------------------
   AI state machine states
   ----------------------------------------------------------------------- */
typedef enum {
    BOTSTATE_IDLE    = 0, /* no immediate task; wandering or waiting         */
    BOTSTATE_PATROL,      /* moving along a patrol route                     */
    BOTSTATE_HUNT,        /* moving toward last known enemy position         */
    BOTSTATE_COMBAT,      /* actively attacking a visible enemy              */
    BOTSTATE_FLEE,        /* retreating to safety or to heal                 */
    BOTSTATE_DEFEND,      /* holding a position or objective                 */
    BOTSTATE_ESCORT,      /* following and protecting a teammate             */
    BOTSTATE_BUILD,       /* constructing or repairing a base structure      */
    BOTSTATE_EVOLVE,      /* alien: waiting to spend evos at the Overmind    */
    BOTSTATE_UPGRADE,     /* human: traveling to Armory to spend credits     */

    BOTSTATE_MAX
} bot_ai_state_t;

/* -----------------------------------------------------------------------
   Weapon / combat state
   ----------------------------------------------------------------------- */
typedef enum {
    BOT_WEAPON_IDLE = 0,  /* not attacking                                  */
    BOT_WEAPON_ACQUIRE,   /* acquiring target, aligning aim                  */
    BOT_WEAPON_FIRING,    /* actively firing                                 */
    BOT_WEAPON_RELOADING, /* waiting for reload / cooldown                   */
    BOT_WEAPON_SWITCHING  /* switching to a better weapon                    */
} bot_weapon_state_t;

/* -----------------------------------------------------------------------
   Build state (engineer / granger bots)
   ----------------------------------------------------------------------- */
typedef struct {
    build_priority_t priority;         /* current highest build priority      */
    gloom_struct_type_t what_to_build; /* structure type currently planned    */
    vec3_t           build_origin;     /* world position chosen for the build */
    int              build_points;     /* resource points available to spend  */
    float            build_start;      /* game time when build began          */
    qboolean         building;         /* currently in the act of building    */
    qboolean         reactor_exists;   /* human: Reactor is alive (cached)    */
    qboolean         overmind_exists;  /* alien: Overmind is alive (cached)   */
    int              spawn_count;      /* number of Telenodes / Eggs alive    */
} bot_build_state_t;

/* -----------------------------------------------------------------------
   Navigation state
   Wall-walk flag is critical for alien pathfinding: alien bots can
   traverse walls and ceilings, requiring 3-D (Z-axis) nav awareness.
   ----------------------------------------------------------------------- */
typedef struct {
    int      current_node;                    /* nav node at/near current pos  */
    int      goal_node;                       /* final destination nav node    */
    int      path[BOT_MAX_PATH_NODES];        /* computed path (node indices)  */
    int      path_length;                     /* valid entries in path[]       */
    int      path_index;                      /* current position in path[]    */
    vec3_t   goal_origin;                     /* world position of goal        */
    float    arrived_dist;                    /* "arrived" threshold (units)   */
    qboolean path_valid;                      /* is the current path usable?   */
    qboolean wall_walking;                    /* alien: currently wall/ceiling */
    vec3_t   wall_normal;                     /* surface normal when wall-walk */
} bot_nav_state_t;

/* -----------------------------------------------------------------------
   Combat state
   Target priority follows the game's asymmetric design:
     Alien bots  → Reactor → Builders → Turrets → Marines
     Human bots  → Overmind → Grangers → Alien structures → Aliens
   ----------------------------------------------------------------------- */
typedef struct {
    edict_t           *target;            /* current enemy (NULL = none)     */
    target_priority_t  target_priority;   /* how this target was chosen      */
    vec3_t             target_last_pos;   /* last known enemy position       */
    float              target_last_seen;  /* game time of last sighting      */
    float              target_dist;       /* current distance to target      */
    qboolean           target_visible;    /* is target in line-of-sight?     */
    bot_weapon_state_t weapon_state;
    float              next_attack_time;  /* earliest time bot may fire      */
    float              aim_error;         /* aim offset (skill-dependent)    */
    float              engagement_range;  /* preferred combat range (class)  */
    qboolean           prefer_cover;      /* aliens: try to use cover/walls  */
    qboolean           max_range_engage;  /* humans: engage at max distance  */
} bot_combat_state_t;

/* -----------------------------------------------------------------------
   Memory: recently seen enemies
   ----------------------------------------------------------------------- */
typedef struct {
    edict_t *ent;
    vec3_t   last_pos;
    float    last_seen;
} bot_enemy_memory_t;

/* -----------------------------------------------------------------------
   Memory: known teammate locations
   ----------------------------------------------------------------------- */
typedef struct {
    edict_t *ent;
    vec3_t   last_pos;
    float    last_update;
} bot_team_memory_t;

/* -----------------------------------------------------------------------
   Master bot state structure
   ----------------------------------------------------------------------- */
typedef struct bot_state_s {
    /* Identity ---------------------------------------------------------- */
    edict_t      *ent;          /* the edict this bot controls               */
    int           bot_index;    /* index into g_bots[] array                 */
    char          name[32];     /* bot name, e.g. "Bot_Marine_01"            */

    /* Class & team ------------------------------------------------------ */
    gloom_class_t gloom_class;  /* one of the 16 Gloom classes               */
    int           team;         /* TEAM_HUMAN or TEAM_ALIEN                  */

    /* Skill ------------------------------------------------------------- */
    float         skill;        /* 0.0 (easy) to 1.0 (expert)               */

    /* Resources --------------------------------------------------------- */
    /*
     * credits : human currency; earned by kills/objectives.
     *           Spent at the Armory to unlock weapon/armour upgrades.
     * evos    : alien currency; earned by kills/damage.
     *           Spent at the Overmind to evolve into the next class tier.
     */
    int           credits;      /* human: accumulated credits                */
    int           evos;         /* alien: accumulated evolution points       */

    /* AI state machine -------------------------------------------------- */
    bot_ai_state_t ai_state;          /* current high-level behaviour         */
    bot_ai_state_t prev_ai_state;     /* state before last transition         */
    float          state_enter_time;  /* game time when ai_state was entered  */

    /* Navigation -------------------------------------------------------- */
    bot_nav_state_t nav;

    /* Combat ------------------------------------------------------------ */
    bot_combat_state_t combat;

    /* Build ------------------------------------------------------------- */
    bot_build_state_t  build;

    /* Timing ------------------------------------------------------------ */
    float  next_think_time;   /* game time of the next think call            */
    float  think_interval;    /* seconds between thinks (BOT_THINK_RATE)    */
    float  reaction_time;     /* aim/decision delay (seconds, skill-based)  */

    /* Progression ------------------------------------------------------- */
    int    frag_count;        /* frags this life                             */
    int    class_upgrades;    /* number of times upgraded (credits/evos)    */

    /* Memory ------------------------------------------------------------ */
    bot_enemy_memory_t  enemy_memory[BOT_MAX_REMEMBERED_ENEMIES];
    int                 enemy_memory_count;
    bot_team_memory_t   team_memory[BOT_MAX_REMEMBERED_TEAMMATES];
    int                 team_memory_count;

    vec3_t last_damage_origin;  /* origin of last damage received            */
    float  last_damage_time;    /* game time of last damage                  */

    /* Flags ------------------------------------------------------------- */
    qboolean in_use;        /* slot is occupied                              */
    qboolean initialized;   /* bot has completed spawn setup                 */
} bot_state_t;

/* -----------------------------------------------------------------------
   Global bot array (defined in bot_main.c)
   ----------------------------------------------------------------------- */
extern bot_state_t g_bots[MAX_BOTS];
extern int         num_bots;

/* -----------------------------------------------------------------------
   Public bot API (implemented in bot_main.c)
   ----------------------------------------------------------------------- */
void         Bot_Init(void);
void         Bot_Shutdown(void);
bot_state_t *Bot_Connect(edict_t *ent, int team, float skill);
void         Bot_Disconnect(edict_t *ent);
void         Bot_Frame(void);
void         Bot_Think(bot_state_t *bs);

/* Inline helper: return bot_state_t for an edict, or NULL */
static inline bot_state_t *Bot_GetState(const edict_t *ent)
{
    if (!ent || !ent->client) return NULL;
    return (bot_state_t *)ent->client->bot_state;
}

/* Inline helper: is this bot on the alien team? */
static inline qboolean Bot_IsAlien(const bot_state_t *bs)
{
    return bs && (bs->team == TEAM_ALIEN);
}

/* Inline helper: can this bot wall-walk right now? */
static inline qboolean Bot_CanWallWalk(const bot_state_t *bs)
{
    return bs && Gloom_ClassCanWallWalk(bs->gloom_class);
}

#endif /* BOT_H */
