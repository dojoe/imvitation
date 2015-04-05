#version 330
precision mediump float;

uniform float u_alpha = 1.0;
uniform mat4 u_matrix;

layout(location=0) in  vec3 inPosition;
layout(location=3) in  vec4 inColor;

out vec4 xColor;

void main()
{
	xColor = vec4(inColor.rgb, inColor.a * u_alpha);
	gl_Position = u_matrix * vec4(inPosition, 1.0f);
}

//__FRAGMENT_SHADER__

#version 330
precision mediump float;

in vec4 xColor;
out vec4 outputColor;

void main()
{
	outputColor = xColor;
}
