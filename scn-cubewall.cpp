#include "k.i.t.t.h"
#include "scenes.h"

GlslShader* cubeShader;
GlslShader* floorShader;
GlslShader* floorClearShader;
GlslShader* kratzerShader;
GlslShader* creditsShader;
uint8_t* textBitmap;
float*	 randomValues;
int textBitmapWidth, textBitmapHeight, textBitmapBPP;
Texture *kratzer, *credits;
Fbo *fboOverlay;
int fboSizeX, fboSizeY;

vec4* cubePosScale = NULL;
vec4* cubeCol = NULL;

static float semicube[] = {
	//l
	-1,  1, -1,
	-1,  1,  1,
	-1, -1, -1,
	-1, -1,  1,

	//b
	 1, -1, -1,
	 1, -1,  1,

	//r
	 1,  1, -1,
	 1,  1,  1,

	//t
	-1,  1, -1,
	-1,  1,  1,

	 1,  1,  1 , //flip side dummy vertex
	//v
	-1, -1,  1,
	 1, -1,  1
};

static float floorVerts[] = {
	-100, 0,  100,
	 100, 0,  100,
	 100, 0, -100,
	-100, 0, -100,
};

CubewallScene::CubewallScene(float aStartTime, float aEndTime) : Scene("cubewall", aStartTime, aEndTime)
{
	cubeShader = new GlslShader("data/glsl/cubewall_cube.glsl");
	floorShader = new GlslShader("data/glsl/cubewall_floor.glsl");
	floorClearShader = new GlslShader("data/glsl/cubewall_floorClear.glsl");
	kratzerShader = new GlslShader("data/glsl/cubewall_kratzer.glsl");
	creditsShader = new GlslShader("data/glsl/cubewall_credits.glsl");

	textBitmap = SOIL_load_image("data/cubewall/text.png", &textBitmapWidth, &textBitmapHeight, &textBitmapBPP, SOIL_LOAD_RGBA);
	randomValues = new float[textBitmapWidth*textBitmapHeight];
	for (int r = 0; r < textBitmapWidth*textBitmapHeight; r++)
	{
		randomValues[r] = rand() / (float)RAND_MAX;
	}

	kratzer = new Texture("data/cubewall/kratzer.png", SOIL_LOAD_AUTO, 0);
	credits = new Texture("data/cubewall/credits.png", SOIL_LOAD_AUTO, 0);

	fboOverlay = gFullScreenFbo;

	int maxCubes = textBitmapWidth*textBitmapHeight;

	cubePosScale = new vec4[maxCubes];
	cubeCol      = new vec4[maxCubes];
}

void CubewallScene::_draw(float time)
{
	int textBitmapWidthL = min(textBitmapWidth, (int)(time*30.f));

	time -= SceneTime::outStorm2cubewall;

	float fadeIn = min(1.0f, 1.f+time*0.33f);
	bool fadeActive = (fadeIn < 1.0);

	if (!fadeActive)
	{
		fboOverlay->bind();
	}

	float fade2 = max(0.f, min(1.f, time));

	setBlendMode(NO_BLEND);
	setCullMode(NO_CULL);
	setDepthMode(NO_DEPTH);
	glClearDepth(1.0f);

	glClearColor(56.f/256.f * fade2, 60.f/256.f * fade2, 50.f/256.f * fade2, 1.0f);
	glClear((fadeActive ? 0 : GL_COLOR_BUFFER_BIT) | GL_DEPTH_BUFFER_BIT);

	float ctime = max(0.f, time);
	int cctime = clamp((int)(gMusic->getBeat() / 16) - int(SceneTime::cubewallStart / BEAT / 16), 0, 6); //6 Teile,Schluß

	vec3 eye(-1.5*((cctime & 2) ? -1.f : 1.f), 0.5, (cctime & 1) ? 1. : (2. + 0.95*cos(ctime*0.2) + ctime*ctime*0.005));
	vec3 center(0, -0.1 + 0.2*sin(time*0.15) + 0.5*atan(time*0.1), 0);
	vec3 up(0, 1, 0);
	mat4 view = lookAt(eye, center, up);
	mat4 proj = perspective<float>(30.f + 10.f*(float)cos(time*0.1), demo.aspect, 0.001f, 100.f);
	mat4 projView = proj*view;

	floorClearShader->bind();

	setBlendMode(BLEND_ALPHA);
	floorClearShader->uniform("u_fade", fadeIn);
	floorClearShader->uniform("u_matrix", projView);
	drawArray(GL_TRIANGLE_FAN, floorVerts, NULL, NULL, NULL, NULL, sizeof(floorVerts) / (3 * sizeof(float)));

	floorClearShader->unbind();


	setDepthMode(DEPTH_FULL);
	setBlendMode(NO_BLEND);

	cubeShader->bind();

	cubeShader->uniform("u_eye", eye);
	cubeShader->uniform("u_matrix", projView);

	float cubeScaleFade = clamp((Duration() - time - 4.0f) / 1.0f, 0.0f, 1.0f);

	//ab 179: Kötzchen weg
	float explosionTime = max(0.f, time - (179.f - SceneTime::cubewallStart));
	float explosionY = explosionTime*explosionTime;

	float cubeScale = 16.f / (float)textBitmapWidth;

	int cubeInstanceIndex = 0;

	for (int x = 0; x < textBitmapWidthL; x++)
	{
		for (int y = 0; y < textBitmapHeight; y++)
		{
			int cubeIndex = ((textBitmapHeight - 1) - y)*textBitmapWidth + x;
			int pixelPos = cubeIndex * 4;
			int   a = textBitmap[pixelPos + 3];
			if (a == 0) continue;
			float r = (textBitmap[pixelPos]   * a) * (1.f/(255.f*255.f));
			float g = (textBitmap[pixelPos+1] * a) * (1.f/(255.f*255.f));
			float b = (textBitmap[pixelPos+2] * a) * (1.f/(255.f*255.f));

			float random = randomValues[cubeIndex];
			float cubeShiftAmp = 1.f - fract(gMusic->getBeat());// fmod(time * 2 * 1.2f, 1.f);
			float cubeShift = (0.5f - random) * cubeShiftAmp * 2;

			vec4 color(r, g, b, 1.f);
			color *= (1. - (0.5*cubeShift) + cubeShift*random);
			float highlightRandom = randomValues[((textBitmapHeight - 1) - y)*textBitmapWidth + (x / 3 + (int)(gMusic->getBeat() / 4)) % (textBitmapWidth*textBitmapHeight)];
			float highlight = pow(highlightRandom, 10) * 10;
			color *= 1. + highlight;

			for (int mirror = 0; mirror < 2; mirror++)
			{
				vec3 pos(x*cubeScale + time*-0.65f + 1.5f, (y*cubeScale + 0.05f) * (mirror ? -1 : 1), cubeShift*0.5f*cubeScale);
				pos.y -= explosionY*(1.f+0.1f*random);
				pos.x += explosionTime*sin(10.f*random);

				cubePosScale[cubeInstanceIndex] = vec4(pos, cubeScaleFade * 0.4f*cubeScale);
				cubeCol     [cubeInstanceIndex] = color;
				cubeInstanceIndex++;

				if (cubeInstanceIndex == 510) //TODO: allokierter Buffer für alle Werte statt der nur 1024 Konstanten-Register
				{
					cubeShader->uniform("u_posScale", cubePosScale, cubeInstanceIndex);
					cubeShader->uniform("u_color", cubeCol, cubeInstanceIndex);
					drawArray(GL_TRIANGLE_STRIP, semicube, NULL, NULL, NULL, NULL, sizeof(semicube) / (3 * sizeof(float)), cubeInstanceIndex);
					cubeInstanceIndex = 0;
				}
			}
		}
	}

	cubeShader->uniform("u_posScale", cubePosScale, cubeInstanceIndex);
	cubeShader->uniform("u_color", cubeCol, cubeInstanceIndex);
	drawArray(GL_TRIANGLE_STRIP, semicube, NULL, NULL, NULL, NULL, sizeof(semicube) / (3 * sizeof(float)), cubeInstanceIndex);

	cubeShader->unbind();

	floorShader->bind();
	setBlendMode(BLEND_ALPHA);
	floorShader->uniform("u_eye", eye);
	floorShader->uniform("u_matrix", projView);
	floorShader->uniform("u_fade", fadeIn);
	drawArray(GL_TRIANGLE_FAN, floorVerts, NULL, NULL, NULL, NULL, sizeof(floorVerts) / (3 * sizeof(float)));
	floorShader->unbind();

	if (!fadeActive)
	{
		fboOverlay->unbind();

		setBlendMode(NO_BLEND);
		setCullMode(NO_CULL);
		setDepthMode(NO_DEPTH);

		kratzerShader->bind();
		kratzerShader->bindFbo("src", fboOverlay, 0);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		kratzerShader->bindTexture("tex", kratzer, 1);
		kratzerShader->uniform("u_fade", fade2);
		drawFullscreenQuad();
		kratzerShader->unbind();
	}

	setDepthMode(NO_DEPTH);
	setBlendMode(BLEND_ALPHA);
	setCullMode(NO_CULL);

	if (fadeIn >= 1.0f)
	{
		float creditsStart = 3.f;
		float creditsDuration = (SceneTime::lazorsStart - SceneTime::cubewallStart) - creditsStart;
		float creditTime = (time - creditsStart - 1.f) * 3.f / creditsDuration;
		float pos = floor(creditTime);
		creditsShader->bind();
		creditsShader->bindTexture("tex", credits, 0);
		creditsShader->uniform("fade", 2 * (creditTime - pos));
		drawRect(vec2(-1, -1 + 0.1185), vec2(1, -1 + 2 * 0.1185), vec2(0, (1. / 3.)*(pos + 1)), vec2(1, (1. / 3.)*pos));
		creditsShader->unbind();
	}
}
