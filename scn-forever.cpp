#include "k.i.t.t.h"
#include "scenes.h"

ObjLoader *cog, *logo;
Texture *tex;
GlslShader *text, *flat, *flatwc;

static const float bs = (float)0xBE / 256;

static vec3 oCenter(-0.668f, 0.679f, 0.0f);
static float oScale = 1.23f;
static vec3 lCenter(-1.34f, 0.46f, 0);
static float lScale = 1.2f;
static float tY = -0.15f;

ForeverScene::ForeverScene(float aStartTime, float aEndTime) : Scene("forever", aStartTime, aEndTime)
{
	tex = new Texture("data/forever/text.png", SOIL_LOAD_AUTO, 0);
	cog = new ObjLoader("data/forever/cog.obj", 0.27f);
	logo = new ObjLoader("data/forever/nordlicht.obj", 0.3f);
	text = new GlslShader("data/glsl/forever_text.glsl");
	flat = new GlslShader("data/glsl/flat.glsl");
	flatwc = new GlslShader("data/glsl/flat+c.glsl");

	TWEAK_INIT();
	TWEAKA(oCenter.x, "min=-2 max=2 step=0.01");
	TWEAKA(oCenter.y, "min=-2 max=2 step=0.01");
	TWEAKA(oScale, "min=0 max=2 step=0.01");
	TWEAKA(lCenter.x, "min=-2 max=2 step=0.01");
	TWEAKA(lCenter.y, "min=-2 max=2 step=0.01");
	TWEAKA(lScale, "min=0 max=2 step=0.01");
	TWEAKA(tY, "min=-2 max=2 step=0.01");
}

void ForeverScene::_draw(float time)
{
	const float speccyAspect = 4.0f / 3;

	const float DEAD_TIME = 12 * BEAT;
	const float ZOOM_TIME = 10 * BEAT;
	const float PAN_TIME = 1.3f;
	const float END_ZOOM_TIME = 4.0f;
	const float END_PAN_TIME = 3.0f;
	const float FADEOUT_TIME = 2.0f;
	const float COG_FADEOUT_TIME = 1.0f;
	const float CYCLE_START = 28 * BEAT;
	const float CYCLE_TIME = 10.0f;
	const float FADE_TIME = 1.0f;
	const float DISPLAY_TIME = 7.5f;
	const int NUM_CYCLES = 2;

	float zoom;
	
	if (time < CYCLE_START) {
		zoom = clamp(time - DEAD_TIME, 0.0f, ZOOM_TIME) / ZOOM_TIME;
		zoom = mix(1.0f / 0.13f, 1.0f, zoom);
	}
	else {
		zoom = pow(clamp((Duration() - time) / END_ZOOM_TIME, 0.0f, 1.0f), 0.1f);
		zoom = mix(40.0f, 1.0f, zoom);
	}

	float pan = pow(clamp(time - DEAD_TIME - ZOOM_TIME + PAN_TIME, 0.0f, PAN_TIME) / PAN_TIME, 2.3f);
	pan *= 1.0f - pow(smoothstep(Duration() - END_ZOOM_TIME, Duration() - (END_ZOOM_TIME - END_PAN_TIME), time), 1/ 2.3f);
	vec3 center = mix(oCenter, vec3(0.0f), pan);

	mat4 m_base = demo.fix_aspect(mat4(1.0f));
	m_base = scale(m_base, vec3(zoom, zoom, 1.0f));
	m_base = translate(m_base, -center);

	float bgFade = 1.0f -smoothstep(Duration() - FADEOUT_TIME, Duration() - COG_FADEOUT_TIME, time);

	setBlendMode(BLEND_ALPHA);
	flat->bind();
	flat->uniform("u_matrix", m_base);
	flat->uniform("u_color", vec4(bs, bs, bs, bgFade));
	drawRect(vec2(-speccyAspect, -1), vec2(speccyAspect, 2), vec2(0, 0), vec2(1, 1));
	flat->unbind();

	mat4 m = translate(m_base, lCenter);
	m = scale(m, vec3(1) * lScale);

	flatwc->bind();
	flatwc->uniform("u_alpha", bgFade);
	flatwc->uniform("u_matrix", m);
	logo->draw();
	flatwc->unbind();

	flat->bind();
	flat->uniform("u_matrix", m_base);
	flat->uniform("u_color", vec4(0, 0, 0, bgFade));
	drawRect(vec2(-demo.aspect, -1), vec2(-speccyAspect, 2), vec2(0, 0), vec2(1, 1));
	drawRect(vec2(speccyAspect, -1), vec2(demo.aspect, 2), vec2(0, 0), vec2(1, 1));
	flat->unbind();

	m = translate(m_base, oCenter);
	m = scale(m, vec3(1) * oScale);
	m = rotate(m, time * 80.0f, vec3(0, 0, 1));

	flat->bind();
	flat->uniform("u_color", vec4(bs, bs, bs, 1.0f - smoothstep(Duration() - COG_FADEOUT_TIME, Duration(), time)));
	flat->uniform("u_matrix", m);
	cog->draw();
	flat->unbind();

	vec2 texOffset, panelOffset;
	float timeInCycle = fmod(time - CYCLE_START, CYCLE_TIME);
	if (time < CYCLE_START || timeInCycle >(DISPLAY_TIME + 2.0f * FADE_TIME))
		texOffset = vec2(1.0f, 0.75f);
	else if (timeInCycle > FADE_TIME + DISPLAY_TIME)
		texOffset = vec2(1.0f - (timeInCycle - FADE_TIME - DISPLAY_TIME) / FADE_TIME * 2.0f, 0.5f);
	else if (timeInCycle > FADE_TIME)
		texOffset = vec2(1.0f, 0.5f);
	else
		texOffset = vec2(1.0f - timeInCycle / FADE_TIME * 2.0f, 0.75f);

	int textIndex = int((time - CYCLE_START) / CYCLE_TIME);
	panelOffset = vec2((textIndex % 2) * 0.5f, 0.0f);

	if (textIndex < NUM_CYCLES) {
		text->bind();
		text->bindTexture("tex", tex, 0);
		text->uniform("u_matrix", m_base);
		text->uniform("u_texOffset", texOffset - panelOffset);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		drawRect(vec2(-1.0f, -0.5f + tY), vec2(1.0f, 0.5f + tY), vec2(0.00f, 0.25f) + panelOffset, vec2(0.5f, 0.0f) + panelOffset);
		setBlendMode(NO_BLEND);
		text->unbind();
	}
}