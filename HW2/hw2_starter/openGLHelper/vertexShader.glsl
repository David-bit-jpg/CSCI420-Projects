#version 150

in vec3 position; 
in vec3 normal;
in vec4 color;

out vec3 viewPosition;
out vec3 viewNormal;
out vec4 col;

uniform mat4 modelViewMatrix;
uniform mat4 normalMatrix;
uniform mat4 projectionMatrix;


void main()
{
    col = color;
    // gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
    vec4 viewPosition4 = modelViewMatrix * vec4(position, 1.0);
    viewPosition = viewPosition4.xyz;
    gl_Position = projectionMatrix * viewPosition4; 
    viewNormal = normalize((normalMatrix * vec4(normal, 0.0)).xyz);
}
