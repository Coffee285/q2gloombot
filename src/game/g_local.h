/*
 * g_local.h -- local definitions for the q2gloombot game module
 *
 * Adapted from id-Software/Quake-2 game/g_local.h to include hooks for
 * bot initialization and per-frame bot thinking.
 */

#ifndef G_LOCAL_H
#define G_LOCAL_H

#include "q_shared.h"

/*
 * Define GAME_INCLUDE so that game.h exposes the full gclient_t / edict_t
 * structures rather than the abbreviated server-visible stubs.
 */
#define GAME_INCLUDE
#include "game.h"

#define GAMEVERSION "gloombot"

/* protocol bytes that can be directly added to messages */
#define svc_muzzleflash  1
#define svc_muzzleflash2 2
#define svc_temp_entity  3
#define svc_layout       4
#define svc_inventory    5
#define svc_stufftext    11

/* timing */
#define FRAMETIME 0.1f

/* memory tags */
#define TAG_GAME  765
#define TAG_LEVEL 766

/* edict->flags */
#define FL_FLY            0x00000001
#define FL_SWIM           0x00000002
#define FL_IMMUNE_LASER   0x00000004
#define FL_INWATER        0x00000008
#define FL_GODMODE        0x00000010
#define FL_NOTARGET       0x00000020
#define FL_IMMUNE_SLIME   0x00000040
#define FL_IMMUNE_LAVA    0x00000080
#define FL_PARTIALGROUND  0x00000100
#define FL_WATERJUMP      0x00000200
#define FL_TEAMSLAVE      0x00000400
#define FL_NO_KNOCKBACK   0x00000800
#define FL_POWER_ARMOR    0x00001000
#define FL_RESPAWN        0x80000000

/* dead flags */
#define DEAD_NO          0
#define DEAD_DYING       1
#define DEAD_DEAD        2
#define DEAD_RESPAWNABLE 3

/* range classifications */
#define RANGE_MELEE 0
#define RANGE_NEAR  1
#define RANGE_MID   2
#define RANGE_FAR   3

typedef enum {
    MOVETYPE_NONE,
    MOVETYPE_NOCLIP,
    MOVETYPE_PUSH,
    MOVETYPE_STOP,
    MOVETYPE_WALK,
    MOVETYPE_STEP,
    MOVETYPE_FLY,
    MOVETYPE_TOSS,
    MOVETYPE_FLYMISSILE,
    MOVETYPE_BOUNCE
} movetype_t;

typedef enum {
    DAMAGE_NO,
    DAMAGE_YES,
    DAMAGE_AIM
} damage_t;

/* damage flags */
#define DAMAGE_RADIUS         0x00000001
#define DAMAGE_NO_ARMOR       0x00000002
#define DAMAGE_ENERGY         0x00000004
#define DAMAGE_NO_KNOCKBACK   0x00000008
#define DAMAGE_BULLET         0x00000010
#define DAMAGE_NO_PROTECTION  0x00000020

/* ----------------------------------------------------------------
   Full gclient_t — extends the server-visible stub
   ---------------------------------------------------------------- */
struct gclient_s {
    player_state_t  ps;         /* must be first — communicated to clients */
    int             ping;

    /* inventory / persistent data */
    int             frags;
    int             deaths;
    int             team;

    /* used by bot system */
    qboolean        is_bot;
    void           *bot_state;  /* pointer to bot_state_t; NULL for humans */
};

/* ----------------------------------------------------------------
   Full edict_t
   ---------------------------------------------------------------- */
struct edict_s {
    entity_state_t   s;
    struct gclient_s *client;
    qboolean         inuse;
    int              linkcount;

    link_t area;
    int    num_clusters;
    int    clusternums[MAX_ENT_CLUSTERS];
    int    headnode;
    int    areanum, areanum2;

    /* server-visible flags */
    int     svflags;
    vec3_t  mins, maxs;
    vec3_t  absmin, absmax, size;
    solid_t solid;
    int     clipmask;
    edict_t *owner;

    /* game-local fields */
    movetype_t  movetype;
    int         flags;
    int         spawnflags;

    float       freetime;
    float       timestamp;

    float       angle;
    char       *model;
    float       wait;
    float       delay;
    float       random;

    char       *message;
    char       *classname;
    int         spawnflags2;

    float       speed;
    float       accel;
    float       decel;
    vec3_t      movedir;
    vec3_t      pos1, pos2;

    vec3_t      velocity;
    vec3_t      avelocity;
    int         mass;
    float       air_finished;

    float       gravity;

    edict_t    *goalentity;
    edict_t    *movetarget;
    float       yaw_speed;
    float       ideal_yaw;

    float       nextthink;
    void        (*prethink)(edict_t *ent);
    void        (*think)(edict_t *ent);
    void        (*blocked)(edict_t *self, edict_t *other);
    void        (*touch)(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf);
    void        (*use)(edict_t *self, edict_t *other, edict_t *activator);
    void        (*pain)(edict_t *self, edict_t *other, float kick, int damage);
    void        (*die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);

    float       touch_debounce_time;
    float       pain_debounce_time;
    float       damage_debounce_time;
    float       fly_sound_debounce_time;
    float       last_move_time;

    int         health;
    int         max_health;
    int         gib_health;
    int         deadflag;
    qboolean    show_hostile;

    float       powerarmor_time;

    char       *map;

    int         viewheight;
    damage_t    takedamage;
    int         dmg;
    int         radius_dmg;
    float       dmg_radius;
    int         sounds;
    int         count;

    edict_t    *chain;
    edict_t    *enemy;
    edict_t    *oldenemy;
    edict_t    *activator;
    edict_t    *groundentity;
    int         groundentity_linkcount;
    edict_t    *teamchain;
    edict_t    *teammaster;
    edict_t    *mynoise;
    edict_t    *mynoise2;

    int         noise_index;
    int         noise_index2;
    float       volume;
    float       attenuation;
    float       wait2;
    float       delay2;
    float       random2;

    int         watertype;
    int         waterlevel;

    vec3_t      move_origin;
    vec3_t      move_angles;

    int         style;

    int         item;
};

/* ----------------------------------------------------------------
   Level-global state (cleared on each map load)
   ---------------------------------------------------------------- */
typedef struct {
    int    framenum;
    float  time;

    char   level_name[MAX_QPATH];
    char   mapname[MAX_QPATH];
    char   nextmap[MAX_QPATH];

    float  intermissiontime;
    char  *changemap;
    int    exitintermission;
    vec3_t intermission_origin;
    vec3_t intermission_angle;
} level_locals_t;

/* ----------------------------------------------------------------
   Globals shared between game modules
   ---------------------------------------------------------------- */
extern game_import_t   gi;
extern game_export_t   globals;
extern level_locals_t  level;

extern edict_t  *g_edicts;
extern int       num_bots;

extern cvar_t *maxentities;
extern cvar_t *deathmatch;
extern cvar_t *maxclients;
extern cvar_t *sv_gravity;

#define world (&g_edicts[0])

#define FOFS(x)  ((int)(size_t)&(((edict_t  *)0)->x))
#define CLOFS(x) ((int)(size_t)&(((gclient_t*)0)->x))

#define random()  ((rand() & 0x7fff) / ((float)0x7fff))
#define crandom() (2.0f * (random() - 0.5f))

/* ----------------------------------------------------------------
   Function prototypes — g_main.c
   ---------------------------------------------------------------- */
void G_InitEdict(edict_t *e);
edict_t *G_Spawn(void);
void G_FreeEdict(edict_t *e);

#endif /* G_LOCAL_H */
