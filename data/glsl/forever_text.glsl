#version 330
precision mediump float;

uniform mat4 u_matrix;

layout(location=0) in  vec3 inPosition;
layout(location=2) in  vec2 iTexCoord;

out vec2 xTexCoord;

void main()
{
	xTexCoord = iTexCoord;
	gl_Position = u_matrix * vec4(inPosition, 1.0f);
}

//__FRAGMENT_SHADER__

#version 330
precision mediump float;

uniform sampler2D tex;
uniform vec2 u_texOffset;
in vec2 xTexCoord;
out vec4 outputColor;

void main()
{
	outputColor = vec4(texture2D(tex, xTexCoord + u_texOffset).rgb, 1.0 - texture2D(tex, xTexCoord).r);
}
