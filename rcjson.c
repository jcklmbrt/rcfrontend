#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <raycaptcha.h>


int main(int argc, char **argv)
{
	static size_t num_rays = 0;
	static ray_t  rays[MAX_NUM_RAYS];

	size_t len;

	if(argc != 2) {
		fprintf(stderr, "Usage: rcjson <WORD>");
		return EXIT_FAILURE;
	}

	len      = strlen(argv[1]);
	num_rays = raycaptcha(argv[1], len, rays);

	printf("[");
	for(size_t i = 0; i < num_rays; i++) {
		printf("{\"point\": { \"x\": %lf, \"y\": %lf, \"z\": %lf },", rays[i].p[0], rays[i].p[1], rays[i].p[2]);
		printf("\"normal\": { \"x\": %lf, \"y\": %lf, \"z\": %lf }}", rays[i].n[0], rays[i].n[1], rays[i].n[2]);
		if(i + 1 != num_rays) {
			printf(",");
		}
	}
	printf("]");

	return EXIT_SUCCESS;
}