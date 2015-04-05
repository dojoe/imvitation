#include "k.i.t.t.h"
#include "scenes.h"

static ObjLoader *oPark;
static Texture *tPark[6], *tLogo, *tSmoke, *tText;
static GlslShader *sDrawPhong, *sDrawFlat, *sPark[ARRAY_SIZE(tPark)];

static vec3 endCamPos(64.41, 11.34, 24.00), endLookAt(11.37, 8.62, -24.18);

static float logoFadeStart = 5.0f, logoFadeEnd = 6.0f;
static vec3 logoCenter(-0.37, 0.43, 0);
static float logoSize = 1.f;
static vec3 textCenter(.32, -.39, 0);
static float textSize = .58f;
static vec3 smokeCenter(.99, 3.79, .57);
static vec2 smokeSize(.5, 1.19);
static float smokeRot = 69;
static float smokeAlpha = .385;

EndScene::EndScene(float aStartTime, float aEndTime) : Scene("end", aStartTime, aEndTime)
{
	oPark = new ObjLoader("data/park/grill.obj", 0.1f, false, true);
	tPark[0] = loadCacheTexture("data/park/tonne.jpg");
	tPark[1] = loadCacheTexture("data/park/skybox.jpg");
	tPark[2] = loadCacheTexture("data/park/fleisch.jpg");
	tPark[3] = loadCacheTexture("data/park/kasten_innen.jpg");
	tPark[4] = loadCacheTexture("data/park/grill.jpg");
	tPark[5] = loadCacheTexture("data/park/boden.jpg");
	sDrawPhong = new GlslShader("data/glsl/phong+t.glsl");
	sDrawFlat = new GlslShader("data/glsl/flat+t.glsl");
	sPark[0] = sPark[1] = sPark[2] = sDrawFlat;
	sPark[3] = sPark[4] = sPark[5] = sDrawFlat;

	tLogo = new Texture("data/park/nordlicht15.png", SOIL_LOAD_AUTO, SOIL_FLAG_INVERT_Y);
	tText = new Texture("data/park/endtext.png", SOIL_LOAD_AUTO, SOIL_FLAG_INVERT_Y);
	tSmoke = new Texture("data/park/smoketxt.png", SOIL_LOAD_AUTO, SOIL_FLAG_INVERT_Y);

	TWEAK_INIT(false);
	TWEAK(endCamPos);
	TWEAK(endLookAt);
	TWEAKA(logoFadeStart, "max=20");
	TWEAKA(logoFadeEnd, "max=20");
	TWEAK(logoCenter);
	TWEAKA(logoSize, "max=10");
	TWEAK(textCenter);
	TWEAKA(textSize, "max=4");
	TWEAK(smokeCenter);
	TWEAKA(smokeSize.x, "max=4");
	TWEAKA(smokeSize.y, "max=4");
	TWEAKA(smokeRot, "min=-180 max=180 step=1");
	TWEAK(smokeAlpha);
}

static inline vec3 interpolatePath(std::vector<vec3> &path, float pos)
{
	float vertIdx = pos * (path.size() - 1);
	int i = int(floor(vertIdx));
	return mix(path[i], path[i + 1], fract(vertIdx));
}

void EndScene::_draw(float time)
{
	std::vector<vec3> path;

	skrand(625734);
	for (int i = 0; i < 10; i++)
		path.push_back(endCamPos + vec3(rand(-1.f, 1.f), rand(-1.f, 1.f), rand(-1.f, 1.f)));

	mat4 mPersp = perspective(45.0f, demo.aspect, 0.5f, 1000.0f);
	mat4 mCamera = lookAt(interpolatePath(path, time / Duration()) * 0.1f, endLookAt * 0.1f, vec3(0, 1, 0));

	setDepthMode(DEPTH_FULL);
	for (int i = 0; i < ARRAY_SIZE(tPark); i++) {
		GlslShader *sDraw = sPark[i];
		sDraw->bind();
		sDraw->uniform("u_matrix", mPersp * mCamera);
		sDraw->uniform("u_modelview", mCamera);
		sDraw->uniform("u_projection", mPersp);
		sDraw->uniform("u_alpha", 1.0f);
		sDraw->bindTexture("tex", tPark[i], 0);
		oPark->draw(i);
		sDraw->unbind();
	}

	sDrawFlat->bind();
	sDrawFlat->bindTexture("tex", tSmoke, 0);
	setBlendMode(BLEND_ALPHA);
	sDrawFlat->uniform("u_alpha", smokeAlpha);
	mat4 sm(1);
	sm = translate(sm, smokeCenter * 0.1f);
	sm = rotate(sm, smokeRot, vec3(0, 1, 0));
	sm = scale(sm, vec3(smokeSize, 1));
	sDrawFlat->uniform("u_matrix", mPersp * mCamera * sm);

	int animStep = int(floor(time * 24.0f));
	int animX = animStep & 7;
	int animY = (animStep >> 3) & 3;
	float l = float(animX) / 8, r = float(animX + 1) / 8;
	float b = float(animY) / 4, t = float(animY + 1) / 4;

	drawRect(vec2(-0.5, -0.5), vec2(0.5, 0.5), vec2(l, t), vec2(r, b));
	sDrawFlat->unbind();

	setDepthMode(NO_DEPTH);

	float logoFade = clamp((time - logoFadeStart) / (logoFadeEnd - logoFadeStart), 0.f, 1.f);

	setBlendMode(BLEND_ALPHA);
	sDrawFlat->bind();
	sDrawFlat->bindTexture("tex", tLogo, 0);
	sDrawFlat->uniform("u_alpha", logoFade);
	mat4 lm(demo.fix_aspect(mat4(1)));
	lm = translate(lm, logoCenter + vec3(sin(time) * 2, cos(2 * time), 0) * .01f);
	lm = scale(lm, vec3(tLogo->aspect, 1, 1) * logoSize);
	sDrawFlat->uniform("u_matrix", lm);
	drawUnitQuad();
	lm = demo.fix_aspect(mat4(1));
	lm = translate(lm, textCenter + vec3(cos(time + .6135), sin(-2 * time - .6135), 0) * .003f);
	lm = scale(lm, vec3(tText->aspect, 1, 1) * textSize);
	sDrawFlat->uniform("u_matrix", lm);
	sDrawFlat->bindTexture("tex", tText, 0);
	drawUnitQuad();
	sDrawFlat->unbind();
	setBlendMode(NO_BLEND);
}