/* **************************
 * CSCI 420
 * Assignment 3 Raytracer
 * Name: <Jiale Wang>
 * *************************
*/

#ifdef WIN32
  #include <windows.h>
#endif

#if defined(WIN32) || defined(linux)
  #include <GL/gl.h>
  #include <GL/glut.h>
#elif defined(__APPLE__)
  #include <OpenGL/gl.h>
  #include <GLUT/glut.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
  #define strcasecmp _stricmp
#endif

#include <imageIO.h>
#include <math.h>
#include <algorithm>
#include <random>
#include <vector>

#define MAX_TRIANGLES 20000
#define MAX_SPHERES 100
#define MAX_LIGHTS 100

char * filename = NULL;

// The different display modes.
#define MODE_DISPLAY 1
#define MODE_JPEG 2
#define MAX_REFLECTION_DEPTH 10

int isRe = 0;
int mode = MODE_DISPLAY;

// While solving the homework, it is useful to make the below values smaller for debugging purposes.
// The still images that you need to submit with the homework should be at the below resolution (640x480).
// However, for your own purposes, after you have solved the homework, you can increase those values to obtain higher-resolution images.
#define WIDTH 640
#define HEIGHT 480

// The field of view of the camera, in degrees.
#define fov 60.0

// Buffer to store the image when saving it to a JPEG.
unsigned char buffer[HEIGHT][WIDTH][3];

struct Vertex
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double normal[3];
  double shininess;
};

struct Triangle
{
  Vertex v[3];
};

struct Sphere
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double shininess;
  double radius;
};

struct Light
{
  double position[3];
  double color[3];
};

Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];
double ambient_light[3];

int num_triangles = 0;
int num_spheres = 0;
int num_lights = 0;
double alpha;
double beta;
double gamma;
void plot_pixel_display(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel_jpeg(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void normalize(double* v);
void set_vector(float* v, float x, float y, float z);
double dot(const double* a, const double* b);
bool intersectSphere(double* rayPos, double* dir, Sphere& sphere, double& t);
void crossProduct(const double* v1, const double* v2, double* cross);
bool intersectTriangle(double* rayPos, double* dir, Triangle& triangle, double& t);
bool pointInTriangle(double px, double py, double p1x, double p1y, double p2x, double p2y, double p3x, double p3y);
bool pointInTriangle(double* rayPos, double* dir, Triangle& triangle, double t);
bool isShadowed(double* point, Light& light, double closest_t);
void interpolateAttributes(Triangle& tri, double alpha, double beta, double gamma, double* N, double* diffuse, double* specular, double& shininess);
void calculateTrianglePhongShading(double* point, double* N, double* rayDir, Triangle& tri, Light* lights, int numLights, double* finalColor);
void calculateSpherePhongShading(double* point, double* N, double* rayDir, Sphere& sph, Light* lights, int numLights, double* finalColor);
void trace_ray(double* origin, double* direction, double* color, int depth);
void draw_scene_recursive();

// This version has no recursive reflection
void draw_scene()
{
  // calculate ray parameters
  double aspect_ratio = (double) WIDTH / (double) HEIGHT;
  double half_height = tan((fov * M_PI / 180.0f) / 2.0f);
  double half_width = aspect_ratio * half_height;

  //random sampling
  srand(time(NULL));
  double randomOffset = (rand() % 1000) / 1000.0 - 0.5;

  // put camera at 000
  double camera[3] = {0, 0, 0};

  for(unsigned int x = 0; x < WIDTH; x++)
  {
    glPointSize(2.0);
    glBegin(GL_POINTS);
    for(unsigned int y = 0; y < HEIGHT; y++)
    {
      //anti-Aliasing by multi-sampling
      // Multisampling parameters
      const int N = 4; 
      double finalColor[3] = {0, 0, 0};
      bool hit = false;
      for (int s = 0; s < N; ++s) 
      {
        // generate random offsets for subpixel sampling
        double subpixel_offset_x = (rand() % 1000) / 1000.0 - 0.5;
        double subpixel_offset_y = (rand() % 1000) / 1000.0 - 0.5;

        // calculate subpixel ray direction
        double px = ((x + 0.5 + subpixel_offset_x) / WIDTH - 0.5) * 2 * half_width;
        double py = -(0.5 - (y + 0.5 + subpixel_offset_y) / HEIGHT) * 2 * half_height;
        double pz = -1.0;
        double rayDir[3] = {px, py, pz};
        normalize(rayDir);

        //subpixel color
        double color[3] = {0, 0, 0};
        double closest_t = INFINITY;

        // Triangle Intersection
        for (int i = 0; i < num_triangles; i++)
        {
          double t = 0;
          if (intersectTriangle(camera, rayDir, triangles[i], t) && t < closest_t)
          {
            closest_t = t;
            hit = true;
            double point[3] = 
            {
              camera[0] + t * rayDir[0],
              camera[1] + t * rayDir[1],
              camera[2] + t * rayDir[2]
            };
            //printf("r: %f, g: %f, b:%f \n", triangles[i].v[0].color_diffuse[0], triangles[i].v[0].color_diffuse[1], triangles[i].v[0].color_diffuse[1]);
            double N[3], diffuse[3], specular[3], shininess;
            //set up triangle attributes
            interpolateAttributes(triangles[i], alpha, beta, gamma, N, diffuse, specular, shininess);
            calculateTrianglePhongShading(point, N, rayDir, triangles[i], lights, num_lights, color);
          }
        }
        // Sphere Intersection
        for (int i = 0; i < num_spheres; i++)
        {
          double t = 0;
          if (intersectSphere(camera, rayDir, spheres[i], t) && t < closest_t)
          {
            closest_t = t;
            hit = true;
            double point[3] = 
            {
              camera[0] + t * rayDir[0],
              camera[1] + t * rayDir[1],
              camera[2] + t * rayDir[2]
            };
            double N[3] = 
            {
              (point[0] - spheres[i].position[0]) / spheres[i].radius,
              (point[1] - spheres[i].position[1]) / spheres[i].radius,
              (point[2] - spheres[i].position[2]) / spheres[i].radius
            };
            // printf("r: %f, g: %f, b:%f \n", spheres[i].v[0].color_diffuse[0], spheres[i].v[0].color_diffuse[1], spheres[i].v[0].color_diffuse[1]);
            normalize(N);
            calculateSpherePhongShading(point, N, rayDir, spheres[i], lights, num_lights, color);
          }
        }

        if (hit) 
        {
          //add color from samples
          finalColor[0] += color[0] / N;
          finalColor[1] += color[1] / N;
          finalColor[2] += color[2] / N;
        }
      }

      // plot with average
      if (hit) 
      {
        plot_pixel(x, y, finalColor[0] * 255, finalColor[1] * 255, finalColor[2] * 255);
      } 
      else 
      {
        plot_pixel(x, y, 0, 0, 0);
      }
    }
    glEnd();
    glFlush();
  }
  printf("Ray tracing completed.\n");
  fflush(stdout);
}

void draw_scene_recursive() 
{
  // calculate ray parameters
  double aspect_ratio = (double) WIDTH / (double) HEIGHT;
  double half_height = tan((fov * M_PI / 180.0f) / 2.0f);
  double half_width = aspect_ratio * half_height;

  //random sampling
  srand(time(NULL));
  double randomOffset = (rand() % 1000) / 1000.0 - 0.5;

  // put camera at 000
  double camera[3] = {0, 0, 0};

  for(unsigned int x = 0; x < WIDTH; x++)
  {
    glPointSize(2.0);
    glBegin(GL_POINTS);
    for(unsigned int y = 0; y < HEIGHT; y++)
    {
      double finalColor[3] = {0, 0, 0};
      bool hit = false;

      //anti-Aliasing by multi-sampling
      // Multisampling parameters
      const int N = 4;
      for (int s = 0; s < N; ++s) 
      {
        double subpixel_offset_x = (rand() % 1000) / 1000.0 - 0.5;
        double subpixel_offset_y = (rand() % 1000) / 1000.0 - 0.5;
        // calculate subpixel ray direction
        double px = ((x + 0.5 + subpixel_offset_x) / WIDTH - 0.5) * 2 * half_width;
        double py = -(0.5 - (y + 0.5 + subpixel_offset_y) / HEIGHT) * 2 * half_height;
        double rayDir[3] = {px, py, -1.0};
        normalize(rayDir);

        // subpixel color
        double color[3] = {0, 0, 0};
        trace_ray(camera, rayDir, color, 0);
        
        // add to final color
        finalColor[0] += color[0] / N;
        finalColor[1] += color[1] / N;
        finalColor[2] += color[2] / N;

        //different from above: we want to count it as hit when at least one of N samples hit
        hit = hit || (color[0] > 0 || color[1] > 0 || color[2] > 0);
      }

      if (hit) 
      {
        plot_pixel(x, y, finalColor[0] * 255, finalColor[1] * 255, finalColor[2] * 255);
      } 
      else 
      {
        plot_pixel(x, y, 0, 0, 0);
      }
    }
    glEnd();
    glFlush();
  }
  printf("Ray tracing completed.\n");
  fflush(stdout);
}

//use old rays to calculate hit position, and normal of the hit object to calculate reflection ray
void trace_ray(double* origin, double* direction, double* color, int depth) 
{
  //reaches max recursiong depth
  if (depth > MAX_REFLECTION_DEPTH) 
  {
    return;
  }

  double closest_t = INFINITY;
  Sphere* hit_sphere = nullptr;
  Triangle* hit_triangle = nullptr;
  double N[3], point[3];
  bool hit = false;

  // check for intersection and store object
  for (int i = 0; i < num_spheres; i++) 
  {
    double t = 0;
    if (intersectSphere(origin, direction, spheres[i], t) && t < closest_t) 
    {
      closest_t = t;
      hit_sphere = &spheres[i];
      hit = true;
    }
  }

  for (int i = 0; i < num_triangles; i++) 
  {
    double t = 0;
    if (intersectTriangle(origin, direction, triangles[i], t) && t < closest_t) 
    {
      closest_t = t;
      if(hit_sphere)
      {
        hit_sphere = nullptr;
      }
      hit_triangle = &triangles[i];
      hit = true;
    }
  }

  if (hit) 
  {
    double epsilon = 1e-4; 
    double reflection[3] = {0, 0, 0};
    double new_origin[3];
    double N[3];

    //set reflection 
    point[0] = origin[0] + closest_t * direction[0];
    point[1] = origin[1] + closest_t * direction[1];
    point[2] = origin[2] + closest_t * direction[2];
    
    if (hit_sphere) 
    {
      //get normal
      double radius = hit_sphere->radius;
      N[0] = (point[0] - hit_sphere->position[0]) / radius;
      N[1] = (point[1] - hit_sphere->position[1]) / radius;
      N[2] = (point[2] - hit_sphere->position[2]) / radius;
      normalize(N);
      calculateSpherePhongShading(point, N, direction, *hit_sphere, lights, num_lights, color);
    } 
    else if (hit_triangle) 
    {
      //calculate triangle normal
      interpolateAttributes(*hit_triangle, alpha, beta, gamma, N, color, color, gamma);
      calculateTrianglePhongShading(point, N, direction, *hit_triangle, lights, num_lights, color);
    }

    // add epsilon to aviud self-reflection error
    new_origin[0] = point[0] + epsilon * N[0];
    new_origin[1] = point[1] + epsilon * N[1];
    new_origin[2] = point[2] + epsilon * N[2];

    // calculate reflection direction: r = 2(l • n) n – l
    // I tested r = 2(l • n) n – l but it's not working so I reversed the direction it works.
    // so here: r = l - 2(l • n) n
    double reflect_dir[3];
    double dot_product = dot(N, direction);
    reflect_dir[0] = direction[0] - 2 * dot_product * N[0];
    reflect_dir[1] = direction[1] - 2 * dot_product * N[1];
    reflect_dir[2] = direction[2] - 2 * dot_product * N[2];

    normalize(reflect_dir);
    //get recursive reflection
    trace_ray(new_origin, reflect_dir, reflection, depth + 1);

    float direct_light = 0.8;
    float reflection_light = 0.8;
    // add weight to make the reflection
    color[0] = direct_light * color[0] + reflection_light * reflection[0];
    color[1] = direct_light * color[1] + reflection_light * reflection[1];
    color[2] = direct_light * color[2] + reflection_light * reflection[2];
  } 
  else 
  {
    //no intersections, use basic ambient light color
    color[0] = ambient_light[0];
    color[1] = ambient_light[1];
    color[2] = ambient_light[2];
  }
}


void calculateSpherePhongShading(double* point, double* N, double* rayDir, Sphere& sph, Light* lights, int numLights, double* finalColor) 
{
  double V[3] = {-rayDir[0], -rayDir[1], -rayDir[2]};
  normalize(V);
  // make ambient light basic
  finalColor[0] = ambient_light[0];
  finalColor[1] = ambient_light[1];
  finalColor[2] = ambient_light[2];

  for (int i = 0; i < numLights; i++)
  {
    //if it's not shadowed
    if (!isShadowed(point, lights[i], INFINITY)) 
    { 
      //make light ray
      double L[3] = 
      {
        lights[i].position[0] - point[0],
        lights[i].position[1] - point[1],
        lights[i].position[2] - point[2]
      };
      normalize(L);
      //L * N
      double NdotL = std::max(dot(N, L), 0.0);
      //calculate reflection R
      double R[3];
      for (int k = 0; k < 3; k++) 
      {
        R[k] = 2 * NdotL * N[k] - L[k];
      }
      normalize(R);
      //R * V
      double RdotV = std:: max(dot(R, V), 0.0);
      for (int k = 0; k < 3; k++) 
      {
        // Diffuse Reflection
        // lights[i].color[k] = lightColor
        // tri.v[0].color_diffuse[k] = kd
        finalColor[k] += lights[i].color[k] * sph.color_diffuse[k] * NdotL;
        // Specular Reflection
        // tri.v[0].color_specular[k] = ks
        // pow(RdotV, shininess) = (R * V) ^ sh 
        finalColor[k] += lights[i].color[k] * sph.color_specular[k] *pow(RdotV, sph.shininess);
      }
    }
  }
  // Clamp to [0, 1]
  for (int k = 0; k < 3; k++) 
  {
    finalColor[k] = std::min(std::max(finalColor[k], 0.0), 1.0);
  }
}

void calculateTrianglePhongShading(double* point, double* N, double* rayDir, Triangle& tri, Light* lights, int numLights, double* finalColor) 
{
  double V[3] = {-rayDir[0], -rayDir[1], -rayDir[2]};
  normalize(V);
  double diffuse[3], specular[3];
  double shininess = tri.v[0].shininess;

  // make ambient light basic
  finalColor[0] = ambient_light[0];
  finalColor[1] = ambient_light[1];
  finalColor[2] = ambient_light[2];

  for (int i = 0; i < numLights; i++)
  {
    //if it's not shadowed
    if (!isShadowed(point, lights[i], INFINITY)) 
    { 
      //make light ray
      double L[3] = 
      {
        lights[i].position[0] - point[0],
        lights[i].position[1] - point[1],
        lights[i].position[2] - point[2]
      };
      normalize(L);
      //L * N
      double NdotL = std::max(dot(N, L), 0.0);
      //calculate reflection R
      double R[3];
      for (int k = 0; k < 3; k++) 
      {
        R[k] = 2 * NdotL * N[k] - L[k];
      }
      normalize(R);
      //R * V
      double RdotV = std:: max(dot(R, V), 0.0);
      for (int k = 0; k < 3; k++) 
      {
        // Diffuse Reflection
        // lights[i].color[k] = lightColor
        // tri.v[0].color_diffuse[k] = kd
        finalColor[k] += lights[i].color[k] * tri.v[0].color_diffuse[k] * NdotL;
        // Specular Reflection
        // tri.v[0].color_specular[k] = ks
        // pow(RdotV, shininess) = (R * V) ^ sh 
        finalColor[k] += lights[i].color[k] * tri.v[0].color_specular[k] * pow(RdotV, shininess);
      }
    }
  }
  // Clamp to [0, 1]
  for (int k = 0; k < 3; k++) 
  {
    finalColor[k] = std::min(std::max(finalColor[k], 0.0), 1.0);
  }
}

//interpolate for smooth color
void interpolateAttributes(Triangle& tri, double alpha, double beta, double gamma, double* N, double* diffuse, double* specular, double& shininess) 
{
  //use Barycentric Coordinates for normal, diffuse, specular and shininess (all vertice's property)
  for (int i = 0; i < 3; i++) 
  {
    N[i] = alpha * tri.v[0].normal[i] + beta * tri.v[1].normal[i] + gamma * tri.v[2].normal[i];
    diffuse[i] = alpha * tri.v[0].color_diffuse[i] + beta * tri.v[1].color_diffuse[i] + gamma * tri.v[2].color_diffuse[i];
    specular[i] = alpha * tri.v[0].color_specular[i] + beta * tri.v[1].color_specular[i] + gamma * tri.v[2].color_specular[i];
  }
  shininess = alpha * tri.v[0].shininess + beta * tri.v[1].shininess + gamma * tri.v[2].shininess;
  normalize(N);
}

//rayPos = ray starting position
//dir = Ray Direction
bool intersectSphere(double* rayPos, double* dir, Sphere& sphere, double& t) 
{
  //as in the slide:
  double a = 1;
  double b = 2 * ((dir[0]*(rayPos[0] - sphere.position[0])) + 
                  (dir[1]*(rayPos[1] - sphere.position[1])) + 
                  (dir[2]*(rayPos[2] - sphere.position[2])));
  double c = (rayPos[0] - sphere.position[0]) * (rayPos[0] - sphere.position[0]) +
            (rayPos[1] - sphere.position[1]) * (rayPos[1] - sphere.position[1]) +
            (rayPos[2] - sphere.position[2]) * (rayPos[2] - sphere.position[2]) -
            sphere.radius * sphere.radius;

  // Calculating the discriminant of the quadratic equation
  double discriminant = b * b - 4 * a * c;
  if (discriminant < 0) 
  {
      return false;
  }

  t = (-b - sqrt(discriminant)) / 2;
  if (t < 0) 
  {
      t = (-b + sqrt(discriminant)) / 2;
  }
  return t >= 0;
}

bool intersectTriangle(double* rayPos, double* dir, Triangle& triangle, double& t) 
{
  double edge1[3], edge2[3], normal[3];
  // get 2 edges
  for (int i = 0; i < 3; ++i) 
  {
    edge1[i] = triangle.v[1].position[i] - triangle.v[0].position[i];
    edge2[i] = triangle.v[2].position[i] - triangle.v[0].position[i];
  }
  // get normal
  crossProduct(edge1, edge2, normal);
  normalize(normal);
  //construct a plane of the triangle
  //plane equation:
  //ax+by+cz+d=0
  double a = normal[0];
  double b = normal[1];
  double c = normal[2];
  double d = - (a * triangle.v[0].position[0] + b * triangle.v[0].position[1] + c * triangle.v[0].position[2]);
  //check for parallel
  double nd = a * dir[0] + b * dir[1] + c * dir[2];
  if (fabs(nd) <= 0) 
  {
    return false;
  }
  // Calculate t
  t = -(a * rayPos[0] + b * rayPos[1] + c * rayPos[2] + d) / nd;
  if (t < 0) 
  {
    // printf("Intersection behind the camera: t = %f\n", t);  
    return false;
  }
  return pointInTriangle(rayPos, dir, triangle, t);
}

bool pointInTriangle(double* rayPos, double* dir, Triangle& triangle, double t) 
{
  double p[3];
  //get the intersection point p
  for (int i = 0; i < 3; i++) 
  {
    p[i] = rayPos[i] + t * dir[i];
  }

  //get triangle vertices p1 p2 p3
  double p1[3] = {triangle.v[0].position[0], triangle.v[0].position[1], triangle.v[0].position[2]};
  double p2[3] = {triangle.v[1].position[0], triangle.v[1].position[1], triangle.v[1].position[2]};
  double p3[3] = {triangle.v[2].position[0], triangle.v[2].position[1], triangle.v[2].position[2]};

  // Check for correct axis
  int constantAxis = -1;
  if (p1[0] == p2[0] && p1[0] == p3[0]) constantAxis = 0;
  if (p1[1] == p2[1] && p1[1] == p3[1]) constantAxis = 1;
  if (p1[2] == p2[2] && p1[2] == p3[2]) constantAxis = 2;

  // printf("%d\n", constantAxis);

  //reference (only knowledge not code)
  //https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/barycentric-coordinates.html
  //according to slide, first compute the Parallelogram formula

  // printf("%f %f %f\n", (p1[0]), (p1[1]), (p1[2]));
  // printf("%f %f %f\n", (p2[0]), (p2[1]), (p2[2]));
  // printf("%f %f %f\n", (p3[0]), (p3[1]), (p3[2]));
  double area = 0;
  switch (constantAxis) 
  {
    case 0:
      area = (p2[2] - p3[2]) * (p1[1] - p3[1]) + (p3[1] - p2[1]) * (p1[2] - p3[2]);
      alpha = ((p2[2] - p3[2]) * (p[1] - p3[1]) + (p3[1] - p2[1]) * (p[1] - p3[2])) / area;
      break;
    case 1:
      area = (p2[0] - p3[0]) * (p1[2] - p3[2]) + (p3[2] - p2[2]) * (p1[0] - p3[0]);
      alpha = ((p2[0] - p3[0]) * (p[0] - p3[2]) + (p3[2] - p2[2]) * (p[0] - p3[0])) / fabs(area);
      break;
    case 2:
      area = (p2[1] - p3[1]) * (p1[0] - p3[0]) + (p3[0] - p2[0]) * (p1[1] - p3[1]);
      alpha = ((p2[1] - p3[1]) * (p[0] - p3[0]) + (p3[0] - p2[0]) * (p[1] - p3[1])) / area;
      break;
    default:
      area = (p2[1] - p3[1]) * (p1[0] - p3[0]) + (p3[0] - p2[0]) * (p1[1] - p3[1]);
      alpha = ((p2[1] - p3[1]) * (p[0] - p3[0]) + (p3[0] - p2[0]) * (p[1] - p3[1])) / area;
  }
  beta = ((p3[1] - p1[1]) * (p[0] - p3[0]) + (p1[0] - p3[0]) * (p[1] - p3[1])) / area;
  gamma = 1.0 - alpha - beta;
  return alpha >= 0 && beta >= 0 && gamma >= 0;
}

//solved noise problem with reference https://www.cs.cornell.edu/courses/cs4620/2012fa/lectures/35raytracing.pdf
bool isShadowed(double* point, Light& light, double closest_t) 
{
  double epsilon = 1e-4;
  double shadowRay[3] = 
  {
    light.position[0] - point[0],
    light.position[1] - point[1],
    light.position[2] - point[2]
  };
  double distToLight = sqrt(dot(shadowRay, shadowRay));
  normalize(shadowRay);

  //solve Shadow rounding errors
  double shadowRayOrigin[3] = 
  {
      point[0] + epsilon * shadowRay[0],
      point[1] + epsilon * shadowRay[1],
      point[2] + epsilon * shadowRay[2]
  };

  //test if the light ray intersects with something
  for (int i = 0; i < num_spheres; i++) 
  {
    double t = 0;
    if (intersectSphere(shadowRayOrigin, shadowRay, spheres[i], t) && t > epsilon && t < distToLight) 
    {
      return true;
    }
  }
  for (int i = 0; i < num_triangles; i++) 
  {
    double t = 0;
    if (intersectTriangle(shadowRayOrigin, shadowRay, triangles[i], t) && t > 0 && t < distToLight) {
      return true;
    }
  }
  return false;
}

void crossProduct(const double* v1, const double* v2, double* cross) 
{
    cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
    cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
    cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

double dot(const double* a, const double* b) 
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  glColor3f(((float)r) / 255.0f, ((float)g) / 255.0f, ((float)b) / 255.0f);
  glVertex2i(x,y);
}

void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  buffer[y][x][0] = r;
  buffer[y][x][1] = g;
  buffer[y][x][2] = b;
}

void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  plot_pixel_display(x,y,r,g,b);
  if(mode == MODE_JPEG)
    plot_pixel_jpeg(x,y,r,g,b);
}

void save_jpg()
{
  printf("Saving JPEG file: %s\n", filename);
  ImageIO img(WIDTH, HEIGHT, 3, &buffer[0][0][0]);
  if (img.save(filename, ImageIO::FORMAT_JPEG) != ImageIO::OK)
    printf("Error in saving\n");
  else 
    printf("File saved successfully\n");
}

void parse_check(const char *expected, char *found)
{
  if(strcasecmp(expected,found))
  {
    printf("Expected '%s ' found '%s '\n", expected, found);
    printf("Parsing error; abnormal program abortion.\n");
    exit(0);
  }
}

void parse_doubles(FILE* file, const char *check, double p[3])
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check(check,str);
  fscanf(file,"%lf %lf %lf",&p[0],&p[1],&p[2]);
  printf("%s %lf %lf %lf\n",check,p[0],p[1],p[2]);
}

void parse_rad(FILE *file, double *r)
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check("rad:",str);
  fscanf(file,"%lf",r);
  printf("rad: %f\n",*r);
}

void parse_shi(FILE *file, double *shi)
{
  char s[100];
  fscanf(file,"%s",s);
  parse_check("shi:",s);
  fscanf(file,"%lf",shi);
  printf("shi: %f\n",*shi);
}

int loadScene(char *argv)
{
  FILE * file = fopen(argv,"r");
  if (!file)
  {
    printf("Unable to open input file %s. Program exiting.\n", argv);
    exit(0);
  }

  int number_of_objects;
  char type[50];
  Triangle t;
  Sphere s;
  Light l;
  fscanf(file,"%i", &number_of_objects);

  printf("number of objects: %i\n",number_of_objects);

  parse_doubles(file,"amb:",ambient_light);

  for(int i=0; i<number_of_objects; i++)
  {
    fscanf(file,"%s\n",type);
    printf("%s\n",type);
    if(strcasecmp(type,"triangle")==0)
    {
      printf("found triangle\n");
      for(int j=0;j < 3;j++)
      {
        parse_doubles(file,"pos:",t.v[j].position);
        parse_doubles(file,"nor:",t.v[j].normal);
        parse_doubles(file,"dif:",t.v[j].color_diffuse);
        parse_doubles(file,"spe:",t.v[j].color_specular);
        parse_shi(file,&t.v[j].shininess);
      }

      if(num_triangles == MAX_TRIANGLES)
      {
        printf("too many triangles, you should increase MAX_TRIANGLES!\n");
        exit(0);
      }
      triangles[num_triangles++] = t;
    }
    else if(strcasecmp(type,"sphere")==0)
    {
      printf("found sphere\n");

      parse_doubles(file,"pos:",s.position);
      parse_rad(file,&s.radius);
      parse_doubles(file,"dif:",s.color_diffuse);
      parse_doubles(file,"spe:",s.color_specular);
      parse_shi(file,&s.shininess);

      if(num_spheres == MAX_SPHERES)
      {
        printf("too many spheres, you should increase MAX_SPHERES!\n");
        exit(0);
      }
      spheres[num_spheres++] = s;
    }
    else if(strcasecmp(type,"light")==0)
    {
      printf("found light\n");
      parse_doubles(file,"pos:",l.position);
      parse_doubles(file,"col:",l.color);

      if(num_lights == MAX_LIGHTS)
      {
        printf("too many lights, you should increase MAX_LIGHTS!\n");
        exit(0);
      }
      lights[num_lights++] = l;
    }
    else
    {
      printf("unknown type in scene description:\n%s\n",type);
      exit(0);
    }
  }
  return 0;
}

void keyboard(unsigned char key, int x, int y) 
{
  switch(key) 
  {
    case 'k':
      save_jpg();
      break;
    case 27: // ESC key
      exit(0); // exit the program
    break;
    default:
      break;
  }
}

void display()
{
}

void init()
{
  glMatrixMode(GL_PROJECTION);
  glOrtho(0,WIDTH,0,HEIGHT,1,-1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT);
}

void idle()
{
  // Hack to make it only draw once.
  static int once=0;
  if(!once)
  {
    if(isRe == 0)
    {
      draw_scene();
    }
    if(isRe == 1)
    {
      draw_scene_recursive();
    }
    // if(mode == MODE_JPEG)
    //   save_jpg();
  }
  once=1;
}

void normalize(double* v) 
{
    float len = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    v[0] /= len;
    v[1] /= len;
    v[2] /= len;
}

void set_vector(float* v, float x, float y, float z) 
{
    v[0] = x;
    v[1] = y;
    v[2] = z;
}

int main(int argc, char ** argv)
{
  if ((argc < 2) || (argc > 4))
  {  
    printf ("Usage: %s <input scenefile> [output jpegname]\n", argv[0]);
    exit(0);
  }
  if(argc == 3)
  {
    mode = MODE_JPEG;
    filename = argv[2];
  }
  else if(argc == 2)
    mode = MODE_DISPLAY;
  else if(argc == 4)
  {
    mode = MODE_JPEG;
    filename = argv[2];
    isRe = std::stoi(argv[3]);
  }

  glutInit(&argc,argv);
  loadScene(argv[1]);

  glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
  glutInitWindowPosition(0,0);
  glutInitWindowSize(WIDTH,HEIGHT);
  int window = glutCreateWindow("Ray Tracer");
  #ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(WIDTH - 1, HEIGHT - 1);
  #endif
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  glutKeyboardFunc(keyboard);
  init();
  glutMainLoop();
}
