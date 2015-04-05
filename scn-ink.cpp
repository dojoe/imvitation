#include "k.i.t.t.h"
#include "scenes.h"
#include <math.h>
#include <list>

#define NUM_TEXTS 9
const int FBOSIZE = 1024;

Texture *tBlob;
Texture *tTexts[NUM_TEXTS];
GlslShader *sBackground, *sDraw, *sCombine;
Fbo *fbo;

struct InkBlotParams {
	uint32_t seed;
	int nGens, minChildren, maxChildren;
	float size0, sizeDecay;
	float length0, lengthDecay;
	float alpha0, alphaDecay;
	float armWidth, angleExtend;
	float moveMaxAmpl, moveMinAsp, moveMinFreq, moveMaxFreq;
	float alphaMaxAmpl, alphaMinFreq, alphaMaxFreq;
	vec2 shift;
	float rot;
	float textScale, textRot, textBlotFade;
	int textID;
	vec2 screenPos;
	float screenScale;
	float screenRot;
};

void dumpInkBlotParams(InkBlotParams &p) {
	printf(
		"{ %u, %u, %u, %u, %0.3ff, %0.3ff, %0.3ff, %0.3ff, %0.3ff, %0.3ff, %0.3ff, %0.3ff, %0.3ff, %0.3ff, %0.3ff, %0.3ff, %0.3ff, %0.3ff, %0.3ff, vec2(%0.3ff, %0.3ff), %0.3ff, %0.3ff, %0.3ff, %0.3ff, %u, vec2(%0.3ff, %0.3ff), %0.3ff, %0.3ff },\n",
		p.seed, p.nGens, p.minChildren, p.maxChildren, p.size0, p.sizeDecay, p.length0, p.lengthDecay, p.alpha0, p.alphaDecay,
		p.armWidth, p.angleExtend, p.moveMaxAmpl, p.moveMinAsp, p.moveMinFreq, p.moveMaxFreq, p.alphaMaxAmpl, p.alphaMinFreq, p.alphaMaxFreq,
		p.shift.x, p.shift.y, p.rot, p.textScale, p.textRot, p.textBlotFade, p.textID, p.screenPos.x, p.screenPos.y, p.screenScale, p.screenRot
		);
}

bool gDrawArms = false, gDrawBlobs = true, gEditMode = false, gInvert = false;
float uBlack = 0.52f, uWhite = 0.54f;
float gFade = 1.0f, gTextFade = 0.0f;
float gBgScale = 1.585f, gBgOfs = 0.810f;
vec4 gBgColor(1, 1, 1, 1);
float gPaperScale = 0.2f, gBlotScale = 1.0f, gOvlScale = 0.5f;
int gLoadID;

static const mat2 paperMatrix = rotate(mat2(1.0f), 15.0f);

class Arm {
public:
	vec2 dir, normal;
	float size, length, alpha, awf;
	vec2 movelipse1, movelipse2;
	float move_freq;
	float alpha_freq, alpha_ampl;
	std::vector<Arm *> children;
	enum drawType { ARMS, BLOBS };

	Arm(const InkBlotParams &p, int generation, vec2 aDir, float aSize, float aLength, float aAlpha) :
		dir(aDir), normal(aDir.y, -aDir.x), size(aSize), length(aLength), alpha(aAlpha), awf(p.armWidth),
		movelipse1(rand(0.0f, p.moveMaxAmpl), rand(0.0f, p.moveMaxAmpl)), movelipse2(vec2(movelipse1.y, -movelipse1.x) * rand(p.moveMinAsp, 1.0f)),
		move_freq(generation ? rand(-1, 1) * rand(p.moveMinFreq, p.moveMaxFreq) : 0.0f), alpha_freq(rand(p.alphaMinFreq, p.alphaMaxFreq)),
		alpha_ampl(rand(0.0f, p.alphaMaxAmpl)) {}

	void draw(drawType type, vec2 start, float parent_size, float time, float fade);
};

void Arm::draw(drawType type, vec2 start, float parent_size, float time, float fade)
{
	float slength = length * (0.6f + 0.4f * fade);
	float ssize = size * fade;

	vec2 end = start + dir * slength + movelipse1 * sin(move_freq * time) + movelipse2 * cos(move_freq * time);

	sDraw->uniform("u_factor", alpha * fade + sin(alpha_freq * time) * alpha_ampl);
	switch (type) {
	case BLOBS: {
		vec2 delta = vec2(ssize / 2);
		drawRect(end - delta, end + delta, vec2(0.0f, 0.0f), vec2(1.0f, 1.0f));
		break;
	}
	case ARMS: {
		vec2 ne = normal * slength * ssize * awf;
		vec2 ns = normal * slength * ssize * awf;
		if (length != 0.0f)
			drawQuad(start + ns, end + ne, start - ns, end - ne, vec2(0.0f, 0.0f), vec2(1.0f, 1.0f));
			//drawParallelogram(start + n, end + n, start - n, vec2(0.0f, 0.0f), vec2(1.0f, 1.0f));
			//drawRect(vec2(0.0f, 0.0f), vec2(1.0f, 1.0f), vec2(-1.0f, 1.0f), vec2(0.0f, 0.0f), vec2(1.0f, 1.0f));
		break;
	}
	}

	for (unsigned i = 0; i < children.size(); i++)
		children[i]->draw(type, end, ssize, time, fade);
}

class InkBlot {
public:
	InkBlot(const InkBlotParams &p);
	void draw(float time, float totalFade, float textFade);

private:
	typedef std::list<Arm> arms_t;
	arms_t arms;
	InkBlotParams params;
	Texture *tex;

	void generateArms(const InkBlotParams &p, int generation, Arm &parent, float minAngle, float maxAngle, float length, float size, float alpha);
	void drawBlot(float time, float fade);
};


InkBlot::InkBlot(const InkBlotParams &p) : params(p), tex(tTexts[min(p.textID, NUM_TEXTS)])
{
	skrand(p.seed);

	arms.push_back(Arm(p, 0, vec2(0.0f), p.size0, p.length0, p.alpha0));
	generateArms(p, 0, arms.back(), 0, 2 * PI, p.length0, p.size0, p.alpha0);
}

void InkBlot::generateArms(const InkBlotParams &p, int generation, Arm &parent, float minAngle, float maxAngle, float length, float size, float alpha)
{
	int numChildren = rand(p.minChildren, p.maxChildren + generation);
	float angleRange = maxAngle - minAngle;
	float angleSection = angleRange / numChildren;

	if (generation == p.nGens)
		return;

	for (int i = 0; i < numChildren; i++) {
		float angle = minAngle + angleSection * ((float)i + 0.5f) + rand(-angleSection / 2, angleSection / 2);
		vec2 dir(cos(angle), sin(angle));
		arms.push_back(Arm(p, generation, dir, rand(size * p.sizeDecay * p.sizeDecay, size), rand(length * p.lengthDecay, length / p.lengthDecay),
			rand(alpha * p.alphaDecay, alpha)));
		parent.children.push_back(&arms.back());

		generateArms(p, generation + 1, arms.back(), minAngle + angleSection * (float(i) + 0.5f - p.angleExtend), minAngle + angleSection * (float(i) + 0.5f + p.angleExtend),
			length * p.lengthDecay, size * p.sizeDecay, alpha * p.alphaDecay);
	}
}

void InkBlot::drawBlot(float time, float fade)
{
	setBlendMode(BLEND_ADD_ALPHA);

	mat4 m = scale(mat4(1.0f), vec3(1.0f, 1.0f, 1.0f));
	m = translate(m, vec3(params.shift.x, params.shift.y, 0.0f));
	m = rotate(m, params.rot, vec3(0.0f, 0.0f, -1.0f));

	sDraw->bind();
	sDraw->uniform("u_color", vec3(1.0f, 1.0f, 1.0f));
	sDraw->uniform("u_offset", 0.0f);
	sDraw->uniform("u_matrix", m);

	sDraw->bindTexture("tex", tBlob, 0);
	if (gDrawArms)
		arms.front().draw(Arm::ARMS, vec2(0.0f), 0.0f, time, fade);

	sDraw->bindTexture("tex", tBlob, 0);
	if (gDrawBlobs)
		arms.front().draw(Arm::BLOBS, vec2(0.0f), 0.0f, time, fade);

	sDraw->unbind();

	setBlendMode(NO_BLEND);
}

void InkBlot::draw(float time, float totalFade, float textFade)
{
	float blotFade = totalFade * mix(1.0f, params.textBlotFade, textFade);

	fbo->bind();
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	drawBlot(time, blotFade);
	fbo->unbind();

	mat4 m = demo.fix_aspect(mat4(1.0f));
	m = translate(m, vec3(params.screenPos, 0.0f));
	m = rotate(m, params.screenRot, vec3(0.0f, 0.0f, 1.0f));
	m = scale(m, vec3(2.0f * params.screenScale, 1.0f * params.screenScale, 1.0f));

	mat2 om = mat2(1.0f, 0.0f, 0.0f, 0.5f);
	om = rotate(om, params.textRot);
	om = scale(om, vec2(1 / params.textScale, -1 / params.textScale * tex->aspect));

	setBlendMode(gInvert ? BLEND_INVERT_ALPHA : BLEND_ALPHA);
	sCombine->bind();
	sCombine->bindFbo("fBlot", fbo, 0);
	sCombine->bindTexture("tOverlay", tex, 1);
	sCombine->uniform("u_paperscale", gPaperScale);
	sCombine->uniform("u_blotscale", gBlotScale);
	sCombine->uniform("u_ovlscale", textFade);
	sCombine->uniform("u_black", uBlack);
	sCombine->uniform("u_white", uWhite);
	sCombine->uniform("u_matrix", m);
	sCombine->uniform("u_papermat", paperMatrix);
	sCombine->uniform("u_ovlmat", om);
	drawUnitQuad();
	sCombine->unbind();
	setBlendMode(NO_BLEND);
}

InkBlotParams params[] = {
	/* 130         */{ 12342, 3, 2, 7, 1.000f, 0.740f, 0.355f, 0.430f, 0.415f, 0.780f, 0.340f, 1.200f, 0.205f, 0.025f, 0.225f, 1.225f, 0.345f, 0.180f, 1.445f, vec2(0.745f, -0.180f), 180.000f, 1.000f, -11.000f, 0.240f, 1, vec2(0.730f, 0.000f), 1.000f, 0.000f },
	/* cosy        */{ 12340, 3, 3, 7, 1.000f, 0.740f, 0.355f, 0.750f, 0.525f, 0.780f, 0.340f, 1.200f, 0.075f, 0.025f, 0.225f, 1.225f, 0.345f, 0.180f, 1.445f, vec2(0.295f, -0.010f), 48.000f, 0.835f, 11.000f, 0.325f, 0, vec2(-0.850f, 0.285f), 0.735f, 19.000f },
	/* outdoor     */{ 12342, 3, 3, 7, 1.000f, 0.740f, 0.355f, 0.750f, 0.415f, 0.780f, 0.340f, 1.200f, 0.075f, 0.025f, 0.225f, 1.225f, 0.345f, 0.180f, 1.445f, vec2(0.350f, 0.050f), 177.000f, 0.740f, 9.000f, 0.345f, 2, vec2(-0.430f, -0.410f), 0.910f, 4.000f },
	/* BBQ         */{ 12242, 3, 3, 8, 0.915f, 0.740f, 0.310f, 0.575f, 0.345f, 0.845f, 0.340f, 1.200f, 0.125f, 0.025f, 0.630f, 1.130f, 0.470f, 0.180f, 1.445f, vec2(0.350f, -0.035f), -52.000f, 0.990f, 3.000f, 0.050f, 4, vec2(0.835f, 0.335f), 1.000f, -15.000f },
	/* DJs         */{ 12242, 4, 3, 5, 0.920f, 0.490f, 0.250f, 0.360f, 0.970f, 0.920f, 0.340f, 1.200f, 0.175f, 0.025f, 0.630f, 1.130f, 0.470f, 0.180f, 1.445f, vec2(0.425f, -0.300f), -70.000f, 0.990f, 3.000f, 0.050f, 3, vec2(-0.585f, 0.205f), 1.000f, 7.000f },
	/* drinks      */{ 12229, 4, 3, 3, 0.760f, 0.700f, 0.135f, 1.010f, 0.790f, 0.565f, 0.340f, 0.575f, 0.160f, 0.025f, 0.225f, 1.225f, 0.720f, 0.180f, 1.445f, vec2(0.590f, 0.000f), -79.000f, 0.560f, 0.000f, 0.130f, 5, vec2(0.605f, -0.330f), 1.270f, 28.000f },
	/* tents       */{ 12242, 6, 1, 3, 0.730f, 0.650f, 0.000f, 0.360f, 0.350f, 0.900f, 0.340f, 1.075f, 0.235f, 0.550f, 0.120f, 0.390f, 0.470f, 0.180f, 1.445f, vec2(0.455f, -0.150f), -70.000f, 0.375f, 0.000f, 0.290f, 7, vec2(0.530f, 0.660f), 1.980f, -7.000f },
	/* Gravedigger */{ 12243, 4, 3, 5, 0.945f, 0.465f, 0.310f, 0.360f, 0.975f, 0.875f, 0.340f, 1.200f, 0.155f, 0.055f, 1.100f, 2.045f, 0.695f, 0.180f, 1.445f, vec2(0.830f, -0.030f), -180.000f, 0.585f, 7.000f, 0.245f, 8, vec2(-0.675f, -0.605f), 1.305f, -16.000f},
	/* orgas       */{ 12512, 2, 3, 4, 0.925f, 0.930f, 0.325f, 0.750f, 0.415f, 0.780f, 0.340f, 1.200f, 0.075f, 0.025f, 0.225f, 1.225f, 0.345f, 0.180f, 1.445f, vec2(0.960f, 0.050f), 152.000f, 0.835f, 0.000f, 0.345f, 6, vec2(0.000f, 0.000f), 1.620f, 0.000f },
};

InkBlotParams p = { 12340, 3, 3, 7, 1.000f, 0.740f, 0.355f, 0.750f, 0.525f, 0.780f, 0.340f, 1.200f, 0.075f, 0.025f, 0.225f, 1.225f, 0.345f, 0.180f, 1.445f, vec2(0.295f, -0.010f), 48.000f, 0.835f, 11.000f, 0.325f, 0, vec2(-0.850f, 0.285f), 1.050f, 19.000f };

static const int NUM_BLOTS = ARRAY_SIZE(params);
InkBlot *blots[ARRAY_SIZE(params)];

void TW_CALL dumpInkBlotParams(void *)
{
	dumpInkBlotParams(p);
}

void TW_CALL dumpAllParams(void *)
{
	for (int i = 0; i < NUM_BLOTS; i++)
		dumpInkBlotParams(params[i]);
}

void TW_CALL loadParams(void *)
{
	p = params[gLoadID];
}

void TW_CALL saveParams(void *)
{
	params[gLoadID] = p;
	delete blots[gLoadID];
	blots[gLoadID] = new InkBlot(p);
}

InkScene::InkScene(float aStartTime, float aEndTime) : Scene("ink", aStartTime, aEndTime)
{
	tBlob = new Texture("data/ink/blob.png");

	for (int i = 0; i < NUM_TEXTS; i++) {
		char fname[100];
		sprintf(fname, "data/ink/text%u.png", i + 1);
		tTexts[i] = new Texture(fname);
	}
	for (int i = 0; i < NUM_BLOTS; i++)
		blots[i] = new InkBlot(params[i]);

	sBackground = new GlslShader("data/glsl/ink_bg.glsl");
	sDraw = new GlslShader("data/glsl/alpha_tex.glsl");
	sCombine = new GlslShader("data/glsl/ink_combine.glsl");
	fbo = new Fbo(FBOSIZE, FBOSIZE, GL_R8, false);

	TWEAK_INIT(!gEditMode);
	TWEAK(gEditMode);
	TWEAK(gDrawArms);
	TWEAK(gDrawBlobs);
	TWEAK(gLoadID);
	TWEAK_BUTTON("load", loadParams, NULL);
	TWEAK_BUTTON("save", saveParams, NULL);
	TWEAK_BUTTON("dump all", dumpAllParams, NULL);
	TWEAK(uBlack);
	TWEAK(uWhite);
	TWEAKSEP();
	TWEAK(gBgColor);
	TWEAKA(gBgScale, "min=0 max=2");
	TWEAK(gBgOfs);
	TWEAK(gPaperScale);
	TWEAK(gBlotScale);
	TWEAK(gOvlScale);
	TWEAKSEP();
	TWEAK(gInvert);
	TWEAKA(p.shift.x, "min=-3 max=3");
	TWEAKA(p.shift.y, "min=-3 max=3");
	TWEAKA(p.rot, "min=-180 max=180 step=1");
	TWEAK(p.seed);
	TWEAK(p.nGens);
	TWEAKA(p.armWidth, "min=0.01 max=3.0");
	TWEAKA(p.minChildren, "min=1 max=30");
	TWEAKA(p.maxChildren, "min=1 max=30");
	TWEAK(p.length0);
	TWEAKA(p.lengthDecay, "min=0.1 max=3.0");
	TWEAKA(p.size0, "max=3");
	TWEAKA(p.sizeDecay, "min=0.1 max=3.0");
	TWEAK(p.alpha0);
	TWEAK(p.alphaDecay);
	TWEAKA(p.angleExtend, "min=0.1 max=10.0");
	TWEAKSEP();
	TWEAK(p.moveMaxAmpl);
	TWEAK(p.moveMinAsp);
	TWEAKA(p.moveMinFreq, "min=0.1 max=10.0");
	TWEAKA(p.moveMaxFreq, "min=0.1 max=10.0");
	TWEAK(p.alphaMaxAmpl);
	TWEAKA(p.alphaMinFreq, "min=0.1 max=10.0");
	TWEAKA(p.alphaMaxFreq, "min=0.1 max=10.0");
	TWEAK(gFade);
	TWEAKSEP();
	TWEAK(gTextFade);
	TWEAK(p.textID);
	TWEAK(p.textScale);
	TWEAKA(p.textRot, "min=-180 max=180 step=1");
	TWEAK(p.textBlotFade);
	TWEAKSEP();
	TWEAKA(p.screenPos.x, "min=-3 max=3");
	TWEAKA(p.screenPos.y, "min=-3 max=3");
	TWEAKA(p.screenScale, "min=0 max=3");
	TWEAKA(p.screenRot, "min=-180 max=180 step=1");
	TWEAK_BUTTON("dump", dumpInkBlotParams, NULL);
}

static inline float fade(float x, float start, float length)
{
	//return clamp(x - start, 0.0f, length) / length;
	return smoothstep(start, start + length, x);
}

void InkScene::_draw(float time)
{
	time -= SceneTime::park2ink;
	bool toFbo = (time < 0.0f) || (gMusic->getTime() > SceneTime::rocketStart);

	if (toFbo)
		gFullScreenFbo->bind();

	sBackground->bind();
	sBackground->uniform("u_papermat", paperMatrix);
	sBackground->uniform("u_color", gBgColor);
	sBackground->uniform("u_offset", gBgOfs);
	sBackground->uniform("u_scale", gBgScale);
	sBackground->uniform("u_alpha", clamp(time / 0.1f + 1.0f, 0.0f, 1.0f));
	drawFullscreenQuad();
	sBackground->unbind();

	if (gEditMode) {
		InkBlot blot(p);
		blot.draw(time, gFade, gTextFade);
	}
	else {
		struct timing { float delay, fadein, pretext, fadetext, text, fadeout; } timing[NUM_BLOTS] = {
			{ -SceneTime::park2ink, 0.0f, 6.0f + SceneTime::park2ink, 1.0f, 2.0f, 1.0f },
			{  4.5f, 2.0f, 1.5f, 1.0f, 2.0f, 1.0f },
			{  7.0f, 2.0f, 1.5f, 1.0f, 2.0f, 1.0f },
			{  9.5f, 2.0f, 1.5f, 1.0f, 2.0f, 1.0f },
			{ 12.0f, 2.0f, 1.5f, 1.0f, 2.0f, 1.0f },
			{ 14.5f, 2.0f, 1.5f, 1.0f, 2.0f, 1.0f },
			{ 17.0f, 2.0f, 1.5f, 1.0f, 2.0f, 1.0f },
			{ 19.5f, 2.0f, 1.5f, 1.0f, 2.0f, 1.0f },
			{ 23.0f, 2.0f, 1.0f, 1.0f, 1.5f, 1.0f },
		};

		for (int i = 0; i < NUM_BLOTS; i++) {
			struct timing &t = timing[i];
			float blottime = time - t.delay;
			float start_fadetext = t.fadein + t.pretext;
			float start_fadeout = start_fadetext + t.fadetext + t.text;
			float total = start_fadeout + t.fadeout;

			if (blottime < 0 || blottime > total)
				continue;

			float blotfade = fade(blottime, 0.0f, t.fadein) * (1.0f - fade(blottime, start_fadeout, t.fadeout));
			float textfade = fade(blottime, start_fadetext, t.fadetext) * (1.0f - fade(blottime, start_fadeout, t.fadeout));
			blots[i]->draw(blottime, blotfade, textfade);
		}
	}

	if (toFbo)
		gFullScreenFbo->unbind();
}