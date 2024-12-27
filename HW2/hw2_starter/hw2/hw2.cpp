/*
  CSCI 420 Computer Graphics, USC
  Assignment 2: Roller Coaster
  C/C++ starter code

  Student username: <Jiale Wang jialewan@usc.edu>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>

#include "openGLHeader.h"
#include "glutHeader.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "pipelineProgram.h"
#include "vbo.h"
#include "vao.h"

#include <iostream>
#include <cstring>
#include <memory>

#if defined(WIN32) || defined(_WIN32)
  #ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#if defined(WIN32) || defined(_WIN32)
  char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
  char shaderBasePath[1024] = "../openGLHelper";
#endif

using namespace std;
float globalU = 0.0f;
int currentFrame = 0;
const int maxFrames = 1000;
float lastTime = 0.0f; //lastTime from last frame
const float frameInterval = 1000.0f / 15.0f;
GLuint groundTextureHandle;
int mousePos[2]; // x,y screen coordinates of the current mouse position
float previousBinormal[3] = {0.0f, 0.0f, 1.0f};
float previousBinormal_1[3] = {0.0f, 0.0f, 1.0f};
float previousBinormal_2[3] = {0.0f, 0.0f, 1.0f};
float previousBinormal_camera[3] = {0.0f, 0.0f, 1.0f};
int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

// Transformations of the terrain.
float terrainRotate[3] = { 0.0f, 0.0f, 0.0f }; 
// terrainRotate[0] gives the rotation around x-axis (in degrees)
// terrainRotate[1] gives the rotation around y-axis (in degrees)
// terrainRotate[2] gives the rotation around z-axis (in degrees)
float terrainTranslate[3] = { 0.0f, 0.0f, 0.0f };
float terrainScale[3] = { 1.0f, 1.0f, 1.0f };

// Width and height of the OpenGL window, in pixels.
int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 Homework 2";

// Represents one spline control point.
struct Point 
{
  float x, y, z;
};

// Contains the control points of the spline.
struct Spline 
{
  int numControlPoints;
  Point * points;
} spline;
void drawGround(int imageWidth, float y);
void drawSkyTop(int imageWidth, float y);
void drawSkyFront(int imageWidth, float y);
void drawSkyBack(int imageWidth, float y);
void drawSkyLeft(int imageWidth, float y);
void drawSkyRight(int imageWidth, float y);
void pointMode(const std::unique_ptr<ImageIO>& heightmapImage);
void lineMode(const std::unique_ptr<ImageIO>& heightmapImage);
void triangleMode(const std::unique_ptr<ImageIO>& heightmapImage);
void createVBOsForNeighbor(const std::unique_ptr<ImageIO>& heightmapImage, 
                          int i, int j,
                          std::unique_ptr<float[]>& positions_left, 
                          std::unique_ptr<float[]>& positions_right, 
                          std::unique_ptr<float[]>& positions_up, 
                          std::unique_ptr<float[]>& positions_down,
                          int currentIndex, float visualFactor);
void computeSplinePoint(float u, Point p0, Point p1, Point p2, Point p3, float s, float* result);
void computeTangent(float u, Point p0, Point p1, Point p2, Point p3, float s, float* tangent);
void MultiplyMatrices(int m, int p, int n, const float * A, const float * B, float * C);
void CatmullRomMode(float s, float stepSize);
void updateCamera();
float norm(float* v);
void computeNormalAndBinormal(float* tangent, float* previousBinormal, float* normal, float* binormal);
void crossProduct(float* v1, float* v2, float* result);
void normalize(float* v);
void renderTrackSection(float s, float stepSize, float trackWidth);
void computeColor(float* v1, float* v2, float* v3, std::unique_ptr<float[]>& colors, std::unique_ptr<float[]>& normals, int colorIndex);
int initTexture(const char * imageFilename, GLuint textureHandle);
void setUpLight();
void setUpPhong(float* viewMatrix);
// Number of vertices in the single triangle (starter code).
int numVertices;

int width;
int height;

float scale = 1.0f;
float exponent = 1.0f;
bool isFirstUpdate = true;
int mode = 0;
auto renderMode = GL_POINTS;
std::unique_ptr<ImageIO> heightmapImage = std::make_unique<ImageIO>();
std::unique_ptr<ImageIO> groundImage = std::make_unique<ImageIO>();
// CSCI 420 helper classes.
OpenGLMatrix matrix;
PipelineProgram pipelineProgram;
PipelineProgram texturePipelineProgram;
PipelineProgram texturePipelineProgramSkyTop;
PipelineProgram texturePipelineProgramSkyFront;
PipelineProgram texturePipelineProgramSkyBack;
PipelineProgram texturePipelineProgramSkyRight;
PipelineProgram texturePipelineProgramSkyLeft;
VBO vboVertices;
VBO vboColors;
VAO vao;
VAO vaoTexture;
VAO vaoTextureSkyTop;
VAO vaoTextureSkyRight;
VAO vaoTextureSkyLeft;
VAO vaoTextureSkyBack;
VAO vaoTextureSkyFront;
VBO vboLeft;
VBO vboRight;
VBO vboUp;
VBO vboDown;
VBO trackVBO;
VBO vboTexture;
VBO vboTextureColor;
VBO vboTextureSkyTop;
VBO vboTextureColorSkyTop;
VBO vboTextureSkyBack;
VBO vboTextureColorSkyBack;
VBO vboTextureSkyFront;
VBO vboTextureColorSkyFront;
VBO vboTextureSkyLeft;
VBO vboTextureColorSkyLeft;
VBO vboTextureSkyRight;
VBO vboTextureColorSkyRight;
VBO vboNormal;

// Write a screenshot to the specified filename.
void saveScreenshot(const char * filename)
{
  std::unique_ptr<unsigned char[]> screenshotData = std::make_unique<unsigned char[]>(windowWidth * windowHeight * 3);
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData.get());

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData.get());

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;
}

void idleFunc()
{
  //set Timer
  float currentTime = glutGet(GLUT_ELAPSED_TIME);

  if (currentTime - lastTime >= frameInterval && currentFrame < maxFrames)
  {
      char filename[1000];
      // merge name with path
      sprintf(filename, "/Users/davidwang/Documents/CSCI420/HW2/hw2_starter/screenshoots/%04d.jpg", currentFrame);

      saveScreenshot(filename);

      lastTime = currentTime;

      currentFrame++;
  }
  globalU += 0.0007f;  
  if (globalU > 1.0f) globalU -= 1.00f;
  updateCamera();
  glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
  glViewport(0, 0, w, h);

  // When the window has been resized, we need to re-set our projection matrix.
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.LoadIdentity();
  // You need to be careful about setting the zNear and zFar. 
  // Anything closer than zNear, or further than zFar, will be culled.
  const float zNear = 0.1f;
  const float zFar = 10000.0f;
  const float humanFieldOfView = 60.0f;
  matrix.Perspective(humanFieldOfView, 1.0f * w / h, zNear, zFar);
}

void mouseMotionDragFunc(int x, int y)
{
  // Mouse has moved, and one of the mouse buttons is pressed (dragging).

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

  switch (controlState)
  {
    // translate the terrain
    case TRANSLATE:
      if (leftMouseButton)
      {
        // control x,y translation via the left mouse button
        terrainTranslate[0] += mousePosDelta[0] * 0.01f;
        terrainTranslate[1] -= mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z translation via the middle mouse button
        terrainTranslate[2] += mousePosDelta[1] * 0.01f;
      }
      break;

    // rotate the terrain
    case ROTATE:
      if (leftMouseButton)
      {
        // control x,y rotation via the left mouse button
        terrainRotate[0] += mousePosDelta[1];
        terrainRotate[1] += mousePosDelta[0];
      }
      if (middleMouseButton)
      {
        // control z rotation via the middle mouse button
        terrainRotate[2] += mousePosDelta[1];
      }
      break;

    // scale the terrain
    case SCALE:
      if (leftMouseButton)
      {
        // control x,y scaling via the left mouse button
        terrainScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
        terrainScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z scaling via the middle mouse button
        terrainScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
  // Mouse has moved.
  // Store the new mouse position.
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // A mouse button has has been pressed or depressed.

  // Keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables.
  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      leftMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_MIDDLE_BUTTON:
      middleMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_RIGHT_BUTTON:
      rightMouseButton = (state == GLUT_DOWN);
    break;
  }

  // Keep track of whether CTRL and SHIFT keys are pressed.
  switch (glutGetModifiers())
  {
    case GLUT_ACTIVE_CTRL:
      controlState = TRANSLATE;
    break;

    case GLUT_ACTIVE_SHIFT:
      controlState = SCALE;
    break;

    // If CTRL and SHIFT are not pressed, we are in rotate mode.
    default:
      controlState = ROTATE;
    break;
  }

  // Store the new mouse position.
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  switch (key)
  {
    case 27: // ESC key
      exit(0); // exit the program
    break;

    case ' ':
      cout << "You pressed the spacebar." << endl;
    break;

    case 'x':
      // Take a screenshot.
      saveScreenshot("/Users/davidwang/Documents/CSCI420/HW2/hw2_starter/screenshoots/screenshot.jpg");
    break;
  }
}

void displayFunc()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();

  updateCamera();

  float modelViewMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetMatrix(modelViewMatrix);

  float projectionMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(projectionMatrix);
  renderMode = GL_TRIANGLES;

  //phong shading
  pipelineProgram.Bind();
  vao.Bind();
  vao.Gen();
  renderTrackSection(0.5f, 0.003f, 0.03f); //s stepsize trackWidth
  //set up phong shading parameters
  setUpPhong(modelViewMatrix);
  pipelineProgram.SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
  pipelineProgram.SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);
  vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboVertices, "position");
  vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboColors, "color");
  vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboNormal, "normal");
  // CatmullRomMode(0.5f, 0.00001f);
  glDrawArrays(renderMode, 0, numVertices); 
  glBindVertexArray(0);

  //sky bottom
  texturePipelineProgram.Bind();
  vaoTexture.Bind();
  vaoTexture.Gen();
  texturePipelineProgram.SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
  texturePipelineProgram.SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);
  glBindTexture(GL_TEXTURE_2D, groundTextureHandle);
  vaoTexture.ConnectPipelineProgramAndVBOAndShaderVariable(&texturePipelineProgram, &vboTexture, "position");
  vaoTexture.ConnectPipelineProgramAndVBOAndShaderVariable(&texturePipelineProgram, &vboTextureColor, "texCoord");
  drawGround(1000.0f, -100.0f);
  glDrawArrays(renderMode, 0, 6);
  glBindVertexArray(0);

  //sky top
  texturePipelineProgramSkyTop.Bind();
  vaoTextureSkyTop.Bind();
  vaoTextureSkyTop.Gen();
  texturePipelineProgramSkyTop.SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
  texturePipelineProgramSkyTop.SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);
  glBindTexture(GL_TEXTURE_2D, groundTextureHandle);
  vaoTextureSkyTop.ConnectPipelineProgramAndVBOAndShaderVariable(&texturePipelineProgramSkyTop, &vboTextureSkyTop, "position");
  vaoTextureSkyTop.ConnectPipelineProgramAndVBOAndShaderVariable(&texturePipelineProgramSkyTop, &vboTextureColorSkyTop, "texCoord");
  drawSkyTop(1000.0f, -100.0f);
  glDrawArrays(renderMode, 0, 6);
  glBindVertexArray(0);

  //Front
  texturePipelineProgramSkyFront.Bind();
  vaoTextureSkyFront.Bind();
  vaoTextureSkyFront.Gen();
  texturePipelineProgramSkyFront.SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
  texturePipelineProgramSkyFront.SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);
  glBindTexture(GL_TEXTURE_2D, groundTextureHandle);
  vaoTextureSkyFront.ConnectPipelineProgramAndVBOAndShaderVariable(&texturePipelineProgramSkyFront, &vboTextureSkyFront, "position");
  vaoTextureSkyFront.ConnectPipelineProgramAndVBOAndShaderVariable(&texturePipelineProgramSkyFront, &vboTextureColorSkyFront, "texCoord");
  drawSkyFront(1000.0f, -100.0f);
  glDrawArrays(renderMode, 0, 6);
  glBindVertexArray(0);
    
  //Back
  texturePipelineProgramSkyBack.Bind();
  vaoTextureSkyBack.Bind();
  vaoTextureSkyBack.Gen();
  texturePipelineProgramSkyBack.SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
  texturePipelineProgramSkyBack.SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);
  glBindTexture(GL_TEXTURE_2D, groundTextureHandle);
  vaoTextureSkyBack.ConnectPipelineProgramAndVBOAndShaderVariable(&texturePipelineProgramSkyBack, &vboTextureSkyBack, "position");
  vaoTextureSkyBack.ConnectPipelineProgramAndVBOAndShaderVariable(&texturePipelineProgramSkyBack, &vboTextureColorSkyBack, "texCoord");
  drawSkyBack(1000.0f, -100.0f);
  glDrawArrays(renderMode, 0, 6);
  glBindVertexArray(0);

  //Left
  texturePipelineProgramSkyLeft.Bind();
  vaoTextureSkyLeft.Bind();
  vaoTextureSkyLeft.Gen();
  texturePipelineProgramSkyLeft.SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
  texturePipelineProgramSkyLeft.SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);
  glBindTexture(GL_TEXTURE_2D, groundTextureHandle);
  vaoTextureSkyLeft.ConnectPipelineProgramAndVBOAndShaderVariable(&texturePipelineProgramSkyLeft, &vboTextureSkyLeft, "position");
  vaoTextureSkyLeft.ConnectPipelineProgramAndVBOAndShaderVariable(&texturePipelineProgramSkyLeft, &vboTextureColorSkyLeft, "texCoord");
  drawSkyLeft(1000.0f, -100.0f);
  glDrawArrays(renderMode, 0, 6);
  glBindVertexArray(0);

  //Right
  texturePipelineProgramSkyRight.Bind();
  vaoTextureSkyRight.Bind();
  vaoTextureSkyRight.Gen();
  texturePipelineProgramSkyRight.SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
  texturePipelineProgramSkyRight.SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);
  glBindTexture(GL_TEXTURE_2D, groundTextureHandle);
  vaoTextureSkyRight.ConnectPipelineProgramAndVBOAndShaderVariable(&texturePipelineProgramSkyRight, &vboTextureSkyRight, "position");
  vaoTextureSkyRight.ConnectPipelineProgramAndVBOAndShaderVariable(&texturePipelineProgramSkyRight, &vboTextureColorSkyRight, "texCoord");
  drawSkyRight(1000.0f, -100.0f);
  glDrawArrays(renderMode, 0, 6);
  glBindVertexArray(0);
  
  glutSwapBuffers();
}

void setUpPhong(float* viewMatrix)
{
  float lightDirection[4] = {0.0f,1.0f,0.0f,0.0f};
  float viewLightDirection[3];

  float viewLightCalculate[4];
  MultiplyMatrices(4, 4, 1, viewMatrix, lightDirection, viewLightCalculate);
  viewLightDirection[0] = viewLightCalculate[0];
  viewLightDirection[1] = viewLightCalculate[1];
  viewLightDirection[2] = viewLightCalculate[2];

  GLint h_viewLightDirection = glGetUniformLocation(pipelineProgram.GetProgramHandle(), "viewLightDirection");
  glUniform3fv(h_viewLightDirection,1,viewLightDirection);
  float k = 0.2f; //light
  float materialCoeff = 1.0f; //reflection

  GLint La_ = glGetUniformLocation(pipelineProgram.GetProgramHandle(), "La");
  float La[4] = {k, k, k, 1.0f};
  glUniform4fv(La_, 1, La);

  GLint ka_ = glGetUniformLocation(pipelineProgram.GetProgramHandle(), "ka");
  float ka[4] = {materialCoeff, materialCoeff, materialCoeff, materialCoeff};
  glUniform4fv(ka_, 1, ka);

  GLint Ld_ = glGetUniformLocation(pipelineProgram.GetProgramHandle(), "Ld");
  float Ld[4] = {k, k, k, 1.0f};
  glUniform4fv(Ld_, 1, Ld);

  GLint kd_ = glGetUniformLocation(pipelineProgram.GetProgramHandle(), "kd");
  float kd[4] = {materialCoeff, materialCoeff, materialCoeff, materialCoeff};
  glUniform4fv(kd_, 1, kd);

  GLint Ls_ = glGetUniformLocation(pipelineProgram.GetProgramHandle(), "Ls");
  float Ls[4] = {k, k, k, 1.0f};
  glUniform4fv(Ls_, 1, Ls);

  GLint ks_ = glGetUniformLocation(pipelineProgram.GetProgramHandle(), "ks");
  float ks[4] = {materialCoeff, materialCoeff, materialCoeff, materialCoeff};
  glUniform4fv(ks_, 1, ks);

  GLint alpha_ = glGetUniformLocation(pipelineProgram.GetProgramHandle(), "alpha");
  glUniform1f(alpha_, 1.0f);

  // normal matrix
  float n[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetNormalMatrix(n);
  pipelineProgram.SetUniformVariableMatrix4fv("normalMatrix", GL_FALSE, n);
}

void pointMode(const std::unique_ptr<ImageIO>& heightmapImage)
{
    height = heightmapImage->getHeight();
    width = heightmapImage->getWidth();

    numVertices = height * width; 
    std::unique_ptr<float[]> positions = std::make_unique<float[]>(numVertices * 3);
    std::unique_ptr<float[]> colors = std::make_unique<float[]>(numVertices * 4);

    for (int i = 0; i < width; i ++) 
    {
      for (int j = 0; j < height; j ++) 
      {
        int index = (i * height + j) * 3;
        float heightValue = heightmapImage->getPixel(i, j, 0) / 255.0f;

        positions[index] = i; // x
        positions[index + 1] = heightValue * 40.0f; // y * 40 for visual impact
        positions[index + 2] = -j; // z

        //assign heightValue 0-1 as color
        colors[(index / 3) * 4] = heightValue; // r 
        colors[(index / 3) * 4 + 1] = heightValue; // g
        colors[(index / 3) * 4 + 2] = heightValue; // b
        colors[(index / 3) * 4 + 3] = 1.0f; // a
      }
    }

    vboVertices.Gen(numVertices, 3, positions.get(), GL_STATIC_DRAW);
    vboColors.Gen(numVertices, 4, colors.get(), GL_STATIC_DRAW);
}


void lineMode(const std::unique_ptr<ImageIO>& heightmapImage)
{
    height = heightmapImage->getHeight();
    width = heightmapImage->getWidth();

    //seperate vertical and horizontal
    //for horizontal : there are height * width-1 lines because we are counting spaces in between
    //vertical the same idea
    int numLines = (height * (width - 1)) + (width * (height - 1));//vertical + horizontal lines
    numVertices = numLines * 2;
    std::unique_ptr<float[]> positions = std::make_unique<float[]>(numVertices * 3);
    std::unique_ptr<float[]> colors = std::make_unique<float[]>(numVertices * 4);

    int currentIndex = 0;

    // horizontal : width - 1 because we are iterating the vertices on the right
    for (int i = 0; i < width-1; i++)
    {
      for (int j = 0; j < height; j++)
      {
        float heightValue1 = heightmapImage->getPixel(i, j, 0) / 255.0f;

        positions[currentIndex * 3] = i; // x
        positions[currentIndex * 3 + 1] = heightValue1 * 40.0f; // y
        positions[currentIndex * 3 + 2] = -j; // z

        colors[currentIndex * 4] = heightValue1;
        colors[currentIndex * 4 + 1] = heightValue1;
        colors[currentIndex * 4 + 2] = heightValue1;
        colors[currentIndex * 4 + 3] = 1.0f;

        currentIndex++;

        // the vertex on the right
        float heightValue2 = heightmapImage->getPixel(i + 1, j, 0) / 255.0f;
        positions[currentIndex * 3] = i + 1; // x
        positions[currentIndex * 3 + 1] = heightValue2 * 40.0f; // y
        positions[currentIndex * 3 + 2] = -j; // z

        colors[currentIndex * 4] = heightValue2;
        colors[currentIndex * 4 + 1] = heightValue2;
        colors[currentIndex * 4 + 2] = heightValue2;
        colors[currentIndex * 4 + 3] = 1.0f;
        currentIndex++;
      }
    }

    //vertical : height - 1 because we are iterating the vertices below
    for (int i = 0; i < width; i++)
    {
      for (int j = 0; j < height-1; j++)
      {
        float heightValue1 = heightmapImage->getPixel(i, j, 0) / 255.0f;
        positions[currentIndex * 3] = i; // x
        positions[currentIndex * 3 + 1] = heightValue1 * 40.0f; // y
        positions[currentIndex * 3 + 2] = -j; // z

        colors[currentIndex * 4] = heightValue1;
        colors[currentIndex * 4 + 1] = heightValue1;
        colors[currentIndex * 4 + 2] = heightValue1;
        colors[currentIndex * 4 + 3] = 1.0f;

        currentIndex++;
        //the vertex below
        float heightValue2 = heightmapImage->getPixel(i, j + 1, 0) / 255.0f;
        positions[currentIndex * 3] = i; // x
        positions[currentIndex * 3 + 1] = heightValue2 * 40.0f; // y
        positions[currentIndex * 3 + 2] = -(j + 1); // z

        colors[currentIndex * 4] = heightValue2;
        colors[currentIndex * 4 + 1] = heightValue2;
        colors[currentIndex * 4 + 2] = heightValue2;
        colors[currentIndex * 4 + 3] = 1.0f;

        currentIndex++;
      }
    }

    vboVertices.Gen(numVertices, 3, positions.get(), GL_STATIC_DRAW);
    vboColors.Gen(numVertices, 4, colors.get(), GL_STATIC_DRAW);
}

void triangleMode(const std::unique_ptr<ImageIO>& heightmapImage)
{
  float visualFactor = 40.0f;
  if(mode == 1)
    visualFactor = 1.0f;
  height = heightmapImage->getHeight();
  width = heightmapImage->getWidth();
  //for every 4 vertices: (i, j), (i+1, j), (i, j+1), (i+1, j+1)
  //there are 2 triangles to render
  //so we should iterate from 0 -> width-1, 0 -> height-1
  //2 * 2 -> 2 triangles, 3 * 3 -> 8, 4 * 4 -> 18  => formula (width - 1) * (height - 1) * 2
  int numTriangles = (width - 1) * (height - 1) * 2; //
  numVertices = numTriangles * 3;
  std::unique_ptr<float[]> positions = std::make_unique<float[]>(numVertices * 3);
  std::unique_ptr<float[]> colors = std::make_unique<float[]>(numVertices * 4);

  //vbos for mode 4
  std::unique_ptr<float[]> positions_left = std::make_unique<float[]>(numVertices * 3);
  std::unique_ptr<float[]> positions_right = std::make_unique<float[]>(numVertices * 3);
  std::unique_ptr<float[]> positions_up = std::make_unique<float[]>(numVertices * 3);
  std::unique_ptr<float[]> positions_down = std::make_unique<float[]>(numVertices * 3);

  int currentIndex = 0;

  for (int i = 0; i < width - 1; i++)
  {
    for (int j = 0; j < height - 1; j++)
    {
      //get info for the 3 points that makes a triangle. 
      //becaue of the for loop, we need to get 4 points at a time. 
      //for these 4 points, we can find 2 triangles to render
      float heightValue1 = heightmapImage->getPixel(i, j, 0) / 255.0f;
      //for the first triangle formed by point (i, j) (i+1, j), (i, j+1)
      //(i, j)
      positions[currentIndex * 3] = i; // x
      positions[currentIndex * 3 + 1] = heightValue1 * visualFactor; // y
      positions[currentIndex * 3 + 2] = -j; // z

      colors[currentIndex * 4] = heightValue1;
      colors[currentIndex * 4 + 1] = heightValue1;
      colors[currentIndex * 4 + 2] = heightValue1;
      colors[currentIndex * 4 + 3] = 1.0f;
      //get neighbor's info for vertex shader
      createVBOsForNeighbor(heightmapImage, i, j, positions_left, positions_right, positions_up, positions_down, currentIndex, visualFactor);
      currentIndex++;

      //(i+1, j)
      float heightValue2 = heightmapImage->getPixel(i + 1, j, 0) / 255.0f;
      positions[currentIndex * 3] = i + 1; // x
      positions[currentIndex * 3 + 1] = heightValue2 * visualFactor; // y
      positions[currentIndex * 3 + 2] = -j; // z

      colors[currentIndex * 4] = heightValue2;
      colors[currentIndex * 4 + 1] = heightValue2;
      colors[currentIndex * 4 + 2] = heightValue2;
      colors[currentIndex * 4 + 3] = 1.0f;
      createVBOsForNeighbor(heightmapImage, i+1, j, positions_left, positions_right, positions_up, positions_down, currentIndex, visualFactor);
      currentIndex++;

      //(i, j+1)
      float heightValue3 = heightmapImage->getPixel(i, j + 1, 0) / 255.0f;
      positions[currentIndex * 3] = i; // x
      positions[currentIndex * 3 + 1] = heightValue3 * visualFactor; // y
      positions[currentIndex * 3 + 2] = -(j + 1); // z

      colors[currentIndex * 4] = heightValue3;
      colors[currentIndex * 4 + 1] = heightValue3;
      colors[currentIndex * 4 + 2] = heightValue3;
      colors[currentIndex * 4 + 3] = 1.0f;
      createVBOsForNeighbor(heightmapImage, i, j+1, positions_left, positions_right, positions_up, positions_down, currentIndex, visualFactor);
      currentIndex++;

      //for the second triangle formed by point (i+1, j) (i+1, j+1), (i, j+1)
      //(i+1, j)
      positions[currentIndex * 3] = i + 1; // x
      positions[currentIndex * 3 + 1] = heightValue2 * visualFactor; // y
      positions[currentIndex * 3 + 2] = -j; // z

      colors[currentIndex * 4] = heightValue2;
      colors[currentIndex * 4 + 1] = heightValue2;
      colors[currentIndex * 4 + 2] = heightValue2;
      colors[currentIndex * 4 + 3] = 1.0f;
      createVBOsForNeighbor(heightmapImage, i+1, j, positions_left, positions_right, positions_up, positions_down, currentIndex, visualFactor);
      currentIndex++;

      //(i+1, j+1)
      float heightValue4 = heightmapImage->getPixel(i + 1, j + 1, 0) / 255.0f;
      positions[currentIndex * 3] = i + 1; // x2
      positions[currentIndex * 3 + 1] = heightValue4 * visualFactor; // y2
      positions[currentIndex * 3 + 2] = -(j + 1); // z2

      colors[currentIndex * 4] = heightValue4;
      colors[currentIndex * 4 + 1] = heightValue4;
      colors[currentIndex * 4 + 2] = heightValue4;
      colors[currentIndex * 4 + 3] = 1.0f;
      createVBOsForNeighbor(heightmapImage, i+1, j+1, positions_left, positions_right, positions_up, positions_down, currentIndex, visualFactor);
      currentIndex++;

      //(i, j+1)
      positions[currentIndex * 3] = i; // x3
      positions[currentIndex * 3 + 1] = heightValue3 * visualFactor; // y3
      positions[currentIndex * 3 + 2] = -(j + 1); // z3

      colors[currentIndex * 4] = heightValue3;
      colors[currentIndex * 4 + 1] = heightValue3;
      colors[currentIndex * 4 + 2] = heightValue3;
      colors[currentIndex * 4 + 3] = 1.0f;
      createVBOsForNeighbor(heightmapImage, i, j+1, positions_left, positions_right, positions_up, positions_down, currentIndex, visualFactor);
      currentIndex++;
    }
  }

  vboVertices.Gen(numVertices, 3, positions.get(), GL_STATIC_DRAW);
  vboColors.Gen(numVertices, 4, colors.get(), GL_STATIC_DRAW);

  //Create vbos for the neighbors
  vboLeft.Gen(numVertices, 3, positions_left.get(), GL_STATIC_DRAW);
  vboRight.Gen(numVertices, 3, positions_right.get(), GL_STATIC_DRAW);
  vboUp.Gen(numVertices, 3, positions_up.get(), GL_STATIC_DRAW);
  vboDown.Gen(numVertices, 3, positions_down.get(), GL_STATIC_DRAW);
}

void createVBOsForNeighbor(const std::unique_ptr<ImageIO>& heightmapImage, int i, int j,
                          std::unique_ptr<float[]>& positions_left, 
                          std::unique_ptr<float[]>& positions_right, 
                          std::unique_ptr<float[]>& positions_up, 
                          std::unique_ptr<float[]>& positions_down,
                          int currentIndex, float visualFactor)
{
  float originalHeight = heightmapImage->getPixel(i, j, 0) / 255.0f * visualFactor;
  //left (i-1, j)
  if (i > 0)
  {
    float heightLeft = heightmapImage->getPixel(i - 1, j, 0) / 255.0f * visualFactor;
    positions_left[currentIndex * 3] = i - 1;
    positions_left[currentIndex * 3 + 1] = heightLeft;
    positions_left[currentIndex * 3 + 2] = -j;
  } 
  else 
  {
    positions_left[currentIndex * 3] = i;
    positions_left[currentIndex * 3 + 1] = originalHeight;
    positions_left[currentIndex * 3 + 2] = -j;
  }

  //right (i+1, j)
  if (i < width - 1) 
  {
    float heightRight = heightmapImage->getPixel(i + 1, j, 0) / 255.0f * visualFactor;
    positions_right[currentIndex * 3] = i + 1;
    positions_right[currentIndex * 3 + 1] = heightRight;
    positions_right[currentIndex * 3 + 2] = -j;
  } 
  else 
  {
    positions_right[currentIndex * 3] = i;
    positions_right[currentIndex * 3 + 1] = originalHeight;
    positions_right[currentIndex * 3 + 2] = -j;
  }

  //up (i, j-1)
  if (j > 0) 
  {
    float heightUp = heightmapImage->getPixel(i, j - 1, 0) / 255.0f * visualFactor;
    positions_up[currentIndex * 3] = i;
    positions_up[currentIndex * 3 + 1] = heightUp;
    positions_up[currentIndex * 3 + 2] = -(j - 1);
  } 
  else 
  {
    positions_up[currentIndex * 3] = i;
    positions_up[currentIndex * 3 + 1] = originalHeight;
    positions_up[currentIndex * 3 + 2] = -j;
  }

  //down (i, j+1)
  if (j < height - 1) 
  {
    float heightDown = heightmapImage->getPixel(i, j + 1, 0) / 255.0f * visualFactor;
    positions_down[currentIndex * 3] = i;
    positions_down[currentIndex * 3 + 1] = heightDown;
    positions_down[currentIndex * 3 + 2] = -(j + 1);
  } 
  else 
  {
    positions_down[currentIndex * 3] = i;
    positions_down[currentIndex * 3 + 1] =  originalHeight;
    positions_down[currentIndex * 3 + 2] = -j;
  }
}


void loadSpline(char * argv) 
{
  FILE * fileSpline = fopen(argv, "r");
  if (fileSpline == NULL) 
  {
    printf ("Cannot open file %s.\n", argv);
    exit(1);
  }

  // Read the number of spline control points.
  fscanf(fileSpline, "%d\n", &spline.numControlPoints);
  printf("Detected %d control points.\n", spline.numControlPoints);

  // Allocate memory.
  spline.points = (Point *) malloc(spline.numControlPoints * sizeof(Point));
  // Load the control points.
  for(int i=0; i<spline.numControlPoints; i++)
  {
    if (fscanf(fileSpline, "%f %f %f", 
           &spline.points[i].x, 
	   &spline.points[i].y, 
	   &spline.points[i].z) != 3)
    {
      printf("Error: incorrect number of control points in file %s.\n", argv);
      exit(1);
    }
  }
}

void initScene(int argc, char *argv[])
{
  glGenTextures(1, &groundTextureHandle);

  // Load the ground texture
  if (initTexture("heightmap/ground.jpg", groundTextureHandle) != 0) 
  {
      cout << "Error loading ground texture." << endl;
      exit(EXIT_FAILURE);
  }
  // Load the image from a jpeg disk file into main memory.
  if (groundImage->loadJPEG("heightmap/ground.jpg") != ImageIO::OK)
  {
    cout << "Error reading image " << argv[1] << "." << endl;
    exit(EXIT_FAILURE);
  }
  loadSpline(argv[1]);

  // Set the background color.
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black color.

  // Enable z-buffering (i.e., hidden surface removal using the z-buffer algorithm).
  glEnable(GL_DEPTH_TEST);

  if (pipelineProgram.BuildShadersFromFiles(shaderBasePath, "vertexShader.glsl", "fragmentShader.glsl") != 0)
  {
    cout << "Failed to build the pipeline program." << endl;
    throw 1;
  } 
  cout << "Successfully built the pipeline program." << endl;
  pipelineProgram.Bind();
  vao.Bind();
  //bottom
  if (texturePipelineProgram.BuildShadersFromFiles(shaderBasePath, "textureVertexShader.glsl", "textureFragmentShader.glsl") != 0)
  {
      cout << "Failed to build the texture pipeline program." << endl;
      throw 1;
  } 
  cout << "Successfully built the texture pipeline program." << endl;
  texturePipelineProgram.Bind();
  vaoTexture.Bind();
  //top
  if (texturePipelineProgramSkyTop.BuildShadersFromFiles(shaderBasePath, "textureVertexShaderTop.glsl", "textureFragmentShaderTop.glsl") != 0)
  {
      cout << "Failed to build the texture pipeline program." << endl;
      throw 1;
  } 
  cout << "Successfully built the texture Sky pipeline program." << endl;
  texturePipelineProgramSkyTop.Bind();
  vaoTextureSkyTop.Bind();
  //front
  if (texturePipelineProgramSkyFront.BuildShadersFromFiles(shaderBasePath, "textureVertexShaderFront.glsl", "textureFragmentShaderFront.glsl") != 0)
  {
      cout << "Failed to build the texture pipeline program." << endl;
      throw 1;
  } 
  cout << "Successfully built the texture Sky pipeline program." << endl;
  texturePipelineProgramSkyFront.Bind();
  vaoTextureSkyFront.Bind();
  //left
  if (texturePipelineProgramSkyLeft.BuildShadersFromFiles(shaderBasePath, "textureVertexShaderLeft.glsl", "textureFragmentShaderLeft.glsl") != 0)
  {
      cout << "Failed to build the texture pipeline program." << endl;
      throw 1;
  } 
  cout << "Successfully built the texture Sky pipeline program." << endl;
  texturePipelineProgramSkyLeft.Bind();
  vaoTextureSkyLeft.Bind();
  //right
  if (texturePipelineProgramSkyRight.BuildShadersFromFiles(shaderBasePath, "textureVertexShaderRight.glsl", "textureFragmentShaderRight.glsl") != 0)
  {
      cout << "Failed to build the texture pipeline program." << endl;
      throw 1;
  } 
  cout << "Successfully built the texture Sky pipeline program." << endl;
  texturePipelineProgramSkyRight.Bind();
  vaoTextureSkyRight.Bind();
  //back
  if (texturePipelineProgramSkyBack.BuildShadersFromFiles(shaderBasePath, "textureVertexShaderBack.glsl", "textureFragmentShaderBack.glsl") != 0)
  {
      cout << "Failed to build the texture pipeline program." << endl;
      throw 1;
  } 
  cout << "Successfully built the texture Sky pipeline program." << endl;
  texturePipelineProgramSkyBack.Bind();
  vaoTextureSkyBack.Bind();

  std::cout << "GL error status is: " << glGetError() << std::endl;
}

void drawGround(int imageWidth, float y) 
{
  std::unique_ptr<float[]> positions = std::make_unique<float[]>(6 * 3);
  std::unique_ptr<float[]> texCoords = std::make_unique<float[]>(6 * 2);
  float halfWidth = imageWidth / 2.0f;

  //first tri
  int currentIndex = 0;
  positions[currentIndex * 3 + 0] = -halfWidth; // x
  positions[currentIndex * 3 + 1] = y;          // y
  positions[currentIndex * 3 + 2] = halfWidth;   // z
  texCoords[currentIndex * 2 + 0] = 0.0f;       // u
  texCoords[currentIndex * 2 + 1] = 1.0f;       // v
  currentIndex++;

  positions[currentIndex * 3 + 0] = halfWidth;   // x
  positions[currentIndex * 3 + 1] = y;           // y
  positions[currentIndex * 3 + 2] = halfWidth;   // z
  texCoords[currentIndex * 2 + 0] = 1.0f;       // u
  texCoords[currentIndex * 2 + 1] = 1.0f;       // v
  currentIndex++;

  positions[currentIndex * 3 + 0] = -halfWidth;  // x
  positions[currentIndex * 3 + 1] = y;           // y
  positions[currentIndex * 3 + 2] = -halfWidth;  // z
  texCoords[currentIndex * 2 + 0] = 0.0f;       // u
  texCoords[currentIndex * 2 + 1] = 0.0f;       // v
  currentIndex++;

  //second tri
  positions[currentIndex * 3 + 0] = -halfWidth;  // x
  positions[currentIndex * 3 + 1] = y;           // y
  positions[currentIndex * 3 + 2] = -halfWidth;  // z
  texCoords[currentIndex * 2 + 0] = 0.0f;       // u
  texCoords[currentIndex * 2 + 1] = 0.0f;       // v
  currentIndex++;

  positions[currentIndex * 3 + 0] = halfWidth;   // x
  positions[currentIndex * 3 + 1] = y;           // y
  positions[currentIndex * 3 + 2] = -halfWidth;  // z
  texCoords[currentIndex * 2 + 0] = 1.0f;       // u
  texCoords[currentIndex * 2 + 1] = 0.0f;       // v
  currentIndex++;

  positions[currentIndex * 3 + 0] = halfWidth;   // x
  positions[currentIndex * 3 + 1] = y;           // y
  positions[currentIndex * 3 + 2] = halfWidth;   // z
  texCoords[currentIndex * 2 + 0] = 1.0f;       // u
  texCoords[currentIndex * 2 + 1] = 1.0f;       // v

  vboTexture.Gen(6, 3, positions.get(), GL_STATIC_DRAW);
  vboTextureColor.Gen(6, 2, texCoords.get(), GL_STATIC_DRAW);
}

void drawSkyTop(int imageWidth, float y) 
{
  std::unique_ptr<float[]> positions = std::make_unique<float[]>(6 * 3);
  std::unique_ptr<float[]> texCoords = std::make_unique<float[]>(6 * 2);
  float halfWidth = imageWidth / 2.0f;

  //top
  //first tri
  int currentIndex = 0;
  positions[currentIndex * 3 + 0] = -halfWidth; // x
  positions[currentIndex * 3 + 1] = y + imageWidth; // y
  positions[currentIndex * 3 + 2] = halfWidth;   // z
  texCoords[currentIndex * 2 + 0] = 0.0f;       // u
  texCoords[currentIndex * 2 + 1] = 1.0f;       // v
  currentIndex++;

  positions[currentIndex * 3 + 0] = halfWidth;   // x
  positions[currentIndex * 3 + 1] = y + imageWidth; // y
  positions[currentIndex * 3 + 2] = halfWidth;   // z
  texCoords[currentIndex * 2 + 0] = 1.0f;       // u
  texCoords[currentIndex * 2 + 1] = 1.0f;       // v
  currentIndex++;

  positions[currentIndex * 3 + 0] = -halfWidth;  // x
  positions[currentIndex * 3 + 1] = y + imageWidth; // y
  positions[currentIndex * 3 + 2] = -halfWidth;  // z
  texCoords[currentIndex * 2 + 0] = 0.0f;       // u
  texCoords[currentIndex * 2 + 1] = 0.0f;       // v
  currentIndex++;

  //second tri
  positions[currentIndex * 3 + 0] = -halfWidth;  // x
  positions[currentIndex * 3 + 1] = y + imageWidth; // y
  positions[currentIndex * 3 + 2] = -halfWidth;  // z
  texCoords[currentIndex * 2 + 0] = 0.0f;       // u
  texCoords[currentIndex * 2 + 1] = 0.0f;       // v
  currentIndex++;

  positions[currentIndex * 3 + 0] = halfWidth;   // x
  positions[currentIndex * 3 + 1] = y + imageWidth; // y
  positions[currentIndex * 3 + 2] = -halfWidth;  // z
  texCoords[currentIndex * 2 + 0] = 1.0f;       // u
  texCoords[currentIndex * 2 + 1] = 0.0f;       // v
  currentIndex++;

  positions[currentIndex * 3 + 0] = halfWidth;   // x
  positions[currentIndex * 3 + 1] = y + imageWidth;// y
  positions[currentIndex * 3 + 2] = halfWidth;   // z
  texCoords[currentIndex * 2 + 0] = 1.0f;       // u
  texCoords[currentIndex * 2 + 1] = 1.0f;       // v

  vboTextureSkyTop.Gen(6, 3, positions.get(), GL_STATIC_DRAW);
  vboTextureColorSkyTop.Gen(6, 2, texCoords.get(), GL_STATIC_DRAW);
}

void drawSkyBack(int imageWidth, float y) 
{
  std::unique_ptr<float[]> positions = std::make_unique<float[]>(6 * 3);
  std::unique_ptr<float[]> texCoords = std::make_unique<float[]>(6 * 2);
  float halfWidth = imageWidth / 2.0f;

  // Back face
  // First triangle
  int currentIndex = 0;
  positions[currentIndex * 3 + 0] = -halfWidth; // x
  positions[currentIndex * 3 + 1] = y;          // y
  positions[currentIndex * 3 + 2] = -halfWidth; // z
  texCoords[currentIndex * 2 + 0] = 0.0f;       // u
  texCoords[currentIndex * 2 + 1] = 0.0f;       // v
  currentIndex++;

  positions[currentIndex * 3 + 0] = halfWidth;  // x
  positions[currentIndex * 3 + 1] = y;          // y
  positions[currentIndex * 3 + 2] = -halfWidth; // z
  texCoords[currentIndex * 2 + 0] = 1.0f;       // u
  texCoords[currentIndex * 2 + 1] = 0.0f;       // v
  currentIndex++;

  positions[currentIndex * 3 + 0] = halfWidth;  // x
  positions[currentIndex * 3 + 1] = y + imageWidth; // y
  positions[currentIndex * 3 + 2] = -halfWidth; // z
  texCoords[currentIndex * 2 + 0] = 1.0f;       // u
  texCoords[currentIndex * 2 + 1] = 1.0f;       // v
  currentIndex++;

  // Second triangle
  positions[currentIndex * 3 + 0] = -halfWidth; // x
  positions[currentIndex * 3 + 1] = y;          // y
  positions[currentIndex * 3 + 2] = -halfWidth; // z
  texCoords[currentIndex * 2 + 0] = 0.0f;       // u
  texCoords[currentIndex * 2 + 1] = 0.0f;       // v
  currentIndex++;

  positions[currentIndex * 3 + 0] = halfWidth;  // x
  positions[currentIndex * 3 + 1] = y + imageWidth; // y
  positions[currentIndex * 3 + 2] = -halfWidth; // z
  texCoords[currentIndex * 2 + 0] = 1.0f;       // u
  texCoords[currentIndex * 2 + 1] = 1.0f;       // v
  currentIndex++;

  positions[currentIndex * 3 + 0] = -halfWidth; // x
  positions[currentIndex * 3 + 1] = y + imageWidth; // y
  positions[currentIndex * 3 + 2] = -halfWidth; // z
  texCoords[currentIndex * 2 + 0] = 0.0f;       // u
  texCoords[currentIndex * 2 + 1] = 1.0f;       // v

  vboTextureSkyBack.Gen(6, 3, positions.get(), GL_STATIC_DRAW);
  vboTextureColorSkyBack.Gen(6, 2, texCoords.get(), GL_STATIC_DRAW);
}

void drawSkyFront(int imageWidth, float y) 
{
  std::unique_ptr<float[]> positions = std::make_unique<float[]>(6 * 3);
  std::unique_ptr<float[]> texCoords = std::make_unique<float[]>(6 * 2);
  float halfWidth = imageWidth / 2.0f;

  // Front face
  // First triangle
  int currentIndex = 0;
  positions[currentIndex * 3 + 0] = -halfWidth; // x
  positions[currentIndex * 3 + 1] = y;          // y
  positions[currentIndex * 3 + 2] = halfWidth;  // z
  texCoords[currentIndex * 2 + 0] = 0.0f;       // u
  texCoords[currentIndex * 2 + 1] = 0.0f;       // v
  currentIndex++;

  positions[currentIndex * 3 + 0] = halfWidth;  // x
  positions[currentIndex * 3 + 1] = y;          // y
  positions[currentIndex * 3 + 2] = halfWidth;  // z
  texCoords[currentIndex * 2 + 0] = 1.0f;       // u
  texCoords[currentIndex * 2 + 1] = 0.0f;       // v
  currentIndex++;

  positions[currentIndex * 3 + 0] = halfWidth;  // x
  positions[currentIndex * 3 + 1] = y + imageWidth; // y
  positions[currentIndex * 3 + 2] = halfWidth;  // z
  texCoords[currentIndex * 2 + 0] = 1.0f;       // u
  texCoords[currentIndex * 2 + 1] = 1.0f;       // v
  currentIndex++;

  // Second triangle
  positions[currentIndex * 3 + 0] = -halfWidth; // x
  positions[currentIndex * 3 + 1] = y;          // y
  positions[currentIndex * 3 + 2] = halfWidth;  // z
  texCoords[currentIndex * 2 + 0] = 0.0f;       // u
  texCoords[currentIndex * 2 + 1] = 0.0f;       // v
  currentIndex++;

  positions[currentIndex * 3 + 0] = halfWidth;  // x
  positions[currentIndex * 3 + 1] = y + imageWidth; // y
  positions[currentIndex * 3 + 2] = halfWidth;  // z
  texCoords[currentIndex * 2 + 0] = 1.0f;       // u
  texCoords[currentIndex * 2 + 1] = 1.0f;       // v
  currentIndex++;

  positions[currentIndex * 3 + 0] = -halfWidth; // x
  positions[currentIndex * 3 + 1] = y + imageWidth; // y
  positions[currentIndex * 3 + 2] = halfWidth;  // z
  texCoords[currentIndex * 2 + 0] = 0.0f;       // u
  texCoords[currentIndex * 2 + 1] = 1.0f;       // v

  vboTextureSkyFront.Gen(6, 3, positions.get(), GL_STATIC_DRAW);
  vboTextureColorSkyFront.Gen(6, 2, texCoords.get(), GL_STATIC_DRAW);
}

void drawSkyLeft(int imageWidth, float y) 
{
  std::unique_ptr<float[]> positions = std::make_unique<float[]>(6 * 3);
  std::unique_ptr<float[]> texCoords = std::make_unique<float[]>(6 * 2);
  float halfWidth = imageWidth / 2.0f;

  // Left face
  // First triangle
  int currentIndex = 0;
  positions[currentIndex * 3 + 0] = -halfWidth; // x
  positions[currentIndex * 3 + 1] = y;          // y
  positions[currentIndex * 3 + 2] = halfWidth;  // z
  texCoords[currentIndex * 2 + 0] = 0.0f;       // u
  texCoords[currentIndex * 2 + 1] = 0.0f;       // v
  currentIndex++;

  positions[currentIndex * 3 + 0] = -halfWidth;  // x
  positions[currentIndex * 3 + 1] = y;           // y
  positions[currentIndex * 3 + 2] = -halfWidth;  // z
  texCoords[currentIndex * 2 + 0] = 1.0f;        // u
  texCoords[currentIndex * 2 + 1] = 0.0f;        // v
  currentIndex++;

  positions[currentIndex * 3 + 0] = -halfWidth;  // x
  positions[currentIndex * 3 + 1] = y + imageWidth; // y
  positions[currentIndex * 3 + 2] = -halfWidth;  // z
  texCoords[currentIndex * 2 + 0] = 1.0f;        // u
  texCoords[currentIndex * 2 + 1] = 1.0f;        // v
  currentIndex++;

  // Second triangle
  positions[currentIndex * 3 + 0] = -halfWidth; // x
  positions[currentIndex * 3 + 1] = y;           // y
  positions[currentIndex * 3 + 2] = halfWidth;   // z
  texCoords[currentIndex * 2 + 0] = 0.0f;        // u
  texCoords[currentIndex * 2 + 1] = 0.0f;        // v
  currentIndex++;

  positions[currentIndex * 3 + 0] = -halfWidth;  // x
  positions[currentIndex * 3 + 1] = y + imageWidth; // y
  positions[currentIndex * 3 + 2] = -halfWidth;  // z
  texCoords[currentIndex * 2 + 0] = 1.0f;        // u
  texCoords[currentIndex * 2 + 1] = 1.0f;        // v
  currentIndex++;

  positions[currentIndex * 3 + 0] = -halfWidth;  // x
  positions[currentIndex * 3 + 1] = y + imageWidth; // y
  positions[currentIndex * 3 + 2] = halfWidth;   // z
  texCoords[currentIndex * 2 + 0] = 0.0f;        // u
  texCoords[currentIndex * 2 + 1] = 1.0f;        // v

  vboTextureSkyLeft.Gen(6, 3, positions.get(), GL_STATIC_DRAW);
  vboTextureColorSkyLeft.Gen(6, 2, texCoords.get(), GL_STATIC_DRAW);
}

void drawSkyRight(int imageWidth, float y) 
{
  std::unique_ptr<float[]> positions = std::make_unique<float[]>(6 * 3);
  std::unique_ptr<float[]> texCoords = std::make_unique<float[]>(6 * 2);
  float halfWidth = imageWidth / 2.0f;

  // Right face
  // First triangle
  int currentIndex = 0;
  positions[currentIndex * 3 + 0] = halfWidth; // x
  positions[currentIndex * 3 + 1] = y;          // y
  positions[currentIndex * 3 + 2] = halfWidth; // z
  texCoords[currentIndex * 2 + 0] = 0.0f;       // u
  texCoords[currentIndex * 2 + 1] = 0.0f;       // v
  currentIndex++;

  positions[currentIndex * 3 + 0] = halfWidth;  // x
  positions[currentIndex * 3 + 1] = y;          // y
  positions[currentIndex * 3 + 2] = -halfWidth; // z
  texCoords[currentIndex * 2 + 0] = 1.0f;       // u
  texCoords[currentIndex * 2 + 1] = 0.0f;       // v
  currentIndex++;

  positions[currentIndex * 3 + 0] = halfWidth;  // x
  positions[currentIndex * 3 + 1] = y + imageWidth; // y
  positions[currentIndex * 3 + 2] = -halfWidth; // z
  texCoords[currentIndex * 2 + 0] = 1.0f;       // u
  texCoords[currentIndex * 2 + 1] = 1.0f;       // v
  currentIndex++;

  // Second triangle
  positions[currentIndex * 3 + 0] = halfWidth; // x
  positions[currentIndex * 3 + 1] = y;          // y
  positions[currentIndex * 3 + 2] = halfWidth; // z
  texCoords[currentIndex * 2 + 0] = 0.0f;       // u
  texCoords[currentIndex * 2 + 1] = 0.0f;       // v
  currentIndex++;

  positions[currentIndex * 3 + 0] = halfWidth;  // x
  positions[currentIndex * 3 + 1] = y + imageWidth; // y
  positions[currentIndex * 3 + 2] = -halfWidth; // z
  texCoords[currentIndex * 2 + 0] = 1.0f;       // u
  texCoords[currentIndex * 2 + 1] = 1.0f;       // v
  currentIndex++;

  positions[currentIndex * 3 + 0] = halfWidth; // x
  positions[currentIndex * 3 + 1] = y + imageWidth; // y
  positions[currentIndex * 3 + 2] = halfWidth; // z
  texCoords[currentIndex * 2 + 0] = 0.0f;       // u
  texCoords[currentIndex * 2 + 1] = 1.0f;       // v

  vboTextureSkyRight.Gen(6, 3, positions.get(), GL_STATIC_DRAW);
  vboTextureColorSkyRight.Gen(6, 2, texCoords.get(), GL_STATIC_DRAW);
}

void computeSplinePoint(float u, Point p0, Point p1, Point p2, Point p3, float s, float* result)
{
  //basis matrix
  float M[16] = {
      -s,  2.0f * s,   -s ,  0,
      2.0f - s, s - 3.0f,  0.0f, 1.0f,
      s - 2.0f,  3.0f-2.0f*s,  s,  0.0f,
      s,  -s, 0.0f,  0.0f
  };

  //control matrix
  float C[12] = { 
      p0.x, p1.x, p2.x, p3.x,
      p0.y, p1.y, p2.y, p3.y, 
      p0.z, p1.z, p2.z, p3.z,
  };


  //[u^3, u^2, u, 1]
  float U[4] = { u * u * u, u * u, u, 1.0f };

  //[x, y, z]
  float UM[4]; // U * M
  MultiplyMatrices(1, 4, 4, U, M, UM);

  //p(u) = U * (M * C)
  float temp[3]; //[x, y, z]
  MultiplyMatrices(1, 4, 3, UM, C, temp);

  result[0] = temp[0]; // x
  result[1] = temp[1]; // y
  result[2] = temp[2]; // z
}

void computeTangent(float u, Point p0, Point p1, Point p2, Point p3, float s, float* tangent) 
{
  //basis matrix
  float M[16] = {
      -s,  2.0f * s,   -s ,  0,
      2.0f - s, s - 3.0f,  0.0f, 1.0f,
      s - 2.0f,  3.0f-2.0f*s,  s,  0.0f,
      s,  -s, 0.0f,  0.0f
  };

  //control matrix
  float C[12] = { 
      p0.x, p1.x, p2.x, p3.x,
      p0.y, p1.y, p2.y, p3.y, 
      p0.z, p1.z, p2.z, p3.z,
  };

  float U[4] = {3*u*u, 2*u, 1, 0};

  float UM[4]; // U * M
  MultiplyMatrices(1, 4, 4, U, M, UM);

  float temp[3]; // T(u) = U * M * C
  MultiplyMatrices(1, 4, 3, UM, C, temp);

  tangent[0] = temp[0];
  tangent[1] = temp[1];
  tangent[2] = temp[2];

  //normalize
  float length = sqrt(tangent[0] * tangent[0] + tangent[1] * tangent[1] + tangent[2] * tangent[2]);
  if (length != 0) 
  {
      tangent[0] /= length;
      tangent[1] /= length;
      tangent[2] /= length;
  }
}

void updateCamera() 
{
  float result[3], tangent[3], binormal[3], normal[3];
  int numSegments = spline.numControlPoints - 3;//total segments
  int currentSegment = floor(globalU * numSegments);  //current segment
  float localU = (globalU * numSegments) - currentSegment;  //local u value within this curve

  if (currentSegment < numSegments) 
  {
      computeSplinePoint(localU, spline.points[currentSegment], spline.points[currentSegment + 1], spline.points[currentSegment + 2], spline.points[currentSegment + 3], 0.5, result);
      computeTangent(localU, spline.points[currentSegment], spline.points[currentSegment + 1], spline.points[currentSegment + 2], spline.points[currentSegment + 3], 0.5, tangent);
      computeNormalAndBinormal(tangent, previousBinormal_camera, normal, binormal);
      memcpy(previousBinormal_camera, binormal, sizeof(binormal));
      float offset = 0.2f;
      float cameraPosition[3] = 
      {
        result[0] + normal[0] * offset,
        result[1] + normal[1] * offset,
        result[2] + normal[2] * offset
      };

      matrix.SetMatrixMode(OpenGLMatrix::ModelView);
      matrix.LoadIdentity();
      matrix.LookAt(cameraPosition[0], cameraPosition[1], cameraPosition[2],
                    result[0] + tangent[0], result[1] + tangent[1], result[2] + tangent[2],
                    normal[0], normal[1], normal[2]);
  }
}

void computeNormalAndBinormal(float* tangent, float* previousBinormal, float* normal, float* binormal) 
{
  crossProduct(previousBinormal, tangent, normal);
  normalize(normal);

  crossProduct(tangent, normal, binormal);
  normalize(binormal);
}

float norm(float* v) 
{
    return sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

void crossProduct(float* v1, float* v2, float* result) 
{
    result[0] = v1[1] * v2[2] - v1[2] * v2[1];
    result[1] = v1[2] * v2[0] - v1[0] * v2[2];
    result[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

void normalize(float* v) 
{
    float length = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (length == 0) length = 1;
    v[0] /= length;
    v[1] /= length;
    v[2] /= length;
}

void renderTrackSection(float s, float stepSize, float trackWidth) 
{
  int numSegments = spline.numControlPoints - 3;
  int pointsPerSegment = (1.0f / stepSize) - 1; // points per segment
  numVertices = static_cast<int>(numSegments * pointsPerSegment * 4 * 2 * 3);
  int currentIndex = 0;
  int colorIndex = 0;
  std::unique_ptr<float[]> positions = std::make_unique<float[]>(numVertices * 3);
  std::unique_ptr<float[]> colors = std::make_unique<float[]>(numVertices * 4);
  std::unique_ptr<float[]> normals = std::make_unique<float[]>(numVertices * 3);
  for (int i = 0; i < numSegments; ++i) 
  {
    //p_{i-1}, p_i, p_{i+1}, p_{i+2}
    Point p0 = spline.points[i];
    Point p1 = spline.points[i + 1];
    Point p2 = spline.points[i + 2];
    Point p3 = spline.points[i + 3];

    for (float u = 0.0f; u <= 1.0f; u += stepSize) 
    {
      //1
      float result_1[3], tangent_1[3], normal_1[3], binormal_1[3];
      computeSplinePoint(u, p0, p1, p2, p3, s, result_1);
      computeTangent(u, p0, p1, p2, p3, s, tangent_1);
      computeNormalAndBinormal(tangent_1, previousBinormal_1, normal_1, binormal_1);
      memcpy(previousBinormal_1, binormal_1, sizeof(binormal_1));
      //2
      float result_2[3], tangent_2[3], normal_2[3], binormal_2[3];
      computeSplinePoint(u + stepSize, p0, p1, p2, p3, s, result_2);
      computeTangent(u + stepSize, p0, p1, p2, p3, s, tangent_2);
      computeNormalAndBinormal(tangent_2, previousBinormal_2, normal_2, binormal_2);
      memcpy(previousBinormal_2, binormal_2, sizeof(binormal_2));
      float one[3];
      float two[3];
      float three[3]; //track vertices
      //vertices_1
      float quadVertices_1[12];
      for (int j = 0; j < 4; j++) 
      {
        float sign1 = (j % 2) ? 1.0f : -1.0f;
        float sign2 = (j < 2) ? 1.0f : -1.0f;
        quadVertices_1[j * 3 + 0] = result_1[0] + sign1 * trackWidth * normal_1[0] + sign2 * trackWidth * binormal_1[0];
        quadVertices_1[j * 3 + 1] = result_1[1] + sign1 * trackWidth * normal_1[1] + sign2 * trackWidth * binormal_1[1];
        quadVertices_1[j * 3 + 2] = result_1[2] + sign1 * trackWidth * normal_1[2] + sign2 * trackWidth * binormal_1[2];
      }

      //vertices_2
      float quadVertices_2[12];
      for (int j = 0; j < 4; j++) 
      {
        float sign1 = (j % 2) ? 1.0f : -1.0f;
        float sign2 = (j < 2) ? 1.0f : -1.0f;
        quadVertices_2[j * 3 + 0] = result_2[0] + sign1 * trackWidth * normal_2[0] + sign2 * trackWidth * binormal_2[0];
        quadVertices_2[j * 3 + 1] = result_2[1] + sign1 * trackWidth * normal_2[1] + sign2 * trackWidth * binormal_2[1];
        quadVertices_2[j * 3 + 2] = result_2[2] + sign1 * trackWidth * normal_2[2] + sign2 * trackWidth * binormal_2[2];
      }
      // 0  1   0  1
      // 2  3   2  3

      //Face 1
      // First triangle (vertices A0, A1, B1)
      positions[currentIndex * 3 + 0] = quadVertices_1[0];
      positions[currentIndex * 3 + 1] = quadVertices_1[1];
      positions[currentIndex * 3 + 2] = quadVertices_1[2];
      one[0] = quadVertices_1[0];
      one[1] = quadVertices_1[1];
      one[2] = quadVertices_1[2];
      currentIndex++;

      positions[currentIndex * 3 + 0] = quadVertices_1[3];
      positions[currentIndex * 3 + 1] = quadVertices_1[4];
      positions[currentIndex * 3 + 2] = quadVertices_1[5];
      two[0] = quadVertices_1[3];
      two[1] = quadVertices_1[4];
      two[2] = quadVertices_1[5];
      currentIndex++;

      positions[currentIndex * 3 + 0] = quadVertices_2[3];
      positions[currentIndex * 3 + 1] = quadVertices_2[4];
      positions[currentIndex * 3 + 2] = quadVertices_2[5];
      three[0] = quadVertices_2[3];
      three[1] = quadVertices_2[4];
      three[2] = quadVertices_2[5];
      currentIndex++;

      computeColor(one, two ,three, colors, normals, colorIndex);
      colorIndex++;
      computeColor(one, two ,three, colors, normals, colorIndex);
      colorIndex++;
      computeColor(one, two ,three, colors, normals, colorIndex);
      colorIndex++;

      // Second triangle (vertices A0, B1, B0)
      positions[currentIndex * 3 + 0] = quadVertices_1[0];
      positions[currentIndex * 3 + 1] = quadVertices_1[1];
      positions[currentIndex * 3 + 2] = quadVertices_1[2];
      one[0] = quadVertices_1[0];
      one[1] = quadVertices_1[1];
      one[2] = quadVertices_1[2];
      currentIndex++;

      positions[currentIndex * 3 + 0] = quadVertices_2[3];
      positions[currentIndex * 3 + 1] = quadVertices_2[4];
      positions[currentIndex * 3 + 2] = quadVertices_2[5];
      two[0] = quadVertices_2[3];
      two[1] = quadVertices_2[4];
      two[2] = quadVertices_2[5];
      currentIndex++;

      positions[currentIndex * 3 + 0] = quadVertices_2[0];
      positions[currentIndex * 3 + 1] = quadVertices_2[1];
      positions[currentIndex * 3 + 2] = quadVertices_2[2];
      three[0] = quadVertices_2[0];
      three[1] = quadVertices_2[1];
      three[2] = quadVertices_2[2];
      currentIndex++;

      computeColor(one, two ,three, colors, normals, colorIndex);
      colorIndex++;
      computeColor(one, two ,three, colors, normals, colorIndex);
      colorIndex++;
      computeColor(one, two ,three, colors, normals, colorIndex);
      colorIndex++;

      //Face 2
      // First triangle (vertices A0, A2, B2)
      positions[currentIndex * 3 + 0] = quadVertices_1[0];
      positions[currentIndex * 3 + 1] = quadVertices_1[1];
      positions[currentIndex * 3 + 2] = quadVertices_1[2];
      one[0] = quadVertices_1[0];
      one[1] = quadVertices_1[1];
      one[2] = quadVertices_1[2];
      currentIndex++;

      positions[currentIndex * 3 + 0] = quadVertices_1[6];
      positions[currentIndex * 3 + 1] = quadVertices_1[7];
      positions[currentIndex * 3 + 2] = quadVertices_1[8];
      two[0] = quadVertices_1[6];
      two[1] = quadVertices_1[7];
      two[2] = quadVertices_1[8];
      currentIndex++;

      positions[currentIndex * 3 + 0] = quadVertices_2[6];
      positions[currentIndex * 3 + 1] = quadVertices_2[7];
      positions[currentIndex * 3 + 2] = quadVertices_2[8];
      three[0] = quadVertices_2[6];
      three[1] = quadVertices_2[7];
      three[2] = quadVertices_2[8];
      currentIndex++;

      computeColor(one, two ,three, colors, normals, colorIndex);
      colorIndex++;
      computeColor(one, two ,three, colors, normals, colorIndex);
      colorIndex++;
      computeColor(one, two ,three, colors, normals, colorIndex);
      colorIndex++;

      // Second triangle (vertices A0, B2, B0)
      positions[currentIndex * 3 + 0] = quadVertices_1[0];
      positions[currentIndex * 3 + 1] = quadVertices_1[1];
      positions[currentIndex * 3 + 2] = quadVertices_1[2];
      one[0] = quadVertices_1[0];
      one[1] = quadVertices_1[1];
      one[2] = quadVertices_1[2];
      currentIndex++;

      positions[currentIndex * 3 + 0] = quadVertices_2[6];
      positions[currentIndex * 3 + 1] = quadVertices_2[7];
      positions[currentIndex * 3 + 2] = quadVertices_2[8];
      two[0] = quadVertices_2[6];
      two[1] = quadVertices_2[7];
      two[2] = quadVertices_2[8];
      currentIndex++;

      positions[currentIndex * 3 + 0] = quadVertices_2[0];
      positions[currentIndex * 3 + 1] = quadVertices_2[1];
      positions[currentIndex * 3 + 2] = quadVertices_2[2];
      three[0] = quadVertices_2[0];
      three[1] = quadVertices_2[1];
      three[2] = quadVertices_2[2];
      currentIndex++;

      computeColor(one, two ,three, colors, normals, colorIndex);
      colorIndex++;
      computeColor(one, two ,three, colors, normals, colorIndex);
      colorIndex++;
      computeColor(one, two ,three, colors, normals, colorIndex);
      colorIndex++;

      //Face 3
      // First triangle (vertices A2, A3, B3)
      positions[currentIndex * 3 + 0] = quadVertices_1[6];
      positions[currentIndex * 3 + 1] = quadVertices_1[7];
      positions[currentIndex * 3 + 2] = quadVertices_1[8];
      one[0] = quadVertices_1[6];
      one[1] = quadVertices_1[7];
      one[2] = quadVertices_1[8];
      currentIndex++;

      positions[currentIndex * 3 + 0] = quadVertices_1[9];
      positions[currentIndex * 3 + 1] = quadVertices_1[10];
      positions[currentIndex * 3 + 2] = quadVertices_1[11];
      two[0] = quadVertices_1[9];
      two[1] = quadVertices_1[10];
      two[2] = quadVertices_1[11];
      currentIndex++;

      positions[currentIndex * 3 + 0] = quadVertices_2[9];
      positions[currentIndex * 3 + 1] = quadVertices_2[10];
      positions[currentIndex * 3 + 2] = quadVertices_2[11];
      three[0] = quadVertices_2[9];
      three[1] = quadVertices_2[10];
      three[2] = quadVertices_2[11];
      currentIndex++;

      computeColor(one, two ,three, colors, normals, colorIndex);
      colorIndex++;
      computeColor(one, two ,three, colors, normals, colorIndex);
      colorIndex++;
      computeColor(one, two ,three, colors, normals, colorIndex);
      colorIndex++;

      // Second triangle (vertices A2, B3, B2)
      positions[currentIndex * 3 + 0] = quadVertices_1[6];
      positions[currentIndex * 3 + 1] = quadVertices_1[7];
      positions[currentIndex * 3 + 2] = quadVertices_1[8];
      one[0] = quadVertices_1[6];
      one[1] = quadVertices_1[7];
      one[2] = quadVertices_1[8];
      currentIndex++;

      positions[currentIndex * 3 + 0] = quadVertices_2[9];
      positions[currentIndex * 3 + 1] = quadVertices_2[10];
      positions[currentIndex * 3 + 2] = quadVertices_2[11];
      two[0] = quadVertices_2[9];
      two[1] = quadVertices_2[10];
      two[2] = quadVertices_2[11];
      currentIndex++;

      positions[currentIndex * 3 + 0] = quadVertices_2[6];
      positions[currentIndex * 3 + 1] = quadVertices_2[7];
      positions[currentIndex * 3 + 2] = quadVertices_2[8];
      three[0] = quadVertices_2[6];
      three[1] = quadVertices_2[7];
      three[2] = quadVertices_2[8];
      currentIndex++;

      computeColor(one, two ,three, colors, normals, colorIndex);
      colorIndex++;
      computeColor(one, two ,three, colors, normals, colorIndex);
      colorIndex++;
      computeColor(one, two ,three, colors, normals, colorIndex);
      colorIndex++;

      //Face 4
      // First triangle (vertices A1, A3, B3)
      positions[currentIndex * 3 + 0] = quadVertices_1[3];
      positions[currentIndex * 3 + 1] = quadVertices_1[4];
      positions[currentIndex * 3 + 2] = quadVertices_1[5];
      one[0] = quadVertices_1[3];
      one[1] = quadVertices_1[4];
      one[2] = quadVertices_1[5];
      currentIndex++;

      positions[currentIndex * 3 + 0] = quadVertices_1[9];
      positions[currentIndex * 3 + 1] = quadVertices_1[10];
      positions[currentIndex * 3 + 2] = quadVertices_1[11];
      two[0] = quadVertices_1[9];
      two[1] = quadVertices_1[10];
      two[2] = quadVertices_1[11];
      currentIndex++;

      positions[currentIndex * 3 + 0] = quadVertices_2[9];
      positions[currentIndex * 3 + 1] = quadVertices_2[10];
      positions[currentIndex * 3 + 2] = quadVertices_2[11];
      three[0] = quadVertices_2[9];
      three[1] = quadVertices_2[10];
      three[2] = quadVertices_2[11];
      currentIndex++;

      computeColor(one, two ,three, colors, normals, colorIndex);
      colorIndex++;
      computeColor(one, two ,three, colors, normals, colorIndex);
      colorIndex++;
      computeColor(one, two ,three, colors, normals, colorIndex);
      colorIndex++;

      // Second triangle (vertices A1, B3, B1)
      positions[currentIndex * 3 + 0] = quadVertices_1[3];
      positions[currentIndex * 3 + 1] = quadVertices_1[4];
      positions[currentIndex * 3 + 2] = quadVertices_1[5];
      one[0] = quadVertices_1[3];
      one[1] = quadVertices_1[4];
      one[2] = quadVertices_1[5];
      currentIndex++;

      positions[currentIndex * 3 + 0] = quadVertices_2[9];
      positions[currentIndex * 3 + 1] = quadVertices_2[10];
      positions[currentIndex * 3 + 2] = quadVertices_2[11];
      two[0] = quadVertices_2[9];
      two[1] = quadVertices_2[10];
      two[2] = quadVertices_2[11];
      currentIndex++;

      positions[currentIndex * 3 + 0] = quadVertices_2[3];
      positions[currentIndex * 3 + 1] = quadVertices_2[4];
      positions[currentIndex * 3 + 2] = quadVertices_2[5];
      three[0] = quadVertices_2[3];
      three[1] = quadVertices_2[4];
      three[2] = quadVertices_2[5];
      currentIndex++;

      computeColor(one, two ,three, colors, normals, colorIndex);
      colorIndex++;
      computeColor(one, two ,three, colors, normals, colorIndex);
      colorIndex++;
      computeColor(one, two ,three, colors, normals, colorIndex);
      colorIndex++;
    }
  }
  vboVertices.Gen(numVertices, 3, positions.get(), GL_STATIC_DRAW);
  vboColors.Gen(numVertices, 4, colors.get(), GL_STATIC_DRAW);
  vboNormal.Gen(numVertices, 3, normals.get(), GL_STATIC_DRAW);
}

void computeColor(float* v1, float* v2, float* v3, std::unique_ptr<float[]>& colors, std::unique_ptr<float[]>& normals, int colorIndex) 
{
    //edge vector
    float e1[3] = {v2[0] - v1[0], v2[1] - v1[1], v2[2] - v1[2]};
    float e2[3] = {v3[0] - v1[0], v3[1] - v1[1], v3[2] - v1[2]};
    
    //normal 
    float normal[3];
    crossProduct(e1, e2, normal);
    normalize(normal);

    normals[colorIndex * 3 + 0] = std::abs(normal[0]);
    normals[colorIndex * 3 + 1] = std::abs(normal[1]);
    normals[colorIndex * 3 + 2] = std::abs(normal[2]);

    colors[colorIndex * 4 + 0] = std::abs(normal[0]);
    colors[colorIndex * 4 + 1] = std::abs(normal[1]);
    colors[colorIndex * 4 + 2] = std::abs(normal[2]);
    colors[colorIndex * 4 + 3] = 1.0f;
}

void CatmullRomMode(float s, float stepSize)
{
  int numSegments = spline.numControlPoints - 3;
  numVertices = 0;

  for (int i = 0; i < numSegments; ++i) 
  {
      numVertices += (1.0f / stepSize);
  }
  int currentIndex = 0;
  std::unique_ptr<float[]> positions = std::make_unique<float[]>(numVertices * 3);
  std::unique_ptr<float[]> colors = std::make_unique<float[]>(numVertices * 4);
  for (int i = 0; i < numSegments; ++i) 
  {
      //p_{i-1}, p_i, p_{i+1}, p_{i+2}
      Point p0 = spline.points[i];
      Point p1 = spline.points[i + 1];
      Point p2 = spline.points[i + 2];
      Point p3 = spline.points[i + 3];

      for (float u = 0.0f; u <= 1.0f; u += stepSize) 
      {
          float result[3];

          computeSplinePoint(u, p0, p1, p2, p3, s, result);

          positions[currentIndex * 3] = result[0];    // x
          positions[currentIndex * 3 + 1] = result[1]; // y
          positions[currentIndex * 3 + 2] = result[2]; // z

          colors[currentIndex * 4] = 1.0f;   // r
          colors[currentIndex * 4 + 1] = 1.0f; // g
          colors[currentIndex * 4 + 2] = 1.0f; // b
          colors[currentIndex * 4 + 3] = 1.0f; // alpha

          currentIndex++;
      }
  }

  vboVertices.Gen(numVertices, 3, positions.get(), GL_STATIC_DRAW);
  vboColors.Gen(numVertices, 4, colors.get(), GL_STATIC_DRAW);
}

// Multiply C = A * B, where A is a m x p matrix, and B is a p x n matrix.
// All matrices A, B, C must be pre-allocated (say, using malloc or similar).
// The memory storage for C must *not* overlap in memory with either A or B. 
// That is, you **cannot** do C = A * C, or C = C * B. However, A and B can overlap, and so C = A * A is fine, as long as the memory buffer for A is not overlaping in memory with that of C.
// Very important: All matrices are stored in **column-major** format.
// Example. Suppose 
//      [ 1 8 2 ]
//  A = [ 3 5 7 ]
//      [ 0 2 4 ]
//  Then, the storage in memory is
//   1, 3, 0, 8, 5, 2, 2, 7, 4. 
void MultiplyMatrices(int m, int p, int n, const float * A, const float * B, float * C)
{
  for(int i=0; i<m; i++)
  {
    for(int j=0; j<n; j++)
    {
      float entry = 0.0;
      for(int k=0; k<p; k++)
        entry += A[k * m + i] * B[j * p + k];
      C[m * j + i] = entry;
    }
  }
}

int initTexture(const char * imageFilename, GLuint textureHandle)
{
  // Read the texture image.
  ImageIO img;
  ImageIO::fileFormatType imgFormat;
  ImageIO::errorType err = img.load(imageFilename, &imgFormat);

  if (err != ImageIO::OK) 
  {
    printf("Loading texture from %s failed.\n", imageFilename);
    return -1;
  }

  // Check that the number of bytes is a multiple of 4.
  if (img.getWidth() * img.getBytesPerPixel() % 4) 
  {
    printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
    return -1;
  }

  // Allocate space for an array of pixels.
  int width = img.getWidth();
  int height = img.getHeight();
  unsigned char * pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

  // Fill the pixelsRGBA array with the image pixels.
  memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
  for (int h = 0; h < height; h++)
    for (int w = 0; w < width; w++) 
    {
      // assign some default byte values (for the case where img.getBytesPerPixel() < 4)
      pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
      pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
      pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
      pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

      // set the RGBA channels, based on the loaded image
      int numChannels = img.getBytesPerPixel();
      for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
        pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
    }

  // Bind the texture.
  glBindTexture(GL_TEXTURE_2D, textureHandle);

  // Initialize the texture.
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

  // Generate the mipmaps for this texture.
  glGenerateMipmap(GL_TEXTURE_2D);

  // Set the texture parameters.
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // Query support for anisotropic texture filtering.
  GLfloat fLargest;
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
  printf("Max available anisotropic samples: %f\n", fLargest);
  // Set anisotropic texture filtering.
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

  // Query for any errors.
  GLenum errCode = glGetError();
  if (errCode != 0) 
  {
    printf("Texture initialization error. Error code: %d.\n", errCode);
    return -1;
  }
  
  // De-allocate the pixel array -- it is no longer needed.
  delete [] pixelsRGBA;

  return 0;
}


int main(int argc, char *argv[])
{
  if (argc < 2)
  {  
    printf ("Usage: %s <spline file>\n", argv[0]);
    exit(0);
  }

  cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);

  cout << "Initializing OpenGL..." << endl;

  #ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);  
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

  #ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(windowWidth - 1, windowHeight - 1);
  #endif

  // Tells GLUT to use a particular display function to redraw.
  glutDisplayFunc(displayFunc);
  // Perform animation inside idleFunc.
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for idle mouse movement
  glutPassiveMotionFunc(mouseMotionFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);

  // init glew
  #ifdef __APPLE__
    // nothing is needed on Apple
  #else
    // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
      cout << "error: " << glewGetErrorString(result) << endl;
      exit(EXIT_FAILURE);
    }
  #endif

  // Perform the initialization.
  initScene(argc, argv);

  // Load spline from the provided filename.
  // loadSpline(argv[1]);


  printf("Loaded spline with %d control point(s).\n", spline.numControlPoints);

  // Sink forever into the GLUT loop.
  glutMainLoop();

  return 0;
}

