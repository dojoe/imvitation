#include "k.i.t.t.h"
#include "scenes.h"

ObjLoader *oRocket;
Texture *tRocket, *tEngine, *tFire;
GlslShader *sRocket, *sFire;

static float gX = 23.345f, gY = -0.035f, gZ = 3.25f, gRot = 25.0f;
static float lX = -60.0f, lY = 0, lZ = 0, lOfs = 60, lFreq = 1.5;
static float r0 = 46.0f;

static float fsX = 0.707f, fsZ = 2.0f, fZ = 0.26f, fAnimSpeed = 24.0f;

static float ft0 = -6.28f, ftS = 1.135f, ftExp = 1.07f, ftR = -68.0f, ftsX = 3.15f, ftsY = 1.0f;

static float maskY = -0.555f;
static bool maskVis = false;

RocketScene::RocketScene(float aStartTime, float aEndTime) : Scene("rocket", aStartTime, aEndTime)
{
	oRocket = new ObjLoader("data/rocket/rocket.obj", 0.3f);
	tRocket = new Texture("data/rocket/rocket.png", SOIL_LOAD_AUTO, SOIL_FLAG_INVERT_Y);
	sRocket = new GlslShader("data/glsl/phong+t.glsl");
	tEngine = new Texture("data/rocket/antrieb.png", SOIL_LOAD_AUTO, SOIL_FLAG_INVERT_Y);
	sFire = new GlslShader("data/glsl/flat+t.glsl");
	tFire = new Texture("data/rocket/fireanim.png", SOIL_LOAD_AUTO, SOIL_FLAG_INVERT_Y);
	TWEAK_INIT();
	TWEAKA(r0, "min=-180 max=180");
	TWEAKA(lX, "min=-200 max=200");
	TWEAKA(lY, "min=-200 max=200");
	TWEAKA(lZ, "min=-200 max=200");
	TWEAKA(lOfs, "min=-180 max=180");
	TWEAKA(lFreq, "min=0 max=20");
	TWEAKA(gX, "min=-200 max=200");
	TWEAKA(gY, "min=-200 max=200");
	TWEAKA(gZ, "min=-20 max=20");
	TWEAKA(gRot, "min=-180 max=180 step=1");
	TWEAKSEP();
	TWEAKA(fsX, "min=0, max=2");
	TWEAKA(fsZ, "min=0, max=10");
	TWEAKA(fZ, "min=0, max=2");
	TWEAKA(fAnimSpeed, "min=0, max=50");
	TWEAKSEP();
	TWEAKA(ftsX, "min=0, max=10");
	TWEAKA(ftsY, "min=0, max=10");
	TWEAKA(ft0, "min=-10, max=10");
	TWEAKA(ftS, "min=0, max=10");
	TWEAKA(ftExp, "min=0, max=10");
	TWEAKA(ftR, "min=-180 max=180 step=1");
	TWEAKA(maskY, "min=-3 max=3");
	TWEAK(maskVis);
}

mat4 FireMatrix(float time, float z)
{
	mat4 mf(1.0);
	mf = rotate(mf, ftR, vec3(0, 0, 1));
	mf = translate(mf, vec3(0, ft0 + pow(time, ftExp) * ftS, z));
	mf = scale(mf, vec3(ftsX, ftsY, 1));
	return mf;
}

void RocketScene::_draw(float time)
{
	glClearDepth(1.0f);
	glClear(GL_DEPTH_BUFFER_BIT);

	/* Rocket */
	mat4 mPersp = perspective(45.0f, demo.aspect, 0.5f, 1000.0f);

	mat4 mTrans(1);
	mTrans = rotate(mTrans, gRot, vec3(0, 0, 1));
	mTrans = translate(mTrans, vec3(gX, gY, lX + gZ));
	mTrans = rotate(mTrans, lOfs + time * lFreq, vec3(0, 1, 0));
	mTrans = translate(mTrans, vec3(lX, lY, lZ));
	mTrans = rotate(mTrans, r0 + time * 10.0f, vec3(0, 0, 1));

	setBlendMode(NO_BLEND);
	setDepthMode(DEPTH_FULL);
	sRocket->bind();
	sRocket->uniform("u_projection", mPersp);
	sRocket->uniform("u_modelview", mTrans);
	sRocket->uniform("u_lightpos", vec3(-0.6, 0.5, 0.5));
	sRocket->uniform("u_alpha", 1.0f);
	sRocket->bindTexture("tex", tRocket, 0);
	oRocket->draw();		
	sRocket->unbind();

	/* Ink scene from framebuffer */
	sFire->bind();
	sFire->uniform("u_matrix", FireMatrix(time, 0.98f));
	if (!maskVis)
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	drawRect(vec2(-10, -100), vec2(10, maskY), vec2(0, 0), vec2(0, 0));
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	sFire->uniform("u_matrix", translate(mat4(1), vec3(0, 0, 0.99f)));
	sFire->bindFbo("tex", gFullScreenFbo, 0);
	drawFullscreenQuad(false);
	sFire->unbind();

	/* Big fire */
	int animStep = int(floor(time * fAnimSpeed));
	int animX = animStep & 7;
	int animY = (animStep >> 3) & 3;
	float l = float(animX) / 8, r = float(animX + 1) / 8;
	float b = float(animY) / 4, t = float(animY + 1) / 4;

	setBlendMode(BLEND_ALPHA);
	sFire->bind();
	sFire->uniform("u_matrix", FireMatrix(time, 0.97f));
	sFire->bindTexture("tex", tFire, 0);
	setTexFilter(0, GL_LINEAR);
	drawRect(vec2(-1, -1), vec2(1, 1), vec2(l, b), vec2(r, t));
	sFire->unbind();

	/* Rocket engine exhaust */
	vec3 f0(0, 0, fZ), f1(0, 0, fZ - fsZ);
	f0 = vec3(mTrans * vec4(f0, 1)); f1 = vec3(mTrans * vec4(f1, 1));
	vec3 fAxis1(f1 - f0);
	vec3 fAxis2(normalize(cross(fAxis1, vec3(0, 0, 1))) * fsX);
	
	vec3 fVerts[] = { f0 - fAxis2, f0 + fAxis2, f0 + fAxis1 + fAxis2, f0 + fAxis1 - fAxis2 };
	vec2 fUV[] = { vec2(l, b), vec2(r, b), vec2(r, t), vec2(l, t) };

	sFire->bind();
	sFire->uniform("u_matrix", mPersp);
	sFire->bindTexture("tex", tEngine, 0);
	setTexFilter(0, GL_LINEAR);
	drawArray(GL_QUADS, value_ptr(*fVerts), NULL, value_ptr(*fUV), NULL, NULL, 4);
	sFire->unbind();
	setDepthMode(NO_DEPTH);
	setBlendMode(NO_BLEND);
}