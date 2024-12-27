#version 150

in vec3 position;
in vec3 position_left;
in vec3 position_right;
in vec3 position_up;
in vec3 position_down;
in vec4 color;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform float scale = 1.0f;
uniform float exponent = 1.0f;
uniform int mode;

out vec4 col;

void main()
{
    //compute the transformed and projected vertex position (into gl_Position)
    //compute the vertex color (into col)
    vec3 finalPosition = position;
    if (mode == 1) 
    {
        finalPosition = (position + position_left + position_right + position_up + position_down) / 5.0f;
        col = pow(finalPosition.y, exponent) * color / color;
        finalPosition.y = scale * pow(finalPosition.y, exponent);
        finalPosition.y *= 40.0f;
    }
    else
    {
        col = color;
    }
    gl_Position = projectionMatrix * modelViewMatrix * vec4(finalPosition, 1.0f);
}
