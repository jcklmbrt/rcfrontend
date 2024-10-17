#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <gl/GL.h>
#else
#include <GL/gl.h>
#endif
#include <GLFW/glfw3.h>

#include <raycaptcha.h>


static double disp = 0.0;
static double rot  = M_PI_2;


static void norm(double p[3])
{
	double len = sqrt(p[0] * p[0] + 
	                  p[1] * p[1] + 
	                  p[2] * p[2]);

	if(len == 0.0) {
		return;
	}

	for(int i = 0; i < 3 && len != 0.0; i++) {
		p[i] = p[i] / len;
	}
}


static void ortho_basis(const double n[3], double v[2][3])
{
	/* n will never face up as we only rotate along the y-axis */
	/* v0 = n x {0, 1, 0} */
	v[0][0] = -n[2];
	v[0][1] = 0.0;
	v[0][2] = n[0];

	norm(v[0]);

	/* v1 = n x v0 */
	v[1][0] = n[1] * v[0][2] - n[2] * v[0][1];
	v[1][1] = n[2] * v[0][0] - n[0] * v[0][2];
	v[1][2] = n[0] * v[0][1] - n[1] * v[0][0];

	norm(v[1]);
}


static void perspective(double fovy, double aspect, double znear, double zfar, double m[4][4])
{
	double halffovy = fovy / 2.0;
	double f = cos(halffovy) / sin(halffovy);
	double d = znear - zfar;

	m[0][0] = f / aspect; m[0][1] = 0.0; m[0][2] = 0.0;                      m[0][3] = 0.0;
	m[1][0] = 0.0;        m[1][1] = f;   m[1][2] = 0.0;                      m[1][3] = 0.0;
	m[2][0] = 0.0;        m[2][1] = 0.0; m[2][2] = (zfar + znear) / d;       m[2][3] = -1.0;
	m[3][0] = 0.0;        m[3][1] = 0.0; m[3][2] = (2.0 * zfar * znear) / d; m[3][3] = 0.0;
}


static void yrotview(double yrot, double d, double zoff, double m[4][4])
{
	double sr = sin(yrot);
	double cr = cos(yrot);

	m[0][0] = cr;  m[0][1] = 0.0; m[0][2] = -sr;      m[0][3] = 0.0;
	m[1][0] = 0.0; m[1][1] = 1.0; m[1][2] = 0.0;      m[1][3] = 0.0;
	m[2][0] = sr;  m[2][1] = 0.0; m[2][2] = cr;       m[2][3] = 0.0;
	m[3][0] = 0.0; m[3][1] = 0.0; m[3][2] = d + zoff; m[3][3] = 1.0;
}


static void yplane(double yrot, double disp, plane_t *plane)
{
	plane->n[0] = cos(yrot);
	plane->n[1] = 0.0;
	plane->n[2] = sin(yrot);
	plane->d    = disp;
}


static void draw_plane(const plane_t *plane, double size)
{
	double p[3];
	p[0] = plane->n[0] * plane->d;
	p[1] = plane->n[1] * plane->d;
	p[2] = plane->n[2] * plane->d;

	double v[2][3];
	ortho_basis(plane->n, v);

	double quad[4][3];
	for(int i = 0; i < 3; i++) {
		quad[0][i] = p[i] + -v[0][i] * size + -v[1][i] * size;
		quad[1][i] = p[i] +  v[0][i] * size + -v[1][i] * size;
		quad[2][i] = p[i] +  v[0][i] * size +  v[1][i] * size;
		quad[3][i] = p[i] + -v[0][i] * size +  v[1][i] * size;
	}

	glBegin(GL_QUADS);
		glColor4d(1.0, 1.0, 1.0, 0.5);
		glVertex3dv(quad[0]);
		glVertex3dv(quad[1]);
		glVertex3dv(quad[2]);
		glVertex3dv(quad[3]);
	glEnd();

	glLineWidth(4);
	glBegin(GL_LINES);
		glColor4d(0.0, 0.0, 0.0, 0.5);
		glVertex3dv(quad[0]); glVertex3dv(quad[1]);
		glVertex3dv(quad[1]); glVertex3dv(quad[2]);
		glVertex3dv(quad[2]); glVertex3dv(quad[3]);
		glVertex3dv(quad[3]); glVertex3dv(quad[0]);
	glEnd();
	glLineWidth(1);
}


static void draw_ray(const ray_t *ray, double size)
{
	double proj[3];
	proj[0] = ray->p[0] + (ray->n[0] * 0.2);
	proj[1] = ray->p[1] + (ray->n[1] * 0.2);
	proj[2] = ray->p[2] + (ray->n[2] * 0.2);

	glBegin(GL_LINES);
		glColor4d(0, 0, 1.0, 0.5);
		glVertex3dv(ray->p);
		glVertex3dv(proj);
	glEnd();

	glBegin(GL_POINTS);
		glColor4d(0.0, 0.0, 1.0, 0.5);
		glVertex3dv(ray->p);
	glEnd();
}


static bool ray_plane_intersection(const ray_t *ray, const plane_t *plane, double p[3])
{
	double nn = ray->n[0] * plane->n[0] + 
	            ray->n[1] * plane->n[1] +
	            ray->n[2] * plane->n[2];

	if(nn == 0.0) {
		return false;
	}

	double np = ray->p[0] * plane->n[0] +
	            ray->p[1] * plane->n[1] +
	            ray->p[2] * plane->n[2];

	double t = -(np + plane->d) / nn;

	p[0] = ray->p[0] + ray->n[0] * t;
	p[1] = ray->p[1] + ray->n[1] * t;
	p[2] = ray->p[2] + ray->n[2] * t;

	return true;
}


static void draw_intersection(const ray_t *ray, const plane_t *plane, double plane_size)
{
	double p[3];
	if(!ray_plane_intersection(ray, plane, p)) {
		return;
	}

	double v[2][3];
	ortho_basis(plane->n, v);

	double sx = p[0] * v[0][0] + p[1] * v[0][1] + p[2] * v[0][2];
	double sy = p[0] * v[1][0] + p[1] * v[1][1] + p[2] * v[1][2];
	if(sx > -plane_size && sx < plane_size && sy > -plane_size && sy < plane_size) {
		glBegin(GL_POINTS);
			glColor4d(1.0, 0, 0, 1.0);
			glVertex3dv(p);
		glEnd();
	}
}


void draw(ray_t rays[], size_t num_rays)
{
	plane_t plane;
	yplane(rot, disp, &plane);

	const double extents = 1.0;
	const double rotoff  = M_PI_4 * -0.25; /* at a slight angle for a 3d effect */

	double m[4][4];
	yrotview(rot + M_PI_2 + rotoff, -disp, -1.5, m);

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixd(m);

	draw_plane(&plane, extents);

	for(size_t i = 0; i < num_rays; i++) {
		draw_ray(&rays[i], 0.2);
	}

	/* draw points on top of everything */
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	for(size_t i = 0; i < num_rays; i++) {
		draw_intersection(&rays[i], &plane, extents);
	}
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
}


void fbsizecb(GLFWwindow *window, int w, int h)
{
	glViewport(0, 0, w, h);

	double aspect = (double)w / (double)h;

	double m[4][4];
	perspective(M_PI_2, aspect, 0.1, 100.0, m);

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(m);
}


void keycb(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if(action == GLFW_RELEASE) {
		return;
	}

	const double maxrot = M_PI_2 + MAX_ROTATION;
	const double minrot = M_PI_2 + MIN_ROTATION;

	const double maxoffsetsqr = MAX_OFFSET_X * MAX_OFFSET_X +
	                            MAX_OFFSET_Y * MAX_OFFSET_Y +
	                            MAX_OFFSET_Z * MAX_OFFSET_Z;

	const double minoffsetsqr = MIN_OFFSET_X * MIN_OFFSET_X +
	                            MIN_OFFSET_Y * MIN_OFFSET_Y +
	                            MIN_OFFSET_Z * MIN_OFFSET_Z;

	switch(key) {
	case 'j':
	case 'J':
	case GLFW_KEY_DOWN:
		disp += 0.001;
		//if(disp > maxoffset) {
			//disp = maxoffset;
		//}
		break;
	case 'k':
	case 'K':
	case GLFW_KEY_UP:
		disp -= 0.001;
		//if(disp < minoffset) {
			//disp = minoffset;
		//}
		break;
	case 'h':
	case 'H':
	case GLFW_KEY_LEFT:
		rot += 0.01;
		if(rot > maxrot) {
			rot = maxrot;
		}
		break;
	case 'l':
	case 'L':
	case GLFW_KEY_RIGHT:
		rot -= 0.01;
		if(rot < minrot) {
			rot = minrot;
		}
		break;
	}
}

int main(int argc, char **argv)
{
	static size_t num_rays = 0;
	static ray_t  rays[MAX_NUM_RAYS];

	GLFWwindow *window = NULL;
	const char str[] = "test";
	num_rays = raycaptcha(str, sizeof(str), rays);

	if(glfwInit() != GLFW_TRUE) {
		return EXIT_FAILURE;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_SAMPLES, 4);

	window = glfwCreateWindow(640, 480, "rcfontend", NULL, NULL);
	if(window == NULL) {
		glfwTerminate();
		return EXIT_FAILURE;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	glPointSize(4);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glfwSetFramebufferSizeCallback(window, fbsizecb);
	glfwSetKeyCallback(window, keycb);
	fbsizecb(window, 640, 480);

	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		glfwSwapBuffers(window);

		glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		draw(rays, num_rays);
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return EXIT_SUCCESS;
}