
#ifndef _RAYCAPTCHA_H
#define _RAYCAPTCHA_H

#define _USE_MATH_DEFINES
#include <math.h>

#define FONT_WIDTH     8
#define FONT_HEIGHT   16 
#define MAX_CHAR_POS (FONT_WIDTH * FONT_HEIGHT)
#define MAX_STR_SIZE  16
#define MAX_NUM_RAYS  MAX_STR_SIZE * MAX_CHAR_POS

/* limit rotation so that it doesn't end up back to front. */
#define MAX_ROTATION  M_PI_4
#define MIN_ROTATION -M_PI_4

#define MAX_OFFSET_X  0.2
#define MAX_OFFSET_Y  0.2
#define MAX_OFFSET_Z  0.2
#define MIN_OFFSET_X -0.2
#define MIN_OFFSET_Y -0.2
#define MIN_OFFSET_Z -0.2

struct ray_s {
	double p[3];
	double n[3];
};

struct plane_s {
	double n[3];
	double d;
};

typedef struct plane_s plane_t;
typedef struct ray_s   ray_t;

size_t raycaptcha(const char *s, size_t len, ray_t rays[]);

#endif