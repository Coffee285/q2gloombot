/*
 * bot_mapcontrol.c -- map control and zone tracking for q2gloombot
 *
 * Divides the map into sectors using nav node clusters and tracks which
 * team controls each sector.  Provides route planning guidance and
 * spawn-point hunting assistance.
 *
 * ZONE CONTROL
 * ------------
 * A zone is "controlled" by a team when:
 *   - That team has structures (spawn points, turrets, eggs) in the zone, OR
 *   - That team has more bots present in the zone than the enemy
 *
 * ZONE STATES
 * -----------
 * ZONE_HUMAN    — clearly in human hands
 * ZONE_ALIEN    — clearly in alien hands
 * ZONE_CONTESTED — both teams active
 * ZONE_NEUTRAL  — no strong presence from either team
 *
 * SPAWN HUNTING
 * -------------
 * Bots actively search for hidden enemy spawn points by patrolling
 * zones classified as ZONE_NEUTRAL or ZONE_ENEMY.  When a spawn point
 * is found, it is reported to teammates via BotTeamwork_ShareEnemyPos.
 */

#include "bot_team.h"
#include "bot_strategy.h"
#include "../nav/bot_nodes.h"

/* -----------------------------------------------------------------------
   Constants
   ----------------------------------------------------------------------- */
#define MAPCTRL_MAX_ZONES      32     /* maximum tracked map sectors        */
#define MAPCTRL_ZONE_RADIUS    512.0f /* radius of a single zone            */
#define MAPCTRL_UPDATE_INTERVAL 5.0f  /* seconds between zone updates       */
#define MAPCTRL_CONTROL_BOTS   2      /* bots needed to claim zone control  */

/* -----------------------------------------------------------------------
   Zone state enum
   ----------------------------------------------------------------------- */
typedef enum {
    ZONE_NEUTRAL   = 0,
    ZONE_HUMAN     = 1,
    ZONE_ALIEN     = 2,
    ZONE_CONTESTED = 3
} zone_control_t;

/* -----------------------------------------------------------------------
   Zone record
   ----------------------------------------------------------------------- */
typedef struct {
    vec3_t         center;       /* centre point of the zone               */
    zone_control_t control;      /* current control state                  */
    int            human_count;  /* humans present in zone                 */
    int            alien_count;  /* aliens present in zone                 */
    qboolean       has_human_struct; /* human structure in zone            */
    qboolean       has_alien_struct; /* alien structure in zone            */
    qboolean       in_use;
} map_zone_t;

/* -----------------------------------------------------------------------
   Module state
   ----------------------------------------------------------------------- */
static map_zone_t  s_zones[MAPCTRL_MAX_ZONES];
static int         s_zone_count  = 0;
static float       s_next_update = 0.0f;

/* -----------------------------------------------------------------------
   BotMapControl_Init
   Seed initial zones from nav node positions (cluster around NAV_CAMP
   and NAV_AMBUSH nodes as zone centres).
   ----------------------------------------------------------------------- */
void BotMapControl_Init(void)
{
    int i;
    s_zone_count  = 0;
    s_next_update = 0.0f;

    memset(s_zones, 0, sizeof(s_zones));

    for (i = 0; i < nav_node_count && s_zone_count < MAPCTRL_MAX_ZONES; i++) {
        int j;
        qboolean too_close = false;

        if (nav_nodes[i].id == BOT_INVALID_NODE) continue;

        /* Only seed zones from key nodes */
        if (!(nav_nodes[i].flags & (NAV_CAMP | NAV_AMBUSH |
                                    NAV_TELEPORTER | NAV_EGG))) continue;

        /* Don't create zones too close to existing ones */
        for (j = 0; j < s_zone_count; j++) {
            vec3_t delta;
            float  dist;

            delta[0] = s_zones[j].center[0] - nav_nodes[i].origin[0];
            delta[1] = s_zones[j].center[1] - nav_nodes[i].origin[1];
            delta[2] = s_zones[j].center[2] - nav_nodes[i].origin[2];
            dist = (float)sqrt((double)(delta[0]*delta[0] + delta[1]*delta[1] +
                                        delta[2]*delta[2]));

            if (dist < MAPCTRL_ZONE_RADIUS) { too_close = true; break; }
        }

        if (!too_close) {
            s_zones[s_zone_count].center[0] = nav_nodes[i].origin[0];
            s_zones[s_zone_count].center[1] = nav_nodes[i].origin[1];
            s_zones[s_zone_count].center[2] = nav_nodes[i].origin[2];
            s_zones[s_zone_count].control   = ZONE_NEUTRAL;
            s_zones[s_zone_count].in_use    = true;
            s_zone_count++;
        }
    }
}

/* -----------------------------------------------------------------------
   BotMapControl_GetZoneForPos
   Return the index of the zone containing the given world position,
   or -1 if the position falls outside all known zones.
   ----------------------------------------------------------------------- */
static int GetZoneForPos(vec3_t pos)
{
    int   i;
    int   best    = -1;
    float best_d  = MAPCTRL_ZONE_RADIUS;

    for (i = 0; i < s_zone_count; i++) {
        vec3_t delta;
        float  dist;

        if (!s_zones[i].in_use) continue;

        delta[0] = s_zones[i].center[0] - pos[0];
        delta[1] = s_zones[i].center[1] - pos[1];
        delta[2] = s_zones[i].center[2] - pos[2];
        dist = (float)sqrt((double)(delta[0]*delta[0] + delta[1]*delta[1] +
                                    delta[2]*delta[2]));

        if (dist < best_d) {
            best_d = dist;
            best   = i;
        }
    }
    return best;
}

/* -----------------------------------------------------------------------
   BotMapControl_UpdateZones
   Walk all bots and count how many of each team are in each zone.
   Determine control state from counts and structure presence.
   ----------------------------------------------------------------------- */
static void UpdateZones(void)
{
    int i;

    /* Reset counts */
    for (i = 0; i < s_zone_count; i++) {
        s_zones[i].human_count = 0;
        s_zones[i].alien_count = 0;
        s_zones[i].has_human_struct = false;
        s_zones[i].has_alien_struct = false;
    }

    /* Count bots per zone */
    for (i = 0; i < MAX_BOTS; i++) {
        int zone_idx;
        bot_state_t *bs = &g_bots[i];
        if (!bs->in_use || !bs->ent || !bs->ent->inuse) continue;

        zone_idx = GetZoneForPos(bs->ent->s.origin);
        if (zone_idx < 0) continue;

        if (bs->team == TEAM_HUMAN)
            s_zones[zone_idx].human_count++;
        else
            s_zones[zone_idx].alien_count++;
    }

    /* Determine control */
    for (i = 0; i < s_zone_count; i++) {
        int h = s_zones[i].human_count + (s_zones[i].has_human_struct ? 2 : 0);
        int a = s_zones[i].alien_count + (s_zones[i].has_alien_struct ? 2 : 0);

        if (h == 0 && a == 0)
            s_zones[i].control = ZONE_NEUTRAL;
        else if (h >= MAPCTRL_CONTROL_BOTS && a == 0)
            s_zones[i].control = ZONE_HUMAN;
        else if (a >= MAPCTRL_CONTROL_BOTS && h == 0)
            s_zones[i].control = ZONE_ALIEN;
        else if (h > 0 && a > 0)
            s_zones[i].control = ZONE_CONTESTED;
        else
            s_zones[i].control = ZONE_NEUTRAL;
    }
}

/* -----------------------------------------------------------------------
   BotMapControl_Frame
   Called from BotTeam_Frame() when the update timer fires.
   ----------------------------------------------------------------------- */
void BotMapControl_Frame(void)
{
    if (level.time < s_next_update) return;
    s_next_update = level.time + MAPCTRL_UPDATE_INTERVAL;
    UpdateZones();
}

/* -----------------------------------------------------------------------
   BotMapControl_IsZoneContested
   Returns true if the zone containing the given position is contested.
   ----------------------------------------------------------------------- */
qboolean BotMapControl_IsZoneContested(vec3_t pos)
{
    int idx = GetZoneForPos(pos);
    if (idx < 0) return false;
    return (s_zones[idx].control == ZONE_CONTESTED) ? true : false;
}

/* -----------------------------------------------------------------------
   BotMapControl_GetEnemyControlledZone
   Return the centre of the nearest enemy-controlled zone.
   team = the calling bot's team (1=HUMAN, 2=ALIEN).
   Returns true on success, fills *out_pos.
   ----------------------------------------------------------------------- */
qboolean BotMapControl_GetEnemyControlledZone(int team, vec3_t from,
                                               vec3_t out_pos)
{
    int   i;
    int   best   = -1;
    float best_d = 9999999.0f;
    zone_control_t enemy_control = (team == TEAM_HUMAN) ? ZONE_ALIEN : ZONE_HUMAN;

    for (i = 0; i < s_zone_count; i++) {
        vec3_t delta;
        float  dist;

        if (!s_zones[i].in_use) continue;
        if (s_zones[i].control != enemy_control) continue;

        delta[0] = s_zones[i].center[0] - from[0];
        delta[1] = s_zones[i].center[1] - from[1];
        delta[2] = s_zones[i].center[2] - from[2];
        dist = (float)sqrt((double)(delta[0]*delta[0] + delta[1]*delta[1] +
                                    delta[2]*delta[2]));

        if (dist < best_d) {
            best_d = dist;
            best   = i;
        }
    }

    if (best < 0) return false;

    out_pos[0] = s_zones[best].center[0];
    out_pos[1] = s_zones[best].center[1];
    out_pos[2] = s_zones[best].center[2];
    return true;
}
