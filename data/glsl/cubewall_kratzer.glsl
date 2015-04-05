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
uniform sampler2D src;

uniform float u_fade;

in vec2 xTexCoord;
out vec4 outputColor;

void main()
{
	vec4 colSrc = texture2D(src, vec2(xTexCoord.x, 1.-xTexCoord.y));

	//outputColor = colSrc; return;

	vec4 col    = texture2D(tex, xTexCoord);

	float aspect = 1./1.6;
	float steps = 5.;
	float range = 0.03;
	vec3 colBloom = vec3(0.);
/*	for(float y = -range; y < range; y += range/steps)
	{
		for(float x = -range*aspect; x < range*aspect; x += range/steps*aspect)
		{
			colBloom += texture2D(src, vec2(xTexCoord.x + x, 1.-(xTexCoord.y + y))).rgb;
		}
	}*/
	float steps2 = 4.*steps*steps;
	for(float r = 0.; r < range; r += range/steps2)
	{
		float a = r*steps2;
		float d = sqrt(r/range)*range;
		float x = sin(a)*d*aspect;
		float y = cos(a)*d;
		colBloom += texture2D(src, vec2(xTexCoord.x + x, 1.-(xTexCoord.y + y))).rgb;
	}

	colBloom -= 2.*steps*steps;
	colBloom *= 3.;
	colBloom = clamp(colBloom, vec3(0.), vec3(1.));
	colBloom = colBloom*.3 + vec3(max(colBloom.g, colBloom.b)*.7);

	float bloom = col.a;
	float stepsA = 5.;
	float rangeA = 0.002;
	for(float yA = -rangeA; yA < rangeA; yA += rangeA/stepsA)
	{
		for(float xA = -rangeA*aspect; xA < rangeA*aspect; xA += rangeA/stepsA*aspect)
		{
			bloom += texture2D(tex, vec2(xTexCoord.x + xA, xTexCoord.y + yA*.7)).a;
		}
	}
	bloom = clamp(bloom/(0.4*stepsA*stepsA), 0., 1.);
	bloom = bloom*bloom*bloom;

 	outputColor = vec4(mix(colSrc.rgb, col.rgb*0.1, min(u_fade, col.a*0.7))+colBloom*bloom*u_fade, 1.);
}
