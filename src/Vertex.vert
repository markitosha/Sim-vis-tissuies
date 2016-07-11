#version 330

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 objectMatrix;

uniform vec3 g_sunDir;

in vec4 vertex;
in vec3 normal;
in vec2 texCoord;

in vec3 eyePos;

out vec3 fragmentWorldPos;
out vec3 fragmentNormal;
out vec2 fragmentTexCoord;

out vec3 fragmentSunView;
out vec3 fragmentNormalView;
out vec3 fragmentEyeView;

void main(void)
{
  vec4 worldPos    = objectMatrix*vertex;
  vec4 viewPos     = modelViewMatrix*worldPos;
  
  fragmentWorldPos = worldPos.xyz;
  fragmentNormal   = normalize(mat3(objectMatrix)*normal);
  fragmentTexCoord = texCoord;

  fragmentSunView    = g_sunDir - normalize(vec3(viewPos));
  fragmentNormalView = fragmentNormal;
  fragmentEyeView = eyePos - normalize(vec3(viewPos));

  gl_Position        = projectionMatrix*viewPos;
}