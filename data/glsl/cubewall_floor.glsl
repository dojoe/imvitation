#version 330
precision mediump float;

uniform mat4 u_matrix;

layout(location=0) in  vec3 inPosition;

out vec3 vertexPos;

void main()
{
	gl_Position = u_matrix * vec4(inPosition, 1.0f);
	vertexPos = inPosition;
}

//__FRAGMENT_SHADER__

#version 330
precision mediump float;

uniform vec3 u_eye;
uniform float u_fade;


in vec3 vertexPos;
out vec4 outputColor;

void main()
{
	vec3 lightPos = vec3(2.0, 5.0, 5.0);

	vec4 diffCol = vec4(0.9, 0.2, 0.1, 0.0)*.5;
	vec4 specCol = vec4(0.9, 0.7, 0.5, 0.0)*.5;


	vec3 dir = normalize(vertexPos - lightPos);
	vec3 normal = vec3(0., 1., 0.);

	vec3 hw = normalize(dir + normalize(vertexPos - u_eye));

	float diffuse  = clamp(dot(-dir, normal), 0., 1.);
	float specular = clamp(dot(-hw, normal), 0., 1.);

	outputColor = vec4((diffuse*diffCol + specCol*(pow(specular, 4.))).rgb, 0.5*u_fade);
}
