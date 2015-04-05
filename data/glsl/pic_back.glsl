#version 330
precision mediump float;

layout(location=0) in  vec3 inPosition;
layout(location=2) in  vec2 iTexCoord;

out vec2 xTexCoord;

void main()
{
	xTexCoord = iTexCoord;
	gl_Position = vec4(inPosition, 1.0f);
}

//__FRAGMENT_SHADER__

#version 330
precision mediump float;

uniform vec2  u_pixel;
uniform vec4  u_color1, u_color2;
uniform float u_scale;
in vec2 xTexCoord;
out vec4 outputColor;

void main()
{
	outputColor = mix(u_color1, u_color2, distance(xTexCoord, vec2(0.4, 0.7)));
}
