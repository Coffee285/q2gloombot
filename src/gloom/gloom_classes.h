/*
 * gloom_classes.h -- Gloom class definitions and helper API
 *
 * Defines the 8 alien and 8 human Gloom classes, per-class metadata
 * (engagement range, credit/evo costs, capabilities), and the inline
 * helper functions used throughout the bot subsystem.
 *
 * TEAM CONSTANTS
 *   TEAM_HUMAN / TEAM_ALIEN — used in bot_state_t::team and elsewhere.
 *
 * CANONICAL CLASS NAMES
 *   Human:  GRUNT, ST, BIOTECH, HT, COMMANDO, EXTERMINATOR, ENGINEER, MECH
 *   Alien:  HATCHLING, DRONE, WRAITH, KAMIKAZE, STINGER, GUARDIAN, BREEDER, STALKER
 *
 * LEGACY ALIASES
 *   The original Tremulous-derived stub files used different names.  Preprocessor
 *   aliases are provided so those files continue to compile unchanged.
 */

#ifndef GLOOM_CLASSES_H
#define GLOOM_CLASSES_H

/* -----------------------------------------------------------------------
   Team constants
   ----------------------------------------------------------------------- */
#define TEAM_HUMAN  1
#define TEAM_ALIEN  2

/* -----------------------------------------------------------------------
   Class enumeration
   The first 8 values are human classes; values 8-15 are alien classes.
   GLOOM_CLASS_MAX is always 16 — use it for array sizing and loop limits.
   ----------------------------------------------------------------------- */
typedef enum {
    /* ---- Human classes (indices 0-7) ---- */
    GLOOM_CLASS_GRUNT        = 0,  /* basic infantry                        */
    GLOOM_CLASS_ST           = 1,  /* Shock Trooper — shotgun/armour        */
    GLOOM_CLASS_BIOTECH      = 2,  /* medic / healer support                */
    GLOOM_CLASS_HT           = 3,  /* Heavy Trooper — rocket launcher       */
    GLOOM_CLASS_COMMANDO     = 4,  /* fast assault with uzi + grenades      */
    GLOOM_CLASS_EXTERMINATOR = 5,  /* heavy assault — pulse rifle           */
    GLOOM_CLASS_ENGINEER     = 6,  /* human builder                         */
    GLOOM_CLASS_MECH         = 7,  /* power armour — dual lasers            */

    /* ---- Alien classes (indices 8-15) ---- */
    GLOOM_CLASS_HATCHLING    = 8,  /* fast, weak starter                    */
    GLOOM_CLASS_DRONE        = 9,  /* upgraded melee, wall-climb            */
    GLOOM_CLASS_WRAITH       = 10, /* flying, acid spit                     */
    GLOOM_CLASS_KAMIKAZE     = 11, /* suicide bomber                        */
    GLOOM_CLASS_STINGER      = 12, /* hybrid melee + ranged                 */
    GLOOM_CLASS_GUARDIAN     = 13, /* stealth tank with invisibility        */
    GLOOM_CLASS_BREEDER      = 14, /* alien builder                         */
    GLOOM_CLASS_STALKER      = 15, /* ultimate melee tank                   */

    GLOOM_CLASS_MAX          = 16  /* sentinel — do not use as a real class */
} gloom_class_t;

/* -----------------------------------------------------------------------
   Legacy name aliases
   Old Tremulous-derived stub files used these names; keep them compiling.
   ----------------------------------------------------------------------- */
#define GLOOM_CLASS_MARINE_LIGHT    GLOOM_CLASS_GRUNT
#define GLOOM_CLASS_MARINE_ASSAULT  GLOOM_CLASS_ST
#define GLOOM_CLASS_MARINE_HEAVY    GLOOM_CLASS_HT
#define GLOOM_CLASS_MARINE_LASER    GLOOM_CLASS_MECH
#define GLOOM_CLASS_MARINE_BATTLE   GLOOM_CLASS_COMMANDO
#define GLOOM_CLASS_MARINE_ELITE    GLOOM_CLASS_EXTERMINATOR
#define GLOOM_CLASS_BUILDER         GLOOM_CLASS_ENGINEER
#define GLOOM_CLASS_BUILDER_ADV     GLOOM_CLASS_ENGINEER

#define GLOOM_CLASS_DRETCH          GLOOM_CLASS_HATCHLING
#define GLOOM_CLASS_MARAUDER        GLOOM_CLASS_DRONE
#define GLOOM_CLASS_DRAGOON         GLOOM_CLASS_WRAITH
#define GLOOM_CLASS_SPIKER          GLOOM_CLASS_STINGER
#define GLOOM_CLASS_GRANGER         GLOOM_CLASS_BREEDER
#define GLOOM_CLASS_TYRANT          GLOOM_CLASS_STALKER

/* -----------------------------------------------------------------------
   Per-class metadata
   ----------------------------------------------------------------------- */
typedef struct {
    const char *name;            /* printable class name                    */
    int         team;            /* TEAM_HUMAN or TEAM_ALIEN                */
    float       preferred_range; /* preferred combat engagement range       */
    int         credit_cost;     /* human: credits needed to unlock         */
    int         evo_cost;        /* alien: evos needed to evolve into       */
    int         max_health;      /* base max health                         */
    int         can_wall_walk;   /* non-zero if class can traverse walls    */
    int         can_fly;         /* non-zero if class has flight            */
    int         can_build;       /* non-zero if class can construct structs */
    int         is_stealth;      /* non-zero if class has invisibility      */
    gloom_class_t next_evo;      /* alien: next evolution; GLOOM_CLASS_MAX if none */
} gloom_class_info_t;

/* Declared in gloom_classes.c */
extern const gloom_class_info_t gloom_class_info[GLOOM_CLASS_MAX];

/* -----------------------------------------------------------------------
   Inline helper functions
   ----------------------------------------------------------------------- */

static inline const char *Gloom_ClassName(gloom_class_t cls)
{
    if (cls < 0 || cls >= GLOOM_CLASS_MAX) return "unknown";
    return gloom_class_info[cls].name;
}

static inline int Gloom_ClassTeam(gloom_class_t cls)
{
    if (cls < 0 || cls >= GLOOM_CLASS_MAX) return TEAM_HUMAN;
    return gloom_class_info[cls].team;
}

static inline int Gloom_ClassCanWallWalk(gloom_class_t cls)
{
    if (cls < 0 || cls >= GLOOM_CLASS_MAX) return 0;
    return gloom_class_info[cls].can_wall_walk;
}

static inline int Gloom_ClassCanFly(gloom_class_t cls)
{
    if (cls < 0 || cls >= GLOOM_CLASS_MAX) return 0;
    return gloom_class_info[cls].can_fly;
}

static inline int Gloom_ClassCanBuild(gloom_class_t cls)
{
    if (cls < 0 || cls >= GLOOM_CLASS_MAX) return 0;
    return gloom_class_info[cls].can_build;
}

static inline int Gloom_ClassIsStealth(gloom_class_t cls)
{
    if (cls < 0 || cls >= GLOOM_CLASS_MAX) return 0;
    return gloom_class_info[cls].is_stealth;
}

static inline gloom_class_t Gloom_NextEvolution(gloom_class_t cls)
{
    if (cls < 0 || cls >= GLOOM_CLASS_MAX) return GLOOM_CLASS_MAX;
    return gloom_class_info[cls].next_evo;
}

static inline int Gloom_NextEvoCost(gloom_class_t cls)
{
    gloom_class_t next = Gloom_NextEvolution(cls);
    if (next == GLOOM_CLASS_MAX) return 9999;
    return gloom_class_info[next].evo_cost;
}

#endif /* GLOOM_CLASSES_H */
