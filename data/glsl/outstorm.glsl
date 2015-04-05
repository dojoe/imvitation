#version 330
precision mediump float;

layout(location=0) in  vec3 inPosition;

void main()
{
	gl_Position = vec4(inPosition, 1.0);
}

//__FRAGMENT_SHADER__

#version 330
precision mediump float;

out vec4 outputColor;

uniform float time;
uniform vec2 resolution;

uniform float layers = 2.0;
uniform float colorDetail = 8.;
uniform float variance = 1.;
uniform float colorFactor = 0.5;
uniform float blockSize = 8.0;
uniform float origin = 0.;
uniform float flatness = 1.0;
uniform float alpha = 1.;

vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}



void main( void ) {

	float pixel = 0.;
	for(int layer=0; layer<int(ceil(layers)); layer++)
	{
		float ratio = resolution.x/resolution.y;
		vec2 p = floor(gl_FragCoord.xy/blockSize)*blockSize / resolution.xy;
		p-=vec2(.5);
		p.x+=origin;
		p.x*=ratio*flatness*flatness;
		
		float f = ((layer == 0) ? .1 : -1.);
	
		float r = time*.4*f*(origin-.5);
		p = vec2(cos(r)*p.x + sin(r)*p.y, cos(r)*p.y + -sin(r)*p.x);
		
		float freq = 30.+5.*cos(time*.015);
		
		float a = p.x*sin(time*.5);
		float b = p.y*cos(time*.5);
		float d = freq*(time*.1+mix(abs(a), a, 2.*origin)-abs(b));
		
		d += variance * (2.*sin(d)+.5*sin(d)+.5*sin(3.*d));
		
		float strips = 2. + colorDetail;
		d = floor(mod(d*strips*.2, strips))*1./strips;
		
		float mixer = sqrt(.5+0.5*sin(time*0.5));
		if(layer == 1) mixer = 1. - mixer;
		pixel += d*.5*mixer*max(0.0f, 5.0 * min(1.0, layers - float(layer)) - 4.0);
	}
	float a = (alpha - pixel)*colorDetail*.5;
	outputColor = vec4(hsv2rgb(vec3(pixel*2., colorFactor, mix(pixel, 1., colorFactor)))*alpha, a);
}
