#version 150

in vec3 viewPosition;
in vec3 viewNormal;
in vec4 col;

out vec4 c;

uniform vec4 La;
uniform vec4 Ld;
uniform vec4 Ls;
uniform vec3 viewLightDirection;

uniform vec4 ka;
uniform vec4 kd;
uniform vec4 ks;
uniform float alpha;

void main()
{
  vec3 normalizedviewNormal = normalize(viewNormal);

  vec3 eyeDir = normalize(vec3(0.0, 0.0, 0.0) - viewPosition);

  vec3 reflectDir = -reflect(viewLightDirection, normalizedviewNormal);

  float d = max(dot(viewLightDirection, normalizedviewNormal), 0.0);
  float s = max(dot(reflectDir, eyeDir), 0.0);

  c = ka * La + d * kd * Ld + pow(s, alpha) * ks * Ls;

  // c = vec4(normalizedviewNormal, 1.0f);
  // c = col;
}
