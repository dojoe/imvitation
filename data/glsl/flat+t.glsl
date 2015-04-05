#version 330
precision mediump float;

uniform mat4 u_matrix;

layout(location=0) in  vec3 inPosition;
layout(location=1) in  vec3 inNormal;
layout(location=2) in  vec2 inTexCoord;
layout(location=3) in  vec4 inColor;

out vec4 xTexCoord;

void main()
{
	xTexCoord = vec4(inTexCoord, 0.0, 1.0);
	gl_Position = u_matrix * vec4(inPosition, 1.0);
}

//__FRAGMENT_SHADER__

#version 330
precision mediump float;

uniform sampler2D tex;
uniform float u_alpha = 1.0;

in vec4 xTexCoord;
out vec4 outputColor;

void main()
{
	vec4 texColor = texture2D(tex, xTexCoord.xy);
	outputColor = vec4(texColor.rgb, texColor.a * u_alpha);
}
