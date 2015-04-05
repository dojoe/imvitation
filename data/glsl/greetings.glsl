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

uniform sampler2D tex;
uniform float fade;

in vec2 xTexCoord;
out vec4 outputColor;

void main()
{
	vec4 col = texture2D(tex, xTexCoord);

	outputColor = fade*col;
}
