#include "k.i.t.t.h"
#include "scenes.h"

GlslShader* outstormShader;
GlslShader* greetingsShader;
Texture* greetings;
Texture* clouds[6];

static float stormstart = 12.f;

OutstormScene::OutstormScene(float aStartTime, float aEndTime) : Scene("outstorm", aStartTime, aEndTime)
{
	 outstormShader = new GlslShader("data/glsl/outstorm.glsl");
	greetingsShader = new GlslShader("data/glsl/greetings.glsl");

	greetings = new Texture("data/outstorm/greetings.png", SOIL_LOAD_AUTO, 0);

	std::string cloudTex = "data/outstorm/w1.png";
	for (int c = 0; c < 6; c++)
	{
		clouds[c] = new Texture(cloudTex.c_str(), SOIL_LOAD_AUTO, 0);
		cloudTex[cloudTex.length() - 5]++;
	}

	TWEAK_INIT();
	TWEAKA(stormstart, "min=0 max=20");
}

void OutstormScene::_draw(float time)
{
	const float startTransition = SceneTime::ink2outStorm; //3.f;
	const float stormEnd = SceneTime::cubewallStart-SceneTime::outStormStart; // 30.0f;
	const float endTransition = SceneTime::outStorm2cubewall; // 3.0f;

	setBlendMode(/*BLEND_ALPHA*/NO_BLEND);
	setCullMode(NO_CULL);
	setDepthMode(NO_DEPTH);

	/*if(time >= startTransition)
	{
		glClearColor(0,0,0, 1.0f);  //glClearColor(56.f / 256., 60.f / 256., 50.f / 256., 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}*/

	outstormShader->bind();

	outstormShader->uniform("resolution", vec2(demo.sizex, demo.sizey));
	outstormShader->uniform("time", time);

	float fxtime = time - startTransition;

	float layers = max(1.0f, min((0.5f*time), (4.f - (0.2f / stormstart)*fxtime)) - 1.0f);
	float colorDetail = max(4.f, (32.f - (16.f/stormstart)*fxtime));
	float variance = max(0.f, min(1.f, 2.f*(stormstart-1.f - fxtime)));
	float colorFactor = max(0.f, min(1.f, (-stormstart-2.f + fxtime)*.5f));
	float blockSize = max(1.f, min(10.f, 3.f*(-stormstart+1.f + fxtime)));
	float origin      = colorFactor*0.5f;

	float endfactor = (fxtime - (stormEnd - endTransition));
	float flatness = 1.f / max(1.0f, 10.f*endfactor);

	outstormShader->uniform("layers", layers);
	outstormShader->uniform("colorDetail", colorDetail);
	outstormShader->uniform("variance", variance);
	outstormShader->uniform("colorFactor", colorFactor);
	outstormShader->uniform("blockSize", blockSize);
	outstormShader->uniform("origin", origin);
	outstormShader->uniform("flatness", flatness);
	outstormShader->uniform("alpha", min(1.f, min(pow(time/startTransition, 2.f), (stormEnd+endTransition+1.f)-time )));

	drawFullscreenQuad();

	outstormShader->unbind();


	setDepthMode(NO_DEPTH);
	setBlendMode(BLEND_ALPHA);
	setCullMode(NO_CULL);

	greetingsShader->bind();
	greetingsShader->uniform("fade", 1.f);

	float s = cos(time*.5f);
	float w = (((s < 0) ? (time*.5f) : (PI - time*.5f)) + PI);
	float time2 = min(time, stormEnd);

	vec2 tex[] = { vec2(0, 1), vec2(1, 1), vec2(1, 0), vec2(0, 0) };

	glEnable(GL_SCISSOR_TEST);
	setTexFilter(0, GL_NEAREST);

	for (int side = -1; side <= 1; side += 2)
	{
		glScissor(0, (side == -1) ? 0 : (demo.sizey/2), demo.sizex, demo.sizey / 2);

		for (int cloudIdx = 0; cloudIdx < 20; cloudIdx++)
		{
			Texture* cloud = clouds[cloudIdx % (sizeof(clouds) / sizeof(cloud[0]))];
			greetingsShader->bindTexture("tex", cloud, 0);

			float endfactor2 = max(0.f, min(1.f, sqrt(1.f - endfactor)));
			float size = 0.3f*colorFactor;
			vec3 dir = size*mix(vec3(1.f, 0., 0.), vec3(cos(w), -sin(w), 0.), endfactor2);
			vec3 dir2(-dir.y, dir.x, dir.z);

			//		vec3 pos(-5. + 0.5f*cloudIdx + abs(sin(time*.5f + PI*.5f)), .4f + .25f*sin((float)cloudIdx*2.f), 0);
			vec3 pos(-5. + 0.5f*cloudIdx, .4f + .5f*sin((float)cloudIdx*2.3f), 0);
			pos += dir*(abs(sin(time2*.5f + PI*.5f)));
			pos += (4.f*dir2*(cos(time2*.5f) - 0.125f*time2));
			pos.x = -2.f + fmod(pos.x, 4.f);
			pos.y = -.25f + fmod(pos.y + 200.f, 1.5f);

			vec3 box[] = { pos, pos + dir, pos + dir + dir2, pos + dir2 };
			box[0].x /= demo.aspect;
			box[1].x /= demo.aspect;
			box[2].x /= demo.aspect;
			box[3].x /= demo.aspect;
			box[0].y *= side;
			box[1].y *= side;
			box[2].y *= side;
			box[3].y *= side;

			drawArray(GL_QUADS, value_ptr(box[0]), NULL, value_ptr(tex[0]), NULL, NULL, 4);
		}
	}

	glDisable(GL_SCISSOR_TEST);

	float creditTime = (time - (stormstart + 8.f)) / (3 * BEAT);
	float pos = floor(creditTime);
	if ((pos >= 0) && (pos < 10))
	{
		const float credRatio = 96.f / 256;
		const float credStep = 96.f / 1024.f;
		const float creditSize = 1.0;

		greetingsShader->bindTexture("tex", greetings, 0);

		drawRect(vec2(-creditSize / demo.aspect, -credRatio*creditSize), vec2(creditSize / demo.aspect, credRatio*creditSize), vec2(0, credStep*(pos + 1)), vec2(1, credStep*pos));
	}

	setTexFilter(0, GL_LINEAR);
	greetingsShader->unbind();
}
