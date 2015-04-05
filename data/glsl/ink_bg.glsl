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

uniform float u_scale;
uniform float u_offset;
uniform mat2 u_papermat;
uniform float u_alpha;

in vec2 xTexCoord;
out vec4 outputColor;

uniform vec4 u_color;

float hash( float n ) { return fract(sin(n)*43758.5453123); }

float noise( in vec2 x )
{
    vec2 p = floor(x);
    vec2 f = fract(x);
    f = f*f*(3.0-2.0*f);
	
    float n = p.x + p.y*157.0;
    return mix(mix( hash(n+  0.0), hash(n+  1.0),f.x),
               mix( hash(n+157.0), hash(n+158.0),f.x),f.y);
}

float paper(in vec2 v)
{
	return noise(vec2(v.x / 23.0 + hash(v.y), v.y)) * noise(vec2(v.x / 7.76 + hash(v.y), v.y));
}

void main()
{
	vec2 fc = u_papermat * gl_FragCoord.xy;
	outputColor = vec4(u_color.rgb * clamp(u_offset + u_scale * paper(fc), 0.0, 1.0), u_alpha);
}
