/*
 * bot_main.c -- bot lifecycle management for q2gloombot
 *
 * PUBLIC API
 * ----------
 *  Bot_Init()          called once at DLL load; initialises subsystems
 *  Bot_Shutdown()      called on DLL unload; cleans up all bots
 *  Bot_Connect()       spawns a new bot edict into the game
 *  Bot_Disconnect()    removes a bot from the game
 *  Bot_Frame()         called every server frame; iterates and ticks bots
 *  Bot_Think()         per-bot think; runs the AI state machine
 *
 * SERVER CONSOLE COMMANDS
 * -----------------------
 *  sv addbot [team] [class] [skill]  — spawn a bot
 *  sv removebot <name>               — remove a named bot
 *  sv listbots                       — list all active bots
 *
 * AI BEHAVIOUR OVERVIEW
 * ---------------------
 * BUILDER BOTS (both teams) — ordered priority:
 *   1. Verify / build the primary structure (Reactor / Overmind).
 *   2. Verify / build spawn points (Telenodes / Eggs).
 *   3. Expand and repair perimeter defenses.
 *
 * ALIEN COMBAT BOTS — combat target priority:
 *   Reactor → human Builders → automated Turrets → Marines
 *   Navigation prefers walls/ceilings to avoid turret fire.
 *   Kamikaze class closes to melee range and detonates on buildings.
 *
 * HUMAN COMBAT BOTS — combat target priority:
 *   Overmind → alien Grangers → alien structures → alien players
 *   Engage at maximum effective range to exploit firepower advantage.
 *   Protect Builder teammates; fall back toward the Reactor if overwhelmed.
 */

#include "bot.h"
#include "bot_safety.h"
#include "bot_cvars.h"
#include "bot_debug.h"
#include "bot_nav.h"
#include "bot_combat.h"
#include "bot_team.h"
#include "bot_strategy.h"
#include "bot_build.h"
#include "bot_upgrade.h"
#include "bot_personality.h"
#include "bot_humanize.h"

/* -----------------------------------------------------------------------
   Module globals
   ----------------------------------------------------------------------- */
bot_state_t g_bots[MAX_BOTS];
int         num_bots = 0;

/* Forward declarations — state handlers */
static void Bot_StateIdle(bot_state_t *bs);
static void Bot_StatePatrol(bot_state_t *bs);
static void Bot_StateHunt(bot_state_t *bs);
static void Bot_StateCombat(bot_state_t *bs);
static void Bot_StateFlee(bot_state_t *bs);
static void Bot_StateDefend(bot_state_t *bs);
static void Bot_StateEscort(bot_state_t *bs);
static void Bot_StateBuild(bot_state_t *bs);
static void Bot_StateEvolve(bot_state_t *bs);
static void Bot_StateUpgrade(bot_state_t *bs);

/* Forward declarations — helpers */
static void           Bot_SetState(bot_state_t *bs, bot_ai_state_t new_state);
static void           Bot_SetClass(bot_state_t *bs, int team,
                                    gloom_class_t requested_class);
static void           Bot_UpdateBuildPriority(bot_state_t *bs);
static build_priority_t Bot_AssessBuildNeeds(bot_state_t *bs);

/* -----------------------------------------------------------------------
   Bot_Init
   ----------------------------------------------------------------------- */
void Bot_Init(void)
{
    int i;

    gi.dprintf("Bot_Init: initialising bot subsystem\n");

    memset(g_bots, 0, sizeof(g_bots));
    num_bots = 0;

    for (i = 0; i < MAX_BOTS; i++) {
        g_bots[i].in_use    = false;
        g_bots[i].bot_index = i;
    }

    /* Initialise subsystems */
    BotCvars_Init();
    BotConfig_Init();
    BotAutofill_Init();
    BotDebug_Init();
    BotNav_Init();
    BotBuild_Init();
    BotTeam_Init();
    BotStrategy_Init();
    BotUpgrade_Init();

    gi.dprintf("GloomBot v%s initialized (%d slots)\n", GLOOMBOT_VERSION, MAX_BOTS);
}

/* -----------------------------------------------------------------------
   Bot_Shutdown
   ----------------------------------------------------------------------- */
void Bot_Shutdown(void)
{
    int i;

    gi.dprintf("Bot_Shutdown\n");

    for (i = 0; i < MAX_BOTS; i++) {
        if (g_bots[i].in_use)
            Bot_Disconnect(g_bots[i].ent);
    }
    num_bots = 0;
}

/* -----------------------------------------------------------------------
   Bot_Connect
   ----------------------------------------------------------------------- */
bot_state_t *Bot_Connect(edict_t *ent, int team, float skill)
{
    int          i;
    bot_state_t *bs = NULL;
    gloom_class_t cls;

    if (!ent || !ent->client) {
        gi.dprintf("Bot_Connect: NULL entity/client\n");
        return NULL;
    }

    if (num_bots >= MAX_BOTS) {
        gi.dprintf("Bot_Connect: max bots (%d) reached\n", MAX_BOTS);
        return NULL;
    }

    for (i = 0; i < MAX_BOTS; i++) {
        if (!g_bots[i].in_use) { bs = &g_bots[i]; break; }
    }
    if (!bs) {
        gi.dprintf("Bot_Connect: no free slots\n");
        return NULL;
    }

    memset(bs, 0, sizeof(*bs));
    bs->bot_index = i;
    bs->in_use    = true;
    bs->ent       = ent;

    /* Clamp skill [0, 1] */
    if (skill < 0.0f) skill = 0.0f;
    if (skill > 1.0f) skill = 1.0f;
    bs->skill = skill;

    /* Team */
    if (team != TEAM_HUMAN && team != TEAM_ALIEN)
        team = (num_bots % 2 == 0) ? TEAM_HUMAN : TEAM_ALIEN;
    bs->team = team;

    /* Default class: Alien bots start as Dretch (basic tier-1);
     * Human bots start as Marine_Light and accumulate credits. */
    cls = (team == TEAM_HUMAN) ? GLOOM_CLASS_MARINE_LIGHT : GLOOM_CLASS_DRETCH;
    Bot_SetClass(bs, team, cls);

    Com_sprintf(bs->name, sizeof(bs->name), "Bot_%s_%02d",
                Gloom_ClassName(bs->gloom_class), i);

    /* Timing */
    bs->think_interval  = BOT_THINK_RATE;
    bs->reaction_time   = 0.5f - (skill * 0.4f); /* 0.5s (novice) → 0.1s (expert) */
    bs->next_think_time = 0.0f;

    /* Resources */
    bs->credits = 0;
    bs->evos    = 0;

    /* Navigation */
    bs->nav.current_node = BOT_INVALID_NODE;
    bs->nav.goal_node    = BOT_INVALID_NODE;
    bs->nav.path_valid   = false;
    bs->nav.arrived_dist = 32.0f;
    bs->nav.wall_walking = false;

    /* Combat defaults */
    bs->combat.target         = NULL;
    bs->combat.target_visible = false;
    bs->combat.engagement_range =
        gloom_class_info[bs->gloom_class].preferred_range;

    /*
     * Asymmetric combat defaults:
     *  Aliens   — prefer cover and closing distance; avoid long sightlines
     *  Humans   — engage at maximum effective range
     */
    if (team == TEAM_ALIEN) {
        bs->combat.prefer_cover      = true;
        bs->combat.max_range_engage  = false;
    } else {
        bs->combat.prefer_cover      = false;
        bs->combat.max_range_engage  = true;
    }

    /* Build state */
    bs->build.priority      = BUILD_PRIORITY_NONE;
    bs->build.what_to_build = STRUCT_NONE;
    bs->build.build_points  = 0;
    bs->build.building      = false;

    /* AI state */
    bs->ai_state         = BOTSTATE_IDLE;
    bs->prev_ai_state    = BOTSTATE_IDLE;
    bs->state_enter_time = 0.0f;

    /* Link back through edict */
    ent->client->is_bot    = true;
    ent->client->bot_state = bs;
    ent->client->team      = team;

    /* Generate personality traits (seeded from bot name hash) */
    Bot_GeneratePersonality(bs);

    /* Initialise human-like behaviour state */
    Bot_Humanize_Init(bs);

    bs->initialized = true;
    num_bots++;

    gi.dprintf("Bot_Connect: '%s' (team=%d skill=%.2f class=%s)\n",
               bs->name, team, skill, Gloom_ClassName(bs->gloom_class));
    return bs;
}

/* -----------------------------------------------------------------------
   Bot_Disconnect
   ----------------------------------------------------------------------- */
void Bot_Disconnect(edict_t *ent)
{
    bot_state_t *bs;

    if (!ent) return;
    bs = Bot_GetState(ent);
    if (!bs) return;

    gi.dprintf("Bot_Disconnect: removing '%s'\n", bs->name);

    if (ent->client) {
        ent->client->is_bot    = false;
        ent->client->bot_state = NULL;
        gi.TagFree(ent->client);
        ent->client = NULL;
    }

    memset(bs, 0, sizeof(*bs));
    bs->in_use = false;

    if (num_bots > 0) num_bots--;
}

/* -----------------------------------------------------------------------
   Bot_Frame
   Called every server frame from G_RunFrame() in g_main.c.
   ----------------------------------------------------------------------- */
void Bot_Frame(void)
{
    int i;

    /* If bots are disabled via cvar, skip all processing */
    if (bot_enable && (int)bot_enable->value == 0)
        return;

    /* If bots are paused (debug), skip thinking */
    if (bot_paused)
        return;

    /* 0. Auto-fill population management */
    BotAutofill_Frame();

    /* 1. Per-frame global state updates */
    BotUpgrade_UpdateGameState();
    BotTeam_Frame();

    /* 2. Team strategy updates */
    BotStrategy_Frame();

    /* 3. Per-frame build structure tracking */
    BotBuild_UpdateStructures(TEAM_HUMAN);
    BotBuild_UpdateStructures(TEAM_ALIEN);

    /* 4. Individual bot think */
    for (i = 0; i < MAX_BOTS; i++) {
        bot_state_t *bs = &g_bots[i];

        if (!bs->in_use || !bs->initialized)
            continue;
        if (!bs->ent || !bs->ent->inuse || !bs->ent->client)
            continue;
        if (level.time < bs->next_think_time)
            continue;

        bs->next_think_time = level.time + bs->think_interval;
        Bot_Think(bs);
    }

    /* 5. Debug display updates */
    BotDebug_FrameUpdate();
}

/* -----------------------------------------------------------------------
   Bot_Think
   ----------------------------------------------------------------------- */
void Bot_Think(bot_state_t *bs)
{
    if (!bs || !bs->ent || !bs->ent->inuse) return;

    /* 1. Update awareness — scan for enemies, update memory */
    Bot_UpdateAwareness(bs);

    /* 2. Evaluate class upgrade — check if should change class */
    Bot_EvaluateClassUpgrade(bs);

    /* 3. Run class-specific behavior via AI state machine */
    Bot_RunClassBehavior(bs);

    /* 4. Update human-like behaviour (aim smoothing, drift, hesitation) */
    Bot_Humanize_Think(bs);

    /* 5. Finalize movement — apply movement to usercmd_t */
    Bot_FinalizeMovement(bs);

    /* 6. Update timers — advance all timing counters */
    Bot_UpdateTimers(bs);
}

/* =======================================================================
   Integration helpers
   ======================================================================= */

/* -----------------------------------------------------------------------
   Bot_UpdateAwareness  —  scan for enemies, update memory
   ----------------------------------------------------------------------- */
void Bot_UpdateAwareness(bot_state_t *bs)
{
    edict_t *target;

    /* Refresh cached base-state flags */
    if (bs->team == TEAM_HUMAN) {
        bs->build.reactor_exists = BotTeam_ReactorAlive();
        bs->build.spawn_count    = BotTeam_CountSpawnPoints(TEAM_HUMAN);
    } else {
        bs->build.overmind_exists = BotTeam_OvmindAlive();
        bs->build.spawn_count     = BotTeam_CountSpawnPoints(TEAM_ALIEN);
    }

    /* Pick the best target using combat priority system */
    target = BotCombat_PickTarget(bs);
    if (target) {
        bs->combat.target = target;
        bs->combat.target_visible = true;
        bs->combat.target_last_seen = level.time;
        VectorCopy(target->s.origin, bs->combat.target_last_pos);
    } else if (bs->combat.target) {
        /* Target no longer visible; keep memory for a while */
        bs->combat.target_visible = false;
        if (level.time - bs->combat.target_last_seen > BOT_ENEMY_MEMORY_TIME) {
            bs->combat.target = NULL;
        }
    }

    /* Update wall-walk state for aliens */
    if (bs->team == TEAM_ALIEN && Gloom_ClassCanWallWalk(bs->gloom_class))
        BotNav_UpdateWallWalk(bs);

    /* Update navigation node */
    if (bs->ent)
        bs->nav.current_node = BotNav_NearestNode(bs->ent->s.origin,
                                                    Bot_CanWallWalk(bs));

    BotDebug_Log(BOT_DEBUG_STATE, "%s: awareness update target=%s node=%d\n",
                 bs->name,
                 bs->combat.target ? "YES" : "no",
                 bs->nav.current_node);
}

/* -----------------------------------------------------------------------
   Bot_EvaluateClassUpgrade  —  check if should change class
   ----------------------------------------------------------------------- */
void Bot_EvaluateClassUpgrade(bot_state_t *bs)
{
    gloom_class_t old_class = bs->gloom_class;
    gloom_class_t new_class;

    /* Only evaluate when idle or patrolling — not in combat */
    if (bs->ai_state != BOTSTATE_IDLE && bs->ai_state != BOTSTATE_PATROL)
        return;

    /* Aliens with enough evos, or humans with enough credits */
    if (bs->team == TEAM_ALIEN) {
        gloom_class_t next = Gloom_NextEvolution(bs->gloom_class);
        if (next == GLOOM_CLASS_MAX)
            return;
        if (bs->evos < Gloom_NextEvoCost(bs->gloom_class))
            return;
    } else {
        /* Check if bot has any credits to potentially upgrade with */
        if (bs->credits <= 0)
            return;
        if (bs->class_upgrades > 0)
            return;  /* Already upgraded this life */
    }

    new_class = (gloom_class_t)Bot_ChooseClass(bs);
    if (new_class != old_class) {
        BotDebug_Log(BOT_DEBUG_UPGRADE,
                     "%s: upgrade %s -> %s\n",
                     bs->name,
                     Gloom_ClassName(old_class),
                     Gloom_ClassName(new_class));
    }
}

/* -----------------------------------------------------------------------
   Bot_RunClassBehavior  —  execute class-specific AI via state machine
   ----------------------------------------------------------------------- */
void Bot_RunClassBehavior(bot_state_t *bs)
{
    /* Consult strategy for role assignment */
    bot_role_t role = BotStrategy_GetRole(bs);

    /* Builder bots may need to override role for critical builds */
    if (Gloom_ClassCanBuild(bs->gloom_class)) {
        Bot_UpdateBuildPriority(bs);
        if (bs->build.priority <= BUILD_PRIORITY_SPAWNS &&
            bs->ai_state != BOTSTATE_COMBAT) {
            Bot_SetState(bs, BOTSTATE_BUILD);
        }
    }

    /* Role-based state nudges (when idle) */
    if (bs->ai_state == BOTSTATE_IDLE) {
        switch (role) {
        case ROLE_ATTACKER:
            /* Attackers go patrol aggressively */
            Bot_SetState(bs, BOTSTATE_PATROL);
            break;
        case ROLE_DEFENDER:
            Bot_SetState(bs, BOTSTATE_DEFEND);
            break;
        case ROLE_ESCORT:
            Bot_SetState(bs, BOTSTATE_ESCORT);
            break;
        default:
            break;
        }
    }

    /* Execute current AI state */
    switch (bs->ai_state) {
    case BOTSTATE_IDLE:    Bot_StateIdle(bs);    break;
    case BOTSTATE_PATROL:  Bot_StatePatrol(bs);  break;
    case BOTSTATE_HUNT:    Bot_StateHunt(bs);    break;
    case BOTSTATE_COMBAT:  Bot_StateCombat(bs);  break;
    case BOTSTATE_FLEE:    Bot_StateFlee(bs);    break;
    case BOTSTATE_DEFEND:  Bot_StateDefend(bs);  break;
    case BOTSTATE_ESCORT:  Bot_StateEscort(bs);  break;
    case BOTSTATE_BUILD:   Bot_StateBuild(bs);   break;
    case BOTSTATE_EVOLVE:  Bot_StateEvolve(bs);  break;
    case BOTSTATE_UPGRADE: Bot_StateUpgrade(bs); break;
    default:
        bs->ai_state = BOTSTATE_IDLE;
        break;
    }

    /* If in combat, run combat subsystem */
    if (bs->ai_state == BOTSTATE_COMBAT && bs->combat.target_visible)
        BotCombat_Think(bs);

    /* Navigation toward goal when moving */
    if (bs->ai_state == BOTSTATE_PATROL ||
        bs->ai_state == BOTSTATE_HUNT ||
        bs->ai_state == BOTSTATE_ESCORT)
        BotNav_MoveTowardGoal(bs);
}

/* -----------------------------------------------------------------------
   Bot_FinalizeMovement  —  apply movement to usercmd_t
   ----------------------------------------------------------------------- */
void Bot_FinalizeMovement(bot_state_t *bs)
{
    /*
     * Actual usercmd_t synthesis is handled by the engine interface.
     * This function is the integration point for converting bot nav/combat
     * decisions into the movement command sent to the server.
     */
    (void)bs;
}

/* -----------------------------------------------------------------------
   Bot_UpdateTimers  —  advance all timing counters
   ----------------------------------------------------------------------- */
void Bot_UpdateTimers(bot_state_t *bs)
{
    /* Expire stale enemy memories — iterate backwards to avoid index issues */
    int i;
    for (i = bs->enemy_memory_count - 1; i >= 0; i--) {
        if (bs->enemy_memory[i].ent &&
            level.time - bs->enemy_memory[i].last_seen > BOT_ENEMY_MEMORY_TIME) {
            /* Shift remaining entries down */
            int j;
            for (j = i; j < bs->enemy_memory_count - 1; j++)
                bs->enemy_memory[j] = bs->enemy_memory[j + 1];
            bs->enemy_memory_count--;
            memset(&bs->enemy_memory[bs->enemy_memory_count], 0,
                   sizeof(bot_enemy_memory_t));
        }
    }
}

/* =======================================================================
   Internal helper: state transition
   ======================================================================= */

static void Bot_SetState(bot_state_t *bs, bot_ai_state_t new_state)
{
    if (bs->ai_state == new_state) return;

    BotDebug_Log(BOT_DEBUG_STATE, "%s: %s -> %s\n",
                 bs->name,
                 BotDebug_StateName(bs->ai_state),
                 BotDebug_StateName(new_state));

    bs->prev_ai_state    = bs->ai_state;
    bs->ai_state         = new_state;
    bs->state_enter_time = level.time;
}

/* =======================================================================
   Build-priority assessment
   (Full structure-scan logic will be in bot_build.c)
   ======================================================================= */

/*
 * Determine the highest-priority build task for this builder bot.
 *
 * Priority order (both teams):
 *   1. CRITICAL  — primary structure (Reactor / Overmind) is absent
 *   2. SPAWNS    — no spawn points (Telenodes / Eggs) available
 *   3. DEFENSE   — fewer than desired perimeter defenses
 *   4. REPAIR    — existing structures are damaged
 */
static build_priority_t Bot_AssessBuildNeeds(bot_state_t *bs)
{
    /* Primary structure: if the Reactor or Overmind is gone, build NOW */
    if (!bs->build.reactor_exists && bs->team == TEAM_HUMAN)
        return BUILD_PRIORITY_CRITICAL;
    if (!bs->build.overmind_exists && bs->team == TEAM_ALIEN)
        return BUILD_PRIORITY_CRITICAL;

    /* Spawn points: team needs at least one */
    if (bs->build.spawn_count == 0)
        return BUILD_PRIORITY_SPAWNS;

    /* Delegate remaining priority decisions to bot_build.c which walks
     * the full build-order table and checks repair needs. */
    BotBuild_ChooseNext(bs);
    return bs->build.priority;
}

static void Bot_UpdateBuildPriority(bot_state_t *bs)
{
    build_priority_t urgent = Bot_AssessBuildNeeds(bs);

    /* Critical and spawn needs override the build-order table result */
    if (urgent == BUILD_PRIORITY_CRITICAL) {
        bs->build.priority      = BUILD_PRIORITY_CRITICAL;
        bs->build.what_to_build = Gloom_PrimaryStruct(bs->team);
    } else if (urgent == BUILD_PRIORITY_SPAWNS) {
        bs->build.priority      = BUILD_PRIORITY_SPAWNS;
        bs->build.what_to_build = Gloom_SpawnStruct(bs->team);
    }
    /* Otherwise BotBuild_ChooseNext (called in Bot_AssessBuildNeeds)
     * has already set bs->build.priority and bs->build.what_to_build */
}

/* =======================================================================
   AI state handlers
   ======================================================================= */

static void Bot_StateIdle(bot_state_t *bs)
{
    /* Combat interrupt: enemy in sight */
    if (bs->combat.target_visible) {
        Bot_SetState(bs, BOTSTATE_COMBAT);
        return;
    }

    /* Combat interrupt: enemy position remembered */
    if (bs->combat.target != NULL) {
        Bot_SetState(bs, BOTSTATE_HUNT);
        return;
    }

    /* Builder bots assess base needs first */
    if (Gloom_ClassCanBuild(bs->gloom_class)) {
        Bot_UpdateBuildPriority(bs);
        if (bs->build.priority < BUILD_PRIORITY_NONE) {
            Bot_SetState(bs, BOTSTATE_BUILD);
            return;
        }
    }

    /* Aliens with enough evos head to the Overmind to evolve */
    if (bs->team == TEAM_ALIEN && !Gloom_ClassCanBuild(bs->gloom_class)) {
        gloom_class_t next = Gloom_NextEvolution(bs->gloom_class);
        if (next != GLOOM_CLASS_MAX && bs->evos >= Gloom_NextEvoCost(bs->gloom_class)) {
            Bot_SetState(bs, BOTSTATE_EVOLVE);
            return;
        }
    }

    /* Humans with enough credits head to the Armory */
    if (bs->team == TEAM_HUMAN && !Gloom_ClassCanBuild(bs->gloom_class)) {
        if (bs->credits >= gloom_class_info[bs->gloom_class].credit_cost &&
            bs->class_upgrades == 0) {
            Bot_SetState(bs, BOTSTATE_UPGRADE);
            return;
        }
    }

    /* Default: begin patrolling after a short pause */
    if (level.time - bs->state_enter_time > 2.0f)
        Bot_SetState(bs, BOTSTATE_PATROL);
}

static void Bot_StatePatrol(bot_state_t *bs)
{
    if (bs->combat.target_visible) {
        Bot_SetState(bs, BOTSTATE_COMBAT);
        return;
    }
    if (bs->combat.target != NULL) {
        Bot_SetState(bs, BOTSTATE_HUNT);
        return;
    }
    /* Navigation along patrol route handled by bot_nav.c */
}

static void Bot_StateHunt(bot_state_t *bs)
{
    /* Found the target again */
    if (bs->combat.target_visible) {
        Bot_SetState(bs, BOTSTATE_COMBAT);
        return;
    }
    /* Target gone / cleaned up */
    if (!bs->combat.target || !bs->combat.target->inuse) {
        bs->combat.target = NULL;
        Bot_SetState(bs, BOTSTATE_IDLE);
        return;
    }
    /* Memory expired */
    if (level.time - bs->combat.target_last_seen > BOT_ENEMY_MEMORY_TIME) {
        bs->combat.target = NULL;
        Bot_SetState(bs, BOTSTATE_IDLE);
        return;
    }
    /* Navigation toward last known position handled by bot_nav.c */
}

static void Bot_StateCombat(bot_state_t *bs)
{
    /* Target destroyed or gone */
    if (!bs->combat.target || !bs->combat.target->inuse) {
        bs->combat.target         = NULL;
        bs->combat.target_visible = false;
        Bot_SetState(bs, BOTSTATE_IDLE);
        return;
    }

    /* Lost sight — switch to hunt */
    if (!bs->combat.target_visible) {
        Bot_SetState(bs, BOTSTATE_HUNT);
        return;
    }

    /* Critical health → flee (threshold modified by personality) */
    if (bs->ent->health <= (int)(bs->ent->max_health * Bot_FleeHealthThreshold(bs))) {
        Bot_SetState(bs, BOTSTATE_FLEE);
        return;
    }

    /*
     * Alien bots: close the distance using walls/cover.
     * Human bots: maintain maximum effective range.
     * Weapon-fire and movement handled by bot_combat.c.
     */
}

static void Bot_StateFlee(bot_state_t *bs)
{
    /* Recovered enough health to fight again */
    if (bs->ent->health >= (int)(bs->ent->max_health * 0.5f))
        Bot_SetState(bs, BOTSTATE_IDLE);

    /* Human bots flee toward the Reactor / Medistation.
     * Alien bots flee to any nearby Egg to re-spawn/heal. */
}

static void Bot_StateDefend(bot_state_t *bs)
{
    if (bs->combat.target_visible)
        Bot_SetState(bs, BOTSTATE_COMBAT);
}

static void Bot_StateEscort(bot_state_t *bs)
{
    /* Protect the designated teammate (usually a Builder) */
    if (bs->combat.target_visible)
        Bot_SetState(bs, BOTSTATE_COMBAT);
}

/*
 * Bot_StateBuild
 *
 * Builder bot priority:
 *   1. Reactor / Overmind  — if absent, build immediately before anything else.
 *   2. Spawn points        — Telenodes (human) / Eggs (alien).
 *   3. Defenses            — Turrets / Teslas / Acid Tubes / Traps.
 *   4. Repair              — restore damaged structures.
 *
 * Each priority level transitions to the next only once the current need
 * is satisfied.
 */
static void Bot_StateBuild(bot_state_t *bs)
{
    /* Abort if attacked and not in the middle of a critical build */
    if (bs->combat.target_visible &&
        bs->build.priority != BUILD_PRIORITY_CRITICAL) {
        bs->build.building = false;
        Bot_SetState(bs, BOTSTATE_COMBAT);
        return;
    }

    /* Re-assess in case something changed since we entered this state */
    Bot_UpdateBuildPriority(bs);

    if (bs->build.priority == BUILD_PRIORITY_NONE) {
        bs->build.building = false;
        Bot_SetState(bs, BOTSTATE_IDLE);
        return;
    }

    /* Actual placement / pathing is handled by bot_build.c */
}

/*
 * Bot_StateEvolve
 * Alien bot travels to the Overmind and spends evos to evolve.
 */
static void Bot_StateEvolve(bot_state_t *bs)
{
    gloom_class_t next = Gloom_NextEvolution(bs->gloom_class);

    /* If Overmind is gone, can't evolve — go fight instead */
    if (!bs->build.overmind_exists) {
        Bot_SetState(bs, BOTSTATE_IDLE);
        return;
    }

    /* If attacked, fight first — unless almost at the Overmind */
    if (bs->combat.target_visible) {
        Bot_SetState(bs, BOTSTATE_COMBAT);
        return;
    }

    if (next == GLOOM_CLASS_MAX || bs->evos < Gloom_NextEvoCost(bs->gloom_class)) {
        Bot_SetState(bs, BOTSTATE_IDLE);
        return;
    }

    /* Actual evolution (navigation to Overmind, stat change) in bot_nav.c
     * and resolved when the bot edict reaches the Overmind entity. */
}

/*
 * Bot_StateUpgrade
 * Human bot travels to the Armory to purchase a better weapon / loadout.
 */
static void Bot_StateUpgrade(bot_state_t *bs)
{
    /* Interrupted by enemy contact */
    if (bs->combat.target_visible) {
        Bot_SetState(bs, BOTSTATE_COMBAT);
        return;
    }

    /* If Reactor is offline, Armory is unpowered — can't upgrade */
    if (!bs->build.reactor_exists) {
        Bot_SetState(bs, BOTSTATE_DEFEND);
        return;
    }

    /* Actual purchase handled when bot reaches Armory entity. */
}

/* =======================================================================
   Bot class selection
   ======================================================================= */

static void Bot_SetClass(bot_state_t *bs, int team, gloom_class_t requested_class)
{
    if (requested_class < GLOOM_CLASS_MAX &&
        Gloom_ClassTeam(requested_class) == team) {
        bs->gloom_class = requested_class;
    } else {
        bs->gloom_class = (team == TEAM_HUMAN)
            ? GLOOM_CLASS_MARINE_LIGHT
            : GLOOM_CLASS_DRETCH;
    }
    bs->combat.engagement_range =
        gloom_class_info[bs->gloom_class].preferred_range;
}
