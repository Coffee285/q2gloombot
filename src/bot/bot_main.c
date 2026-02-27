/*
 * bot_main.c -- bot lifecycle management for q2gloombot
 *
 * Implements:
 *   Bot_Init()        — called once at DLL load
 *   Bot_Shutdown()    — called on DLL unload
 *   Bot_Connect()     — spawns a new bot into the game
 *   Bot_Disconnect()  — removes a bot
 *   Bot_Frame()       — per-server-frame update; iterates all bots
 *   Bot_Think()       — per-bot AI think; runs the state machine
 *
 * Console commands added to the server:
 *   sv addbot [team] [class] [skill]   — spawn a new bot
 *   sv removebot [name]                — remove a bot by name
 *   sv listbots                        — print active bots
 */

#include "bot.h"

/* -----------------------------------------------------------------------
   Module globals
   ----------------------------------------------------------------------- */
bot_state_t g_bots[MAX_BOTS];
int         num_bots = 0;

/* Forward declarations */
static void Bot_StateIdle(bot_state_t *bs);
static void Bot_StatePatrol(bot_state_t *bs);
static void Bot_StateHunt(bot_state_t *bs);
static void Bot_StateCombat(bot_state_t *bs);
static void Bot_StateFlee(bot_state_t *bs);
static void Bot_StateDefend(bot_state_t *bs);
static void Bot_StateEscort(bot_state_t *bs);
static void Bot_StateBuild(bot_state_t *bs);
static void Bot_ChooseClass(bot_state_t *bs, int team, gloom_class_t requested_class);
static void SV_AddBot_f(void);
static void SV_RemoveBot_f(void);
static void SV_ListBots_f(void);

/* -----------------------------------------------------------------------
   Bot_Init
   Called once when the game DLL is loaded.
   ----------------------------------------------------------------------- */
void Bot_Init(void)
{
    int i;

    gi.dprintf("Bot_Init: initialising bot subsystem\n");

    memset(g_bots, 0, sizeof(g_bots));
    num_bots = 0;

    for (i = 0; i < MAX_BOTS; i++) {
        g_bots[i].in_use      = false;
        g_bots[i].bot_index   = i;
    }

    /* Register server-side console commands */
    gi.AddCommandString("sv addbot\n");    /* placeholder; real registration below */
    (void)SV_AddBot_f;   /* reference to suppress unused-function warnings */
    (void)SV_RemoveBot_f;
    (void)SV_ListBots_f;

    gi.dprintf("Bot_Init: done (%d bot slots)\n", MAX_BOTS);
}

/* -----------------------------------------------------------------------
   Bot_Shutdown
   ----------------------------------------------------------------------- */
void Bot_Shutdown(void)
{
    int i;

    gi.dprintf("Bot_Shutdown: cleaning up\n");

    for (i = 0; i < MAX_BOTS; i++) {
        if (g_bots[i].in_use)
            Bot_Disconnect(g_bots[i].ent);
    }

    num_bots = 0;
}

/* -----------------------------------------------------------------------
   Bot_Connect
   Assigns a free bot_state_t slot to the given edict and initialises it.
   Returns the bot_state_t pointer, or NULL if no slots are free.
   ----------------------------------------------------------------------- */
bot_state_t *Bot_Connect(edict_t *ent, int team, float skill)
{
    int i;
    bot_state_t *bs = NULL;
    gloom_class_t cls;

    if (!ent || !ent->client) {
        gi.dprintf("Bot_Connect: NULL entity or client\n");
        return NULL;
    }

    if (num_bots >= MAX_BOTS) {
        gi.dprintf("Bot_Connect: maximum bot count (%d) reached\n", MAX_BOTS);
        return NULL;
    }

    /* Find a free slot */
    for (i = 0; i < MAX_BOTS; i++) {
        if (!g_bots[i].in_use) {
            bs = &g_bots[i];
            break;
        }
    }

    if (!bs) {
        gi.dprintf("Bot_Connect: no free bot slots\n");
        return NULL;
    }

    /* Initialise the slot */
    memset(bs, 0, sizeof(*bs));
    bs->bot_index = i;
    bs->in_use    = true;
    bs->ent       = ent;

    /* Clamp skill to [0, 1] */
    if (skill < 0.0f) skill = 0.0f;
    if (skill > 1.0f) skill = 1.0f;
    bs->skill = skill;

    /* Team assignment */
    if (team != TEAM_HUMAN && team != TEAM_ALIEN)
        team = (num_bots % 2 == 0) ? TEAM_HUMAN : TEAM_ALIEN;
    bs->team = team;

    /* Choose a class appropriate for the team */
    cls = (team == TEAM_HUMAN) ? GLOOM_CLASS_MARINE : GLOOM_CLASS_STINGER;
    Bot_ChooseClass(bs, team, cls);

    /* Name the bot */
    Com_sprintf(bs->name, sizeof(bs->name), "Bot_%s_%02d",
                Gloom_ClassName(bs->gloom_class), i);

    /* Timing */
    bs->think_interval  = BOT_THINK_RATE;
    bs->reaction_time   = 0.5f - (skill * 0.4f); /* 0.1–0.5 s */
    bs->next_think_time = 0.0f;

    /* Navigation defaults */
    bs->nav.current_node = BOT_INVALID_NODE;
    bs->nav.goal_node    = BOT_INVALID_NODE;
    bs->nav.path_valid   = false;
    bs->nav.arrived_dist = 32.0f;

    /* Combat defaults */
    bs->combat.target         = NULL;
    bs->combat.target_visible = false;
    bs->combat.engagement_range =
        (team == TEAM_ALIEN) ? 200.0f : 400.0f;

    /* AI state */
    bs->ai_state         = BOTSTATE_IDLE;
    bs->prev_ai_state    = BOTSTATE_IDLE;
    bs->state_enter_time = 0.0f;

    /* Link bot_state back through the edict */
    ent->client->is_bot    = true;
    ent->client->bot_state = bs;
    ent->client->team      = team;

    bs->initialized = true;
    num_bots++;

    gi.dprintf("Bot_Connect: added bot '%s' (team=%d skill=%.2f)\n",
               bs->name, team, skill);

    return bs;
}

/* -----------------------------------------------------------------------
   Bot_Disconnect
   ----------------------------------------------------------------------- */
void Bot_Disconnect(edict_t *ent)
{
    bot_state_t *bs;

    if (!ent)
        return;

    bs = Bot_GetState(ent);
    if (!bs)
        return;

    gi.dprintf("Bot_Disconnect: removing bot '%s'\n", bs->name);

    if (ent->client) {
        ent->client->is_bot    = false;
        ent->client->bot_state = NULL;
    }

    memset(bs, 0, sizeof(*bs));
    bs->in_use = false;

    if (num_bots > 0)
        num_bots--;
}

/* -----------------------------------------------------------------------
   Bot_Frame
   Called every server frame (from G_RunFrame in g_main.c).
   Iterates all active bots and calls Bot_Think for each.
   ----------------------------------------------------------------------- */
void Bot_Frame(void)
{
    int i;

    for (i = 0; i < MAX_BOTS; i++) {
        bot_state_t *bs = &g_bots[i];

        if (!bs->in_use || !bs->initialized)
            continue;

        if (!bs->ent || !bs->ent->inuse || !bs->ent->client)
            continue;

        /* Rate-limit: only think at BOT_THINK_RATE */
        if (level.time < bs->next_think_time)
            continue;

        bs->next_think_time = level.time + bs->think_interval;

        Bot_Think(bs);
    }
}

/* -----------------------------------------------------------------------
   Bot_Think
   Main per-bot think function. Dispatches to the appropriate state handler.
   ----------------------------------------------------------------------- */
void Bot_Think(bot_state_t *bs)
{
    if (!bs || !bs->ent || !bs->ent->inuse)
        return;

    /* Dispatch to current AI state handler */
    switch (bs->ai_state) {
    case BOTSTATE_IDLE:
        Bot_StateIdle(bs);
        break;
    case BOTSTATE_PATROL:
        Bot_StatePatrol(bs);
        break;
    case BOTSTATE_HUNT:
        Bot_StateHunt(bs);
        break;
    case BOTSTATE_COMBAT:
        Bot_StateCombat(bs);
        break;
    case BOTSTATE_FLEE:
        Bot_StateFlee(bs);
        break;
    case BOTSTATE_DEFEND:
        Bot_StateDefend(bs);
        break;
    case BOTSTATE_ESCORT:
        Bot_StateEscort(bs);
        break;
    case BOTSTATE_BUILD:
        Bot_StateBuild(bs);
        break;
    default:
        bs->ai_state = BOTSTATE_IDLE;
        break;
    }
}

/* =======================================================================
   State handlers (stubs — full implementations in per-state modules)
   ======================================================================= */

static void Bot_SetState(bot_state_t *bs, bot_ai_state_t new_state)
{
    if (bs->ai_state == new_state)
        return;
    bs->prev_ai_state    = bs->ai_state;
    bs->ai_state         = new_state;
    bs->state_enter_time = level.time;
}

static void Bot_StateIdle(bot_state_t *bs)
{
    /* Scan for enemies; if found, transition to HUNT or COMBAT */
    if (bs->combat.target_visible) {
        Bot_SetState(bs, BOTSTATE_COMBAT);
        return;
    }
    if (bs->combat.target != NULL) {
        Bot_SetState(bs, BOTSTATE_HUNT);
        return;
    }

    /* Engineers / Breeders with build points look for build opportunities */
    if (Gloom_ClassCanBuild(bs->gloom_class) && bs->build.build_points > 0) {
        Bot_SetState(bs, BOTSTATE_BUILD);
        return;
    }

    /* Default: start patrolling after a short pause */
    if (level.time - bs->state_enter_time > 2.0f)
        Bot_SetState(bs, BOTSTATE_PATROL);
}

static void Bot_StatePatrol(bot_state_t *bs)
{
    /* Interrupt patrol if an enemy becomes visible */
    if (bs->combat.target_visible) {
        Bot_SetState(bs, BOTSTATE_COMBAT);
        return;
    }
    /* Navigation logic handled by bot_nav.c */
}

static void Bot_StateHunt(bot_state_t *bs)
{
    /* Navigate towards the last known enemy position */
    if (bs->combat.target_visible) {
        Bot_SetState(bs, BOTSTATE_COMBAT);
        return;
    }
    if (bs->combat.target == NULL) {
        Bot_SetState(bs, BOTSTATE_IDLE);
        return;
    }
    /* Forget the enemy if too old */
    if (level.time - bs->combat.target_last_seen > BOT_ENEMY_MEMORY_TIME) {
        bs->combat.target = NULL;
        Bot_SetState(bs, BOTSTATE_IDLE);
    }
}

static void Bot_StateCombat(bot_state_t *bs)
{
    /* If target is dead or gone, return to idle */
    if (!bs->combat.target || !bs->combat.target->inuse) {
        bs->combat.target         = NULL;
        bs->combat.target_visible = false;
        Bot_SetState(bs, BOTSTATE_IDLE);
        return;
    }

    /* If target is no longer visible, switch to hunt */
    if (!bs->combat.target_visible) {
        Bot_SetState(bs, BOTSTATE_HUNT);
        return;
    }

    /* Flee if critically low on health */
    if (bs->ent->health < (int)(bs->ent->max_health * 0.2f)) {
        Bot_SetState(bs, BOTSTATE_FLEE);
        return;
    }

    /* Combat logic (aim, fire) handled by bot_combat.c */
}

static void Bot_StateFlee(bot_state_t *bs)
{
    /* Stop fleeing once health recovers */
    if (bs->ent->health > (int)(bs->ent->max_health * 0.5f))
        Bot_SetState(bs, BOTSTATE_IDLE);
    /* Navigation handled by bot_nav.c */
}

static void Bot_StateDefend(bot_state_t *bs)
{
    /* Attack if an enemy is visible */
    if (bs->combat.target_visible) {
        Bot_SetState(bs, BOTSTATE_COMBAT);
        return;
    }
}

static void Bot_StateEscort(bot_state_t *bs)
{
    /* Follow designated teammate; fight if attacked */
    if (bs->combat.target_visible) {
        Bot_SetState(bs, BOTSTATE_COMBAT);
        return;
    }
}

static void Bot_StateBuild(bot_state_t *bs)
{
    /* Abort build if attacked */
    if (bs->combat.target_visible) {
        bs->build.building = false;
        Bot_SetState(bs, BOTSTATE_COMBAT);
        return;
    }
    /* Build logic handled by bot_build.c */
}

/* =======================================================================
   Helpers
   ======================================================================= */

static void Bot_ChooseClass(bot_state_t *bs, int team, gloom_class_t requested_class)
{
    /* Validate that requested class matches the team; fall back otherwise */
    if (requested_class < GLOOM_CLASS_MAX &&
        Gloom_ClassTeam(requested_class) == team) {
        bs->gloom_class = requested_class;
    } else {
        bs->gloom_class = (team == TEAM_HUMAN)
            ? GLOOM_CLASS_MARINE
            : GLOOM_CLASS_STINGER;
    }
}

/* =======================================================================
   Server console command handlers
   ======================================================================= */

/*
 * "sv addbot [team] [class] [skill]"
 */
static void SV_AddBot_f(void)
{
    int         team  = TEAM_HUMAN;
    float       skill = 0.5f;
    int         argc;
    const char *arg;
    edict_t    *ent;
    int         i;

    argc = gi.argc();

    if (argc >= 2) {
        arg = gi.argv(1);
        if (Q_stricmp((char *)arg, "alien") == 0)
            team = TEAM_ALIEN;
        else if (Q_stricmp((char *)arg, "human") == 0)
            team = TEAM_HUMAN;
        else
            team = atoi(arg) ? TEAM_ALIEN : TEAM_HUMAN;
    }

    if (argc >= 4)
        skill = (float)atof(gi.argv(3));

    /* Find a free client slot */
    ent = NULL;
    for (i = 0; i < (int)globals.max_edicts; i++) {
        if (!globals.edicts[i].inuse) {
            ent = &globals.edicts[i];
            break;
        }
    }

    if (!ent) {
        gi.dprintf("addbot: no free edict slots\n");
        return;
    }

    G_InitEdict(ent);
    ent->client = (gclient_t *)gi.TagMalloc(sizeof(gclient_t), TAG_GAME);
    if (!ent->client) {
        gi.dprintf("addbot: out of memory\n");
        return;
    }
    memset(ent->client, 0, sizeof(gclient_t));

    Bot_Connect(ent, team, skill);
}

/*
 * "sv removebot [name]"
 */
static void SV_RemoveBot_f(void)
{
    const char *name;
    int         i;

    if (gi.argc() < 2) {
        gi.dprintf("Usage: sv removebot <name>\n");
        return;
    }

    name = gi.argv(1);

    for (i = 0; i < MAX_BOTS; i++) {
        if (g_bots[i].in_use &&
            Q_stricmp(g_bots[i].name, (char *)name) == 0) {
            Bot_Disconnect(g_bots[i].ent);
            return;
        }
    }

    gi.dprintf("removebot: bot '%s' not found\n", name);
}

/*
 * "sv listbots"
 */
static void SV_ListBots_f(void)
{
    int i, count = 0;

    gi.dprintf("Active bots (%d / %d):\n", num_bots, MAX_BOTS);
    for (i = 0; i < MAX_BOTS; i++) {
        if (g_bots[i].in_use) {
            gi.dprintf("  [%2d] %-20s team=%d class=%-10s skill=%.2f state=%d\n",
                       i,
                       g_bots[i].name,
                       g_bots[i].team,
                       Gloom_ClassName(g_bots[i].gloom_class),
                       g_bots[i].skill,
                       (int)g_bots[i].ai_state);
            count++;
        }
    }
    if (count == 0)
        gi.dprintf("  (none)\n");
}
