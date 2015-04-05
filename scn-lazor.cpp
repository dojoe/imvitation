#include "k.i.t.t.h"
#include "scenes.h"

GlslShader* lazorShader;
GlslShader* burnShader;
GlslShader* flatShader;
Texture* beam;
Texture* burn;
Texture* spot;
Texture* noise;

static float beamSize = 0.05f;
static vec4 burnColor = vec4(24, 17, 12, 218) / 255.f;
static vec2 burnIntens(.24f, .3f);

static float shardt0 = 10.425f;
static float shardRot = 50.f;
static float shardPow = 2.f;
static float shardSep = 1.0f;
static float shardDelay[] = { 0.38f, 5.56f, 5.07f, 5.2f, 0.f };
static float redSpeed = 1.f;

static vec2 ac;
static float fade;
static bool red;

void initShards();

LazorScene::LazorScene(float aStartTime, float aEndTime) : Scene("laz0r", aStartTime, aEndTime)
{
	initShards();

	lazorShader = new GlslShader("data/glsl/lazor.glsl");
	flatShader = new GlslShader("data/glsl/flat+t.glsl");
	burnShader = new GlslShader("data/glsl/lazor_burn.glsl");

	beam = new Texture("data/lazor/beam.png", SOIL_LOAD_AUTO, 0);
	burn = new Texture("data/lazor/burn.png");
	spot = new Texture("data/lazor/spot.png", SOIL_LOAD_AUTO, 0);
	noise = new Texture("data/lazor/noise.png", SOIL_LOAD_AUTO, 0);

	TWEAK_INIT();
	TWEAKA(beamSize, "step=0.005");
	TWEAK(burnColor);
	TWEAK(burnIntens.x);
	TWEAK(burnIntens.y);
	TWEAKA(redSpeed, "max=5");
	TWEAKA(shardt0, "min=0 max=30");
	TWEAKA(shardRot, "min=-180 max=180");
	TWEAKA(shardPow, "max=4");
	TWEAKA(shardSep, "max=2");
	TWEAKA(shardDelay[0], "max=8");
	TWEAKA(shardDelay[1], "max=8");
	TWEAKA(shardDelay[2], "max=8");
	TWEAKA(shardDelay[3], "max=8");
	TWEAKA(shardDelay[4], "max=8");
}

const vec2 lazorPositions[] = { vec2(-0.075f, -0.25f), vec2(-0.025f, -0.5f), vec2(-0.2f, -0.1f) };
const vec2 lazorPaths[3][2] = { 
	{ vec2(-1.1,-.7), vec2( -.5, .8) },
	{ vec2(  .5,-.8), vec2( -.5, .8) },
	{ vec2(  .5,-.8), vec2( 1.1, .7) }
};

static inline vec2 liY(int nPath, int nIntersect, int yOfs)
{
	const vec2 *paths = lazorPaths[nPath];
	vec2 d = paths[1] - paths[0];
	float p = ((1.f + nIntersect * 2.f) - paths[0].y) / d.y;
	vec2 ret = paths[0] + p  * d;
	ret.y = fmod(ret.y + 1.f, 2.f) - 1.f + yOfs * 2.f;
	return ret;
}

static inline vec2 liX(int nPath, int nIntersect)
{
	const vec2 *paths = lazorPaths[nPath];
	vec2 d = paths[1] - paths[0];
	float p = ((demo.aspect + nIntersect * 2.f * demo.aspect) - paths[0].x) / d.x;
	vec2 ret = paths[0] + p  * d;
	ret.y = fmod(ret.y + 1.f, 2.f) - 1.0f;
	return ret;
}

static vec2 shards[100];

void initShards()
{
	const vec2 init[] = {
		vec2(-demo.aspect, -1), liY(1, 0, 0), liX(1, -1),
		liY(1, 0, 0), liY(0, 0, 0), liY(0, 1, 1), vec2(-demo.aspect, 1), liX(1, -1),
		liY(0, 0, 0), liY(0, 1, 0), liY(0, 2, 1), liY(0, 1, 1),
		liY(0, 1, 0), liY(0, 2, 0), liX(0, 0), vec2(demo.aspect, 1), liY(0, 2, 1),
		liY(0, 2, 0), vec2(demo.aspect, -1), liX(0, 0),
	};
	memcpy(shards, init, sizeof(init));
}

const int shardRanges[][2] = {
	{ 0, 3 }, { 3, 5 }, { 8, 4 }, { 12, 5 }, { 17, 3 }
};

const vec2 shardCenters[] = {
	vec2(.1f, .1f), vec2(.3f, .5f), vec2(.5f, .5f), vec2(.7f, .5f), vec2(.9f, .1f)
};

void drawShard(int n)
{
	vec3 verts[10];
	vec2 uvs[10];
	const vec2 *start = shards + shardRanges[n][0];
	int nverts = shardRanges[n][1];

	for (int i = 0; i < nverts; i++) {
		verts[i] = vec3(start[i], 0);
		uvs[i] = vec2(start[i].x / (2 * demo.aspect) + .5f, start[i].y / 2.f + .5f);
	}

	drawArray(GL_TRIANGLE_FAN, value_ptr(*verts), NULL, value_ptr(*uvs), NULL, NULL, nverts);
}

void drawBeam(vec2 from, vec2 to, vec2 intensity, vec4 color)
{
	lazorShader->bindTexture("tex", beam, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	vec2 beamVec = to - from;
	vec2 beamDir = normalize(beamVec)*beamSize;
	vec2 beamDir2 = vec2(-beamDir.y, beamDir.x);

	float beamLen = length(beamVec);

	lazorShader->uniform("color", color);
	lazorShader->uniform("intensity", intensity);

	drawParallelogram((from - beamDir2 - beamDir)*ac, (from + beamDir2 - beamDir)*ac, (from - beamDir2 + beamVec)*ac, vec2(0, 0), vec2(1, .5 + .5*beamLen / beamSize));
}

void drawBurn(vec2 from, vec2 to, vec2 intensity, vec4 color)
{
	burnShader->bindTexture("tex", burn, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	vec2 beamVec = to - from;
	vec2 beamDir = normalize(beamVec)*beamSize;
	vec2 beamDir2 = vec2(-beamDir.y, beamDir.x);

	burnShader->uniform("color", color);
	burnShader->uniform("intensity", intensity);

	float beamLen = length(beamVec);

	drawParallelogram((from + beamDir2 - beamDir)*ac, (from - beamDir2 - beamDir)*ac, (to + beamDir2 + beamDir)*ac, vec2(0, (.5 + .5*beamLen / beamSize) / 5), vec2(1, 0));
}

void drawBurnMarks(const vec2 path[2], float p)
{
	vec2 p0 = path[0];
	vec2 d = path[1] - path[0];

	for (int i = 1; i < 5; i++) {
		float pw0 = ((-1.f + i * 2.f) - p0.y) / d.y;
		float pw1 = ((1.f + i * 2.f) - p0.y) / d.y;
		if (p < pw0)
			break;

		vec2 v0 = p0 + pw0 * d * 0.9f - float(i) * vec2(0, 2);
		vec2 v1 = p0 + min(pw1 * 1.1f, p) * d - float(i) * vec2(0, 2);
		drawBurn(v0, v1, vec2(2.f, 0.5f), vec4(250.f, 6.f, 50.f, 255.f) / 128.0f);
	}
}

void drawSpot(vec2 targetPos, float size, vec2 intensity, vec4 color)
{
	float r = 0.1f*targetPos.x - targetPos.y;
	vec2 dir = vec2(sin(r), cos(r))*size;
	vec2 dir2(-dir.y, dir.x);

	lazorShader->uniform("color", color);
	lazorShader->uniform("intensity", intensity);

	lazorShader->bindTexture("tex", spot, 0);
	drawParallelogram((targetPos - dir - dir2)*ac, (targetPos + dir - dir2)*ac, (targetPos - dir + dir2)*ac, vec2(0, 1), vec2(1, 0));
}

void LazorScene::_draw(float time)
{
	fade = min(1.f, time*.3f);
		
	time -= 3.f;

	setCullMode(NO_CULL);
	setDepthMode(NO_DEPTH);

	setBlendMode(BLEND_ADD_ALPHA);

	gFullScreenFbo->bind();

	float c = max(0.f, min(1.f, -15.f*.5f + time*.5f));
	glClearColor(c, c, c, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	lazorShader->bind();

	lazorShader->bindTexture("noise", noise, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	lazorShader->uniform("time", time);

	ac = vec2(demo.inv_aspect, 1.);
	float p = time*.3f;
	float pLogo = min(p, 1.f);
	red = pLogo < p;

	for (int lazorIdx = 0; lazorIdx < 3; lazorIdx++)
	{
		auto lazorPath = lazorPaths[lazorIdx];

		float t = time*0.5f + (float)lazorIdx;

		vec2 targetPos0 = lazorPath[0];
		vec2 targetPos = mix(targetPos0, lazorPath[1], pLogo);

		drawBeam(targetPos0, targetPos, vec2(0.5f, red ? 8. : 1.f),
			vec4(200.f, 255.f, 240.f, 255.f)*fade*fade*fade*fade / 255.f);

		for (int mirror = 0; mirror < 2; mirror++)
		{
			lazorShader->bindTexture("tex", beam, 0);

			vec2 lazorPos = lazorPositions[lazorIdx];
			vec2 targetPos0 = lazorPath[0];
			vec2 targetPos1 = lazorPath[1];
			vec2 targetPos  = mix(targetPos0, targetPos1, p);

			while (targetPos.y > 1.f) targetPos.y -= 2.f;
			if (mirror)
			{
				targetPos.y = min(targetPos0.y, targetPos1.y);
			}

			drawBeam(lazorPos, targetPos, vec2(2.f, 0.5f),
				(red ? vec4(250.f, 6.f, 50.f, mirror ? 32.f : 255.f) : vec4(50.f, 128.f, 240.f, mirror ? 32.f : 255.f))*fade / 128.f);

			drawSpot(targetPos, (mirror ? 0.02f : 0.1f)*(red ? 0.5f : 1.f) + (red ? 0.02f : 0.002f)*(sin(t*310.f) + sin(t*480.f)),
				vec2(1.f, 1.f),
				(red ? vec4(250.f, 6.f, 50.f, 255.f) : vec4(200.f, 255.f, 240.f, mirror ? 64.f : 255.f))*fade / 128.f
				);
		}
		if (red)
		{
			drawSpot(lazorPath[1], 0.1f + 0.002f*(sin(t*310.f) + sin(t*480.f)), vec2(1.f, 1.f), vec4(200.f, 255.f, 240.f, 255.f)*fade / 128.f);
		}
	}

	lazorShader->unbind();

	setBlendMode(BLEND_ADD_AND_DIM);
	burnShader->bind();
	burnShader->bindTexture("noise", noise, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	burnShader->uniform("u_alpha", 1.0f);
	burnShader->uniform("u_matrix", mat4(1));
	for (int lazorIdx = 0; lazorIdx < 3; lazorIdx++)
	{
		auto lazorPath = lazorPaths[lazorIdx];
		drawBurnMarks(lazorPath, p);
	}
	burnShader->unbind();
	gFullScreenFbo->unbind();

	setBlendMode(BLEND_ALPHA);

	flatShader->bind();
	mat4 mat(1.);
	flatShader->uniform("u_alpha", fade);
	flatShader->bindFbo("tex", gFullScreenFbo, 0);

	skrand(1234);
	for (int i = 0; i < 5; i++) {
		vec3 center(shardCenters[i] * 2.f - vec2(1, 1), 0);
		float t = max(0.f, time + 3.f - shardt0 - (shardSep * shardDelay[i]));
		mat4 m = perspective(35.0f, demo.aspect, .1f, 10.0f);
		m = translate(m, vec3(0, -pow(t, shardPow), -1 / tan(35.0f / 360 * PI)));
		m = translate(m, center);
		m = rotate(m, t * shardRot, vec3(randsign() * rand(3.f, 12.f), rand(-6.f, 6.f), randsign()));
		m = translate(m, -center);
		flatShader->uniform("u_matrix", m);
		drawShard(i);
	}

	flatShader->unbind();
}
