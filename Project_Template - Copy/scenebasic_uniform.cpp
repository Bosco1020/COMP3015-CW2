#include "scenebasic_uniform.h"

#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <sstream>

#include <string>
using std::string;

#include <iostream>
using std::cerr;
using std::endl;

#include <glm/gtc/matrix_transform.hpp>
using glm::vec3;
using glm::mat4;

#include "helper/glutils.h"

#include "scenebasic_uniform.h"

#include "helper/texture.h"
#include "helper/particleutils.h";

float speed = 0.0f;
// plane(7.2f, 7.2f, 1, 1)

SceneBasic_Uniform::SceneBasic_Uniform() : time(0), particleLifetime(6.0f), nParticles(16000), cube(0.6f), floor(2.0f, 2.0f, 1, 1),
                                            emitterPos(-11.3, 0.4, -15), emitterDir(5, 10, 0), column(0.7f, 0.3f, 50, 50), ball (0.01f, 1.0f, 80, 80),
                                            plane(24.0f, 24.0f, 1, 1), donuut(6.0f, 0.8f, 80, 80), teapot(14, glm::mat4(1.0f))
{
    shadowMapWidth = 512;
    shadowMapHeight = 512;
    samplesU = 4;
    samplesV = 8;
    jitterMapSize = 8;
    radius = 7.0f;
}

void SceneBasic_Uniform::initScene()
{
    compile();

    glClearColor(0.5f, 0.5f, 0.5f, 0.5f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    angle = glm::half_pi<float>();

    setupFBO();
    buildJitterTex();

    initBuffers();

    prog.use();
    prog.setUniform("ParticleTex", 0);
    prog.setUniform("ParticleLifetime", particleLifetime);
    prog.setUniform("ParticleSize", 0.1f);
    prog.setUniform("Gravity", vec3(0.0f, -0.4f, 0.0f));
    prog.setUniform("EmitterPos", emitterPos);

    flatProg.use();
    flatProg.setUniform("Color", glm::vec4(0.4f, 0.4f, 0.4f, 1.0f));


    GLuint programHandle = transProg.getHandle();
    pass1Index = glGetSubroutineIndex(programHandle, GL_FRAGMENT_SHADER, "recordDepth");
    pass2Index = glGetSubroutineIndex(programHandle, GL_FRAGMENT_SHADER, "shadeWithShadow");

    shadowBias = mat4(glm::vec4(0.5f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.5f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 0.5f, 0.0f), glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));

    blinnProg.use();
    vec3 lightPos = vec3(2.5f, 6.0f, 8.0f);
    blinnProg.setUniform("lights[0].Position", lightPos);
    blinnProg.setUniform("Fog.MaxDist", 0.0f);
    blinnProg.setUniform("Fog.MinDist", -2.5f);
    blinnProg.setUniform("Fog.Color", vec3(0.2f, 0.8, 0.9) * vec3(1.5f));
    blinnProg.setUniform("fogStrength", 1.0f);

    lightFrustrum.orient(lightPos, vec3(0.0f), vec3(0.0f, 1.0f, 0.0f));
    lightFrustrum.setPerspective(50.0f, 1.0f, 1.0f, 25.0f);

    lightPV = shadowBias * lightFrustrum.getProjectionMatrix() * lightFrustrum.getViewMatrix();


    textProg.use();
    textProg.setUniform("lights[0].Position", lightPos);
    textProg.setUniform("lights[0].L", vec3(0.9, 0.9, 0.9));
    textProg.setUniform("lights[0].La", vec3(0.2, 0.2, 0.2));

    blinnProg.use();
    blinnProg.setUniform("lights[0].Position", lightPos);
    blinnProg.setUniform("lights[0].L", vec3(0.9, 0.9, 0.9));
    blinnProg.setUniform("lights[0].La", vec3(0.2, 0.2, 0.2));

    transProg.use();
    transProg.setUniform("ShadowMap", 0);
    transProg.setUniform("lights[0].Position", lightPos);
    transProg.setUniform("lights[0].L", vec3(0.9, 0.9, 0.9));
    transProg.setUniform("lights[0].La", vec3(0.2, 0.2, 0.2));
    transProg.setUniform("offsetTex", 1);
    transProg.setUniform("Radius", radius / 512.0f);
    transProg.setUniform("OffsetTexSize", vec3(jitterMapSize, jitterMapSize, samplesU * samplesV / 2.0f));
}

void SceneBasic_Uniform::initBuffers()
{
    glGenBuffers(1, &initVel);
    glGenBuffers(1, &startTime);

    int size = nParticles * sizeof(float);
    glBindBuffer(GL_ARRAY_BUFFER, initVel);
    glBufferData(GL_ARRAY_BUFFER, size * 3, 0, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, startTime);
    glBufferData(GL_ARRAY_BUFFER, size, 0, GL_STATIC_DRAW);

    glm::mat3 emitterBasis = ParticleUtils::makeArbitraryBasis(emitterDir);
    vec3 v(0.0f);
    float velocity, theta, phi;
    std::vector<GLfloat> data(nParticles * 3);
    for (int i = 0; i < nParticles; i++) {

        theta = glm::mix(0.0f, glm::pi<float>() / 20.0f, randFloat());
        phi = glm::mix(0.0f, glm::two_pi<float>(), randFloat());

        v.x = sinf(theta) * cosf(phi);
        v.y = cosf(theta);
        v.z = sinf(theta) * sinf(phi);

        velocity = glm::mix(1.25f, 1.5f, randFloat());
        v = glm::normalize(emitterBasis * v) * velocity;

        data[3 * i] = v.x;
        data[3 * i + 1] = v.y;
        data[3 * i + 2] = v.z;
    }

    glBindBuffer(GL_ARRAY_BUFFER, initVel);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size * 3, data.data());

    float rate = particleLifetime / nParticles;
    for (int i = 0; i < nParticles; i++) {
        data[i] = rate * i;
    }
    glBindBuffer(GL_ARRAY_BUFFER, startTime);
    glBufferSubData(GL_ARRAY_BUFFER, 0, nParticles * sizeof(float), data.data());

    glBindBuffer(GL_ARRAY_BUFFER, 1);

    glGenVertexArrays(1, &particles);
    glBindVertexArray(particles);
    glBindBuffer(GL_ARRAY_BUFFER, initVel);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, startTime);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    glVertexAttribDivisor(0, 1);
    glVertexAttribDivisor(1, 1);

    glBindVertexArray(0);
}

float SceneBasic_Uniform::randFloat()
{
    return rand.nextFloat();
}

float SceneBasic_Uniform::jitter()
{
    // return a random number within range
    static std::default_random_engine generator;
    static std::uniform_real_distribution<float> distrib(-0.5f, 0.5f);
    return distrib(generator);
}

void SceneBasic_Uniform::buildJitterTex()
{
    int size = jitterMapSize;
    int samples = samplesU * samplesV;
    int bufSize = size * size * samples * 2;
    float* data = new float[bufSize];
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            for (int k = 0; k < samples; k++)
            {
                int x1, y1, x2, y2;
                x1 = k % (samplesU);
                y1 = (samples - 1 - k) / samplesU;
                x2 = (k + 1) % samplesU;
                y2 = (samples - 1 - k - 1) / samplesU;

                glm::vec4 v;
                v.x = (x1 + 0.5f) + jitter();
                v.y = (y1 + 0.5f) + jitter();
                v.z = (x2 + 0.5f) + jitter();
                v.w = (y2 + 0.5f) + jitter();

                v.x /= samplesU;
                v.y /= samplesU;
                v.z /= samplesU;
                v.w /= samplesU;

                int cell = ((k / 2) * size * size + j * size + i) * 4;
                data[cell + 0] = sqrtf(v.y) * cosf(glm::two_pi<float>() * v.x);
                data[cell + 1] = sqrtf(v.y) * cosf(glm::two_pi<float>() * v.x);
                data[cell + 2] = sqrtf(v.w) * cosf(glm::two_pi<float>() * v.z);
                data[cell + 3] = sqrtf(v.w) * cosf(glm::two_pi<float>() * v.z);
            }
        }
    }

    glActiveTexture(GL_TEXTURE1);
    GLuint texID;
    glGenTextures(1, &texID);

    glBindTexture(GL_TEXTURE_3D, texID);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA32F, size, size, samples / 2);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, size, size, samples / 2, GL_RGBA, GL_FLOAT, data);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    delete[] data;
}

void SceneBasic_Uniform::compile()
{
	try {
        prog.compileShader("shader/fountain_vert.vert");
        prog.compileShader("shader/fountain_frag.frag");
        prog.link();
        prog.use();

        solidProg.compileShader("shader/solid.vert", GLSLShader::VERTEX);
        solidProg.compileShader("shader/solid.frag", GLSLShader::FRAGMENT);
        solidProg.link();

        textProg.compileShader("shader/BlinnPhongTextured.frag");
        textProg.compileShader("shader/BlinnPhongTextured.vert");
        textProg.link();

        flatProg.compileShader("shader/flat_frag.frag");
        flatProg.compileShader("shader/flat_vert.vert");
        flatProg.link();

        blinnProg.compileShader("shader/BlinnPhong.frag");
        blinnProg.compileShader("shader/BlinnPhong.vert");
        blinnProg.link();

        transProg.compileShader("shader/BlinnPhongTransparent.frag");
        transProg.compileShader("shader/BlinnPhongTransparent.vert");
        transProg.link();

	} catch (GLSLProgramException &e) {
		cerr << e.what() << endl;
		exit(EXIT_FAILURE);
	}
}

void SceneBasic_Uniform::update( float t )
{
    /*
    speed += 1.0f;
    if (speed > 360)
    {
        speed = 0.0f;
    }*/



    float deltaT = t - tPrev;
    if (tPrev == 0.0f)
        deltaT = 0.0f;

     tPrev = t;

    timerA += 0.1f * deltaT;
    timerB += 0.3 * deltaT;
    angle += 0.2f * deltaT;

    if (angle > glm::two_pi<float>())
        angle -= glm::two_pi<float>();

    if (timerA > glm::two_pi<float>())
        timerA -= glm::two_pi<float>();

    if (timerB > glm::two_pi<float>())
        timerB -= glm::two_pi<float>();

     time += deltaT;
    if (time >= (particleLifetime + 7.0f)) {
        time = 0;
    }

    //time = t;
    angle = std::fmod(angle + 0.01f, glm::two_pi<float>());
}

void SceneBasic_Uniform::setupFBO() {

    // Bind Textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    GLuint fountain = Texture::loadTexture("../Project_Template/media/texture/bluewater.png");

    GLuint Tex1 = Texture::loadTexture("../Project_Template/media/texture/cement.jpg");
    GLuint Tex2 = Texture::loadTexture("../Project_Template/media/texture/moss.png");
    GLuint Tex3 = Texture::loadTexture("../Project_Template/media/texture/smoke.png");
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, Tex1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, Tex2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, Tex3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, fountain);

    // Bind Shadow Map
    GLfloat border[] = { 1.0f, 0.0f, 0.0f, 0.0f };

    GLuint depthTex;
    glGenTextures(1, &depthTex);
    glBindTexture(GL_TEXTURE_2D, depthTex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT24, shadowMapWidth, shadowMapHeight);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthTex);

    glGenFramebuffers(1, &shadowFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);

    GLenum drawBuffers[] = { GL_NONE };
    glDrawBuffers(1, drawBuffers);

    GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (result == GL_FRAMEBUFFER_COMPLETE) {
        printf("Framebuffer is complete. \n");
    }
    else {
        printf("Framebuffer is not complete.\n");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);   
}

void SceneBasic_Uniform::render()
{
    //vec3 lightPos = vec3(4.5f + sin(angle), 6.0f, 8.0f + cos(angle));
    /*vec3 lightPos = vec3(1.5f, 6.0f, 8.0f);

    lightFrustrum.orient(lightPos, vec3(0.0f), vec3(0.0f, 1.0f, 0.0f));
    lightFrustrum.setPerspective(50.0f, 1.0f, 1.0f, 25.0f);

    lightPV = shadowBias * lightFrustrum.getProjectionMatrix() * lightFrustrum.getViewMatrix();

    textProg.use();
    textProg.setUniform("lights[0].Position", lightPos);

    blinnProg.use();
    blinnProg.setUniform("lights[0].Position", lightPos);

    transProg.use();
    transProg.setUniform("lights[0].Position", lightPos);*/


    //Pass 1
    view = lightFrustrum.getViewMatrix();
    projection = lightFrustrum.getProjectionMatrix();
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, shadowMapWidth, shadowMapHeight);
    glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &pass1Index);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.5f, 10.0f);
    drawScene();
    glDisable(GL_POLYGON_OFFSET_FILL);
    glCullFace(GL_BACK);
    glFlush();

    //Pass 2
    float c = 1.0f;
    //vec3 cameraPos(c * 11.5f * cos(angle), c * 7.0f, c * 11.5f * sin(angle));
    vec3 cameraPos(c * 0.0f, c * 3.5f, c * 10.0f);
    view = glm::lookAt(cameraPos, vec3(0.0f, 2.0f, 1.0f), vec3(0.0f, 1.0f, 0.0f));
    model = mat4(1.0f);

    textProg.setUniform("lights[0].Position", view * glm::vec4(lightFrustrum.getOrigin(), 1.0f));
    blinnProg.setUniform("lights[0].Position", view * glm::vec4(lightFrustrum.getOrigin(), 1.0f));
    transProg.setUniform("lights[0].Position", view * glm::vec4(lightFrustrum.getOrigin(), 1.0f));

    projection = glm::perspective(glm::radians(50.0f), (float)width / height, 0.1f, 100.0f);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, width, height);
    glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &pass2Index);

    drawScene();

    flatProg.use();
    setMatrices(flatProg);

    glDepthMask(GL_FALSE);
    prog.use();
    glActiveTexture(GL_TEXTURE4);
    prog.setUniform("ParticleTex", 4);

    GLuint fountain = Texture::loadTexture("../Project_Template/media/texture/bluewater.png");
    glBindTexture(GL_TEXTURE_2D, fountain);

    model = mat4(1.0f);
    model = glm::scale(model, vec3(0.3f));
    mat4 mv = model * view;
    prog.setUniform("MV", mv);
    prog.setUniform("Proj", projection);
    prog.setUniform("Time", time);

    glBindVertexArray(particles);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, nParticles);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    
    /*
    solidProg.use();
    solidProg.setUniform("Color", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    mv = view * lightFrustrum.getInverseViewMatrix();
    solidProg.setUniform("MVP", projection * mv);
    lightFrustrum.render();
    */
}

void SceneBasic_Uniform::resize(int w, int h)
{
    glViewport(0, 0, w, h);
    width = w;
    height = h;
}

void SceneBasic_Uniform::setMatrices(GLSLProgram& p)
{
    mat4 mv = model * view;
    p.setUniform("ModelViewMatrix", mv);

    p.setUniform("NormalMatrix", glm::mat3(vec3(mv[0]),
        vec3(mv[1]), vec3(mv[2])));

    p.setUniform("MVP", projection * mv);
    p.setUniform("ShadowMatrix", lightPV * model);
}

void SceneBasic_Uniform::drawScene() {
    vec3 color = vec3(1.0f, 1.0f, 1.0f);
    transProg.use();
    transProg.setUniform("Material.Kd", color);
    transProg.setUniform("Material.Ks", 0.9f, 0.9f, 0.9f);
    transProg.setUniform("Material.Ka", color * 0.05f);
    transProg.setUniform("Material.Shininess", 150.0f);

    // Transparent Floor
    model = mat4(1.0f);
    model = glm::translate(model, vec3(-3.6f, 0.0f, 1.2f));
    setMatrices(transProg);
    plane.render();
    //floor.render();

    textProg.use();
    textProg.setUniform("Material.Kd", color);
    textProg.setUniform("Material.Ks", 0.9f, 0.9f, 0.9f);
    textProg.setUniform("Material.Ka", color * 0.05f);
    textProg.setUniform("Material.Shininess", 150.0f);

    //Floor Tiles
    model = mat4(1.0f);
    model = glm::translate(model, vec3(4.0f, 1.0f, 2.2f));
    setMatrices(textProg);
    //plane.render();

    model = mat4(1.0f);
    model = glm::translate(model, vec3(3.8f, 0.5f, 10.4f));
    setMatrices(textProg);
    //plane.render();

    model = mat4(1.0f);
    model = glm::translate(model, vec3(-3.0f, 0.0f, 9.9f));
    setMatrices(textProg);
    //plane.render();


    //left
    model = mat4(1.0f);
    model = glm::translate(model, vec3(-3.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(-90.0f), vec3(0.0f, 0.0f, 1.0f));
    setMatrices(textProg);
    //plane.render();

    //right
    model = mat4(1.0f);
    model = glm::translate(model, vec3(3.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(90.0f), vec3(0.0f, 0.0f, 1.0f));
    setMatrices(textProg);
    //plane.render();

    //back
    model = mat4(1.0f);
    model = glm::translate(model, vec3(3.0f, 1.0f, -11.0f));
    model = glm::rotate(model, glm::radians(-90.0f), vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(80.0f), vec3(1.0f, 0.0f, 0.0f));
    setMatrices(textProg);
    //plane.render();


    // Columns
    /*
    model = mat4(1.0f);
    model = glm::scale(model, vec3(1.0f, 8.0f, 1.0f));
    model = glm::translate(model, vec3(4.0f, 2.0f, 4.5f));
    setMatrices(textProg);
    cube.render();

    model = mat4(1.0f);
    model = glm::scale(model, vec3(1.0f, 8.0f, 1.0f));
    model = glm::translate(model, vec3(-4.0f, 1.9f, 4.5f));
    setMatrices(textProg);
    cube.render();
    */

    model = mat4(1.0f);
    model = glm::scale(model, vec3(1.0f, 8.0f, 1.0f));
    model = glm::translate(model, vec3(4.0f, 2.0f, -3.5f));
    setMatrices(textProg);
    //cube.render();

    model = mat4(1.0f);
    model = glm::scale(model, vec3(1.0f, 8.0f, 1.0f));
    model = glm::translate(model, vec3(-4.0f, 2.0f, -3.5f));
    setMatrices(textProg);
    //cube.render();


    blinnProg.use();
    color = vec3(1.0f, 1.0f, 1.0f);

    blinnProg.setUniform("Material.Kd", color);
    blinnProg.setUniform("Material.Ks", 0.9f, 0.9f, 0.9f);
    blinnProg.setUniform("Material.Ka", color * 0.05f);
    blinnProg.setUniform("Material.Shininess", 150.0f);
    blinnProg.setUniform("fogStrength", 1.0f);

    // Pool Bottom
    model = mat4(1.0f);
    model = glm::translate(model, vec3(-4.2f, -2.5f, 0.0f));
    setMatrices(blinnProg);
    plane.render();

    color = vec3(0.3f, 0.4f, 0.7f);
    blinnProg.setUniform("Material.Kd", color);
    blinnProg.setUniform("Material.Ka", color * 0.05f);

    /*
    model = mat4(1.0f);
    model = glm::scale(model, vec3(8.0f, 1.0f, 1.0f));
    model = glm::translate(model, vec3(0.0f, 0.0f, 3.0f));
    setMatrices();
    cube.render();

    model = mat4(1.0f);
    model = glm::scale(model, vec3(8.0f, 1.0f, 1.0f));
    model = glm::translate(model, vec3(0.0f, 1.0f, -2.0f));
    setMatrices();
    cube.render();

    //model = mat4(1.0f);
    //model = glm::scale(model, vec3(80.0f, 1.0f, 1.0f));
    //model = glm::translate(model, vec3(-0.5f, 1.0f, 0.0f));
    //model = glm::rotate(model, glm::radians(-5.0f), vec3(0.0f, 1.0f, 0.0f));
    //setMatrices();
    //cube.render();


    model = mat4(1.0f);
    model = glm::rotate(model, glm::radians(-90.0f), vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(-7.0f), vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(-10.0f), vec3(1.0f, 0.0f, 0.0f));
    model = glm::scale(model, vec3(8.0f, 1.0f, 1.0f));
    model = glm::translate(model, vec3(-1.2f, -1.2f, 12.0f));
    setMatrices();
    cube.render();*/

    // Big Ring
    model = mat4(1.0f);
    model = glm::translate(model, vec3(-3.2f, 4.2f, -9.3f));
    model = glm::rotate(model, glm::radians(-90.0f), vec3(1.0f, 0.0f, 0.0f));
    model = glm::scale(model, vec3(0.6f, 0.6f, 0.6f));
    setMatrices(blinnProg);
    donuut.render();

    model = glm::translate(model, vec3(0.0f, 0.0f, -1.0f));
    setMatrices(blinnProg);
    donuut.render();

    color = vec3(0.9f, 0.9f, 0.9f);
    blinnProg.setUniform("Material.Kd", color);
    blinnProg.setUniform("Material.Ka", color * 0.05f);
    blinnProg.setUniform("Material.Ks", 0.9f, 0.9f, 0.9f);
    blinnProg.setUniform("Material.Shininess", 150.0f);

    model = mat4(1.0f);
    //model = glm::scale(model, vec3(1.0f, 4.0f, 1.0f));
    model = glm::translate(model, vec3(-5.2f, 3.1f, -9.3f));
    model = glm::rotate(model, glm::radians(-90.0f), vec3(1.0f, 0.0f, 0.0f));
    //model = glm::rotate(model, glm::radians(45.0f), vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, vec3(0.5f));

    //model = glm::translate(model, vec3(0.0f, -1.0f, 0.0f));
    setMatrices(blinnProg);
    column.render();

    for (int i = 0; i < 5; i++)
    {
        model = glm::translate(model, vec3(0.0f, 0.0f, 0.5f));
        setMatrices(blinnProg);
        column.render();
    }


    color = vec3(0.3f, 0.5f, 0.9f);
    blinnProg.setUniform("Material.Kd", color);
    blinnProg.setUniform("Material.Ka", color * 0.05f);

    model = mat4(1.0f);
    model = glm::translate(model, vec3(-5.3f, 4.3f, -9.5f));
    model = glm::rotate(model, glm::radians(-90.0f), vec3(1.0f, 0.0f, 0.0f));
    //model = glm::rotate(model, glm::radians(45.0f), vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, vec3(0.5f));
    setMatrices(blinnProg);
    teapot.render();


    color = vec3(0.9f, 0.8f, 0.01f);
    blinnProg.setUniform("Material.Kd", color - (sin(timerC) * 0.1f));
    blinnProg.setUniform("Material.Ka", (color - (sin(timerC) * 0.1f)) * 0.55f);
    blinnProg.setUniform("Material.Ks", vec3(0.9f));
    blinnProg.setUniform("Material.Shininess", 180.0f);
    blinnProg.setUniform("fogStrength", 0.0f);

    model = mat4(1.0f);
    model = glm::scale(model, vec3(0.2f));
    model = glm::translate(model, vec3(5.0f, -1.0f, -18.5f));
    model = glm::translate(model, vec3(1.5f * sin(angle), 0.0f, -1.5f * cos(timerA)));
    setMatrices(blinnProg);
    ball.render();

    blinnProg.setUniform("Material.Kd", (color * 0.8f) + (cos(angle) * 0.1f));
    blinnProg.setUniform("Material.Ka", ((color * 0.8f) + (cos(angle) * 0.1f)) * 0.55f);
    model = mat4(1.0f);
    model = glm::scale(model, vec3(0.2f));
    model = glm::translate(model, vec3(9.0f, 0.0f, -17.0f));
    model = glm::translate(model, vec3(1.5f * sin(timerB), 0.0f, -1.5f * cos(angle)));
    setMatrices(blinnProg);
    ball.render();

    blinnProg.setUniform("Material.Kd", (color * 0.8f) + (tan(timerA) * vec3(0.1f)));
    blinnProg.setUniform("Material.Ka", ((color * 0.8f) + (tan(timerA) * vec3(0.1f))) * 0.55f);
    model = mat4(1.0f);
    model = glm::scale(model, vec3(0.2f));
    model = glm::translate(model, vec3(7.5f, -0.5f, -18.0f));
    model = glm::translate(model, vec3(1.5f, 2.5f * cos(timerA), -2.5f * sin(timerB)));
    setMatrices(blinnProg);
    ball.render();


    model = mat4(1.0f);
    //model = glm::scale(model, vec3(0.6f));
    model = glm::translate(model, vec3(6.5f, 2.5f, -1.0f));
    model = glm::translate(model, vec3(2.5f * cos(angle), 0.0f, -2.5f * sin(angle)));
    setMatrices(blinnProg);
    //ball.render();


    /*
    model = mat4(1.0f);
    model = glm::scale(model, vec3(0.5f, 5.0f, 0.5f));
    model = glm::translate(model, vec3(1.0f, 1.0f, -5.0f));
    model = glm::rotate(model, glm::radians(-90.0f), vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(45.0f), vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, vec3(0.3f));

    model = glm::translate(model, vec3(0.42f, 0.0f, -0.42f));
    setMatrices();
    column.render();

    model = mat4(1.0f);
    model = glm::translate(model, vec3(1.0f, 1.0f, -5.0f));
    model = glm::rotate(model, glm::radians(-90.0f), vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(45.0f), vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, vec3(0.3f));
    setMatrices();
    teapot.render();
    */
    
}

void SceneBasic_Uniform::spitOutDepthBuffer()
{

}

void repo() {
    /*
    vec3 color = vec3(1.0f, 1.0f, 1.0f);
    transProg.use();
    transProg.setUniform("Material.Kd", color);
    transProg.setUniform("Material.Ks", 0.9f, 0.9f, 0.9f);
    transProg.setUniform("Material.Ka", color * 0.05f);
    transProg.setUniform("Material.Shininess", 150.0f);

    // Transparent Floor
    model = mat4(1.0f);
    model = glm::translate(model, vec3(-3.2f, 0.5f, 2.2f));
    setMatrices(transProg);
    plane.render();
    //floor.render();

    textProg.use();
    textProg.setUniform("Material.Kd", color);
    textProg.setUniform("Material.Ks", 0.9f, 0.9f, 0.9f);
    textProg.setUniform("Material.Ka", color * 0.05f);
    textProg.setUniform("Material.Shininess", 150.0f);

    //Floor Tiles
    model = mat4(1.0f);
    model = glm::translate(model, vec3(4.0f, 1.0f, 2.2f));
    setMatrices(textProg);
    //plane.render();

    model = mat4(1.0f);
    model = glm::translate(model, vec3(3.8f, 0.5f, 10.4f));
    setMatrices(textProg);
    //plane.render();

    model = mat4(1.0f);
    model = glm::translate(model, vec3(-3.0f, 0.0f, 9.9f));
    setMatrices(textProg);
    //plane.render();


    //left
    model = mat4(1.0f);
    model = glm::translate(model, vec3(-3.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(-90.0f), vec3(0.0f, 0.0f, 1.0f));
    setMatrices(textProg);
    //plane.render();

    //right
    model = mat4(1.0f);
    model = glm::translate(model, vec3(3.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(90.0f), vec3(0.0f, 0.0f, 1.0f));
    setMatrices(textProg);
    //plane.render();

    //back
    model = mat4(1.0f);
    model = glm::translate(model, vec3(3.0f, 1.0f, -11.0f));
    model = glm::rotate(model, glm::radians(-90.0f), vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(80.0f), vec3(1.0f, 0.0f, 0.0f));
    setMatrices(textProg);
    //plane.render();


    model = mat4(1.0f);
    model = glm::scale(model, vec3(1.0f, 8.0f, 1.0f));
    model = glm::translate(model, vec3(4.0f, 2.0f, -3.5f));
    setMatrices(textProg);
    //cube.render();

    model = mat4(1.0f);
    model = glm::scale(model, vec3(1.0f, 8.0f, 1.0f));
    model = glm::translate(model, vec3(-4.0f, 2.0f, -3.5f));
    setMatrices(textProg);
    //cube.render();


    blinnProg.use();
    color = vec3(1.0f, 1.0f, 1.0f);

    blinnProg.setUniform("Material.Kd", color);
    blinnProg.setUniform("Material.Ks", 0.9f, 0.9f, 0.9f);
    blinnProg.setUniform("Material.Ka", color * 0.05f);
    blinnProg.setUniform("Material.Shininess", 150.0f);
    blinnProg.setUniform("fogStrength", 1.0f);

    // Pool Bottom
    model = mat4(1.0f);
    model = glm::translate(model, vec3(-4.2f, -0.5f, 0.0f));
    setMatrices();
    //plane.render();

    color = vec3(0.3f, 0.4f, 0.7f);
    blinnProg.setUniform("Material.Kd", color);
    blinnProg.setUniform("Material.Ka", color * 0.05f);

    // Big Ring
    model = mat4(1.0f);
    model = glm::translate(model, vec3(-3.2f, 5.0f, -9.3f));
    model = glm::rotate(model, glm::radians(-90.0f), vec3(1.0f, 0.0f, 0.0f));
    model = glm::scale(model, vec3(0.6f, 0.6f, 0.6f));
    setMatrices();
    //donuut.render();

    model = glm::translate(model, vec3(0.0f, 0.0f, -1.0f));
    setMatrices();
    //donuut.render();

    color = vec3(0.9f, 0.9f, 0.9f);
    blinnProg.setUniform("Material.Kd", color);
    blinnProg.setUniform("Material.Ka", color * 0.05f);
    blinnProg.setUniform("Material.Ks", 0.9f, 0.9f, 0.9f);
    blinnProg.setUniform("Material.Shininess", 150.0f);

    model = mat4(1.0f);
    //model = glm::scale(model, vec3(1.0f, 4.0f, 1.0f));
    model = glm::translate(model, vec3(-5.2f, 3.1f, -9.3f));
    model = glm::rotate(model, glm::radians(-90.0f), vec3(1.0f, 0.0f, 0.0f));
    //model = glm::rotate(model, glm::radians(45.0f), vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, vec3(0.5f));

    //model = glm::translate(model, vec3(0.0f, -1.0f, 0.0f));
    setMatrices();
    //column.render();

    for (int i = 0; i < 5; i++)
    {
        model = glm::translate(model, vec3(0.0f, 0.0f, 0.5f));
        setMatrices();
        //column.render();
    }


    color = vec3(0.3f, 0.5f, 0.9f);
    blinnProg.setUniform("Material.Kd", color);
    blinnProg.setUniform("Material.Ka", color * 0.05f);

    model = mat4(1.0f);
    model = glm::translate(model, vec3(-5.3f, 4.3f, -9.5f));
    model = glm::rotate(model, glm::radians(-90.0f), vec3(1.0f, 0.0f, 0.0f));
    //model = glm::rotate(model, glm::radians(45.0f), vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, vec3(0.5f));
    setMatrices();
    //teapot.render();


    color = vec3(0.9f, 0.9f, 0.9f);
    blinnProg.setUniform("Material.Kd", color);
    blinnProg.setUniform("Material.Ka", color * 0.05f);
    blinnProg.setUniform("Material.Ks", 0.9f, 0.9f, 0.9f);
    blinnProg.setUniform("Material.Shininess", 180.0f);
    blinnProg.setUniform("fogStrength", 0.0f);

    model = mat4(1.0f);
    model = glm::scale(model, vec3(0.3f));
    model = glm::translate(model, vec3(5.5f, -2.0f, -10.0f));
    setMatrices();
    ball.render();

    model = mat4(1.0f);
    model = glm::scale(model, vec3(0.3f));
    model = glm::translate(model, vec3(5.5f, -1.0f, -20.0f));
    setMatrices();
    ball.render();


    model = mat4(1.0f);
    model = glm::scale(model, vec3(0.6f));
    model = glm::translate(model, vec3(6.5f, 1.0f, -5.0f));
    setMatrices();
    ball.render();
    */
}
