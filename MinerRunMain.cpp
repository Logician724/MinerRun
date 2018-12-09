#include <stdio.h>
#include "TextureBuilder.h"
#include "Model_3DS.h"
#include "GLTexture.h"
#include <glut.h>
#include <math.h>
#include <iostream>
#include <vector>

// Sound Library
#include <include/irrKlang.h>
#pragma comment(lib, "irrKlang.lib")
irrklang::ISoundEngine* soundEgnine;

#define GLUT_KEY_ESCAPE 27
#define DEG2RAD(a) (a * 0.0174532925)

#define CAMERA_ROTATION_SPEED 5
#define CAMERA_MOVEMENT_SPEED 1


#define INTERACTABLES_SIZE 50
int WIDTH = 1280;
int HEIGHT = 720;

// game display flags and variables
bool isDesert = true;
bool isThirdPersonPerspective = true;
float groundSegmentsZTranslation[10];

// jump variables
float jumpOffset = 0;
bool isGoingUp = true;
bool isJumping = false;

void initializeGroundSegments() {
	float beginning = 240.0;
	for (int i = 0; i < 10; i++) {
		groundSegmentsZTranslation[i] = beginning;
		beginning -= 30.0;
	}
}

// Camera
class Vector3f {
public:
	float x, y, z;

	Vector3f(float _x = 0.0f, float _y = 0.0f, float _z = 0.0f) {
		x = _x;
		y = _y;
		z = _z;
	}

	Vector3f operator+(Vector3f &v) {
		return Vector3f(x + v.x, y + v.y, z + v.z);
	}

	Vector3f operator-(Vector3f &v) {
		return Vector3f(x - v.x, y - v.y, z - v.z);
	}

	Vector3f operator*(float n) {
		return Vector3f(x * n, y * n, z * n);
	}

	Vector3f operator/(float n) {
		return Vector3f(x / n, y / n, z / n);
	}

	Vector3f unit() {
		return *this / sqrt(x * x + y * y + z * z);
	}

	Vector3f cross(Vector3f v) {
		return Vector3f(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
	}
};
class Camera {
public:
	Vector3f eye, center, up;

	Camera(float eyeX = 0.0f, float eyeY = 10.0f, float eyeZ = 260.0f, float centerX = 0.0f, float centerY = 0.0f, float centerZ = 238.0f, float upX = 0.0f, float upY = 1.0f, float upZ = 0.0f) {
		eye = Vector3f(eyeX, eyeY, eyeZ);
		center = Vector3f(centerX, centerY, centerZ);
		up = Vector3f(upX, upY, upZ);
	}

	void moveX(float d) {
		Vector3f right = up.cross(center - eye).unit();
		eye = eye + right * d;
		center = center + right * d;
	}

	void moveY(float d) {
		eye = eye + up.unit() * d;
		center = center + up.unit() * d;
	}

	void moveZ(float d) {
		Vector3f view = (center - eye).unit();
		eye = eye + view * d;
		center = center + view * d;
	}

	void rotateX(float a) {
		Vector3f view = (center - eye).unit();
		Vector3f right = up.cross(view).unit();
		view = view * cos(DEG2RAD(a)) + up * sin(DEG2RAD(a));
		up = view.cross(right);
		center = eye + view;
	}

	void rotateY(float a) {
		Vector3f view = (center - eye).unit();
		Vector3f right = up.cross(view).unit();
		view = view * cos(DEG2RAD(a)) + right * sin(DEG2RAD(a));
		right = view.cross(up);
		center = eye + view;
	}

	void look() {
		gluLookAt(
			eye.x, eye.y, eye.z,
			center.x, center.y, center.z,
			up.x, up.y, up.z
		);
	}
};
Camera camera;

// 3D Projection Options
GLdouble fovy = 45.0;
GLdouble aspectRatio = (GLdouble)WIDTH / (GLdouble)HEIGHT;
GLdouble zNear = 0.1;
GLdouble zFar = 700;

int cameraZoom = 0;
int sceneMotion = 0;
float characterX = 0.0;

GLuint tex;
char title[] = "Miner Run";

// Model Variables
Model_3DS metalFence;
Model_3DS woodenFence;
Model_3DS cactus;
Model_3DS goldChest;
Model_3DS goldBag;
Model_3DS trafficCone;
Model_3DS goldArtifact;
Model_3DS roadBarrier;

// Textures
GLTexture tex_desert, tex_street, tex_shirt, tex_hair, tex_pants, tex_sleeves;

// road dimensions

float zLow = -300.0f;
float zHigh = 300.0f;
float xLow = -6.0f;
float xHigh = 6.0f;

enum InteractableType { OBSTACLE, COLLECTIBLE };
typedef struct {
	InteractableType type;
	Vector3f offset;
} Interactable;
std::vector<Interactable> interactables;

//=======================================================================
// Camera Perspectives 3rd and 1st
//=======================================================================
void switchPerspective() {
	if (isThirdPersonPerspective) {
		// transition to 1st person
		isThirdPersonPerspective = false;
		camera.eye.x = characterX;
		camera.eye.y = 4;
		camera.eye.z = 249;
		camera.center.x = characterX;
		camera.center.y = 0;
		camera.center.z = 220;
	}
	else {
		// transition to 3rd person
		isThirdPersonPerspective = true;
		camera.eye.x = 0;
		camera.eye.y = 10;
		camera.eye.z = 260;
		camera.center.x = 0;
		camera.center.y = 0;
		camera.center.z = 238;
	}

	camera.look();
}

//=======================================================================
// Lighting Configuration Function
//=======================================================================
void InitLightSource()
{
	// Enable Lighting for this OpenGL Program
	glEnable(GL_LIGHTING);

	// Enable Light Source number 0
	// OpengL has 8 light sources
	glEnable(GL_LIGHT0);

	// Define Light source 0 ambient light
	GLfloat ambient[] = { 0.1f, 0.1f, 0.1, 1.0f };
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);

	// Define Light source 0 diffuse light
	GLfloat diffuse[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

	// Define Light source 0 Specular light
	GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

	// Finally, define light source 0 position in World Space
	GLfloat light_position[] = { 0.0f, 10.0f, 0.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
}

//=======================================================================
// Camera Configuration Function
//=======================================================================
void InitCamera(void) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(fovy, aspectRatio, zNear, zFar);
	//*******************************************************************************************//
	// fovy:			Angle between the bottom and top of the projectors, in degrees.			 //
	// aspectRatio:		Ratio of width to height of the clipping plane.							 //
	// zNear and zFar:	Specify the front and back clipping planes distances from camera.		 //
	//*******************************************************************************************//

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	camera.look();
}

//=======================================================================
// Material Configuration Function
//======================================================================
void InitMaterial()
{
	// Enable Material Tracking
	glEnable(GL_COLOR_MATERIAL);

	// Sich will be assigneet Material Properties whd by glColor
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	// Set Material's Specular Color
	// Will be applied to all objects
	GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glMaterialfv(GL_FRONT, GL_SPECULAR, specular);

	// Set Material's Shine value (0->128)
	GLfloat shininess[] = { 96.0f };
	glMaterialfv(GL_FRONT, GL_SHININESS, shininess);
}

//=======================================================================
// OpengGL Configuration Function
//=======================================================================
void myInit(void)
{
	glClearColor(0.0, 0.0, 0.0, 0.0);

	InitMaterial();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	for (int i = 0; i < INTERACTABLES_SIZE; i++) {

		float z = zLow + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (zHigh - zLow)));
		float x = xLow + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (xHigh - xLow)));

		Vector3f offset = Vector3f(x, 0, z);
		InteractableType currentType;
		if (i > INTERACTABLES_SIZE / 2) {
			currentType = OBSTACLE;
		}
		else {
			currentType = COLLECTIBLE;
		}
		interactables.push_back({ currentType, offset });
	}
}

//=======================================================================
// Render Ground Function
//=======================================================================
void RenderGround()
{
	glDisable(GL_LIGHTING);	// Disable lighting 

	glColor3f(0.6, 0.6, 0.6);	// Dim the ground texture a bit

	glEnable(GL_TEXTURE_2D);	// Enable 2D texturing

	if (isDesert)
		glBindTexture(GL_TEXTURE_2D, tex_desert.texture[0]);	// Bind the ground texture
	else
		glBindTexture(GL_TEXTURE_2D, tex_street.texture[0]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glPushMatrix();
	glBegin(GL_QUADS);
	glNormal3f(0, 1, 0);    // Set quad normal direction.
	glTexCoord2f(0, 0);     // Set tex coordinates ( Using (0,0) -> (5,5) with texture wrapping set to GL_REPEAT to simulate the ground repeated grass texture).
	glVertex3f(-7, 0, -15);
	glTexCoord2f(1, 0);
	glVertex3f(7, 0, -15);
	glTexCoord2f(1, 1);
	glVertex3f(7, 0, 15);
	glTexCoord2f(0, 1);
	glVertex3f(-7, 0, 15);
	glEnd();
	glPopMatrix();

	glEnable(GL_LIGHTING);	// Enable lighting again for other entites coming throung the pipeline.

	glColor3f(1, 1, 1);	// Set material back to white instead of grey used for the ground texture.
}

void drawRoadBarrier() {
	glPushMatrix();
	glTranslatef(-8.1, 0, 0);
	glScalef(0.5, 0.5, 3.0);
	glRotatef(90.f, 1, 0, 0);
	roadBarrier.Draw();
	glPopMatrix();
	glPushMatrix();
	glTranslatef(8.3, 0, 0);
	glScalef(0.5, 0.5, 3);
	glRotatef(90.f, 1, 0, 0);
	roadBarrier.Draw();
	glPopMatrix();
}

void drawMetalFence() {
	glPushMatrix();
	glTranslatef(-7, 0, 0.35);
	glScalef(0.5, 0.5, 1.9);
	glRotatef(90, 0, 0, 1);
	glRotatef(-90, 0, 1, 0);
	metalFence.Draw();
	glPopMatrix();
	glPushMatrix();
	glTranslatef(7, 0, 0.35);
	glScalef(0.5, 0.5, 1.9);
	glRotatef(180, 0, 1, 0);
	glRotatef(-90, 0, 0, 1);
	glRotatef(90, 0, 1, 0);
	metalFence.Draw();
	glPopMatrix();
}

void drawGroundSegment() {

	for (int i = 0; i < 10; i++) {
		glPushMatrix();
		
		glTranslatef(0, 0, groundSegmentsZTranslation[i]);
		RenderGround();
		if (isDesert) {
			drawRoadBarrier();
		}
		else {
			drawMetalFence();
		}

		glPopMatrix();
	}

}


void drawInteractables() {
	for (int i = 0; i < interactables.size(); i++) {
		Interactable currentInteractable = interactables[i];
		Vector3f currentOffset = currentInteractable.offset;
		//std::cout << currentOffset.x << " : " << currentOffset.y << " : " << currentOffset.z << "\n";
		// draw if within camera range
		if (sceneMotion + currentOffset.z <= camera.eye.z && sceneMotion + currentOffset.z >= camera.eye.z - 250) {
			glPushMatrix();
			glTranslated(currentOffset.x, currentOffset.y, currentOffset.z);
			if (currentInteractable.type == COLLECTIBLE) {
				if (isDesert) {
					glScaled(0.3, 0.3, 0.3);
					goldArtifact.Draw();
				}
				else {
					glScaled(0.05, 0.05, 0.05);
					goldBag.Draw();
				}
			}
			else {
				if (currentInteractable.type == OBSTACLE) {
					if (isDesert) {
						glScaled(0.01, 0.01, 0.01);
						cactus.Draw();
					}
					else {
						glTranslated(0, 0.95f, 0);
						glRotated(90, 1, 0, 0);
						glScaled(0.05, 0.05, 0.05);
						trafficCone.Draw();
					}
				}
			}
			glPopMatrix();
		}
	}

}

GLdouble rotationOfArms = -30;
bool swing = false;

void drawCharacter() {
	glColor3f(0.88, 0.88, 0.88);

	glPushMatrix();
	{
		// Head
		glPushMatrix();
		{
			glEnable(GL_TEXTURE_GEN_S);
			glEnable(GL_TEXTURE_GEN_T);
			glBindTexture(GL_TEXTURE_2D, tex_hair.texture[0]);
			glTranslatef(0, 1.75, 0);
			glScalef(0.8, 0.8, 0.8);
			glutSolidCube(1);
			glDisable(GL_TEXTURE_GEN_S);
			glDisable(GL_TEXTURE_GEN_T);
		}
		glPopMatrix();

		glColor3f(1, 0, 0);
		// Eye 1
		glPushMatrix();
		{
			glTranslatef(0.2, 2, 0.35);
			glScalef(0.16, 0.16, 0.16);
			glutSolidCube(1);
		}
		glPopMatrix();

		// Eye 2
		glPushMatrix();
		{
			glTranslatef(-0.2, 2, 0.35);
			glScalef(0.16, 0.16, 0.16);
			glutSolidCube(1);
		}
		glPopMatrix();

		// Mouth
		glPushMatrix();
		{
			glTranslatef(0, 1.5, 0.35);
			glScalef(0.48, 0.16, 0.16);
			glutSolidCube(1);
		}
		glPopMatrix();
	}
	glPopMatrix();

	glColor3f(0.88, 0.88, 0.88);
	// Body
	glPushMatrix();
	{
		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);
		glBindTexture(GL_TEXTURE_2D, tex_shirt.texture[0]);
		glTranslatef(0, 0.6, 0);
		glScalef(0.6, 1.8, 0.6);
		glutSolidCube(1);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
	}
	glPopMatrix();

	// Left Arm
	glPushMatrix();
	{
		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);
		glBindTexture(GL_TEXTURE_2D, tex_sleeves.texture[0]);
		glTranslatef(-0.4, 1.3, 0);
		glRotatef(rotationOfArms, 1, 0, 0);
		glTranslatef(0, -0.6, 0);
		glScalef(0.2, 1, 0.2);
		glutSolidCube(1);
	}
	glPopMatrix();

	// Right Arm
	glPushMatrix();
	{
		glTranslatef(0.4, 1.3, 0);
		glRotatef(-rotationOfArms, 1, 0, 0);
		glTranslatef(0, -0.6, 0);
		glScalef(0.2, 1, 0.2);
		glutSolidCube(1);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
	}
	glPopMatrix();

	// Left Leg
	glPushMatrix();
	{
		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);
		glBindTexture(GL_TEXTURE_2D, tex_pants.texture[0]);
		glTranslatef(-0.2, -0.3, 0);
		glRotatef(-rotationOfArms, 1, 0, 0);
		glTranslatef(0, -0.7, 0);
		glScalef(0.3, 1.4, 0.3);
		glutSolidCube(1);
	}
	glPopMatrix();

	// Right Leg
	glPushMatrix();
	{
		glTranslatef(0.2, -0.3, 0);
		glRotatef(rotationOfArms, 1, 0, 0);
		glTranslatef(0, -0.7, 0);
		glScalef(0.3, 1.4, 0.3);
		glutSolidCube(1);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
	}
	glPopMatrix();
}


//=======================================================================
// Display Function
//=======================================================================


void myDisplay(void)
{
	InitCamera();
	InitLightSource();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GLfloat lightIntensity[] = { 0.7, 0.7, 0.7, 1.0f };
	GLfloat lightPosition[] = { 0.0f, 100.0f, 0.0f, 0.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
	glLightfv(GL_LIGHT0, GL_AMBIENT, lightIntensity);

	glPushMatrix();
	{
		glTranslatef(characterX, 2 + jumpOffset, 250);
		glRotatef(180, 0, 1, 0);
		drawCharacter();
	}
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0, 0, sceneMotion);
	// Draw Ground
	drawGroundSegment();
	// Draw interactables (obstacles, and collectibles)
	drawInteractables();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0, 0, 250);
	GLUquadricObj * qobj;
	qobj = gluNewQuadric();
	glTranslated(50, 0, 0);
	glRotated(90, 1, 0, 1);
	glBindTexture(GL_TEXTURE_2D, tex);
	gluQuadricTexture(qobj, true);
	gluQuadricNormals(qobj, GL_SMOOTH);
	gluSphere(qobj, 300, 100, 100);
	gluDeleteQuadric(qobj);

	glPopMatrix();



	glutSwapBuffers();
}

//=======================================================================
// Keyboard Function
//=======================================================================
void specialKeysEvents(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_LEFT:
		if (characterX > -6) {
			characterX -= 0.5;
		}
		break;

	case GLUT_KEY_RIGHT: 
		if (characterX < 6) {
			characterX += 0.5;
		}
		break;
	}
	
	if (!isThirdPersonPerspective) {
		camera.eye.x = characterX;
		camera.center.x = characterX;
		camera.look();
	}

	glutPostRedisplay();
}

void keysEvents(unsigned char key, int x, int y) {
	switch (key) {
	case 'z': switchPerspective(); break;   // Switch from 1st person to 3rd person or vice versa.
	case 'w': camera.moveY(CAMERA_MOVEMENT_SPEED); break;
	case 's': camera.moveY(-CAMERA_MOVEMENT_SPEED); break;
	case 'a': camera.moveX(CAMERA_MOVEMENT_SPEED); break;
	case 'd': camera.moveX(-CAMERA_MOVEMENT_SPEED); break;
	case 'q': camera.moveZ(CAMERA_MOVEMENT_SPEED); break;
	case 'e': camera.moveZ(-CAMERA_MOVEMENT_SPEED); break;
	case 'i': camera.rotateX(CAMERA_ROTATION_SPEED); break;
	case 'k': camera.rotateX(-CAMERA_ROTATION_SPEED); break;
	case 'j': camera.rotateY(CAMERA_ROTATION_SPEED); break;
	case 'l': camera.rotateY(-CAMERA_ROTATION_SPEED); break;
	case ' ': isJumping = true;	break; // Make the character jump.
	case GLUT_KEY_ESCAPE: exit(EXIT_SUCCESS); break;
	}

	glutPostRedisplay();
}

void playerMouseMovement(int x, int y) {
	std::cout << x << std::endl;
	if (x < 1100 && x > 300) {
		characterX = ((x - 300) / (800 / 12)) - 6;
	}

	if (!isThirdPersonPerspective) {
		camera.eye.x = characterX;
		camera.center.x = characterX;
		camera.look();
	}

	glutPostRedisplay();
}

void characterMouseJump(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		isJumping = true;
	}
}

//=======================================================================
// Assets Loading Function
//=======================================================================
void LoadAssets()
{
	// Loading Model files
	metalFence.Load("models/metal_fence/fance.3ds");
	woodenFence.Load("models/wooden_fence/13076_Gothic_Wood_Fence_Panel_v2_l3.3ds");
	cactus.Load("models/cactus/10436_Cactus_v1_max2010_it2.3ds");
	goldChest.Load("models/gold_chest/ChestCartoon.3ds");
	goldBag.Load("models/gold_bag/13450_Bag_of_Gold_v1_L3.3ds");
	trafficCone.Load("models/traffic_cone/cone1_obj.3ds");
	goldArtifact.Load("models/gold_artifact/13455_Gold_Doubloon_v1_l1.3ds");
	roadBarrier.Load("models/road_barrier/road_barrier.3ds");

	// Loading texture files
	tex_desert.Load("Textures/desert.bmp");
	tex_street.Load("Textures/asphalt_road.bmp");
	loadBMP(&tex, "Textures/blu-sky-3.bmp", true);
	
	// Character Textures
	tex_hair.Load("Textures/hair.bmp");
	tex_sleeves.Load("Textures/sleeves.bmp");
	tex_pants.Load("Textures/pants.bmp");
	tex_shirt.Load("Textures/shirt.bmp");

}
//=======================================================================
// Animation Functions
//=======================================================================
void characterJump(int val) {
	
	if (jumpOffset >= 4) {
		isGoingUp = false;
	}

	if (isGoingUp) {
		jumpOffset += 0.5;
		if (!isThirdPersonPerspective) {
			camera.eye.y += 0.5;
			camera.look();
		}
	}
	else {
		jumpOffset -= 0.5;
		if (!isThirdPersonPerspective) {
			camera.eye.y -= 0.5;
			camera.look();
		}
	}

	if (jumpOffset <= 0) {
		isGoingUp = true;
		isJumping = false;
	}
}

void sceneAnim() {
	sceneMotion += 1;

	for (int i = 0; i < 10; i++) {
		if (sceneMotion + groundSegmentsZTranslation[i] >= 270) {
			groundSegmentsZTranslation[i] -= 300;
		}
	}

	if (!swing) {
		if (rotationOfArms > 30.0) {
			swing = true;
		}
		else {
			rotationOfArms += 25;
		}
	}
	else {
		if (rotationOfArms < -30.0) {
			swing = false;
		}
		else {
			rotationOfArms -= 25;
		}
	}

	if (isJumping)
		characterJump(0);

	glutPostRedisplay();
}

//=======================================================================
// Main Function
//=======================================================================
void main(int argc, char** argv)
{
	//soundEgnine = irrklang::createIrrKlangDevice();

	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

	glutInitWindowSize(WIDTH, HEIGHT);

	glutInitWindowPosition(100, 150);

	glutCreateWindow(title);

	glutDisplayFunc(myDisplay);

	glutPassiveMotionFunc(playerMouseMovement);

	initializeGroundSegments();

	glutSpecialFunc(specialKeysEvents);
	glutKeyboardFunc(keysEvents);
	glutMouseFunc(characterMouseJump);
	myInit();

	LoadAssets();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
	glShadeModel(GL_SMOOTH);
	glutIdleFunc(sceneAnim);

	glutFullScreen();
	glutMainLoop();
}