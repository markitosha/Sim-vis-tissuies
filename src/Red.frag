#version 330

uniform int Blinn;

uniform sampler2D diffuseTexture;
in vec2 fragmentTexCoord;
out vec4 fragColor;

in vec3 fragmentSunView;
in vec3 fragmentNormalView;
in vec3 fragmentEyeView;

void main(void)
{	
  vec4 diffColor = vec4(0.21875, 0.101562, 0, 0);
  if (Blinn == 0) {
   vec3 l2   = normalize(fragmentSunView);
   vec3 n2   = normalize(fragmentNormalView);
   n2 = vec3(n2.x, n2.z, n2.y);
   float koef = abs(dot(n2, l2));
   vec4 diff = diffColor * koef;
   fragColor = diff;
  }
  if (Blinn > 0){
	vec4 specColor = vec4(0.4, 0.4, 0.2, 1.0);
    float specPower = 15.5;
    vec3 l2   = normalize(fragmentSunView);
    vec3 n2   = normalize(fragmentNormalView);
	n2 = vec3(n2.x, n2.z, n2.y);
    vec3 h2 = normalize((fragmentSunView + fragmentEyeView) / length(fragmentSunView + fragmentEyeView));
	vec4 diff = diffColor * abs(dot(n2, l2));
	vec4 spec = specColor * pow(abs(dot(l2, h2)), specPower);
    fragColor = diff + spec;
  }
}


