/*
  CSCI 420 Computer Graphics, Computer Science, USC
  Assignment 1: Height Fields with Shaders.
  C/C++ starter code

  Student username: <Jiale Wang>
*/

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

int currentFrame = 0;
const int maxFrames = 300;
float lastTime = 0.0f; //lastTime from last frame
const float frameInterval = 1000.0f / 15.0f;

int mousePos[2]; // x,y screen coordinates of the current mouse position

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
char windowTitle[512] = "CSCI 420 Homework 1";

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
// Number of vertices in the single triangle (starter code).
int numVertices;

int width;
int height;

float scale = 1.0f;
float exponent = 1.0f;

int mode = 0;

auto renderMode = GL_POINTS;
std::unique_ptr<ImageIO> heightmapImage = std::make_unique<ImageIO>();
// CSCI 420 helper classes.
OpenGLMatrix matrix;
PipelineProgram pipelineProgram;
VBO vboVertices;
VBO vboColors;
VAO vao;
VBO vboLeft;
VBO vboRight;
VBO vboUp;
VBO vboDown;
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
  // Do some stuff... 
  // For example, here, you can save the screenshots to disk (to make the animation).

  //set Timer
  float currentTime = glutGet(GLUT_ELAPSED_TIME);

  if (currentTime - lastTime >= frameInterval && currentFrame < maxFrames)
  {
      char filename[1000];
      //merge name with path
      sprintf(filename, "/Users/davidwang/Documents/CSCI420/HW1/assign1_coreOpenGL_starterCode/screenshots/%03d.jpg", currentFrame);

      saveScreenshot(filename);

      lastTime = currentTime;

      currentFrame++;
  }
  
  // Notify GLUT that it should call displayFunc.
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
    case '1':
      mode = 0;
      pipelineProgram.SetUniformVariablei("mode", mode);
      renderMode = GL_POINTS;
      pointMode(heightmapImage);
      vao.Gen();
      vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboVertices, "position");
      vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboColors, "color");
      glutPostRedisplay(); 
    break;
    case '2':
      mode = 0;
      pipelineProgram.SetUniformVariablei("mode", mode);
      renderMode = GL_LINES;
      lineMode(heightmapImage);
      vao.Gen();
      vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboVertices, "position");
      vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboColors, "color");
      glutPostRedisplay(); 
    break;
    case '3':
      mode = 0;
      pipelineProgram.SetUniformVariablei("mode", mode);
      renderMode = GL_TRIANGLES;
      triangleMode(heightmapImage);
      vao.Gen();
      vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboVertices, "position");
      vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboColors, "color");
      vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboLeft, "position_left");
      vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboRight, "position_right");
      vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboUp, "position_up");
      vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboDown, "position_down");
      glutPostRedisplay(); 
    break;
    case '4':
      mode = 1;
      pipelineProgram.SetUniformVariablei("mode", mode);
      renderMode = GL_TRIANGLES;
      triangleMode(heightmapImage);
      vao.Gen();
      vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboVertices, "position");
      vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboColors, "color");
      vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboLeft, "position_left");
      vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboRight, "position_right");
      vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboUp, "position_up");
      vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboDown, "position_down");
      glutPostRedisplay();
    break;
    case '+': 
      if (mode == 1) 
      {
        scale *= 2.0f;
        renderMode = GL_TRIANGLES;
        pipelineProgram.SetUniformVariablef("scale", scale);
        glutPostRedisplay();
      }
    break;
    case '-':
      if (mode == 1) 
      {
        scale /= 2.0f;
        pipelineProgram.SetUniformVariablef("scale", scale);
        glutPostRedisplay();
      }
    break;
    case '9':
      if (mode == 1) 
      {
        exponent *= 2.0f;
        pipelineProgram.SetUniformVariablef("exponent", exponent);
        glutPostRedisplay();
      }
    break;
    case '0':
      if (mode == 1) 
      {
        exponent /= 2.0f;
        pipelineProgram.SetUniformVariablef("exponent", exponent);
        glutPostRedisplay();
      }
    break;
    case 27: // ESC key
      exit(0); // exit the program
    break;
    case ' ':
      cout << "You pressed the spacebar." << endl;
    break;

    case 'x':
      // Take a screenshot.
      saveScreenshot("/Users/davidwang/Documents/CSCI420/HW1/assign1_coreOpenGL_starterCode/screenshots/screenshot.jpg");
    break;
  }
}

void displayFunc()
{
  // This function performs the actual rendering.
  // First, clear the screen.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Set up the camera position, focus point, and the up vector.
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();
  
  float centerX = width / 2.0f; 
  float centerZ = height / 2.0f;

  float centerInd = 2.0f * std::max(width, height); //move to the center

  matrix.LookAt(centerX, centerZ, centerInd,
                centerX, centerZ, 0.0,
                0.0, 1.0, 0.0);
  // In here, you can do additional modeling on the object, such as performing translations, rotations and scales.
  // ...

  matrix.Translate(terrainTranslate[0], terrainTranslate[1], terrainTranslate[2]);//Translate

  matrix.Rotate(terrainRotate[0], 1.0, 0.0, 0.0); //X axis
  matrix.Rotate(terrainRotate[1], 0.0, 1.0, 0.0); //Y axis
  matrix.Rotate(terrainRotate[2], 0.0, 0.0, 1.0); //Z axis

  matrix.Scale(terrainScale[0], terrainScale[1], terrainScale[2]);
  // Read the current modelview and projection matrices from our helper class.
  // The matrices are only read here; nothing is actually communicated to OpenGL yet.
  float modelViewMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetMatrix(modelViewMatrix);

  float projectionMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(projectionMatrix);

  // Upload the modelview and projection matrices to the GPU. Note that these are "uniform" variables.
  // Important: these matrices must be uploaded to *all* pipeline programs used.
  // In hw1, there is only one pipeline program, but in hw2 there will be several of them.
  // In such a case, you must separately upload to *each* pipeline program.
  // Important: do not make a typo in the variable name below; otherwise, the program will malfunction.
  pipelineProgram.SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
  pipelineProgram.SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);

  // Execute the rendering.
  // Bind the VAO that we want to render. Remember, one object = one VAO. 
  vao.Bind();
  
  glDrawArrays(renderMode, 0, numVertices); // Render the VAO, by rendering "numVertices", starting from vertex 0.

  // Swap the double-buffers.
  glutSwapBuffers();
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

void initScene(int argc, char *argv[])
{
  // Load the image from a jpeg disk file into main memory.
  if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
  {
    cout << "Error reading image " << argv[1] << "." << endl;
    exit(EXIT_FAILURE);
  }

  // Set the background color.
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black color.

  // Enable z-buffering (i.e., hidden surface removal using the z-buffer algorithm).
  glEnable(GL_DEPTH_TEST);

  // Create a pipeline program. This operation must be performed BEFORE we initialize any VAOs.
  // A pipeline program contains our shaders. Different pipeline programs may contain different shaders.
  // In this homework, we only have one set of shaders, and therefore, there is only one pipeline program.
  // In hw2, we will need to shade different objects with different shaders, and therefore, we will have
  // several pipeline programs (e.g., one for the rails, one for the ground/sky, etc.).
  // Load and set up the pipeline program, including its shaders.
  if (pipelineProgram.BuildShadersFromFiles(shaderBasePath, "vertexShader.glsl", "fragmentShader.glsl") != 0)
  {
    cout << "Failed to build the pipeline program." << endl;
    throw 1;
  } 
  cout << "Successfully built the pipeline program." << endl;
  // Bind the pipeline program that we just created. 
  // The purpose of binding a pipeline program is to activate the shaders that it contains, i.e.,
  // any object rendered from that point on, will use those shaders.
  // When the application starts, no pipeline program is bound, which means that rendering is not set up.
  // So, at some point (such as below), we need to bind a pipeline program.
  // From that point on, exactly one pipeline program is bound at any moment of time.

  pipelineProgram.Bind();

  pipelineProgram.SetUniformVariablef("scale", scale);
  pipelineProgram.SetUniformVariablef("exponent", exponent);  
  // Prepare the triangle position and color data for the VBO. 
  // The code below sets up a single triangle (3 vertices).
  // The triangle will be rendered using GL_TRIANGLES (in displayFunc()).

  // pointMode(heightmapImage);
  // lineMode(heightmapImage);
  // triangleMode(heightmapImage);

  // Create the VAOs. There is a single VAO in this example.
  // Important: this code must be executed AFTER we created our pipeline program, and AFTER we set up our VBOs.
  // A VAO contains the geometry for a single object. There should be one VAO per object.
  // In this homework, "geometry" means vertex positions and colors. In homework 2, it will also include
  // vertex normal and vertex texture coordinates for texture mapping.

  // Set up the relationship between the "position" shader variable and the VAO.
  // Important: any typo in the shader variable name will lead to malfunction.

  // Set up the relationship between the "color" shader variable and the VAO.
  // Important: any typo in the shader variable name will lead to malfunction.

  // Check for any OpenGL errors.
  vao.Bind();

  std::cout << "GL error status is: " << glGetError() << std::endl;
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    cout << "The arguments are incorrect." << endl;
    cout << "usage: ./hw1 <heightmap file>" << endl;
    exit(EXIT_FAILURE);
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

  // Sink forever into the GLUT loop.
  glutMainLoop();
}

