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

in vec2 xTexCoord;
out vec4 outputColor;

uniform sampler2D tex;
uniform sampler2D noise;
uniform vec2 intensity;
uniform vec4 color;

void main()
{
	float r = texture2D(noise, gl_FragCoord.xy / 128.0);
	vec4 col = texture2D(tex, xTexCoord);
	col.rgb = pow(col.rgb*(0.75*intensity.y*sqrt(xTexCoord.y)+0.5*r), vec3(intensity.x));
	outputColor = col*color;
}
