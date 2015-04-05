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

uniform sampler2D fBlot;
uniform sampler2D tOverlay;
uniform float u_paperscale;
uniform float u_blotscale;
uniform float u_ovlscale;
uniform float u_black;
uniform float u_white;
uniform mat2 u_papermat;
uniform mat2 u_ovlmat;

in vec2 xTexCoord;
out vec4 outputColor;

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
	float level = u_paperscale * paper(fc) 
				+ u_blotscale  * texture2D(fBlot, vec2(1.99 * (0.5 - abs(xTexCoord.x - 0.5)), xTexCoord.y)).r
				+ u_ovlscale   * texture2D(tOverlay, vec2(0.5) + u_ovlmat * (xTexCoord.xy - vec2(0.5))).r;
	level = smoothstep(u_black, u_white, level);
	outputColor = vec4(0.0f, 0.0f, 0.0f, level);
}
