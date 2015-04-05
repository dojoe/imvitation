#version 330
precision mediump float;

uniform mat4 u_matrix;

layout(location=0) in  vec3 inPosition;

void main()
{
	gl_Position = u_matrix * vec4(inPosition, 1.0f);
}

//__FRAGMENT_SHADER__

#version 330
precision mediump float;

uniform vec4 u_color;
out vec4 outputColor;

void main()
{
	outputColor = u_color;
}
