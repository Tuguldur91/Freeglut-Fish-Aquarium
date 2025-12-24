#include <GL/glut.h>
#include <GL/glu.h>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <SOIL.h>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Game score system
int score = 0;
int highScore = 0;
int feedCount = 0;
int combo = 0;
float comboTimer = 0;
int totalFeedings = 0;
int perfectFeeds = 0;

// Mini-game challenges
bool challengeActive = false;
float challengeTimer = 0;
int challengeTarget = 0;
int challengeComplete = 0;
std::string challengeText = "";

GLuint floorTexture;
GLuint wallTexture;
GLuint sandTexture;
GLuint fishTextures[8];

GLuint loadTexture(const char* filename) {
    GLuint textureID = SOIL_load_OGL_texture(
        filename,
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB
    );

    if(textureID == 0) {
        printf("SOIL loading error: '%s'\n", SOIL_last_result());
        return 0;
    }

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    printf("Texture loaded: %s (ID: %d)\n", filename, textureID);
    return textureID;
}

// Camera variables

float cameraAngleX = 20.0f;
float cameraAngleY = 0.0f;
float cameraDistance = 15.0f;
int lastMouseX = 0, lastMouseY = 0;
bool mouseLeftDown = false;

// Time
float time_val = 0.0f;

// Interactive elements
bool feedingMode = false;
float foodX = 0, foodY = 0, foodZ = 0;
float foodTime = 0;
bool showInfo = true;
int selectedFish = -1;

// Fish structure
struct Fish {
    float x, y, z;
    float angle;
    float speed;
    float phase;
    float size;
    float color[3];
    bool isHungry;
    float hunger;
    bool isSelected;
    float targetY; // For vertical movement
    GLuint textureId;
};
std::vector<Fish> fishes;

struct Plant {
    float x, z;
    float height;
    float sway;
    int bladeCount;
    float color[3];
    float thickness;
};

std::vector<Plant> plants;



// Bubble structure
struct Bubble {
    float x, y, z;
    float speed;
    float size;
    float lifetime;
};

std::vector<Bubble> bubbles;

// Particle for food splash effect
struct Particle {
    float x, y, z;
    float vx, vy, vz;
    float lifetime;
    float maxLifetime;
};

std::vector<Particle> particles;

// Initialize
void init() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_TEXTURE_2D);
    
    for(int i = 0; i < 8; i++) {
        char filename[64];
        sprintf(filename, "final/textures/fish%d.jpg", i + 1);
        fishTextures[i] = loadTexture(filename);
        if (fishTextures[i] == 0) {
            printf("failed to load %s, using fisg1.jpg as fallback.\n", filename);
            fishTextures[i] = fishTextures[0];
        }
    } 
    // fishTexture = loadTexture("final/textures/fish1.jpg");


    floorTexture = loadTexture("final/textures/wood_floor.jpg");
    wallTexture = loadTexture("final/textures/wall.jpg");
    sandTexture = loadTexture("final/textures/sand.jpg");
  

    glDisable(GL_COLOR_MATERIAL);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);

    // Main light
    GLfloat light0_pos[] = {5.0f, 8.0f, 5.0f, 1.0f};
    GLfloat light0_ambient[] = {0.3f, 0.4f, 0.5f, 1.0f};
    GLfloat light0_diffuse[] = {0.8f, 0.9f, 1.0f, 1.0f};
    GLfloat light0_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);

    // Secondary light
    GLfloat light1_pos[] = {-5.0f, 5.0f, -5.0f, 1.0f};
    GLfloat light1_diffuse[] = {0.3f, 0.4f, 0.5f, 1.0f};
    glLightfv(GL_LIGHT1, GL_POSITION, light1_pos);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);

    glClearColor(0.05f, 0.15f, 0.3f, 1.0f);

    srand(static_cast<unsigned int>(time(NULL)));

    // Create fish with vibrant colors
    float fishColors[8][3] = {
        {1.0f, 0.3f, 0.1f},
        {0.1f, 0.6f, 1.0f},
        {1.0f, 0.8f, 0.0f},
        {0.9f, 0.1f, 0.5f},
        {0.2f, 1.0f, 0.3f},
        {0.6f, 0.2f, 0.9f},
        {1.0f, 0.5f, 0.0f},
        {0.0f, 0.9f, 0.9f}
    };

    for(int i = 0; i < 8; i++) {
        Fish f;
        f.x = (rand() % 100 - 50) / 10.0f;
        f.y = (rand() % 60 - 30) / 10.0f;
        f.z = (rand() % 100 - 50) / 10.0f;
        f.angle = static_cast<float>(rand() % 360);
        f.speed = 0.3f + (rand() % 100) / 500.0f;
        f.phase = (rand() % 100) / 10.0f;
        f.size = 0.3f + (rand() % 100) / 300.0f;
        f.color[0] = fishColors[i][0];
        f.color[1] = fishColors[i][1];
        f.color[2] = fishColors[i][2];
        f.isHungry = true;
        f.hunger = 0.5f + (rand() % 50) / 100.0f;
        f.isSelected = false;
        f.targetY = f.y;
        f.textureId = fishTextures[i];
        fishes.push_back(f);
    }
        // Draw plants
    plants.clear();
    
    // Tall kelp-like plants
    for(int i = 0; i < 3; i++) {
        Plant p;
        p.x = -6.0f + i * 2.5f;
        p.z = -5.0f + (rand() % 20) / 10.0f;
        p.height = 2.5f + (rand() % 100) / 50.0f;
        p.sway = (rand() % 100) / 10.0f;
        p.bladeCount = 1;
        p.thickness = 0.2f;
        p.color[0] = 0.1f;
        p.color[1] = 0.5f + (rand() % 30) / 100.0f;
        p.color[2] = 0.15f;
        plants.push_back(p);
    }
    
    // Bushy plants
    for(int i = 0; i < 4; i++) {
        Plant p;
        p.x = -4.0f + (rand() % 80) / 10.0f;
        p.z = 2.0f + (rand() % 60) / 10.0f;
        p.height = 1.2f + (rand() % 80) / 100.0f;
        p.sway = (rand() % 100) / 10.0f;
        p.bladeCount = 6 + rand() % 4;
        p.thickness = 0.15f;
        p.color[0] = 0.15f + (rand() % 20) / 100.0f;
        p.color[1] = 0.7f + (rand() % 30) / 100.0f;
        p.color[2] = 0.2f;
        plants.push_back(p);
    }
    
    // Decorative grass
    for(int i = 0; i < 6; i++) {
        Plant p;
        p.x = -6.0f + (rand() % 120) / 10.0f;
        p.z = -6.0f + (rand() % 120) / 10.0f;
        p.height = 0.6f + (rand() % 50) / 100.0f;
        p.sway = (rand() % 100) / 10.0f;
        p.bladeCount = 8 + rand() % 8;
        p.thickness = 0.08f;
        p.color[0] = 0.2f;
        p.color[1] = 0.8f + (rand() % 20) / 100.0f;
        p.color[2] = 0.3f;
        plants.push_back(p);
    }
    
}

// Draw fish
void drawFish(Fish& fish, int index) {
    glPushMatrix();
    glTranslatef(fish.x, fish.y, fish.z);
    glRotatef(fish.angle, 0, 1, 0);

    float wobble = sin(time_val * 3.0f + fish.phase) * 0.1f;
    glRotatef(wobble * 20, 0, 0, 1);

    if(fish.isSelected) {
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glColor3f(1.0f, 1.0f, 0.0f);
        glutWireSphere(fish.size * 2.2f, 16, 16);
        glEnable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
    }

    // ===== FIXED TEXTURE APPLICATION =====
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, fish.textureId);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    // Use white material to let texture show through
    float hungerFactor = 0.6f + fish.hunger * 0.4f;
    
    GLfloat mat_ambient[] = {0.5f, 0.5f, 0.5f, 1.0f};
    GLfloat mat_diffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};  // White for texture
    GLfloat mat_specular[] = {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat mat_shininess[] = {64.0f};

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);

    // Tint texture with fish color
    glColor4f(fish.color[0] * hungerFactor,
              fish.color[1] * hungerFactor,
              fish.color[2] * hungerFactor,
              1.0f);

    // Body
    glPushMatrix();
    glScalef(fish.size * 1.5f, fish.size * 0.8f, fish.size * 0.6f);
    static GLUquadric* quad = nullptr;
    if (!quad) quad = gluNewQuadric();
    gluQuadricTexture(quad, GL_TRUE);  // Enable UV generation
    gluSphere(quad, 1.0, 20, 20);     // Now textured!
    glPopMatrix();

    // Head
    glPushMatrix();
    glTranslatef(fish.size * 1.2f, 0, 0);
    glScalef(fish.size * 0.7f, fish.size * 0.6f, fish.size * 0.5f);
    gluSphere(quad, 1.0, 16, 16);  // Reuse same quadric
    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
    // Eyes
    glPushMatrix();
    glTranslatef(fish.size * 1.5f, fish.size * 0.3f, fish.size * 0.25f);
    GLfloat eye_ambient[] = {0.05f, 0.05f, 0.05f, 1.0f};
    GLfloat eye_diffuse[] = {0.1f, 0.1f, 0.1f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, eye_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, eye_diffuse);
    glutSolidSphere(fish.size * 0.15f, 8, 8);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(fish.size * 1.5f, fish.size * 0.3f, -fish.size * 0.25f);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, eye_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, eye_diffuse);
    glutSolidSphere(fish.size * 0.15f, 8, 8);
    glPopMatrix();

    // color for tail and fins
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);

    // Tail with animation
    glPushMatrix();
    glTranslatef(-fish.size * 1.5f, 0, 0);
    float tailWave = sin(time_val * 5.0f + fish.phase) * 15.0f;
    glRotatef(tailWave, 0, 1, 0);
    glBegin(GL_TRIANGLES);
    glNormal3f(-1, 0, 0);
    glVertex3f(0, 0, 0);
    glVertex3f(-fish.size * 1.2f, fish.size * 0.8f, 0);
    glVertex3f(-fish.size * 1.2f, -fish.size * 0.8f, 0);
    glEnd();
    glPopMatrix();

    // fin
    glPushMatrix();
    glTranslatef(0, fish.size * 0.8f, 0);
    float finWave = sin(time_val * 4.0f + fish.phase) * 5.0f;
    glRotatef(finWave, 0, 0, 1);
    glBegin(GL_TRIANGLES);
    glNormal3f(0, 1, 0);
    glVertex3f(0, 0, 0);
    glVertex3f(fish.size * 0.4f, fish.size * 0.6f, 0);
    glVertex3f(-fish.size * 0.4f, fish.size * 0.6f, 0);
    glEnd();
    glPopMatrix();

    // Side fins
    glPushMatrix();
    glTranslatef(fish.size * 0.3f, -fish.size * 0.2f, fish.size * 0.5f);
    glRotatef(sin(time_val * 4.0f + fish.phase) * 20.0f, 1, 0, 0);
    glBegin(GL_TRIANGLES);
    glNormal3f(0, 0, 1);
    glVertex3f(0, 0, 0);
    glVertex3f(fish.size * 0.5f, -fish.size * 0.3f, fish.size * 0.2f);
    glVertex3f(0, -fish.size * 0.2f, fish.size * 0.4f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(fish.size * 0.3f, -fish.size * 0.2f, -fish.size * 0.5f);
    glRotatef(sin(time_val * 4.0f + fish.phase) * 20.0f, 1, 0, 0);
    glBegin(GL_TRIANGLES);
    glNormal3f(0, 0, -1);
    glVertex3f(0, 0, 0);
    glVertex3f(0, -fish.size * 0.2f, -fish.size * 0.4f);
    glVertex3f(fish.size * 0.5f, -fish.size * 0.3f, -fish.size * 0.2f);
    glEnd();
    glPopMatrix();

    glPopMatrix();
}

// Draw rock
void drawRock(float x, float y, float z, float size) {
    glPushMatrix();
    glTranslatef(x, y, z);
    GLfloat rock_ambient[] = {0.15f, 0.15f, 0.18f, 1.0f};
    GLfloat rock_diffuse[] = {0.35f, 0.35f, 0.4f, 1.0f};
    GLfloat rock_specular[] = {0.2f, 0.2f, 0.2f, 1.0f};
    GLfloat rock_shininess[] = {8.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, rock_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, rock_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, rock_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, rock_shininess);
    glScalef(size, size * 0.6f, size * 0.8f);
    glutSolidSphere(1.0, 12, 12);
    glPopMatrix();
}



// Draw wooden table
void drawTable() {
    // --- Wood Material ---
    GLfloat wood_ambient[]  = {0.3f, 0.15f, 0.05f, 1.0f};
    GLfloat wood_diffuse[]  = {0.6f, 0.3f, 0.1f, 1.0f};
    GLfloat wood_specular[] = {0.3f, 0.15f, 0.05f, 1.0f};
    GLfloat wood_shininess[] = {20.0f};

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, wood_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, wood_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, wood_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, wood_shininess);

    //  Table dimensions
    float topWidth  = 12.0f;
    float topDepth  = 10.0f;
    float topHeight = 0.5f;
    float legThickness = 0.5f;
    float legHeight    = 5.5f;
    float supportThickness = 0.3f;

    //  Table Top (main surface)
    glPushMatrix();
    glTranslatef(0, -5.2f, 0);
    glScalef(topWidth, topHeight, topDepth);
    glutSolidCube(1.0);
    glPopMatrix();

    //  Underside Frame
    glPushMatrix();
    glTranslatef(0, -5.45f, 0);
    glScalef(topWidth * 0.92f, 0.25f, topDepth * 0.92f);
    glutSolidCube(1.0);
    glPopMatrix();

    //  Leg positions
    float legPos[4][2] = {
        {-topWidth/2 + 0.8f, -topDepth/2 + 0.8f},
        { topWidth/2 - 0.8f, -topDepth/2 + 0.8f},
        {-topWidth/2 + 0.8f,  topDepth/2 - 0.8f},
        { topWidth/2 - 0.8f,  topDepth/2 - 0.8f}
    };

    //  Legs
    for(int i = 0; i < 4; i++) {
        glPushMatrix();
        glTranslatef(legPos[i][0], -8.0f, legPos[i][1]);
        glScalef(legThickness, legHeight, legThickness);
        glutSolidCube(1.0);
        glPopMatrix();
    }

    //  Leg Bottom Extensions
    for(int i = 0; i < 4; i++) {
        glPushMatrix();
        glTranslatef(legPos[i][0], -10.5f, legPos[i][1]);
        glScalef(legThickness * 1.2f, 0.4f, legThickness * 1.2f);
        glutSolidCube(1.0);
        glPopMatrix();
    }

    //  Side Support Beams between legs
    for (int side = -1; side <= 1; side += 2) {
        glPushMatrix();
        glTranslatef(side * (topWidth/2 - 0.8f), -8.0f, 0);
        glScalef(supportThickness, 1.0f, topDepth - 2.0f);
        glutSolidCube(1.0);
        glPopMatrix();
    }

    // Front/Back Support Beams
    for (int side = -1; side <= 1; side += 2) {
        glPushMatrix();
        glTranslatef(0, -8.0f, side * (topDepth/2 - 0.8f));
        glScalef(topWidth - 2.0f, 1.0f, supportThickness);
        glutSolidCube(1.0);
        glPopMatrix();
    }
}

// Draw room walls and floor
void drawRoom() {
    // Back wall
    GLfloat wall_ambient[] = {0.4f, 0.42f, 0.38f, 1.0f};
    GLfloat wall_diffuse[] = {0.7f, 0.72f, 0.68f, 1.0f};
    GLfloat wall_specular[] = {0.1f, 0.1f, 0.1f, 1.0f};
    GLfloat wall_shininess[] = {5.0f};

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, wall_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, wall_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, wall_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, wall_shininess);

    glPushMatrix();
    glTranslatef(0, 0, -15);
    glScalef(30.0f, 20.0f, 0.2f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Floor
    GLfloat floor_ambient[] = {0.25f, 0.2f, 0.15f, 1.0f};
    GLfloat floor_diffuse[] = {0.5f, 0.4f, 0.3f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, floor_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, floor_diffuse);

    glPushMatrix();
    glTranslatef(0, -10.5f, 0);
    glScalef(30.0f, 0.1f, 30.0f);
    glutSolidCube(1.0);
    glPopMatrix();
}

// Draw window
void drawWindow() {
    // Window frame
    GLfloat frame_ambient[] = {0.15f, 0.12f, 0.1f, 1.0f};
    GLfloat frame_diffuse[] = {0.3f, 0.25f, 0.2f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, frame_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, frame_diffuse);

    glPushMatrix();
    glTranslatef(-10, 2, -14.8f);

    // Outer frame
    glPushMatrix();
    glScalef(4.0f, 5.0f, 0.2f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Window pane (semi-transparent blue)
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glColor4f(0.6f, 0.8f, 1.0f, 0.3f);
    glPushMatrix();
    glTranslatef(0, 0, 0.15f);
    glScalef(3.5f, 4.5f, 0.05f);
    glutSolidCube(1.0);
    glPopMatrix();
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);

    glPopMatrix();
}

// Draw shelf
void drawShelf()
{
    GLfloat shelf_ambient[] = {0.35f, 0.32f, 0.28f, 1.0f};
    GLfloat shelf_diffuse[] = {0.65f, 0.60f, 0.55f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, shelf_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, shelf_diffuse);

    //  Shelf Config
    float shelfWidth   = 3.5f;
    float shelfHeight  = 6.0f;
    float shelfDepth   = 1.0f;
    float boardThickness = 0.15f;

    glPushMatrix();
    glTranslatef(10, -2, -14.5f);

    //  SIDE PANELS
    glPushMatrix();
    glTranslatef(-(shelfWidth/2), 0, 0);
    glScalef(boardThickness, shelfHeight, shelfDepth);
    glutSolidCube(1.0);
    glPopMatrix();

    glPushMatrix();
    glTranslatef((shelfWidth/2), 0, 0);
    glScalef(boardThickness, shelfHeight, shelfDepth);
    glutSolidCube(1.0);
    glPopMatrix();

    //  BACK PANEL
    glPushMatrix();
    glTranslatef(0, 0, -(shelfDepth/2));
    glScalef(shelfWidth, shelfHeight, boardThickness);
    glutSolidCube(1.0);
    glPopMatrix();

    //  TOP BOARD
    glPushMatrix();
    glTranslatef(0, shelfHeight/2, 0);
    glScalef(shelfWidth, boardThickness, shelfDepth);
    glutSolidCube(1.0);
    glPopMatrix();

    //  BOTTOM BOARD
    glPushMatrix();
    glTranslatef(0, -(shelfHeight/2), 0);
    glScalef(shelfWidth, boardThickness, shelfDepth);
    glutSolidCube(1.0);
    glPopMatrix();

    //  INTERIOR SHELVES
    int numShelves = 3;
    for(int i = 1; i <= numShelves; i++)
    {
        glPushMatrix();
        float y = (shelfHeight/2) - i * (shelfHeight / (numShelves + 1));
        glTranslatef(0, y, 0);
        glScalef(shelfWidth, boardThickness, shelfDepth);
        glutSolidCube(1.0);
        glPopMatrix();
    }

    glPopMatrix();
}


// Draw plant with animation
void drawPlant(Plant& plant) {
    glPushMatrix();
    glTranslatef(plant.x, -4.5f, plant.z);

    GLfloat plant_ambient[] = {plant.color[0] * 0.3f, plant.color[1] * 0.3f, plant.color[2] * 0.3f, 1.0f};
    GLfloat plant_diffuse[] = {plant.color[0], plant.color[1], plant.color[2], 1.0f};
    GLfloat plant_specular[] = {0.2f, 0.4f, 0.2f, 1.0f};
    GLfloat plant_shininess[] = {32.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, plant_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, plant_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, plant_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, plant_shininess);

    // Draw multiple blades with natural sway
    for(int i = 0; i < plant.bladeCount; i++) {
        float angle = (360.0f / plant.bladeCount) * i;
        float swayOffset = sin(time_val * 1.2f + i * 0.8f + plant.sway) * 15.0f;
        float bendAmount = sin(time_val * 0.8f + i * 0.5f) * 8.0f;
        
        glPushMatrix();
        glRotatef(angle + swayOffset, 0, 1, 0);
        
        // Vary color slightly per blade
        plant_diffuse[0] = plant.color[0] * (0.9f + (i % 3) * 0.05f);
        plant_diffuse[1] = plant.color[1] * (0.9f + (i % 3) * 0.05f);
        plant_diffuse[2] = plant.color[2] * (0.9f + (i % 3) * 0.05f);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, plant_diffuse);
        
        // Draw blade with segments for natural bending
        int segments = 5;
        for(int seg = 0; seg < segments; seg++) {
            float segHeight = plant.height / segments;
            float currentH = segHeight * seg;
            float nextH = segHeight * (seg + 1);
            float widthBase = plant.thickness * (1.0f - seg / (float)segments);
            float widthTop = plant.thickness * (1.0f - (seg + 1) / (float)segments);
            
            float bend = bendAmount * (seg / (float)segments);
            
            glPushMatrix();
            glTranslatef(0, currentH, 0);
            glRotatef(bend, 0, 0, 1);
            
            glBegin(GL_QUADS);
            // Front face
            glNormal3f(0, 0, 1);
            glVertex3f(-widthBase, 0, 0.02f);
            glVertex3f(widthBase, 0, 0.02f);
            glVertex3f(widthTop, segHeight, 0.02f);
            glVertex3f(-widthTop, segHeight, 0.02f);
            
            // Back face
            glNormal3f(0, 0, -1);
            glVertex3f(-widthBase, 0, -0.02f);
            glVertex3f(-widthTop, segHeight, -0.02f);
            glVertex3f(widthTop, segHeight, -0.02f);
            glVertex3f(widthBase, 0, -0.02f);
            glEnd();
            
            glPopMatrix();
        }
        
        // Add leaf tip
        glPushMatrix();
        glTranslatef(0, plant.height, 0);
        glRotatef(bendAmount, 0, 0, 1);
        glBegin(GL_TRIANGLES);
        glNormal3f(0, 0, 1);
        glVertex3f(-plant.thickness * 0.3f, 0, 0);
        glVertex3f(plant.thickness * 0.3f, 0, 0);
        glVertex3f(0, plant.thickness * 0.8f, 0);
        glEnd();
        glPopMatrix();
        
        glPopMatrix();
    }

    glPopMatrix();
}


void addScore(int points) {
    score += points * (combo > 0 ? combo : 1);
    if(score > highScore) {
        highScore = score;
    }
}

void startChallenge() {
    if(!challengeActive) {
        challengeActive = true;
        challengeTimer = 30.0f;
        challengeTarget = 5 + rand() % 3;
        challengeComplete = 0;
        
        int type = rand() % 3;
        if(type == 0) {
            challengeText = "Feed " + std::to_string(challengeTarget) + " fish in 30s!";
        } else if(type == 1) {
            challengeText = "Maintain 3x combo for 20s!";
            challengeTimer = 20.0f;
        } else {
            challengeText = "Feed all hungry fish!";
        }
        printf("Challenge: %s\n", challengeText.c_str());
    }
}

void checkChallenge() {
    if(challengeActive) {
        int type = challengeText[0] == 'F' && challengeText[5] != 'a' ? 0 : 
                   challengeText[0] == 'M' ? 1 : 2;
        
        if(type == 0 && challengeComplete >= challengeTarget) {
            addScore(500);
            challengeActive = false;
            printf("Challenge Complete! +500 bonus points!\n");
        } else if(type == 1 && combo >= 3) {
            challengeTimer -= 0.016f;
            if(challengeTimer <= 0) {
                addScore(750);
                challengeActive = false;
                printf("Challenge Complete! +750 bonus points!\n");
            }
        } else if(type == 2) {
            bool allFed = true;
            for(auto& f : fishes) {
                if(f.hunger < 0.7f) allFed = false;
            }
            if(allFed) {
                addScore(1000);
                challengeActive = false;
                printf("Challenge Complete! +1000 bonus points!\n");
            }
        }
    }
}


// Draw bubble
void drawBubble(Bubble& b) {
    glPushMatrix();
    glTranslatef(b.x, b.y, b.z);
    // Solid colorful bubbles
    GLfloat bubble_ambient[] = {0.3f, 0.35f, 0.45f, 1.0f};
    GLfloat bubble_diffuse[] = {0.6f + sin(b.lifetime * 3.0f + b.x) * 0.2f,
                                 0.7f + cos(b.lifetime * 2.5f) * 0.2f,
                                 0.9f, 1.0f};
    GLfloat bubble_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat bubble_shininess[] = {100.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, bubble_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, bubble_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, bubble_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, bubble_shininess);
    glutSolidSphere(b.size, 10, 10);
    glPopMatrix();
}

// Draw food
void drawFood() {
    if(feedingMode && foodTime > 0) {
        glPushMatrix();
        glTranslatef(foodX, foodY, foodZ);

        // Colorful glowing food pellet
        GLfloat food_ambient[] = {0.9f, 0.3f, 0.8f, 1.0f};  // Magenta glow
        GLfloat food_diffuse[] = {1.0f, 0.4f, 0.9f, 1.0f};  // Bright pink
        GLfloat food_specular[] = {1.0f, 1.0f, 1.0f, 1.0f}; // White highlights
        GLfloat food_shininess[] = {100.0f};
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, food_ambient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, food_diffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, food_specular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, food_shininess);

        // Pulsating effect
        float pulse = 1.0f + sin(time_val * 10.0f) * 0.2f;
        glutSolidSphere(0.12 * pulse, 12, 12);
        glPopMatrix();
    }
}

// Draw particles
void drawParticles() {
    for(auto& p : particles) {
        // Solid colorful particles with proper lighting
        float hue = fmod(p.lifetime * 5.0f, 6.0f);
        float r = 1.0f, g = 0.5f, b = 0.3f;

        if(hue < 1.0f) { r = 1.0f; g = hue; b = 0.0f; }
        else if(hue < 2.0f) { r = 2.0f - hue; g = 1.0f; b = 0.0f; }
        else if(hue < 3.0f) { r = 0.0f; g = 1.0f; b = hue - 2.0f; }
        else if(hue < 4.0f) { r = 0.0f; g = 4.0f - hue; b = 1.0f; }
        else if(hue < 5.0f) { r = hue - 4.0f; g = 0.0f; b = 1.0f; }
        else { r = 1.0f; g = 0.0f; b = 6.0f - hue; }

        GLfloat particle_ambient[] = {r * 0.5f, g * 0.5f, b * 0.5f, 1.0f};
        GLfloat particle_diffuse[] = {r, g, b, 1.0f};
        GLfloat particle_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
        GLfloat particle_shininess[] = {128.0f};

        glPushMatrix();
        glTranslatef(p.x, p.y, p.z);
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, particle_ambient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, particle_diffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, particle_specular);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, particle_shininess);
        glutSolidSphere(0.06, 8, 8);
        glPopMatrix();
    }
}

// Draw text
void drawText(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    for(const char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
}

// Draw HUD
void drawHUD() {
    if(!showInfo) return;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 1200, 0, 800);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    // Background panel
    glEnable(GL_BLEND);
    glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
    glBegin(GL_QUADS);
    glVertex2f(10, 770);
    glVertex2f(360, 770);
    glVertex2f(360, 580);
    glVertex2f(10, 580);
    glEnd();

    char buffer[128];
    glColor3f(0.0f, 1.0f, 1.0f);
    drawText(20, 750, "=== AQUARIUM CONTROLS ===");
    drawText(20, 730, "Mouse Drag: Rotate Camera");
    drawText(20, 710, "Mouse Wheel: Zoom In/Out");
    drawText(20, 690, "+/- Keys: Zoom");

    glColor3f(1.0f, 1.0f, 0.0f);
    sprintf(buffer, "F: Feed Fish (Mode: %s)", feedingMode ? "ON" : "OFF");
    drawText(20, 665, buffer);
    drawText(20, 645, "C: Start Challenge");
    drawText(20, 625, "S: Show Statistics");
    drawText(20, 605, "N: New Game");
    drawText(20, 585, "1-8: Select Fish");
    drawText(20, 565, "I: Toggle Info");
    drawText(20, 545, "R: Reset Camera");

     // Score panel
    glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
    glBegin(GL_QUADS);
    glVertex2f(450, 770);
    glVertex2f(750, 770);
    glVertex2f(750, 650);
    glVertex2f(450, 650);
    glEnd();

    glColor3f(1.0f, 0.9f, 0.0f);
    drawText(460, 750, "=== SCORE ===");
    sprintf(buffer, "Score: %d", score);
    drawText(460, 725, buffer);
    sprintf(buffer, "High Score: %d", highScore);
    drawText(460, 700, buffer);
    sprintf(buffer, "Combo: %dx", combo);
    glColor3f(1.0f, 0.5f, 0.0f);
    drawText(460, 675, buffer);
    
    if(comboTimer > 0) {
        int barWidth = (int)(comboTimer * 10);
        glColor4f(1.0f, 0.3f, 0.0f, 0.8f);
        glBegin(GL_QUADS);
        glVertex2f(460, 660);
        glVertex2f(460 + barWidth, 660);
        glVertex2f(460 + barWidth, 655);
        glVertex2f(460, 655);
        glEnd();
    }

    // Challenge panel
    if(challengeActive) {
        glColor4f(0.5f, 0.0f, 0.5f, 0.8f);
        glBegin(GL_QUADS);
        glVertex2f(400, 400);
        glVertex2f(800, 400);
        glVertex2f(800, 320);
        glVertex2f(400, 320);
        glEnd();
        
        glColor3f(1.0f, 1.0f, 0.0f);
        drawText(420, 380, "=== CHALLENGE ACTIVE ===");
        drawText(420, 355, challengeText.c_str());
        sprintf(buffer, "Time: %.1fs", challengeTimer);
        drawText(420, 330, buffer);
    }

    // Fish info
    if(selectedFish >= 0 && selectedFish < static_cast<int>(fishes.size())) {
        glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
        glBegin(GL_QUADS);
        glVertex2f(10, 560);
        glVertex2f(320, 560);
        glVertex2f(320, 460);
        glVertex2f(10, 460);
        glEnd();

        glColor3f(0.0f, 1.0f, 0.5f);
        sprintf(buffer, "=== FISH #%d SELECTED ===", selectedFish + 1);
        drawText(20, 540, buffer);
        sprintf(buffer, "Hunger: %.0f%%", fishes[selectedFish].hunger * 100);
        drawText(20, 520, buffer);
        sprintf(buffer, "Speed: %.2f", fishes[selectedFish].speed);
        drawText(20, 500, buffer);
        sprintf(buffer, "Position: (%.1f, %.1f, %.1f)",
                fishes[selectedFish].x, fishes[selectedFish].y, fishes[selectedFish].z);
        drawText(20, 480, buffer);
    }

    // Stats
    glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
    glBegin(GL_QUADS);
    glVertex2f(1040, 770);
    glVertex2f(1190, 770);
    glVertex2f(1190, 680);
    glVertex2f(1040, 680);
    glEnd();

    glColor3f(0.5f, 1.0f, 0.5f);
    drawText(1050, 750, "=== STATS ===");
    sprintf(buffer, "Fish: %d", static_cast<int>(fishes.size()));
    drawText(1050, 730, buffer);
    sprintf(buffer, "Bubbles: %d", static_cast<int>(bubbles.size()));
    drawText(1050, 710, buffer);
    sprintf(buffer, "FPS: ~60", static_cast<int>(bubbles.size()));
    drawText(1050, 690, buffer);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// Draw aquarium
void drawAquarium() {
    // Draw glass frame wireframe (transparent walls)
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glLineWidth(2.0f);

    // Draw glass frame edges
    glColor4f(0.3f, 0.5f, 0.7f, 0.3f);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-8, -5, -8);
    glVertex3f(8, -5, -8);
    glVertex3f(8, 5, -8);
    glVertex3f(-8, 5, -8);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(-8, -5, 8);
    glVertex3f(8, -5, 8);
    glVertex3f(8, 5, 8);
    glVertex3f(-8, 5, 8);
    glEnd();

    glBegin(GL_LINES);
    glVertex3f(-8, -5, -8);
    glVertex3f(-8, -5, 8);
    glVertex3f(8, -5, -8);
    glVertex3f(8, -5, 8);
    glVertex3f(8, 5, -8);
    glVertex3f(8, 5, 8);
    glVertex3f(-8, 5, -8);
    glVertex3f(-8, 5, 8);
    glEnd();

    glLineWidth(1.0f);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

// here was solid sand 

    // Water surface effect (subtle, mostly transparent)
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glColor4f(0.3f, 0.5f, 0.7f, 0.1f);
    float wave1 = sin(time_val * 2.0f) * 0.1f;
    float wave2 = sin(time_val * 2.5f + 1.0f) * 0.1f;
    float wave3 = sin(time_val * 1.8f + 2.0f) * 0.1f;
    glBegin(GL_QUADS);
    glVertex3f(-8, 4.8f + wave1, -8);
    glVertex3f(-8, 4.8f + wave2, 8);
    glVertex3f(8, 4.8f + wave3, 8);
    glVertex3f(8, 4.8f + wave2, -8);
    glEnd();
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}
void drawTexturedFloor() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, floorTexture);

    GLfloat floor_ambient[] = {0.4f, 0.4f, 0.4f, 1.0f};
    GLfloat floor_diffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat floor_specular[] = {0.3f, 0.3f, 0.3f, 1.0f};
    GLfloat floor_shininess[] = {20.0f};

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, floor_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, floor_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, floor_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, floor_shininess);

    glPushMatrix();
    glTranslatef(0, -10.5f, 0);

    // Draw floor as quad with texture coordinates
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glTexCoord2f(0, 0);  glVertex3f(-15, 0, -15);
    glTexCoord2f(10, 0); glVertex3f(15, 0, -15);
    glTexCoord2f(10, 10); glVertex3f(15, 0, 15);
    glTexCoord2f(0, 10);  glVertex3f(-15, 0, 15);
    glEnd();

    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
}

// ============================================================================
// 7. ADD TEXTURED WALL FUNCTION
// ============================================================================

void drawTexturedWall() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, wallTexture);

    GLfloat wall_ambient[] = {0.6f, 0.6f, 0.6f, 1.0f};
    GLfloat wall_diffuse[] = {0.9f, 0.9f, 0.9f, 1.0f};
    GLfloat wall_specular[] = {0.2f, 0.2f, 0.2f, 1.0f};
    GLfloat wall_shininess[] = {10.0f};

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, wall_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, wall_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, wall_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, wall_shininess);

    glPushMatrix();
    glTranslatef(0, 0, -15);

    // Back wall with texture
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    glTexCoord2f(0, 0);  glVertex3f(-15, -10, 0);
    glTexCoord2f(5, 0);  glVertex3f(15, -10, 0);
    glTexCoord2f(5, 3);  glVertex3f(15, 10, 0);
    glTexCoord2f(0, 3);  glVertex3f(-15, 10, 0);
    glEnd();

    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
}

// ============================================================================
// 8. ADD TEXTURED SAND FUNCTION (FOR AQUARIUM FLOOR)
// ============================================================================

void drawTexturedSand() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, sandTexture);

    GLfloat sand_ambient[] = {0.6f, 0.6f, 0.6f, 1.0f};
    GLfloat sand_diffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat sand_specular[] = {0.2f, 0.2f, 0.2f, 1.0f};
    GLfloat sand_shininess[] = {4.0f};

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, sand_ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, sand_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, sand_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, sand_shininess);

    float sandY = -5.2f + 0.25f + 0.15f;

    glPushMatrix();
    glTranslatef(0, sandY, 0);

    // Draw as textured quad
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glTexCoord2f(0, 0);  glVertex3f(-8, 0, -8);
    glTexCoord2f(4, 0);  glVertex3f(8, 0, -8);
    glTexCoord2f(4, 4);  glVertex3f(8, 0, 8);
    glTexCoord2f(0, 4);  glVertex3f(-8, 0, 8);
    glEnd();

    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
}
// Display
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Set camera
    float camX = cameraDistance * sin(cameraAngleY * M_PI / 180.0f) * cos(cameraAngleX * M_PI / 180.0f);
    float camY = cameraDistance * sin(cameraAngleX * M_PI / 180.0f);
    float camZ = cameraDistance * cos(cameraAngleY * M_PI / 180.0f) * cos(cameraAngleX * M_PI / 180.0f);

    gluLookAt(camX, camY, camZ, 0, 0, 0, 0, 1, 0);

    // Update light positions
    GLfloat light0_pos[] = {5.0f, 8.0f, 5.0f, 1.0f};
    GLfloat light1_pos[] = {-5.0f, 5.0f, -5.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);
    glLightfv(GL_LIGHT1, GL_POSITION, light1_pos);

    drawTexturedWall();
    drawTexturedFloor();
    drawWindow();    // Draw window
    drawShelf();     // Draw shelf
    drawTable();
    // Draw aquarium
    drawAquarium();
    drawTexturedSand();
    // Draw rocks
    drawRock(-5, -4.0f, -3, 1.5f);
    drawRock(4, -4.2f, 2, 1.2f);
    drawRock(-2, -4.3f, 5, 0.8f);
    drawRock(3, -4.1f, -5, 1.0f);

    // draw plants
    for(auto& plant : plants) {
        drawPlant(plant);
    }
    // Draw fish
    for(size_t i = 0; i < fishes.size(); i++) {
        drawFish(fishes[i], static_cast<int>(i));
    }

    // Draw bubbles (now solid)
    for(auto& bubble : bubbles) {
        drawBubble(bubble);
    }

    // Draw particles
    drawParticles();

    // Draw food
    drawFood();

    // Draw HUD
    drawHUD();

    glutSwapBuffers();
}

// Update
void update(int value) {
    time_val += 0.016f;

    // Update combo timer
    if(comboTimer > 0) {
        comboTimer -= 0.016f;
        if(comboTimer <= 0) {
            combo = 0;
        }
    }

    // Food physics 
    if(foodTime > 0) {
        foodTime -= 0.016f;
        foodY -= 0.015f; 
        if(foodY < -4.5f) {
            foodTime = 0;
            // Create splash particles
            for(int i = 0; i < 10; i++) {
                Particle p;
                p.x = foodX;
                p.y = -4.3f;
                p.z = foodZ;
                p.vx = (rand() % 100 - 50) / 200.0f;
                p.vy = (rand() % 100) / 200.0f;
                p.vz = (rand() % 100 - 50) / 200.0f;
                p.lifetime = 0;
                p.maxLifetime = 0.5f + (rand() % 100) / 200.0f;
                particles.push_back(p);
            }
        }
    }

    // Update particles
    for(auto it = particles.begin(); it != particles.end();) {
        it->x += it->vx;
        it->y += it->vy;
        it->z += it->vz;
        it->vy -= 0.01f; // Gravity
        it->lifetime += 0.016f;
        if(it->lifetime >= it->maxLifetime) {
            it = particles.erase(it);
        } else {
            ++it;
        }
    }

    // Update fish
    for(size_t i = 0; i < fishes.size(); i++) {
        Fish& fish = fishes[i];

        // Target direction (default: current direction)
        float targetAngle = fish.angle;
        bool hasTarget = false;
        bool chasingFood = false;

        // Swim towards food if hungry 
        if(feedingMode && foodTime > 0) {
            float dx = foodX - fish.x;
            float dy = foodY - fish.y;
            float dz = foodZ - fish.z;
            float dist = sqrt(dx*dx + dy*dy + dz*dz);

            if(dist < 8.0f && fish.isHungry && fish.hunger < 0.8f) { 
                targetAngle = atan2(dz, dx) * 180.0f / M_PI;
                hasTarget = true;
                chasingFood = true;
                fish.targetY = foodY;
                fish.speed = 1.0f;  // Faster when chasing food
                
                if(dist < 0.8f) {  //
                    fish.hunger += 0.3f;
                    if(fish.hunger > 1.0f) fish.hunger = 1.0f;
                    
                    // Scoring system
                    int points = 50;
                    if(dist < 0.4f) {
                        points = 100;
                        perfectFeeds++;
                        printf("Perfect Feed! +100 points\n");
                    } else {
                        printf("Good Feed! +50 points\n");
                    }
                    
                    feedCount++;
                    totalFeedings++;
                    combo++;
                    comboTimer = 3.0f;
                    
                    addScore(points);
                    
                    if(challengeActive) {
                        challengeComplete++;
                    }
                    
                    if(fish.hunger > 0.7f) {
                        fish.isHungry = false;
                    }
                    foodTime = 0;  // Remove food
                    
                    printf("Fish #%zu ate! Score: %d | Combo: %dx | Total Feeds: %d\n", 
                           i+1, score, combo, feedCount);
                }
            }
        }

        // Calculate next position
        float rad = fish.angle * M_PI / 180.0f;
        float nextX = fish.x + cos(rad) * fish.speed * 0.05f;
        float nextZ = fish.z + sin(rad) * fish.speed * 0.05f;

        if(!chasingFood) {
            float wallBuffer = 1.5f; // Start turning before hitting wall
            
            if(nextX > (7.0f - wallBuffer)) {
                targetAngle = 180.0f + (rand() % 40 - 20);
                hasTarget = true;
            } else if(nextX < (-7.0f + wallBuffer)) {
                targetAngle = 0.0f + (rand() % 40 - 20);
                hasTarget = true;
            }
            
            if(nextZ > (7.0f - wallBuffer)) {
                targetAngle = 270.0f + (rand() % 40 - 20);
                hasTarget = true;
            } else if(nextZ < (-7.0f + wallBuffer)) {
                targetAngle = 90.0f + (rand() % 40 - 20);
                hasTarget = true;
            }
        }

        if(nextX > 7.5f) nextX = 7.4f;
        if(nextX < -7.5f) nextX = -7.4f;
        if(nextZ > 7.5f) nextZ = 7.4f;
        if(nextZ < -7.5f) nextZ = -7.4f;

        // Smooth turning towards target
        if(hasTarget) {
            float diff = targetAngle - fish.angle;
            
            // Normalize to shortest path
            while (diff > 180.0f) diff -= 360.0f;
            while (diff < -180.0f) diff += 360.0f;
            
            // Faster turning when chasing food
            float turnRate = chasingFood ? 3.0f : 1.5f;  // Even faster when chasing
            if (fabs(diff) < turnRate) {
                fish.angle = targetAngle;
            } else {
                fish.angle += (diff > 0 ? turnRate : -turnRate);
            }
        }

        // Normalize angle
        while(fish.angle > 180.0f) fish.angle -= 360.0f;
        while(fish.angle < -180.0f) fish.angle += 360.0f;

        // Update position
        fish.x = nextX;
        fish.z = nextZ;

        // FASTER vertical movement when chasing food
        if(fabs(fish.y - fish.targetY) > 0.1f) {
            float verticalSpeed = chasingFood ? 0.05f : 0.02f;
            fish.y += (fish.targetY - fish.y) * verticalSpeed;
        }

        // Hunger system
        fish.hunger -= 0.0002f;
        if(fish.hunger < 0.2f) {
            fish.hunger = 0.2f;
            fish.isHungry = true;
        }

        // Keep within vertical bounds
        if(fish.y > 4) {
            fish.targetY = 3.5f;
        } else if(fish.y < -3) {
            fish.targetY = -2.5f;
        }

        // Occasional random wandering (very subtle, not when chasing food)
        if(!chasingFood && !hasTarget && rand() % 600 < 1) {
            fish.angle += (rand() % 10 - 5);
        }

        // Random vertical movement (not when chasing food)
        if(!chasingFood && rand() % 400 < 1) {
            fish.targetY = (rand() % 60 - 30) / 10.0f;
        }

        // Slow down after feeding
        if(!fish.isHungry && fish.speed > 0.4f) {
            fish.speed -= 0.001f;
        }
    }

    // Create bubbles
    if(rand() % 100 < 30 && bubbles.size() < 60) {
        Bubble b;
        b.x = (rand() % 140 - 70) / 10.0f;
        b.y = -4.5f + (rand() % 20) / 20.0f;
        b.z = (rand() % 140 - 70) / 10.0f;
        b.speed = 0.02f + (rand() % 100) / 2000.0f;
        b.size = 0.05f + (rand() % 100) / 2000.0f;
        b.lifetime = 0;
        bubbles.push_back(b);
    }

    // Update bubbles
    for(auto it = bubbles.begin(); it != bubbles.end();) {
        it->y += it->speed;
        it->lifetime += 0.016f;
        it->x += sin(it->lifetime * 2.0f) * 0.01f;
        it->z += cos(it->lifetime * 1.5f) * 0.008f;

        if(it->y > 5.0f) {
            it = bubbles.erase(it);
        } else {
            ++it;
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}
// Reshape
void reshape(int w, int h) {
    if(h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, static_cast<float>(w) / static_cast<float>(h), 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

// Mouse click
void mouse(int button, int state, int x, int y) {
    if(button == GLUT_LEFT_BUTTON) {
        if(state == GLUT_DOWN) {
            if(feedingMode) {
                // Drop food at clicked position
                GLdouble modelMatrix[16];
                GLdouble projMatrix[16];
                GLint viewport[4];

                glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
                glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
                glGetIntegerv(GL_VIEWPORT, viewport);

                GLdouble objX, objY, objZ;
                GLfloat winZ;
                glReadPixels(x, viewport[3] - y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);

                if(winZ < 1.0f) {
                    gluUnProject(x, viewport[3] - y, winZ, modelMatrix, projMatrix, viewport, &objX, &objY, &objZ);
                    foodX = static_cast<float>(objX);
                    foodY = 4.5f;
                    foodZ = static_cast<float>(objZ);

                    // Clamp to aquarium bounds
                    if(foodX > 7.5f) foodX = 7.5f;
                    if(foodX < -7.5f) foodX = -7.5f;
                    if(foodZ > 7.5f) foodZ = 7.5f;
                    if(foodZ < -7.5f) foodZ = -7.5f;

                    foodTime = 15.0f;
                    printf("Food dropped at: (%.2f, %.2f, %.2f)\n", foodX, foodY, foodZ);
                }
            } else {
                mouseLeftDown = true;
                lastMouseX = x;
                lastMouseY = y;
            }
        } else {
            mouseLeftDown = false;
        }
    }

    // Zoom with mouse wheel
    if(button == 3) {
        cameraDistance -= 0.8f;
        if(cameraDistance < 5.0f) cameraDistance = 5.0f;
        glutPostRedisplay();
    } else if(button == 4) {
        cameraDistance += 0.8f;
        if(cameraDistance > 30.0f) cameraDistance = 30.0f;
        glutPostRedisplay();
    }
}

// Mouse motion
void motion(int x, int y) {
    if(mouseLeftDown) {
        cameraAngleY += (x - lastMouseX) * 0.5f;
        cameraAngleX += (y - lastMouseY) * 0.5f;

        if(cameraAngleX > 89.0f) cameraAngleX = 89.0f;
        if(cameraAngleX < -89.0f) cameraAngleX = -89.0f;

        lastMouseX = x;
        lastMouseY = y;

        glutPostRedisplay();
    }
}

// Keyboard
void keyboard(unsigned char key, int x, int y) {
    switch(key) {
        case 'c':
        case 'C':
            if(!challengeActive) {
                startChallenge();
            }
            break;
        case 's':
        case 'S':
            printf("\n=== STATISTICS ===\n");
            printf("Score: %d\n", score);
            printf("High Score: %d\n", highScore);
            printf("Total Feedings: %d\n", totalFeedings);
            printf("Perfect Feeds: %d\n", perfectFeeds);
            printf("Current Combo: %dx\n", combo);
            printf("Fish Count: %d\n", (int)fishes.size());
            break;
        case 'n':
        case 'N':
            // Reset game
            score = 0;
            feedCount = 0;
            combo = 0;
            comboTimer = 0;
            totalFeedings = 0;
            perfectFeeds = 0;
            challengeActive = false;
            printf("Game reset! High score preserved: %d\n", highScore);
            break;

        case 27: // ESC
            exit(0);
            break;
        case '+':
        case '=':
            cameraDistance -= 0.8f;
            if(cameraDistance < 5.0f) cameraDistance = 5.0f;
            printf("Zoom in: %.1f\n", cameraDistance);
            break;
        case '-':
        case '_':
            cameraDistance += 0.8f;
            if(cameraDistance > 30.0f) cameraDistance = 30.0f;
            printf("Zoom out: %.1f\n", cameraDistance);
            break;
        case 'f':
        case 'F':
            feedingMode = !feedingMode;
            printf("Feeding mode: %s\n", feedingMode ? "ON" : "OFF");
            break;
        case 'i':
        case 'I':
            showInfo = !showInfo;
            printf("Info display: %s\n", showInfo ? "ON" : "OFF");
            break;
        case 'r':
        case 'R':
            cameraAngleX = 20.0f;
            cameraAngleY = 0.0f;
            cameraDistance = 15.0f;
            printf("Camera reset\n");
            break;
        case '0':
        case ' ':
            // Unselect all fish
            selectedFish = -1;
            for(auto& f : fishes) {
                f.isSelected = false;
            }
            printf("All fish unselected\n");
            break;
        case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8':
            selectedFish = key - '1';
            if(selectedFish >= static_cast<int>(fishes.size())) {
                selectedFish = -1;
            } else {
                for(size_t i = 0; i < fishes.size(); i++) {
                    fishes[i].isSelected = (static_cast<int>(i) == selectedFish);
                }
                printf("Fish #%d selected (Hunger: %.0f%%)\n",
                       selectedFish + 1, fishes[selectedFish].hunger * 100);
            }
            break;
    }
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1200, 800);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("3D Aquarium Simulation");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(16, update, 0);

    glutMainLoop();
    return 0;
}
