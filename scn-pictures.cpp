#include "k.i.t.t.h"
#include "scenes.h"

class Picture {
public:
	Picture(const char *fname);
	void draw(vec3 pos, float size, float rot, bool shadow = false);
private:
	Texture tex;
	float w, h;
};

#define NUM_PICS 19
#define NUM_TEXTS 6

Texture *shadowTex;
Picture *pics[NUM_PICS];
GlslShader *background, *drawpic, *drawshadow, *sText;
ObjLoader *oTexts[NUM_TEXTS];

Picture::Picture(const char *fname) : tex(fname, SOIL_LOAD_AUTO, SOIL_FLAG_POWER_OF_TWO)
{
	if (tex.width > tex.height) {
		w = 1.0f;
		h = ((float)tex.height) / tex.width;
	}
	else {
		w = ((float)tex.width) / tex.height;
		h = 1.0f;
	}
}

void Picture::draw(vec3 pos, float size, float rotRad, bool shadow)
{
	float rot = rotRad * 180.0f / (float)PI;

	vec3 verts[4] = { vec3(-1.0f, -1.0f, 0.0f), vec3(1.0f, -1.0f, 0.0f), vec3(1.0f, 1.0f, 0.0f), vec3(-1.0f, 1.0f, 0.0f) };
	vec2 texCoords[4] = { vec2(0.0f, 1.0f), vec2(1.0f, 1.0f), vec2(1.0f, 0.0f), vec2(0.0f, 0.0f) };

	if (shadow)
	{
		float shadowOffsetSize = size * 2;
		mat4 m_shad = demo.fix_aspect(mat4(1.0f));
		m_shad = translate(m_shad, vec3(pos.x + 0.1f*shadowOffsetSize, pos.y - 0.07f*shadowOffsetSize, pos.z + 0.000005f));
		m_shad = rotate(m_shad, rot, vec3(0.0f, 0.0f, 1.0f));
		m_shad = scale(m_shad, vec3(w, h, 0.0) * size*1.1f);

		drawshadow->bind();
		drawshadow->bindTexture("tex", shadowTex, 0);
		drawshadow->uniform("u_matrix", m_shad);
		drawshadow->uniform("u_factor", 0.5f);
		drawshadow->uniform("u_offset", 0.0f);
		drawshadow->uniform("u_color", vec3(0.0f));
		setBlendMode(BLEND_ALPHA);
		drawArray(GL_QUADS, value_ptr(verts[0]), NULL, value_ptr(texCoords[0]), NULL, NULL, 4);
		setBlendMode(NO_BLEND);
		drawshadow->unbind();
	}
	else
	{
		mat4 m_pic = demo.fix_aspect(mat4(1.0f));
		m_pic = translate(m_pic, pos);
		m_pic = rotate(m_pic, rot, vec3(0.0f, 0.0f, 1.0f));
		m_pic = scale(m_pic, vec3(w, h, 0.0) * size);

		drawpic->bind();
		drawpic->bindTexture("tex", &tex, 0);
		drawpic->uniform("u_matrix", m_pic);
		drawArray(GL_QUADS, value_ptr(verts[0]), NULL, value_ptr(texCoords[0]), NULL, NULL, 4);
		drawpic->unbind();
	}
}

PicturesScene::PicturesScene(float aStartTime, float aEndTime) : Scene("pics", aStartTime, aEndTime)
{
	char fname[100];

	shadowTex = new Texture("data/pics/shadow.jpeg", SOIL_LOAD_L, SOIL_FLAG_POWER_OF_TWO);

	for (int i = 0; i < NUM_PICS; i++) {
		snprintf(fname, sizeof(fname), "data/pics/%u.jpeg", i + 1);
		pics[i] = new Picture(fname);
	}

	for (int i = 0; i < NUM_TEXTS; i++) {
		sprintf(fname, "data/pics/line%u.obj", i + 1);
		oTexts[i] = new ObjLoader(fname, 0.15f);
	}

	background = new GlslShader("data/glsl/pic_back.glsl");
	drawpic = new GlslShader("data/glsl/flat+t.glsl");
	drawshadow = new GlslShader("data/glsl/alpha_tex.glsl");
	sText = new GlslShader("data/glsl/phong+c.glsl");
}

const float TEXT_START = 5.0f;
const float TEXT_DURATION = 5.0f;
const float TEXT_FADETIME = 0.5f;
const float TEXT_PAUSE = 1.0f;
const float TEXT_CYCLES = TEXT_DURATION - 1.0f;

const float TEXT_SHOWTIME = TEXT_DURATION + 2 * TEXT_FADETIME;
const float TEXT_PERIOD = TEXT_SHOWTIME + TEXT_PAUSE;

void drawTextLine(int i, float time, float y, float alpha)
{
	float myTime = time * 2 * PI * TEXT_CYCLES / TEXT_SHOWTIME;

	mat4 mTextPersp = perspective(45.0f, demo.aspect, 0.5f, 15.0f);
	mat4 mTextMV = translate(mat4(1.0f), vec3(0.0f, y, (pow(cos(myTime) * 0.5f + 0.5f, 1.6f) - 0.5f) * -4.0f - 3.0f));
	mTextMV = rotate(mTextMV, sin(myTime) * 30.0f, vec3(0.0f, 1.0f, 0.0f));
	//mat4 mTextMV = translate(mat4(1.0f), vec3(0.0f, 0.0f, -2.0f));
	//mTextMV = rotate(mTextMV, time * 60.0f, vec3(0.4f, 1.0f, 0.2f));
	sText->bind();
	sText->uniform("u_matrix", mTextPersp * mTextMV);
	sText->uniform("u_modelview", mTextMV);
	sText->uniform("u_projection", mTextPersp);
	sText->uniform("u_lightpos", vec3(-2.0f, -6.0f, 6.0f));
	sText->uniform("u_alpha", alpha);
	oTexts[i]->draw();
	sText->unbind();
}

void drawText(float time)
{
	setDepthMode(DEPTH_FULL);
	setCullMode(CULL_BACK);
	setBlendMode(BLEND_ALPHA);

	float textTime = time - TEXT_START;
	if (textTime > 0 && textTime < NUM_TEXTS / 2 * TEXT_PERIOD) {
		int textIter = (int)floorf(textTime / TEXT_PERIOD);
		float iterTime = fmod(textTime, TEXT_PERIOD);
		float alpha = smoothstep(0.0f, TEXT_FADETIME, iterTime) * smoothstep(0.0f, TEXT_FADETIME, TEXT_SHOWTIME - iterTime);
		drawTextLine(textIter * 2, iterTime, 0.07f, alpha);
		drawTextLine(textIter * 2 + 1, iterTime + 0.10f, -0.07f, alpha);
	}

	setDepthMode(NO_DEPTH);
	setCullMode(NO_CULL);
	setBlendMode(NO_BLEND);
}

void PicturesScene::_draw(float inputTime)
{
	const float maxTime = SceneTime::foreverStart;

	float time = inputTime - 1.0f;
	float fadeBG1 = min(1.0f, inputTime*0.25f);
	float fadeBG2 = min(1.0f, inputTime*0.5f);

	float totalfade = (0.5f - 0.5f*cos(2.f*PI*time/(maxTime-1.f)));

	if (time < maxTime-SceneTime::pic2forever)
	{
		background->bind();
		background->uniform("u_color1", vec4(178.f/256.f, 232.f/256.f, 252.f/256.f, 1.0f)*fadeBG1);
		background->uniform("u_color2", vec4(26.f / 256.f, 154.f / 256.f, 214.f / 256.f, 1.0f)*fadeBG2);
		drawFullscreenQuad();
		background->unbind();
	}

	for (int shadow = 0; shadow < 2; shadow++)
	{
		const int limit = NUM_PICS * 100;
		setDepthMode(shadow ? DEPTH_READ : DEPTH_FULL);
		for (int j = limit - 1; j >= 0; j--)
		{
			int i = (j * 99) % limit;

			float x = fmod(time + (float)i*.27f, 127.3f);

			float d = (time*time*0.3f - j*0.1f)*(totalfade*(1.f+0.01f*j/limit));
			if (d < 0) continue;
			float currentfade = min(d, 1.f);
			float scale = currentfade*0.5f;
			if (scale < 0.001f) continue;

			float y = -1.f + fmod(i*77.77f, 2.f);
			x -= 2.5f;
			if (x < -3.0f || x > 3.0f) continue;
			vec3 pos(x, y, (float)j * -0.00001f + 0.99f);
			float s = 1.f + 0.25f*sin(10.f * i);
			float rotation = 0.08f*sin(1.2f*time*s*(1.f + i*(0.1f / limit)) + i) / s;

			pics[i%NUM_PICS]->draw(pos, scale, rotation, shadow != 0);
		}
	}

	drawText(time);
}
