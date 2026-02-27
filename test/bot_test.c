/*
 * bot_test.c -- testing framework for q2gloombot
 *
 * Standalone test harness that exercises bot subsystems in isolation
 * from the Quake 2 engine.  Provides mock implementations of engine
 * functions (gi.*) and validates bot state transitions, integration
 * ordering, and subsystem interactions.
 *
 * Build:  cmake --build . --target bot_test
 * Run:    ./bot_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

/* -----------------------------------------------------------------------
   Minimal test framework
   ----------------------------------------------------------------------- */
static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static void name(void)

#define ASSERT_TRUE(expr) do { \
    tests_run++; \
    if (expr) { tests_passed++; } \
    else { tests_failed++; \
        printf("  FAIL: %s:%d: %s\n", __FILE__, __LINE__, #expr); } \
} while (0)

#define ASSERT_FALSE(expr)      ASSERT_TRUE(!(expr))
#define ASSERT_EQ(a, b)         ASSERT_TRUE((a) == (b))
#define ASSERT_NE(a, b)         ASSERT_TRUE((a) != (b))
#define ASSERT_NULL(ptr)        ASSERT_TRUE((ptr) == NULL)
#define ASSERT_NOT_NULL(ptr)    ASSERT_TRUE((ptr) != NULL)
#define ASSERT_STR_EQ(a, b)     ASSERT_TRUE(strcmp((a), (b)) == 0)

#define RUN_TEST(name) do { \
    printf("  Running: %s\n", #name); \
    name(); \
} while (0)

/* -----------------------------------------------------------------------
   Mock engine globals and functions
   Provide minimal stubs so bot code can compile and run in test mode.
   ----------------------------------------------------------------------- */

/* Forward declare what we need from bot headers */
#include "g_local.h"

/* Global mocks */
static char mock_print_buf[4096];
static int  mock_print_len = 0;

static void mock_dprintf(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    mock_print_len += vsnprintf(mock_print_buf + mock_print_len,
                                (int)sizeof(mock_print_buf) - mock_print_len,
                                fmt, ap);
    va_end(ap);
}

static void mock_bprintf(int printlevel, char *fmt, ...)
{
    (void)printlevel; (void)fmt;
}

static void mock_cprintf(edict_t *ent, int printlevel, char *fmt, ...)
{
    (void)ent; (void)printlevel; (void)fmt;
}

static void mock_centerprintf(edict_t *ent, char *fmt, ...)
{
    (void)ent; (void)fmt;
}

static void mock_error(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(1);
}

static void *mock_TagMalloc(int size, int tag)
{
    (void)tag;
    return calloc(1, (size_t)size);
}

static void mock_TagFree(void *block)
{
    free(block);
}

static void mock_FreeTags(int tag)
{
    (void)tag;
}

static int mock_argc_val = 0;
static char *mock_argv_vals[8] = { NULL };

static int mock_argc(void) { return mock_argc_val; }
static char *mock_argv(int n)
{
    if (n < 0 || n >= mock_argc_val) return "";
    return mock_argv_vals[n] ? mock_argv_vals[n] : "";
}

static cvar_t mock_maxclients_cvar  = { "maxclients",  "8",    NULL, 0, false, 8.0f,  NULL };
static cvar_t mock_maxentities_cvar = { "maxentities", "1024", NULL, 0, false, 1024.0f, NULL };

static cvar_t *mock_cvar(char *var_name, char *value, int flags)
{
    (void)flags; (void)value;
    if (strcmp(var_name, "maxclients") == 0) return &mock_maxclients_cvar;
    if (strcmp(var_name, "maxentities") == 0) return &mock_maxentities_cvar;
    return &mock_maxclients_cvar;
}

static trace_t mock_trace(vec3_t start, vec3_t mins, vec3_t maxs,
                           vec3_t end, edict_t *passent, int contentmask)
{
    trace_t t;
    (void)start; (void)mins; (void)maxs; (void)end;
    (void)passent; (void)contentmask;
    memset(&t, 0, sizeof(t));
    t.fraction = 1.0f;
    if (end) VectorCopy(end, t.endpos);
    return t;
}

static int mock_pointcontents(vec3_t point)
{
    (void)point;
    return 0;
}

static void mock_noop_edict(edict_t *ent) { (void)ent; }
static void mock_sound(edict_t *ent, int ch, int si, float v, float a, float t)
{ (void)ent; (void)ch; (void)si; (void)v; (void)a; (void)t; }
static void mock_positioned_sound(vec3_t o, edict_t *e, int ch, int si, float v, float a, float t)
{ (void)o; (void)e; (void)ch; (void)si; (void)v; (void)a; (void)t; }
static void mock_configstring(int n, char *s) { (void)n; (void)s; }
static int  mock_modelindex(char *n) { (void)n; return 0; }
static int  mock_soundindex(char *n) { (void)n; return 0; }
static int  mock_imageindex(char *n) { (void)n; return 0; }
static void mock_setmodel(edict_t *e, char *n) { (void)e; (void)n; }
static qboolean mock_inPVS(vec3_t a, vec3_t b) { (void)a; (void)b; return true; }
static qboolean mock_inPHS(vec3_t a, vec3_t b) { (void)a; (void)b; return true; }
static void mock_SetAreaPortalState(int p, qboolean o) { (void)p; (void)o; }
static qboolean mock_AreasConnected(int a, int b) { (void)a; (void)b; return true; }
static int mock_BoxEdicts(vec3_t mins, vec3_t maxs, edict_t **list, int mc, int at)
{ (void)mins; (void)maxs; (void)list; (void)mc; (void)at; return 0; }
static void mock_Pmove(pmove_t *pm) { (void)pm; }
static void mock_multicast(vec3_t o, multicast_t t) { (void)o; (void)t; }
static void mock_unicast(edict_t *e, qboolean r) { (void)e; (void)r; }
static void mock_WriteChar(int c) { (void)c; }
static void mock_WriteByte(int c) { (void)c; }
static void mock_WriteShort(int c) { (void)c; }
static void mock_WriteLong(int c) { (void)c; }
static void mock_WriteFloat(float f) { (void)f; }
static void mock_WriteString(char *s) { (void)s; }
static void mock_WritePosition(vec3_t p) { (void)p; }
static void mock_WriteDir(vec3_t p) { (void)p; }
static void mock_WriteAngle(float f) { (void)f; }
static cvar_t *mock_cvar_set(char *n, char *v) { (void)n; (void)v; return NULL; }
static cvar_t *mock_cvar_forceset(char *n, char *v) { (void)n; (void)v; return NULL; }
static char *mock_args(void) { return ""; }
static void mock_AddCommandString(char *t) { (void)t; }
static void mock_DebugGraph(float v, int c) { (void)v; (void)c; }

/* -----------------------------------------------------------------------
   Engine globals used by the game code
   ----------------------------------------------------------------------- */
game_import_t   gi;
game_export_t   globals;
level_locals_t  level;

edict_t  *g_edicts  = NULL;
gclient_t *g_clients = NULL;

cvar_t *maxentities = NULL;
cvar_t *deathmatch  = NULL;
cvar_t *maxclients  = NULL;
cvar_t *sv_gravity  = NULL;

void G_InitEdict(edict_t *e)
{
    e->inuse     = true;
    e->classname = "noclass";
    e->gravity   = 1.0f;
    e->s.number  = (int)(e - g_edicts);
}

edict_t *G_Spawn(void)
{
    return NULL;
}

void G_FreeEdict(edict_t *e)
{
    (void)e;
}

/* -----------------------------------------------------------------------
   Shared math stubs needed by bot code
   ----------------------------------------------------------------------- */
vec3_t vec3_origin = {0, 0, 0};

vec_t VectorLength(vec3_t v)
{
    return (vec_t)sqrt((double)(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]));
}

vec_t VectorNormalize(vec3_t v)
{
    float len = VectorLength(v);
    if (len) { v[0] /= len; v[1] /= len; v[2] /= len; }
    return len;
}

void VectorMA(vec3_t veca, float scale, vec3_t vecb, vec3_t vecc)
{
    vecc[0] = veca[0] + scale * vecb[0];
    vecc[1] = veca[1] + scale * vecb[1];
    vecc[2] = veca[2] + scale * vecb[2];
}

void VectorScale(vec3_t in, vec_t scale, vec3_t out)
{
    out[0] = in[0] * scale;
    out[1] = in[1] * scale;
    out[2] = in[2] * scale;
}

int Q_stricmp(const char *s1, const char *s2)
{
    int c1, c2;
    do {
        c1 = (unsigned char)*s1++;
        c2 = (unsigned char)*s2++;
        if (c1 >= 'a' && c1 <= 'z') c1 -= 32;
        if (c2 >= 'a' && c2 <= 'z') c2 -= 32;
        if (c1 != c2) return c1 - c2;
    } while (c1);
    return 0;
}

void Com_sprintf(char *dest, int size, char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(dest, (size_t)size, fmt, ap);
    va_end(ap);
}

/* -----------------------------------------------------------------------
   Test setup / teardown
   ----------------------------------------------------------------------- */
static edict_t  test_edicts[32];
static gclient_t test_clients[8];

static void test_setup(void)
{
    memset(&gi, 0, sizeof(gi));
    gi.dprintf          = mock_dprintf;
    gi.bprintf          = mock_bprintf;
    gi.cprintf          = mock_cprintf;
    gi.centerprintf     = mock_centerprintf;
    gi.error            = mock_error;
    gi.TagMalloc        = mock_TagMalloc;
    gi.TagFree          = mock_TagFree;
    gi.FreeTags         = mock_FreeTags;
    gi.argc             = mock_argc;
    gi.argv             = mock_argv;
    gi.cvar             = mock_cvar;
    gi.trace            = mock_trace;
    gi.pointcontents    = mock_pointcontents;
    gi.sound            = mock_sound;
    gi.positioned_sound = mock_positioned_sound;
    gi.configstring     = mock_configstring;
    gi.modelindex       = mock_modelindex;
    gi.soundindex       = mock_soundindex;
    gi.imageindex       = mock_imageindex;
    gi.setmodel         = mock_setmodel;
    gi.inPVS            = mock_inPVS;
    gi.inPHS            = mock_inPHS;
    gi.SetAreaPortalState = mock_SetAreaPortalState;
    gi.AreasConnected   = mock_AreasConnected;
    gi.linkentity       = mock_noop_edict;
    gi.unlinkentity     = mock_noop_edict;
    gi.BoxEdicts        = mock_BoxEdicts;
    gi.Pmove            = mock_Pmove;
    gi.multicast        = mock_multicast;
    gi.unicast          = mock_unicast;
    gi.WriteChar        = mock_WriteChar;
    gi.WriteByte        = mock_WriteByte;
    gi.WriteShort       = mock_WriteShort;
    gi.WriteLong        = mock_WriteLong;
    gi.WriteFloat       = mock_WriteFloat;
    gi.WriteString      = mock_WriteString;
    gi.WritePosition    = mock_WritePosition;
    gi.WriteDir         = mock_WriteDir;
    gi.WriteAngle       = mock_WriteAngle;
    gi.cvar_set         = mock_cvar_set;
    gi.cvar_forceset    = mock_cvar_forceset;
    gi.args             = mock_args;
    gi.AddCommandString = mock_AddCommandString;
    gi.DebugGraph       = mock_DebugGraph;

    memset(&globals, 0, sizeof(globals));
    globals.max_edicts = 32;
    globals.num_edicts = 9;
    globals.edict_size = sizeof(edict_t);

    memset(test_edicts, 0, sizeof(test_edicts));
    memset(test_clients, 0, sizeof(test_clients));
    g_edicts = test_edicts;
    g_clients = test_clients;
    globals.edicts = test_edicts;

    maxclients  = &mock_maxclients_cvar;
    maxentities = &mock_maxentities_cvar;

    memset(&level, 0, sizeof(level));
    level.time = 1.0f;

    mock_print_len = 0;
    memset(mock_print_buf, 0, sizeof(mock_print_buf));
}

/* Include the bot headers — after mocks are set up */
#include "bot.h"
#include "bot_debug.h"
#include "bot_strategy.h"
#include "bot_upgrade.h"

/* =======================================================================
   TEST CASES
   ======================================================================= */

/* ----------------------------------------------------------------------- */
TEST(test_bot_init_shutdown)
{
    int i;
    test_setup();
    Bot_Init();

    ASSERT_EQ(num_bots, 0);
    for (i = 0; i < MAX_BOTS; i++) {
        ASSERT_FALSE(g_bots[i].in_use);
    }

    Bot_Shutdown();
    ASSERT_EQ(num_bots, 0);
}

/* ----------------------------------------------------------------------- */
TEST(test_bot_connect_disconnect)
{
    test_setup();
    Bot_Init();

    /* Create a fake edict with a heap-allocated client
     * (Bot_Disconnect calls gi.TagFree on the client) */
    edict_t ent;
    gclient_t *client = (gclient_t *)calloc(1, sizeof(gclient_t));
    memset(&ent, 0, sizeof(ent));
    ent.inuse = true;
    ent.client = client;
    ent.max_health = 100;
    ent.health = 100;

    bot_state_t *bs = Bot_Connect(&ent, TEAM_HUMAN, 0.5f);
    ASSERT_NOT_NULL(bs);
    ASSERT_EQ(num_bots, 1);
    ASSERT_TRUE(bs->in_use);
    ASSERT_TRUE(bs->initialized);
    ASSERT_EQ(bs->team, TEAM_HUMAN);
    ASSERT_TRUE(bs->skill >= 0.49f && bs->skill <= 0.51f);
    ASSERT_EQ(bs->ai_state, BOTSTATE_IDLE);
    ASSERT_EQ(bs->nav.current_node, BOT_INVALID_NODE);
    ASSERT_EQ(bs->nav.goal_node, BOT_INVALID_NODE);
    ASSERT_FALSE(bs->nav.path_valid);

    /* Disconnect (frees the client via gi.TagFree) */
    Bot_Disconnect(&ent);
    ASSERT_EQ(num_bots, 0);
    ASSERT_FALSE(bs->in_use);

    Bot_Shutdown();
}

/* ----------------------------------------------------------------------- */
TEST(test_bot_connect_alien)
{
    test_setup();
    Bot_Init();

    edict_t ent;
    gclient_t *client = (gclient_t *)calloc(1, sizeof(gclient_t));
    memset(&ent, 0, sizeof(ent));
    ent.inuse = true;
    ent.client = client;
    ent.max_health = 50;
    ent.health = 50;

    bot_state_t *bs = Bot_Connect(&ent, TEAM_ALIEN, 0.8f);
    ASSERT_NOT_NULL(bs);
    ASSERT_EQ(bs->team, TEAM_ALIEN);
    ASSERT_TRUE(bs->combat.prefer_cover);
    ASSERT_FALSE(bs->combat.max_range_engage);

    Bot_Disconnect(&ent);
    Bot_Shutdown();
}

/* ----------------------------------------------------------------------- */
TEST(test_bot_connect_null_entity)
{
    test_setup();
    Bot_Init();

    bot_state_t *bs = Bot_Connect(NULL, TEAM_HUMAN, 0.5f);
    ASSERT_NULL(bs);
    ASSERT_EQ(num_bots, 0);

    Bot_Shutdown();
}

/* ----------------------------------------------------------------------- */
TEST(test_bot_connect_max_bots)
{
    int i;
    edict_t ents[MAX_BOTS + 1];
    gclient_t *clients[MAX_BOTS + 1];
    bot_state_t *bs;

    test_setup();
    Bot_Init();

    memset(ents, 0, sizeof(ents));

    /* Fill all slots */
    for (i = 0; i < MAX_BOTS; i++) {
        clients[i] = (gclient_t *)calloc(1, sizeof(gclient_t));
        ents[i].inuse = true;
        ents[i].client = clients[i];
        ents[i].max_health = 100;
        ents[i].health = 100;
        bs = Bot_Connect(&ents[i], TEAM_HUMAN, 0.5f);
        ASSERT_NOT_NULL(bs);
    }
    ASSERT_EQ(num_bots, MAX_BOTS);

    /* Try one more — should fail */
    clients[MAX_BOTS] = (gclient_t *)calloc(1, sizeof(gclient_t));
    ents[MAX_BOTS].inuse = true;
    ents[MAX_BOTS].client = clients[MAX_BOTS];
    bs = Bot_Connect(&ents[MAX_BOTS], TEAM_HUMAN, 0.5f);
    ASSERT_NULL(bs);
    ASSERT_EQ(num_bots, MAX_BOTS);

    /* Clean up */
    for (i = 0; i < MAX_BOTS; i++)
        Bot_Disconnect(&ents[i]);
    free(clients[MAX_BOTS]);
    Bot_Shutdown();
}

/* ----------------------------------------------------------------------- */
TEST(test_bot_skill_clamping)
{
    bot_state_t *bs1, *bs2;
    edict_t ent1, ent2;
    gclient_t *cl1 = (gclient_t *)calloc(1, sizeof(gclient_t));
    gclient_t *cl2 = (gclient_t *)calloc(1, sizeof(gclient_t));

    test_setup();
    Bot_Init();

    memset(&ent1, 0, sizeof(ent1));
    memset(&ent2, 0, sizeof(ent2));
    ent1.inuse = true; ent1.client = cl1; ent1.max_health = 100; ent1.health = 100;
    ent2.inuse = true; ent2.client = cl2; ent2.max_health = 100; ent2.health = 100;

    /* Skill below 0 should be clamped to 0 */
    bs1 = Bot_Connect(&ent1, TEAM_HUMAN, -5.0f);
    ASSERT_NOT_NULL(bs1);
    ASSERT_TRUE(bs1->skill >= 0.0f);

    /* Skill above 1 should be clamped to 1 */
    bs2 = Bot_Connect(&ent2, TEAM_HUMAN, 999.0f);
    ASSERT_NOT_NULL(bs2);
    ASSERT_TRUE(bs2->skill <= 1.0f);

    Bot_Disconnect(&ent1);
    Bot_Disconnect(&ent2);
    Bot_Shutdown();
}

/* ----------------------------------------------------------------------- */
TEST(test_bot_think_null_safety)
{
    bot_state_t bs;

    test_setup();
    Bot_Init();

    /* Think with NULL should not crash */
    Bot_Think(NULL);

    /* Think with a bot whose ent is NULL should not crash */
    memset(&bs, 0, sizeof(bs));
    bs.ent = NULL;
    Bot_Think(&bs);

    Bot_Shutdown();
}

/* ----------------------------------------------------------------------- */
TEST(test_bot_frame_paused)
{
    edict_t ent;
    gclient_t *client = (gclient_t *)calloc(1, sizeof(gclient_t));
    bot_state_t *bs;

    test_setup();
    Bot_Init();

    memset(&ent, 0, sizeof(ent));
    ent.inuse = true;
    ent.client = client;
    ent.max_health = 100;
    ent.health = 100;

    bs = Bot_Connect(&ent, TEAM_HUMAN, 0.5f);
    ASSERT_NOT_NULL(bs);

    /* Pause bots */
    bot_paused = true;
    bs->next_think_time = 0.0f;
    Bot_Frame();
    /* The state should not change because bots are paused */
    ASSERT_EQ(bs->ai_state, BOTSTATE_IDLE);

    bot_paused = false;
    Bot_Disconnect(&ent);
    Bot_Shutdown();
}

/* ----------------------------------------------------------------------- */
TEST(test_debug_state_names)
{
    test_setup();
    BotDebug_Init();

    ASSERT_STR_EQ(BotDebug_StateName(BOTSTATE_IDLE), "IDLE");
    ASSERT_STR_EQ(BotDebug_StateName(BOTSTATE_COMBAT), "COMBAT");
    ASSERT_STR_EQ(BotDebug_StateName(BOTSTATE_BUILD), "BUILD");
    ASSERT_STR_EQ(BotDebug_StateName(BOTSTATE_UPGRADE), "UPGRADE");

    /* Invalid state */
    ASSERT_STR_EQ(BotDebug_StateName(-1), "UNKNOWN");
    ASSERT_STR_EQ(BotDebug_StateName(BOTSTATE_MAX), "UNKNOWN");
}

/* ----------------------------------------------------------------------- */
TEST(test_debug_strategy_names)
{
    test_setup();

    ASSERT_STR_EQ(BotDebug_StrategyName(STRATEGY_DEFEND), "DEFEND");
    ASSERT_STR_EQ(BotDebug_StrategyName(STRATEGY_PUSH), "PUSH");
    ASSERT_STR_EQ(BotDebug_StrategyName(STRATEGY_ALLIN), "ALLIN");
    ASSERT_STR_EQ(BotDebug_StrategyName(-1), "UNKNOWN");
}

/* ----------------------------------------------------------------------- */
TEST(test_debug_role_names)
{
    test_setup();

    ASSERT_STR_EQ(BotDebug_RoleName(ROLE_BUILDER), "BUILDER");
    ASSERT_STR_EQ(BotDebug_RoleName(ROLE_ATTACKER), "ATTACKER");
    ASSERT_STR_EQ(BotDebug_RoleName(-1), "UNKNOWN");
}

/* ----------------------------------------------------------------------- */
TEST(test_debug_flags_toggle)
{
    test_setup();
    BotDebug_Init();

    ASSERT_EQ(bot_debug_flags, BOT_DEBUG_NONE);

    bot_debug_flags = BOT_DEBUG_ALL;
    ASSERT_TRUE(bot_debug_flags & BOT_DEBUG_STATE);
    ASSERT_TRUE(bot_debug_flags & BOT_DEBUG_NAV);
    ASSERT_TRUE(bot_debug_flags & BOT_DEBUG_COMBAT);

    bot_debug_flags = BOT_DEBUG_NONE;
    ASSERT_FALSE(bot_debug_flags & BOT_DEBUG_STATE);
}

/* ----------------------------------------------------------------------- */
TEST(test_debug_log_filtered)
{
    test_setup();
    BotDebug_Init();

    mock_print_len = 0;
    memset(mock_print_buf, 0, sizeof(mock_print_buf));

    /* With no debug flags, log should produce no output */
    bot_debug_flags = BOT_DEBUG_NONE;
    BotDebug_Log(BOT_DEBUG_STATE, "should not appear\n");
    ASSERT_EQ(mock_print_len, 0);

    /* With STATE flag, log should produce output */
    bot_debug_flags = BOT_DEBUG_STATE;
    BotDebug_Log(BOT_DEBUG_STATE, "visible\n");
    ASSERT_TRUE(mock_print_len > 0);
}

/* ----------------------------------------------------------------------- */
TEST(test_gloom_class_helpers)
{
    test_setup();

    /* Verify class names */
    ASSERT_STR_EQ(Gloom_ClassName(GLOOM_CLASS_GRUNT), "Grunt");
    ASSERT_STR_EQ(Gloom_ClassName(GLOOM_CLASS_HATCHLING), "Hatchling");
    ASSERT_STR_EQ(Gloom_ClassName(GLOOM_CLASS_ENGINEER), "Engineer");
    ASSERT_STR_EQ(Gloom_ClassName(GLOOM_CLASS_BREEDER), "Breeder");

    /* Invalid class */
    ASSERT_STR_EQ(Gloom_ClassName(GLOOM_CLASS_MAX), "unknown");

    /* Team checks */
    ASSERT_EQ(Gloom_ClassTeam(GLOOM_CLASS_GRUNT), TEAM_HUMAN);
    ASSERT_EQ(Gloom_ClassTeam(GLOOM_CLASS_HATCHLING), TEAM_ALIEN);

    /* Builder checks */
    ASSERT_TRUE(Gloom_ClassCanBuild(GLOOM_CLASS_ENGINEER));
    ASSERT_TRUE(Gloom_ClassCanBuild(GLOOM_CLASS_BREEDER));
    ASSERT_FALSE(Gloom_ClassCanBuild(GLOOM_CLASS_GRUNT));
    ASSERT_FALSE(Gloom_ClassCanBuild(GLOOM_CLASS_HATCHLING));
}

/* ----------------------------------------------------------------------- */
TEST(test_gloom_struct_helpers)
{
    test_setup();

    ASSERT_EQ(Gloom_PrimaryStruct(TEAM_HUMAN), STRUCT_REACTOR);
    ASSERT_EQ(Gloom_PrimaryStruct(TEAM_ALIEN), STRUCT_OVERMIND);
    ASSERT_EQ(Gloom_SpawnStruct(TEAM_HUMAN), STRUCT_TELEPORTER);
    ASSERT_EQ(Gloom_SpawnStruct(TEAM_ALIEN), STRUCT_EGG);
}

/* ----------------------------------------------------------------------- */
TEST(test_bot_state_transitions)
{
    edict_t ent;
    gclient_t *client = (gclient_t *)calloc(1, sizeof(gclient_t));
    bot_state_t *bs;

    test_setup();
    Bot_Init();

    memset(&ent, 0, sizeof(ent));
    ent.inuse = true;
    ent.client = client;
    ent.max_health = 100;
    ent.health = 100;

    bs = Bot_Connect(&ent, TEAM_HUMAN, 0.5f);
    ASSERT_NOT_NULL(bs);
    ASSERT_EQ(bs->ai_state, BOTSTATE_IDLE);

    /* Simulate: idle bot with no target should stay idle initially,
       then patrol after 2 seconds */
    level.time = 0.5f;
    bs->state_enter_time = 0.0f;
    bs->combat.target = NULL;
    bs->combat.target_visible = false;
    bs->next_think_time = 0.0f;

    /* After 2+ seconds idle, should transition to patrol */
    level.time = 3.0f;
    Bot_Frame();
    ASSERT_EQ(bs->ai_state, BOTSTATE_PATROL);

    Bot_Disconnect(&ent);
    Bot_Shutdown();
}

/* ----------------------------------------------------------------------- */
TEST(test_bot_combat_transition)
{
    edict_t ent, enemy;
    gclient_t *client = (gclient_t *)calloc(1, sizeof(gclient_t));
    bot_state_t *bs;

    test_setup();
    Bot_Init();

    memset(&ent, 0, sizeof(ent));
    memset(&enemy, 0, sizeof(enemy));
    ent.inuse = true;
    ent.client = client;
    ent.max_health = 100;
    ent.health = 100;
    enemy.inuse = true;

    bs = Bot_Connect(&ent, TEAM_HUMAN, 0.5f);
    ASSERT_NOT_NULL(bs);

    /* Set up visible enemy — should transition to COMBAT */
    bs->combat.target = &enemy;
    bs->combat.target_visible = true;
    bs->next_think_time = 0.0f;
    level.time = 1.0f;

    Bot_Frame();
    ASSERT_EQ(bs->ai_state, BOTSTATE_COMBAT);

    Bot_Disconnect(&ent);
    Bot_Shutdown();
}

/* ----------------------------------------------------------------------- */
TEST(test_bot_flee_transition)
{
    edict_t ent, enemy;
    gclient_t *client = (gclient_t *)calloc(1, sizeof(gclient_t));
    bot_state_t *bs;

    test_setup();
    Bot_Init();

    memset(&ent, 0, sizeof(ent));
    memset(&enemy, 0, sizeof(enemy));
    ent.inuse = true;
    ent.client = client;
    ent.max_health = 100;
    ent.health = 15;  /* Critical health: 15% */
    enemy.inuse = true;

    bs = Bot_Connect(&ent, TEAM_HUMAN, 0.5f);
    ASSERT_NOT_NULL(bs);

    /* Already in combat with low health — should flee */
    bs->ai_state = BOTSTATE_COMBAT;
    bs->combat.target = &enemy;
    bs->combat.target_visible = true;
    bs->next_think_time = 0.0f;
    level.time = 1.0f;

    Bot_Frame();
    ASSERT_EQ(bs->ai_state, BOTSTATE_FLEE);

    Bot_Disconnect(&ent);
    Bot_Shutdown();
}

/* ----------------------------------------------------------------------- */
TEST(test_bot_getstate_helper)
{
    edict_t ent;

    test_setup();
    Bot_Init();

    /* NULL entity */
    ASSERT_NULL(Bot_GetState(NULL));

    /* Entity without client */
    memset(&ent, 0, sizeof(ent));
    ent.client = NULL;
    ASSERT_NULL(Bot_GetState(&ent));

    Bot_Shutdown();
}

/* ----------------------------------------------------------------------- */
TEST(test_bot_is_alien_helper)
{
    bot_state_t bs;

    test_setup();
    memset(&bs, 0, sizeof(bs));

    bs.team = TEAM_ALIEN;
    ASSERT_TRUE(Bot_IsAlien(&bs));

    bs.team = TEAM_HUMAN;
    ASSERT_FALSE(Bot_IsAlien(&bs));

    ASSERT_FALSE(Bot_IsAlien(NULL));
}

/* ----------------------------------------------------------------------- */
TEST(test_bot_update_timers)
{
    edict_t ent, enemy_ent;
    gclient_t *client = (gclient_t *)calloc(1, sizeof(gclient_t));
    bot_state_t *bs;

    test_setup();
    Bot_Init();

    memset(&ent, 0, sizeof(ent));
    memset(&enemy_ent, 0, sizeof(enemy_ent));
    ent.inuse = true;
    ent.client = client;
    ent.max_health = 100;
    ent.health = 100;
    enemy_ent.inuse = true;

    bs = Bot_Connect(&ent, TEAM_HUMAN, 0.5f);
    ASSERT_NOT_NULL(bs);

    /* Add a stale enemy memory */
    bs->enemy_memory[0].ent = &enemy_ent;
    bs->enemy_memory[0].last_seen = 0.0f;  /* seen at t=0 */
    bs->enemy_memory_count = 1;

    /* At t=15 (> BOT_ENEMY_MEMORY_TIME=10), should be expired */
    level.time = 15.0f;
    Bot_UpdateTimers(bs);
    ASSERT_EQ(bs->enemy_memory_count, 0);

    Bot_Disconnect(&ent);
    Bot_Shutdown();
}

/* =======================================================================
   Main
   ======================================================================= */
int main(void)
{
    printf("q2gloombot Test Suite\n");
    printf("=====================\n\n");

    printf("Bot Lifecycle Tests:\n");
    RUN_TEST(test_bot_init_shutdown);
    RUN_TEST(test_bot_connect_disconnect);
    RUN_TEST(test_bot_connect_alien);
    RUN_TEST(test_bot_connect_null_entity);
    RUN_TEST(test_bot_connect_max_bots);
    RUN_TEST(test_bot_skill_clamping);
    RUN_TEST(test_bot_think_null_safety);
    RUN_TEST(test_bot_frame_paused);

    printf("\nDebug System Tests:\n");
    RUN_TEST(test_debug_state_names);
    RUN_TEST(test_debug_strategy_names);
    RUN_TEST(test_debug_role_names);
    RUN_TEST(test_debug_flags_toggle);
    RUN_TEST(test_debug_log_filtered);

    printf("\nGloom Helper Tests:\n");
    RUN_TEST(test_gloom_class_helpers);
    RUN_TEST(test_gloom_struct_helpers);

    printf("\nState Transition Tests:\n");
    RUN_TEST(test_bot_state_transitions);
    RUN_TEST(test_bot_combat_transition);
    RUN_TEST(test_bot_flee_transition);

    printf("\nHelper Function Tests:\n");
    RUN_TEST(test_bot_getstate_helper);
    RUN_TEST(test_bot_is_alien_helper);
    RUN_TEST(test_bot_update_timers);

    printf("\n=====================\n");
    printf("Results: %d tests, %d passed, %d failed\n",
           tests_run, tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
