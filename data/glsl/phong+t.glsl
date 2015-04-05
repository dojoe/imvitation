#version 330
precision mediump float;

uniform mat4 u_modelview;
uniform mat4 u_projection;

layout(location=0) in  vec3 inPosition;
layout(location=1) in  vec3 inNormal;
layout(location=2) in  vec2 inTexCoord;

out vec4 xTexCoord;
out vec3 xVertex;
out vec3 xNormal;

void main()
{
	vec4 p = vec4(inPosition, 1.0);
	xVertex = (u_modelview * p).xyz;
	xNormal = normalize((u_modelview * vec4(inNormal, 0.0)).xyz);
	xTexCoord = vec4(inTexCoord, 0.0f, 1.0f);
	gl_Position = u_projection * u_modelview * p;
}

//__FRAGMENT_SHADER__

#version 330
precision mediump float;

uniform sampler2D tex;

uniform vec3 u_lightpos;
uniform float u_alpha;

in vec4 xTexCoord;
in vec3 xVertex;
in vec3 xNormal;
out vec4 outputColor;

void main()
{
	vec3 light = normalize(u_lightpos - xVertex);
	vec4 texColor = texture2D(tex, xTexCoord.xy);
	outputColor = vec4(texColor.rgb * pow(max(dot(xNormal, light)*1.1, 0.3), 1.5), texColor.a * u_alpha);
}
