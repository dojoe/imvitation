#version 330
precision mediump float;

uniform mat4 u_modelview;
uniform mat4 u_projection;
uniform float u_alpha;

layout(location=0) in  vec3 inPosition;
layout(location=1) in  vec3 inNormal;
layout(location=3) in  vec4 inColor;

out vec4 xColor;
out vec3 xVertex;
out vec3 xNormal;

void main()
{
	xColor = vec4(inColor.rgb, inColor.a * u_alpha);
	vec4 p = vec4(inPosition, 1.0);
	xVertex = (u_modelview * p).xyz;
	xNormal = normalize((u_modelview * vec4(inNormal, 0.0)).xyz);
	gl_Position = u_projection * u_modelview * p;
}

//__FRAGMENT_SHADER__

#version 330
precision mediump float;

uniform vec3 u_lightpos;

in vec4 xColor;
in vec3 xVertex;
in vec3 xNormal;
out vec4 outputColor;

void main()
{
	vec3 light = normalize(u_lightpos - xVertex);
	outputColor = vec4(xColor.rgb * max(dot(xNormal, light), 0.0), xColor.a);
}
