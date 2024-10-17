#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include <raycaptcha.h>


static bool font_bitset(char ch, int x, int y)
{
	extern unsigned char terminus8x16[4096];
	return terminus8x16[ch * FONT_HEIGHT + y] & 1 << (CHAR_BIT - x);
}


static double rand_double(double min, double max)
{
	double r = (double)rand() / (double)RAND_MAX;
	return min + r * (max - min);
}


static void rand_rayto(const double to[3], ray_t *ray)
{
	double d[3];
retry:
	ray->p[0] = rand_double(-0.5, 0.5);
	ray->p[1] = rand_double(-0.5, 0.5);
	ray->p[2] = rand_double(-0.5, 0.5);

	d[0] = to[0] - ray->p[0];
	d[1] = to[1] - ray->p[1];
	d[2] = to[2] - ray->p[2];

	double len = sqrt(d[0] * d[0] +
	                  d[1] * d[1] +
                          d[2] * d[2]);

	if(len == 0) {
		goto retry;
	}

	for(int i = 0; i < 3; i++) {
		ray->n[i] = d[i] / len;
	}
}

static void obfuscate(const double offset[3], double rotation, double point[3])
{
	/* rotate */
	double x = point[0] * cos(rotation);
	double y = point[1];
	double z = point[0] * -sin(rotation);
	/* apply offset */
	point[0] = x + offset[0];
	point[1] = y + offset[1];
	point[2] = z + offset[2];
}


size_t raycaptcha(const char *s, size_t len, ray_t rays[])
{
	srand(time(NULL));

	double offset[3], rotation;
	offset[0] = rand_double(MIN_OFFSET_X, MAX_OFFSET_X);
	offset[1] = rand_double(MIN_OFFSET_Y, MAX_OFFSET_Y);
	offset[2] = rand_double(MIN_OFFSET_Z, MAX_OFFSET_Z);
	rotation  = rand_double(MIN_ROTATION, MAX_ROTATION);

	size_t n    = 0;
	double xoff = 0.0;

	const double max   = len * FONT_WIDTH;
	const double midx  = 0.5;
	const double midy  = (FONT_HEIGHT / max) * 0.5;

	for(size_t i = 0; i < len; i++) {
		if(!isprint(s[i])) {
			continue;
		}
		for(int y = 0; y < FONT_HEIGHT; y++)
		for(int x = 0; x < FONT_WIDTH;  x++) {
			if(font_bitset(s[i], x, y)) {
				double p[3];
				p[0] = midx - ((x + xoff) / max);
				p[1] = midy - ( y         / max);
				p[2] = 0.0;
				obfuscate(offset, rotation, p);
				rand_rayto(p, &rays[n]);
				n++;
			}
		}
		xoff += FONT_WIDTH;
	}

	return n;
}