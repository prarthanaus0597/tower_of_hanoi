#include <stdlib.h>
#include <stdio.h>
#include <glut.h>

#include<windows.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif


#define HEIGHT 0.2f
#define ROD 0.035f
#define SLICES 32
#define INNERSLICES 16
#define LOOPS 1
#define FPS 100 /* more looks nicer, uses more cpu power */
#define FEM 1000.0 / FPS
#define FSEM 0.001f /* speed (bigger is faster) frame speed each millisecond*/

struct config
{
	GLfloat gap;
	GLfloat pinradius;
	GLfloat pinheight;
};

struct action
{
	char fromstack;
	char tostack;
	struct action *next;
};
typedef struct action action;

struct actions
{
	action *head;
	action *tail;
};
typedef struct actions actions;
struct disk
{
	char color;
	GLfloat radius;
	struct disk *next;
	struct disk *prev;
};
typedef struct disk disk;

struct stack
{
	disk *bottom;
	disk *top;
};
typedef struct stack stack;
void *current = GLUT_BITMAP_TIMES_ROMAN_24;
int disks = 3, x = 0, z; // initial number of disks
GLfloat rotX, rotY, zoom, offsetY = 1.5, speed;
GLUquadricObj *quadric;
GLfloat pos;
GLboolean fullscreen;
stack pin[3];
float pinheight[3];
struct config config;
actions actqueue;
action *curaction;
disk *curdisk;
int duration;

int draw, maxdraws;
// function prototypes
void moveDisk(int param);
void hanoiinit(void);
void reset();
void Display(void);
void hanoi(actions *queue, const int n, const char pin1, const char pin2, const char pin3);
void push(stack *pin, disk *item);
disk *pop(stack *pin);
void drawDisk(GLUquadricObj **quadric, const GLfloat outer, const GLfloat inner);
void drawPin(GLUquadricObj **quadric, const GLfloat radius, const GLfloat height);
void drawAllPins(GLUquadricObj **quadric, const GLfloat radius, const GLfloat height, const GLfloat gap);

// tower of hanoi recursive algorithm
void hanoi(actions *queue, const int n, const char pin1, const char pin2, const char pin3)
{
	action *curaction;
	if (n > 0)
	{
		hanoi(queue, n - 1, pin1, pin3, pin2);
		// push action into action queue
		curaction = (action *)malloc(sizeof(action));
		curaction->next = NULL;
		curaction->fromstack = pin1;
		curaction->tostack = pin3;
		if (queue->head == NULL)
			queue->head = curaction;
		if (queue->tail != NULL)
			queue->tail->next = curaction;
		queue->tail = curaction;
		hanoi(queue, n - 1, pin2, pin1, pin3);
	}
}

/** push item to pin */
void push(stack *pin, disk *item)
{
	item->next = NULL;
	if (pin->bottom == NULL)
	{
		pin->bottom = item;
		pin->top = item;
		item->prev = NULL;
	}
	else
	{
		pin->top->next = item;
		item->prev = pin->top;
		pin->top = item;
	}
}

/** pop item from pin */
disk *pop(stack *pin)
{
	disk *tmp;
	if (pin->top != NULL)
	{
		tmp = pin->top;
		if (pin->top->prev != NULL)
		{
			pin->top->prev->next = NULL;
			pin->top = tmp->prev;
		}
		else
		{
			pin->bottom = NULL;
			pin->top = NULL;
		}
		return tmp;
	}
	return NULL;
}

void drawDisk(GLUquadricObj **quadric, const GLfloat outer, const GLfloat inner)
{
	glPushMatrix();
	glRotatef(-90.0, 1.0, 0.0, 0.0);
	gluCylinder(*quadric, outer, outer, HEIGHT, SLICES, LOOPS);
	gluQuadricOrientation(*quadric, GLU_INSIDE);
	if (inner > 0)
		gluCylinder(*quadric, inner, inner, HEIGHT, INNERSLICES, LOOPS);
	gluDisk(*quadric, inner, outer, SLICES, LOOPS);
	gluQuadricOrientation(*quadric, GLU_OUTSIDE);
	glTranslatef(0.0, 0.0, HEIGHT);
	gluDisk(*quadric, inner, outer, SLICES, LOOPS);
	gluQuadricOrientation(*quadric, GLU_OUTSIDE);
	glPopMatrix();
}

void drawPin(GLUquadricObj **quadric, const GLfloat radius, const GLfloat height)
{
	glPushMatrix();
	glRotatef(-90.0, 1.0, 0.0, 0.0);
	// glRotatef(90.0,0.0,1.0, 1.0);
	gluCylinder(*quadric, radius, radius, HEIGHT / 2, SLICES, LOOPS); // surface between upper and lower part of the base
	gluQuadricOrientation(*quadric, GLU_INSIDE);

	gluDisk(*quadric, 0.0, radius, SLICES, LOOPS); // lower part of base
	gluQuadricOrientation(*quadric, GLU_OUTSIDE);
	glTranslatef(0.0, 0.0, HEIGHT / 2);
	// glColor3f(0,0,1);
	gluDisk(*quadric, 0.0, radius, SLICES, LOOPS);				 // upper part of base
	gluCylinder(*quadric, ROD, ROD, height, INNERSLICES, LOOPS); // draw rod
	glTranslatef(0.0, 0.0, height);
	// glColor3f(0,0,1);
	gluDisk(*quadric, 0.0, ROD, INNERSLICES, LOOPS); // top of rod
	glPopMatrix();
}

void drawAllPins(GLUquadricObj **quadric, const GLfloat radius, const GLfloat height, const GLfloat gap)
{
	glPushMatrix();
	glColor3f(0.6, 0.6, 0.6);
	drawPin(quadric, radius, height); // middle pin
	glTranslatef(-gap, 0.0, 0.0);
	glColor3f(0.0, 0.4, 0.4);
	drawPin(quadric, radius, height); // first pin
	glTranslatef(gap * 2, 0.0, 0.0);
	glColor3f(0.1, 0.1, 0.4);
	drawPin(quadric, radius, height); // last pin
	glPopMatrix();
}

void drawBitmapString(float x, float y, float z, char *string)
{
	char *c;
	glRasterPos3f(x, y, z);
	for (c = string; *c != '\0'; c++)
	{
		glColor3f(0, 0.0, 1.0);
		glutBitmapCharacter(current, *c);
	}
}

void drawBitmapInt(GLfloat x, GLfloat y, GLfloat z, int number)
{
	char string[17];
	sprintf(string, "%d", number);
	drawBitmapString(x, y, z, string);
}

void populatePin(void)
{
	int i;
	disk *cur;
	GLfloat radius = 0.1f * disks;
	for (i = 0; i < disks; i++)
	{
		cur = (disk *)malloc(sizeof(disk));
		cur->color = (char)i % 6;
		cur->radius = radius;
		push(&pin[0], cur);
		radius -= 0.1;
	}
	duration = 0;
	draw = 0;
}

void clearPins(void)
{
	int i;
	disk *cur, *tmp;
	free(curdisk);
	curdisk = NULL;
	for (i = 0; i < 3; i++)
	{
		cur = pin[i].top;
		while (cur != NULL)
		{
			tmp = cur->prev;
			free(cur);
			cur = tmp;
		}
		pin[i].top = NULL;
		pin[i].bottom = NULL;
	}
}

void hanoiinit(void)
{
	GLfloat radius;
	speed = 0;						  // FSEM*FEM;
	radius = 0.1f * disks;			  // radius of first disk
	config.pinradius = radius + 0.2f; // radius of base
	config.gap = radius * 2 + 0.6f;	  // radius of 2 first disks +gap between them/distance between 2 centers
	config.pinheight = disks * HEIGHT * 1.5 + 0.2f;
	maxdraws = (2 << (disks - 1)) - 1; // calculate minimum number of moves
	populatePin();
	/* calculate actions; initialize action list */
	actqueue.head = NULL;
	hanoi(&actqueue, disks, 0, 1, 2);
	curaction = actqueue.head;
	curdisk = pop(&pin[(int)curaction->fromstack]);
	pos = 0.001;
}

void reset(void)
{
	clearPins();
	populatePin();
	/* reset actions */
	curaction = actqueue.head;
	curdisk = pop(&pin[(int)curaction->fromstack]);
	pos = 0.001;
	speed = 0;
}

void hanoicleanup(void) // clean up the sequence of operation stored in the structure actqueue,and delete the quadricobject
{
	action *acur, *atmp;
	clearPins();
	acur = actqueue.head;
	do
	{
		atmp = acur->next;
		free(acur);
		acur = atmp;
	} while (acur != NULL);
	gluDeleteQuadric(quadric);
}
void setColor(const int color)
{
	switch (color)
	{
	case 0:
		glColor3f(0.9, 0.0, 0.0);
		break;
	case 1:
		glColor3f(0.0, 0.0, 1.0);
		break;
	case 2:
		glColor3f(1.0, 1.0, 0.0);
		break;
	case 3:
		glColor3f(0.0, 1.0, 1.0);
		break;
	case 4:
		glColor3f(1.0, 0.0, 1.0);
		break;
	case 5:
		glColor3f(0.0, 0.0, 0.0);
		break;
	}
}

void Init(void)
{
	const GLfloat mat_specular[] = {1.0, 1.0, 1.0, 1.0};
	const GLfloat mat_shininess[] = {50.0};
	const GLfloat light_position[] = {0.0, 1.0, 1.0, 0.0};
	glShadeModel(GL_SMOOTH);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);		   /* draw polygons filled */
	glClearColor(1.0, 1.0, 1.0, 1.0);				   /* set screen clear color */
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); /* blending settings */
	glCullFace(GL_BACK);							   /* remove backsides */
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST); // DEPTH BUFFER ALGORITHM
	quadric = gluNewQuadric();
	gluQuadricNormals(quadric, GLU_SMOOTH);
}

/*Is called if the window size is changed */
void Reshape(int width, int height)
{
	glViewport(0, 0, (GLint)width, (GLint)height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, 1, 1.0, 75.0); //
	glMatrixMode(GL_MODELVIEW);
}

/* react to key presses */
void Key(unsigned char key, int x, int y)
{
	switch (key)
	{

		case '1':
			disks=1;
			hanoiinit();
			reset();
			offsetY=0.9;
			zoom=-1.3;
			gluLookAt(0, 2.2, 3.6+zoom, 0.0, offsetY, 0.0, 0.0, 1.0, 0.0);
			reset();
			break;
		case '2':
			disks=2;
			hanoiinit();
			reset();
			offsetY=1.1;
			zoom=-0.8;
			gluLookAt(0.0, 2.2, 3.6+zoom, 0.0, offsetY, 0.0, 0.0, 1.0, 0.0);
			reset();
			break;

	case '3':
		disks = 3;
		hanoiinit();
		reset();
		offsetY = 1.3;
		zoom = -0.3;
		gluLookAt(0.0, 2.9, 3.6 + zoom, 0.0, offsetY, 0.0, 0.0, 1.0, 0.0);
		break;
	case '4':
		disks = 4;
		hanoiinit();
		reset();
		offsetY = 1.5;
		zoom = 0.8;
		gluLookAt(0.0, 2.9, 3.6 + zoom, 0.0, offsetY, 0.0, 0.0, 1.0, 0.0);
		break;
	case '5':
		disks = 5;
		hanoiinit();
		reset();
		offsetY = 1.7;
		zoom = 1.4;
		gluLookAt(0.0, 3.2, 3.6 + zoom, 0.0, offsetY, 0.0, 0.0, 1.0, 0.0);
		break;
    case '6':
			disks=6;
			hanoiinit();
			reset();
			offsetY=1.9;
			zoom=1.8;
			gluLookAt(0.0, 2.2, 3.6+zoom, 0.0, offsetY, 0.0, 0.0, 1.0, 0.0);
			break;
		case '7':
			disks=7;
			hanoiinit();
			reset();
			offsetY=2.1;
			zoom=2.3;
			gluLookAt(0.0, 2.2, 3.6+zoom, 0.0, offsetY, 0.0, 0.0, 1.0, 0.0);
			break;
		case '8':
			disks=8;
			hanoiinit();
			reset();
			offsetY=2.3;
			zoom=2.8;
			gluLookAt(0.0, 2.2, 3.6+zoom, 0.0, offsetY, 0.0, 0.0, 1.0, 0.0);
			break;
		case '9':
			disks=9;
			hanoiinit();
			reset();
			offsetY=2.5;
			zoom=3.3;
			gluLookAt(0.0, 2.2, 3.6+zoom, 0.0, offsetY, 0.0, 0.0, 1.0, 0.0);
			break;

	case ' ':
		rotX = 0.0;
		rotY = 0.0;
		zoom = 0.0;
		offsetY = 1.5;
		speed = FSEM * FEM;
		break;
	case '+':
		zoom -= 0.1;
		break;
	case '-':
		zoom += 0.1;
		break;
	case 'r':
		reset();
		break;
	case 'f':
		if (fullscreen == 0)
		{
			glutFullScreen();
			fullscreen = 1;
		}
		else
		{
			glutReshapeWindow(1200, 600);
			glutPositionWindow(50, 50);
			fullscreen = 0;
		}
		break;
	case 's':
		speed += 0.005;
		break;
	case 'x':
		speed -= 0.005;
		if (speed < 0.0)
			speed = 0.0;
		break;
	case 'p':

		speed = 0;
		break;
	case 'a':
		speed = FSEM * FEM;
		break;
	case 27:
	case 'q':
		exit(EXIT_SUCCESS);

		break;
	}
	glutPostRedisplay();
}

void SpecialKey(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_UP:
		rotX -= 5;
		break;
	case GLUT_KEY_DOWN:
		rotX += 5;
		break;
	case GLUT_KEY_LEFT:
		rotY -= 5;
		break;
	case GLUT_KEY_RIGHT:
		rotY += 5;
		break;
	case GLUT_KEY_PAGE_UP:
		offsetY -= 0.1;
		break;
	case GLUT_KEY_PAGE_DOWN:
		offsetY += 0.1;
		break;
	}
	glutPostRedisplay();
}

void mouse(int button, int state, int x, int y)
{

	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		zoom += 0.1;
	}
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
		zoom -= 0.1;
	if (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN)
		reset();
}

void Display(void)
{
	glClearColor(0.9, 1, 0.9, 1);
	disk *cur;
	int i;
	GLfloat movY;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); /* clear scren */
	glLoadIdentity();




	drawBitmapString(20.0, 30.0, -60.0, "Moves:");

	drawBitmapInt(31.0, 30.0, -60.0, draw);
	drawBitmapString(20.0, 26.0, -60.0, "Minimum Moves:");
	drawBitmapInt(31.0, 26.0, -60.0, maxdraws);

glColor3f(1.0, 0, 0.4);
	drawBitmapString(-30, 30, -60.0, "f		: Fullscreen view/back ");

	glColor3f(1, 0, 0.4);
	drawBitmapString(-30, 26, -60.0, "x		: Decrease the speed");
	glColor3f(1, 0, 0.4);
	drawBitmapString(-30, 22, -60.0, "s		: Increase the speed");
	glColor3f(1, 0, 0.4);
	drawBitmapString(-30, 18, -60.0, " q/esc		: Quit");
	glColor3f(1, 0, 0.4);
	drawBitmapString(-30, 14, -60.0, "arrows		: Rotated views");
drawBitmapString(-30, 18, -60.0, " q/esc		: Quit");
	glColor3f(1, 0, 0.4);
	drawBitmapString(-13, 30, -60.0, "page up/down		: Up/down movement");
glColor3f(1, 0, 0.4);
	drawBitmapString(-13, 26, -60.0, "+/-		: Zoom in /out");

	glColor3f(1, 0, 0.4);
	drawBitmapString(20, 22, -60.0, "Space/S : Start");

	glColor3f(1, 0, 0.4);
	drawBitmapString(-13, 22, -60.0, "1 to 9		: Set number of disks resp.");


	// source
	// background color
	glColor3f(0, 0.4, 0.4);
	drawBitmapString(-13, 18, -60.0, "* : Source");
	glColor3f(0.1, 0.1, 0.7);
	drawBitmapString(-13, 14, -60.0, "* : Destination");












	gluLookAt(0.0, 1.5, 3.6 + zoom, 0.0, offsetY - 0.4, 0.0, 0.0, 1.0, 0.0); // calculate view point
	glRotatef(rotY, 0.0, 1.0, 0.0);											 /* rotate Y axis */
	glRotatef(rotX, 1.0, 0.0, 0.0);											 /* rotate X axis */

	drawAllPins(&quadric, config.pinradius, config.pinheight, config.gap); // draw pins
	glTranslatef(-config.gap, HEIGHT / 2, 0.0);							   // load disk to first rod
	glPushMatrix();
	for (i = 0; i < 3; i++)
	{
		/* fill pins with disks */
		glPushMatrix();
		pinheight[i] = 0;
		if ((cur = pin[i].bottom) != NULL)
		{
			do
			{
				setColor(cur->color);
				drawDisk(&quadric, cur->radius, ROD);
				glTranslatef(0.0, HEIGHT, 0.0); // draw next disk at height
				pinheight[i] += HEIGHT;
				cur = cur->next;
			} while (cur != NULL);
		}
		glPopMatrix();
		glTranslatef(config.gap, 0.0, 0.0); // disk moves to other rod and stay
	}
	glPopMatrix();
	if (curaction != NULL && curaction->fromstack != -1 && curdisk != NULL)
	{
		if (pos <= 1.0)
		{
			movY = pos * (config.pinheight - pinheight[(int)curaction->fromstack]);
			glTranslatef(config.gap * curaction->fromstack, pinheight[(int)curaction->fromstack] + movY, 0.0); // move up
		}
		else
		{
			if (pos < 2.0 && curaction->fromstack != curaction->tostack)
			{
				if (curaction->fromstack != 1 && curaction->tostack != 1)	 // not middle pin
				{															 /* jump 2 pins */
					glTranslatef(config.gap, config.pinheight + 0.05f, 0.0); // move in x and y direction
					if (curaction->fromstack == 0)
						glRotatef(-(pos - 2.0f) * 180 - 90, 0.0, 0.0, 1.0); // rotate to right in z direction
					else
						glRotatef((pos - 2.0f) * 180 + 90, 0.0, 0.0, 1.0); // rotate to left in z direction
					glTranslatef(0.0, config.gap, 0.0);
				}
				else
				{
					if (curaction->fromstack == 0 && curaction->tostack == 1)
					{
						glTranslatef(config.gap / 2, config.pinheight + 0.05f, 0.0); // move in x and y direction
						glRotatef(-(pos - 2.0f) * 180 - 90, 0.0, 0.0, 1.0);			 // rotate to right in z direction
					}
					else
					{
						if (curaction->fromstack == 2 && curaction->tostack == 1)
						{
							glTranslatef(config.gap / 2 * 3, config.pinheight + 0.05f, 0.0); // move in x and y direction
							glRotatef((pos - 2.0f) * 180 + 90, 0.0, 0.0, 1.0);				 // rotate to left in z direction
						}
						else
						{
							if (curaction->fromstack == 1 && curaction->tostack == 2)
							{
								glTranslatef(config.gap / 2 * 3, config.pinheight + 0.05f, 0.0);
								glRotatef(-(pos - 2.0f) * 180 - 90, 0.0, 0.0, 1.0);
							}
							else
							{
								glTranslatef(config.gap / 2, config.pinheight + 0.05f, 0.0);
								glRotatef((pos - 2.0f) * 180 + 90, 0.0, 0.0, 1.0);
							}
						}
					}
					glTranslatef(0.0, config.gap / 2, 0.0);
				}
				glRotatef(-90, 0.0, 0.0, 1.0);
			}
			else if (pos >= 2.0)
			{																											  /* drop disk down */
				movY = config.pinheight - (pos - 2.0f + speed) * (config.pinheight - pinheight[(int)curaction->tostack]); // mov down in y direction
				glTranslatef(config.gap * curaction->tostack, movY, 0.0);
			}
		}
		setColor(curdisk->color); // draw next disk
		drawDisk(&quadric, curdisk->radius, ROD);
	}
	glutSwapBuffers(); /* swap buffers (double buffering) */
}
void moveDisk(int param)
{
	if (param == 1)
		reset();
	if (curaction != NULL)
	{
		if (pos == 0.0 || pos >= 3.0 - speed)
		{ /* 0--1 -> disk goes upwards, 1--2 "disk in air", 2--3 disk goes downwards*/
			pos = 0.0;
			draw++; // current move
			push(&pin[(int)curaction->tostack], curdisk);
			curaction = curaction->next;
			if (curaction != NULL)
				curdisk = pop(&pin[(int)curaction->fromstack]); // hold the disk on the top in curdisk
		}
		pos += speed; // increase position if increased speed
					  // if(pos > 3.0-FSEM)
					  //	pos = 3.0-FSEM;
		glutTimerFunc((unsigned)FEM, moveDisk, 0);
	}
	else
	{
		curdisk = NULL;
		glutTimerFunc(1000, moveDisk, 1);
	}
	glutPostRedisplay();
}

void timer(int param)
{
	if (curaction != NULL)
	{
		// printf(seconds, "Time: %ds", duration++);
	}
	glutTimerFunc(100, timer, 0);
}

void tower(void)
{

	// glutInit(&argc, argv);

	glutInitWindowPosition(0, 0);
	glutInitWindowSize(1200, 900);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB);
	z = glutCreateWindow("Towers of Hanoi Puzzle Simulation");

	/*if(i==1)
{
disks=3;
}
else if(i==2){
disks=4;
}
else if(i==3){

disks=5;
}*/
	hanoiinit();
	reset();
	atexit(hanoicleanup);
	Init();
	glutReshapeFunc(Reshape);
	speed = 0;
	glutKeyboardFunc(Key);
	glutSpecialFunc(SpecialKey);
	glutMouseFunc(mouse);
	glutDisplayFunc(Display);
	glutTimerFunc((unsigned)FEM, moveDisk, 0);

	glutMainLoop();
}
