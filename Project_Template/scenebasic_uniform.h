#ifndef SCENEBASIC_UNIFORM_H
#define SCENEBASIC_UNIFORM_H

#include "helper/scene.h"

#include <glad/glad.h>
#include "helper/glslprogram.h"

#include <glm/gtc/matrix_transform.hpp>
#include "helper/plane.h"
#include "helper/objmesh.h"
#include "helper/Additional files/torus.h"
#include "helper/Additional files/teapot.h"
#include "helper/Additional files/teapotdata.h"
#include "helper/cube.h"
#include <glm/glm.hpp>
#include "helper/frustum.h";
#include "helper/random.h";
#include "helper/grid.h";
#include "helper/Additional files/teapot.h"

class SceneBasic_Uniform : public Scene
{
private:
    GLSLProgram prog, flatProg, blinnProg, textProg, transProg, solidProg;

    Random rand;

    GLuint shadowFBO, pass1Index, pass2Index, initVel, startTime, particles, nParticles;

    //Grid grid;

    glm::vec3 emitterPos, emitterDir;

    float angle, time, particleLifetime, timerA, timerB, timerC;

    Plane plane, floor;

    Teapot teapot;

    Torus donuut, column, ball;

    Cube cube;

    int shadowMapWidth, shadowMapHeight, samplesU, samplesV, jitterMapSize;
    float tPrev, radius;

    glm::mat4 lightPV, shadowBias;

    Frustum lightFrustrum;

    void initBuffers();
    float randFloat();
    float jitter();
    void buildJitterTex();

    void setMatrices(GLSLProgram&);
    
    void compile();    

    void setupFBO();
    void drawScene();
    void spitOutDepthBuffer();

public:
    SceneBasic_Uniform();

    void initScene();
    void update( float t );
    void render();
    void resize(int, int);
};

#endif // SCENEBASIC_UNIFORM_H
