/*
 * q_shared.c -- functions shared between engine and game module
 *
 * Provides implementations of the math, string, and utility helpers
 * declared in q_shared.h.  Compiled into the game DLL so it is
 * self-contained and does not depend on the Quake 2 engine to supply
 * these symbols at load time.
 */

#include "q_shared.h"

#include <math.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* =========================================================================
   Globals
   ========================================================================= */

vec3_t vec3_origin = {0.0f, 0.0f, 0.0f};

/* =========================================================================
   Vector math
   ========================================================================= */

void VectorMA(vec3_t veca, float scale, vec3_t vecb, vec3_t vecc)
{
    vecc[0] = veca[0] + scale * vecb[0];
    vecc[1] = veca[1] + scale * vecb[1];
    vecc[2] = veca[2] + scale * vecb[2];
}

vec_t _DotProduct(vec3_t v1, vec3_t v2)
{
    return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

void _VectorSubtract(vec3_t veca, vec3_t vecb, vec3_t out)
{
    out[0] = veca[0] - vecb[0];
    out[1] = veca[1] - vecb[1];
    out[2] = veca[2] - vecb[2];
}

void _VectorAdd(vec3_t veca, vec3_t vecb, vec3_t out)
{
    out[0] = veca[0] + vecb[0];
    out[1] = veca[1] + vecb[1];
    out[2] = veca[2] + vecb[2];
}

void _VectorCopy(vec3_t in, vec3_t out)
{
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
}

void ClearBounds(vec3_t mins, vec3_t maxs)
{
    mins[0] = mins[1] = mins[2] =  99999.0f;
    maxs[0] = maxs[1] = maxs[2] = -99999.0f;
}

void AddPointToBounds(vec3_t v, vec3_t mins, vec3_t maxs)
{
    int i;
    for (i = 0; i < 3; i++) {
        if (v[i] < mins[i]) mins[i] = v[i];
        if (v[i] > maxs[i]) maxs[i] = v[i];
    }
}

int VectorCompare(vec3_t v1, vec3_t v2)
{
    return (v1[0] == v2[0] && v1[1] == v2[1] && v1[2] == v2[2]);
}

vec_t VectorLength(vec3_t v)
{
    return (vec_t)sqrt((double)(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]));
}

void CrossProduct(vec3_t v1, vec3_t v2, vec3_t cross)
{
    cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
    cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
    cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

vec_t VectorNormalize(vec3_t v)
{
    float len = VectorLength(v);
    if (len) {
        float inv = 1.0f / len;
        v[0] *= inv;
        v[1] *= inv;
        v[2] *= inv;
    }
    return len;
}

vec_t VectorNormalize2(vec3_t v, vec3_t out)
{
    float len = VectorLength(v);
    if (len) {
        float inv = 1.0f / len;
        out[0] = v[0] * inv;
        out[1] = v[1] * inv;
        out[2] = v[2] * inv;
    } else {
        out[0] = out[1] = out[2] = 0.0f;
    }
    return len;
}

void VectorInverse(vec3_t v)
{
    v[0] = -v[0];
    v[1] = -v[1];
    v[2] = -v[2];
}

void VectorScale(vec3_t in, vec_t scale, vec3_t out)
{
    out[0] = in[0] * scale;
    out[1] = in[1] * scale;
    out[2] = in[2] * scale;
}

int Q_log2(int val)
{
    int answer = 0;
    while ((val >>= 1) > 0)
        answer++;
    return answer;
}

/* =========================================================================
   Angle / rotation helpers
   ========================================================================= */

void AngleVectors(vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
    float angle;
    float sr, sp, sy, cr, cp, cy;

    angle = (float)(angles[YAW] * (M_PI * 2.0 / 360.0));
    sy = (float)sin(angle);
    cy = (float)cos(angle);

    angle = (float)(angles[PITCH] * (M_PI * 2.0 / 360.0));
    sp = (float)sin(angle);
    cp = (float)cos(angle);

    angle = (float)(angles[ROLL] * (M_PI * 2.0 / 360.0));
    sr = (float)sin(angle);
    cr = (float)cos(angle);

    if (forward) {
        forward[0] = cp * cy;
        forward[1] = cp * sy;
        forward[2] = -sp;
    }
    if (right) {
        right[0] =  sr * sp * cy + cr * sy;
        right[1] =  sr * sp * sy - cr * cy;
        right[2] =  sr * cp;
    }
    if (up) {
        up[0] = cr * sp * cy - sr * sy;
        up[1] = cr * sp * sy + sr * cy;
        up[2] = cr * cp;
    }
}

float anglemod(float a)
{
    a = (float)(360.0 / 65536.0) * ((int)(a * (65536.0 / 360.0)) & 65535);
    return a;
}

float LerpAngle(float a1, float a2, float frac)
{
    if (a1 - a2 > 180.0f)  a1 -= 360.0f;
    if (a1 - a2 < -180.0f) a1 += 360.0f;
    return a1 + frac * (a2 - a1);
}

int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
    float dist1, dist2;
    int sides = 0;

    switch (p->signbits) {
    case 0:
        dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
        dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
        break;
    case 1:
        dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
        dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
        break;
    case 2:
        dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
        dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
        break;
    case 3:
        dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
        dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
        break;
    case 4:
        dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
        dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
        break;
    case 5:
        dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
        dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
        break;
    case 6:
        dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
        dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
        break;
    case 7:
        dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
        dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
        break;
    default:
        dist1 = dist2 = 0.0f;
        break;
    }

    if (dist1 >= p->dist) sides  = 1;
    if (dist2 < p->dist)  sides |= 2;
    return sides;
}

void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal)
{
    float d = DotProduct(p, normal);
    dst[0] = p[0] - d * normal[0];
    dst[1] = p[1] - d * normal[1];
    dst[2] = p[2] - d * normal[2];
}

void PerpendicularVector(vec3_t dst, const vec3_t src)
{
    int pos, i;
    float minelem = 1.0f;
    vec3_t tempvec;

    pos = 0;
    for (i = 0; i < 3; i++) {
        float v = (float)fabs(src[i]);
        if (v < minelem) {
            pos = i;
            minelem = v;
        }
    }
    tempvec[0] = tempvec[1] = tempvec[2] = 0.0f;
    tempvec[pos] = 1.0f;

    ProjectPointOnPlane(dst, tempvec, src);
    VectorNormalize(dst);
}

void RotatePointAroundVector(vec3_t dst, const vec3_t dir,
                             const vec3_t point, float degrees)
{
    float m[3][3], im[3][3], zrot[3][3], tmpmat[3][3], rot[3][3];
    vec3_t vr, vup, vf;
    int i;
    float rad;

    vf[0] = dir[0]; vf[1] = dir[1]; vf[2] = dir[2];
    PerpendicularVector(vr, dir);
    CrossProduct(vr, vf, vup);

    m[0][0] = vr[0];  m[1][0] = vup[0];  m[2][0] = vf[0];
    m[0][1] = vr[1];  m[1][1] = vup[1];  m[2][1] = vf[1];
    m[0][2] = vr[2];  m[1][2] = vup[2];  m[2][2] = vf[2];

    memcpy(im, m, sizeof(im));
    im[0][1] = m[1][0]; im[0][2] = m[2][0];
    im[1][0] = m[0][1]; im[1][2] = m[2][1];
    im[2][0] = m[0][2]; im[2][1] = m[1][2];

    memset(zrot, 0, sizeof(zrot));
    zrot[2][2] = 1.0f;
    rad = (float)(degrees * (M_PI / 180.0));
    zrot[0][0] =  (float)cos(rad);
    zrot[0][1] =  (float)sin(rad);
    zrot[1][0] = -(float)sin(rad);
    zrot[1][1] =  (float)cos(rad);

    /* tmpmat = m * zrot */
    for (i = 0; i < 3; i++) {
        tmpmat[i][0] = m[i][0]*zrot[0][0] + m[i][1]*zrot[1][0] + m[i][2]*zrot[2][0];
        tmpmat[i][1] = m[i][0]*zrot[0][1] + m[i][1]*zrot[1][1] + m[i][2]*zrot[2][1];
        tmpmat[i][2] = m[i][0]*zrot[0][2] + m[i][1]*zrot[1][2] + m[i][2]*zrot[2][2];
    }
    /* rot = tmpmat * im */
    for (i = 0; i < 3; i++) {
        rot[i][0] = tmpmat[i][0]*im[0][0] + tmpmat[i][1]*im[1][0] + tmpmat[i][2]*im[2][0];
        rot[i][1] = tmpmat[i][0]*im[0][1] + tmpmat[i][1]*im[1][1] + tmpmat[i][2]*im[2][1];
        rot[i][2] = tmpmat[i][0]*im[0][2] + tmpmat[i][1]*im[1][2] + tmpmat[i][2]*im[2][2];
    }

    dst[0] = rot[0][0]*point[0] + rot[0][1]*point[1] + rot[0][2]*point[2];
    dst[1] = rot[1][0]*point[0] + rot[1][1]*point[1] + rot[1][2]*point[2];
    dst[2] = rot[2][0]*point[0] + rot[2][1]*point[1] + rot[2][2]*point[2];
}

/* =========================================================================
   String utilities
   ========================================================================= */

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

int Q_strcasecmp(const char *s1, const char *s2)
{
    return Q_stricmp(s1, s2);
}

int Q_strncasecmp(const char *s1, const char *s2, int n)
{
    int c1, c2;
    do {
        if (!n--) return 0;
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

char *va(char *format, ...)
{
    va_list ap;
    static char str[1024];
    va_start(ap, format);
    vsnprintf(str, sizeof(str), format, ap);
    va_end(ap);
    return str;
}

/* =========================================================================
   COM parsing / path utilities
   ========================================================================= */

char *COM_SkipPath(char *pathname)
{
    char *last = pathname;
    while (*pathname) {
        if (*pathname == '/' || *pathname == '\\')
            last = pathname + 1;
        pathname++;
    }
    return last;
}

void COM_StripExtension(char *in, char *out)
{
    char *dot = NULL;
    char *p = in;
    while (*p) {
        if (*p == '.') dot = p;
        p++;
    }
    if (!dot) {
        strcpy(out, in);
        return;
    }
    while (in < dot)
        *out++ = *in++;
    *out = '\0';
}

void COM_FileBase(char *in, char *out)
{
    char *s = COM_SkipPath(in);
    char *dot = strrchr(s, '.');
    int len = dot ? (int)(dot - s) : (int)strlen(s);
    strncpy(out, s, (size_t)len);
    out[len] = '\0';
}

void COM_FilePath(char *in, char *out)
{
    char *s = in + strlen(in) - 1;
    while (s != in && *s != '/' && *s != '\\')
        s--;
    if (s == in) {
        out[0] = '\0';
    } else {
        int len = (int)(s - in);
        strncpy(out, in, (size_t)len);
        out[len] = '\0';
    }
}

void COM_DefaultExtension(char *path, char *extension)
{
    char *src = path + strlen(path) - 1;
    while (*src != '/' && *src != '\\' && src != path) {
        if (*src == '.') return; /* already has extension */
        src--;
    }
    strcat(path, extension);
}

static char com_token[MAX_TOKEN_CHARS];

char *COM_Parse(char **data_p)
{
    int c, len;
    char *data;

    data = *data_p;
    len = 0;
    com_token[0] = '\0';

    if (!data) {
        *data_p = NULL;
        return com_token;
    }

    /* skip whitespace */
skipwhite:
    while ((c = (unsigned char)*data) <= ' ') {
        if (!c) { *data_p = NULL; return com_token; }
        data++;
    }

    /* skip // comments */
    if (c == '/' && data[1] == '/') {
        while (*data && *data != '\n') data++;
        goto skipwhite;
    }

    if (c == '"') {
        data++;
        while (1) {
            c = (unsigned char)*data++;
            if (!c || c == '"') { com_token[len] = '\0'; *data_p = data; return com_token; }
            if (len < MAX_TOKEN_CHARS - 1) com_token[len++] = (char)c;
        }
    }

    do {
        if (len < MAX_TOKEN_CHARS - 1) com_token[len++] = (char)c;
        data++;
        c = (unsigned char)*data;
    } while (c > ' ');

    com_token[len] = '\0';
    *data_p = data;
    return com_token;
}

void Com_PageInMemory(byte *buffer, int size)
{
    int i;
    for (i = 0; i < size; i += 4096)
        (void)buffer[i];
}

/* =========================================================================
   Byte-order / endian helpers (little-endian only — x86/x86-64)
   ========================================================================= */

void Swap_Init(void) { /* no-op on x86 */ }

short BigShort(short l)
{
    byte b1 = (byte)(l & 0xff);
    byte b2 = (byte)((l >> 8) & 0xff);
    return (short)((b1 << 8) | b2);
}

short LittleShort(short l)
{
    return l;
}

int BigLong(int l)
{
    byte b1 = (byte)(l & 0xff);
    byte b2 = (byte)((l >>  8) & 0xff);
    byte b3 = (byte)((l >> 16) & 0xff);
    byte b4 = (byte)((l >> 24) & 0xff);
    return ((int)b1 << 24) | ((int)b2 << 16) | ((int)b3 << 8) | (int)b4;
}

int LittleLong(int l)
{
    return l;
}

float BigFloat(float l)
{
    union { float f; byte b[4]; } in, out;
    in.f = l;
    out.b[0] = in.b[3];
    out.b[1] = in.b[2];
    out.b[2] = in.b[1];
    out.b[3] = in.b[0];
    return out.f;
}

float LittleFloat(float l)
{
    return l;
}

/* =========================================================================
   Info string helpers
   ========================================================================= */

char *Info_ValueForKey(char *s, char *key)
{
    static char value[MAX_INFO_VALUE];
    char pkey[MAX_INFO_KEY];
    char *o;

    value[0] = '\0';
    if (!s) return value;
    if (*s == '\\') s++;

    while (1) {
        o = pkey;
        while (*s != '\\') {
            if (!*s) return value;
            *o++ = *s++;
        }
        *o = '\0';
        s++;

        o = value;
        while (*s != '\\' && *s) *o++ = *s++;
        *o = '\0';

        if (!Q_stricmp(key, pkey)) return value;

        if (!*s) return value;
        s++;
    }
}

void Info_RemoveKey(char *s, char *key)
{
    char *start;
    char pkey[MAX_INFO_KEY];
    char value[MAX_INFO_VALUE];
    char *o;

    while (1) {
        start = s;
        if (*s == '\\') s++;
        o = pkey;
        while (*s != '\\') {
            if (!*s) return;
            *o++ = *s++;
        }
        *o = '\0';
        s++;

        o = value;
        while (*s != '\\' && *s) *o++ = *s++;
        *o = '\0';

        if (!Q_stricmp(key, pkey)) {
            /* Remove this key=value pair */
            if (*s)
                memmove(start, s + 1, strlen(s + 1) + 1);
            else
                *start = '\0';
            return;
        }

        if (!*s) return;
    }
}

void Info_SetValueForKey(char *s, char *key, char *value)
{
    char newi[MAX_INFO_STRING];

    if (strchr(key, '\\') || strchr(value, '\\')) return;

    Info_RemoveKey(s, key);

    if (!value || !value[0]) return;

    Com_sprintf(newi, sizeof(newi), "\\%s\\%s", key, value);

    if (strlen(newi) + strlen(s) > MAX_INFO_STRING - 1) return;

    strcat(s, newi);
}

qboolean Info_Validate(char *s)
{
    if (strchr(s, '"')) return false;
    if (strchr(s, ';'))  return false;
    return true;
}
