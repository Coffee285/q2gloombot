/*
 * bot.h -- master bot header for q2gloombot
 *
 * Defines bot_state_t and all bot-related enumerations.
 */

#ifndef BOT_H
#define BOT_H

#include "g_local.h"
#include "gloom_classes.h"

/* -----------------------------------------------------------------------
   Limits
   ----------------------------------------------------------------------- */
#define MAX_BOTS         16   /* maximum concurrent bots                   */
#define BOT_THINK_RATE   0.1f /* seconds between bot think calls           */

/* -----------------------------------------------------------------------
   Navigation constants
   ----------------------------------------------------------------------- */
#define BOT_MAX_PATH_NODES 256 /* maximum path length in nodes              */
#define BOT_INVALID_NODE   -1  /* sentinel for "no node"                    */

/* -----------------------------------------------------------------------
   Memory / awareness constants
   ----------------------------------------------------------------------- */
#define BOT_MAX_REMEMBERED_ENEMIES  8  /* slots in recent-enemy memory     */
#define BOT_MAX_REMEMBERED_TEAMMATES 8 /* slots in teammate memory         */
#define BOT_ENEMY_MEMORY_TIME       10.0f /* seconds until enemy is forgotten */

/* -----------------------------------------------------------------------
   AI state machine states
   ----------------------------------------------------------------------- */
typedef enum {
    BOTSTATE_IDLE    = 0, /* no immediate task; wandering or waiting         */
    BOTSTATE_PATROL,      /* moving along a patrol route                     */
    BOTSTATE_HUNT,        /* searching for an enemy whose last pos is known  */
    BOTSTATE_COMBAT,      /* actively attacking a visible enemy              */
    BOTSTATE_FLEE,        /* retreating to safety / heal                     */
    BOTSTATE_DEFEND,      /* holding a position / objective                  */
    BOTSTATE_ESCORT,      /* following and protecting a teammate             */
    BOTSTATE_BUILD,       /* constructing a structure (Engineer / Breeder)   */

    BOTSTATE_MAX
} bot_ai_state_t;

/* -----------------------------------------------------------------------
   Weapon / combat state
   ----------------------------------------------------------------------- */
typedef enum {
    BOT_WEAPON_IDLE = 0,  /* weapon holstered / not firing                  */
    BOT_WEAPON_ACQUIRE,   /* acquiring a target, aligning aim                */
    BOT_WEAPON_FIRING,    /* actively firing                                 */
    BOT_WEAPON_RELOADING, /* waiting for reload / cooldown                   */
    BOT_WEAPON_SWITCHING  /* switching to a better weapon                    */
} bot_weapon_state_t;

/* -----------------------------------------------------------------------
   Build types — used by Engineer and Breeder
   ----------------------------------------------------------------------- */
typedef enum {
    BOT_BUILD_NONE = 0,
    /* Human Engineer structures */
    BOT_BUILD_TURRET,
    BOT_BUILD_AMMO_DISPENSER,
    BOT_BUILD_REPAIR_STATION,
    /* Alien Breeder structures */
    BOT_BUILD_EGG,
    BOT_BUILD_HIVE_NODE,
    BOT_BUILD_ACID_POOL,

    BOT_BUILD_MAX
} bot_build_type_t;

/* -----------------------------------------------------------------------
   Navigation state
   ----------------------------------------------------------------------- */
typedef struct {
    int current_node;                     /* nav node the bot is at/near     */
    int goal_node;                        /* final destination nav node       */
    int path[BOT_MAX_PATH_NODES];         /* computed path (node indices)     */
    int path_length;                      /* number of valid entries in path  */
    int path_index;                       /* current position in path array   */
    vec3_t goal_origin;                   /* world position of current goal   */
    float  arrived_dist;                  /* distance threshold for "arrived" */
    qboolean path_valid;                  /* is the current path usable?      */
} bot_nav_state_t;

/* -----------------------------------------------------------------------
   Combat state
   ----------------------------------------------------------------------- */
typedef struct {
    edict_t *target;             /* current enemy entity (NULL = none)       */
    vec3_t   target_last_pos;    /* last known position of the target        */
    float    target_last_seen;   /* game time when target was last visible    */
    float    target_dist;        /* current distance to target               */
    qboolean target_visible;     /* is target currently in line-of-sight?    */
    bot_weapon_state_t weapon_state;
    float    next_attack_time;   /* earliest time the bot may fire again     */
    float    aim_error;          /* current aim offset (skill-dependent)     */
    float    engagement_range;   /* preferred combat range for this class    */
} bot_combat_state_t;

/* -----------------------------------------------------------------------
   Build state
   ----------------------------------------------------------------------- */
typedef struct {
    bot_build_type_t what_to_build; /* structure type currently planned      */
    vec3_t           build_origin;  /* world position chosen to build at     */
    int              build_points;  /* build points / resources remaining    */
    float            build_start;   /* game time when build began            */
    qboolean         building;      /* currently in the act of building      */
} bot_build_state_t;

/* -----------------------------------------------------------------------
   Memory: recently seen enemies
   ----------------------------------------------------------------------- */
typedef struct {
    edict_t *ent;           /* entity pointer (may be stale)                */
    vec3_t   last_pos;      /* last known world position                    */
    float    last_seen;     /* game time of last sighting                   */
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
    edict_t       *ent;           /* the edict this bot controls             */
    int            bot_index;     /* index into g_bots[] array               */
    char           name[32];      /* bot name (e.g. "Bot_Marine_01")         */

    /* Class & team ------------------------------------------------------ */
    gloom_class_t  gloom_class;   /* one of the 16 Gloom classes             */
    int            team;          /* TEAM_HUMAN or TEAM_ALIEN                */

    /* Skill ------------------------------------------------------------- */
    float          skill;         /* 0.0 (easy) to 1.0 (expert)             */

    /* AI state machine -------------------------------------------------- */
    bot_ai_state_t ai_state;      /* current high-level behaviour state      */
    bot_ai_state_t prev_ai_state; /* state before the last transition        */
    float          state_enter_time; /* game time when ai_state was entered  */

    /* Navigation -------------------------------------------------------- */
    bot_nav_state_t nav;

    /* Combat ------------------------------------------------------------ */
    bot_combat_state_t combat;

    /* Build ------------------------------------------------------------- */
    bot_build_state_t  build;

    /* Timing ------------------------------------------------------------ */
    float  next_think_time;   /* game time of the next think call            */
    float  think_interval;    /* seconds between thinks (1/BOT_THINK_RATE)  */
    float  reaction_time;     /* aim/decision delay in seconds (skill-based) */

    /* Progression ------------------------------------------------------- */
    int    frag_count;        /* frags this life                             */
    int    class_upgrades;    /* number of times class ability was upgraded  */

    /* Memory ------------------------------------------------------------ */
    bot_enemy_memory_t  enemy_memory[BOT_MAX_REMEMBERED_ENEMIES];
    int                 enemy_memory_count;
    bot_team_memory_t   team_memory[BOT_MAX_REMEMBERED_TEAMMATES];
    int                 team_memory_count;

    vec3_t last_damage_origin; /* origin of last damage received             */
    float  last_damage_time;   /* game time of last damage                   */

    /* Flags ------------------------------------------------------------- */
    qboolean in_use;          /* slot is occupied                            */
    qboolean initialized;     /* bot has completed spawn setup               */
} bot_state_t;

/* -----------------------------------------------------------------------
   Global bot array
   ----------------------------------------------------------------------- */
extern bot_state_t g_bots[MAX_BOTS];
extern int         num_bots;

/* -----------------------------------------------------------------------
   Public bot API (implemented in bot_main.c)
   ----------------------------------------------------------------------- */
void      Bot_Init(void);
void      Bot_Shutdown(void);
bot_state_t *Bot_Connect(edict_t *ent, int team, float skill);
void      Bot_Disconnect(edict_t *ent);
void      Bot_Frame(void);
void      Bot_Think(bot_state_t *bs);

/* Inline helper: return bot_state_t for an edict, or NULL */
static inline bot_state_t *Bot_GetState(const edict_t *ent)
{
    if (!ent || !ent->client)
        return NULL;
    return (bot_state_t *)ent->client->bot_state;
}

#endif /* BOT_H */
