#include "k.i.t.t.h"
#include "scenes.h"
#include <GLFW/glfw3.h>

#include <iomanip>
#include <sstream>

void handleGLFWerror(int error, const char *text)
{
	ERR("%s\n", text);
}

void KeyCallback(GLFWwindow* win, int key, int scan, int action, int modifier)
{
	if (AntsyHandleKey(win, key, scan, action, modifier))
		return;

	if (key == GLFW_KEY_ESCAPE)
	{
		glfwSetWindowShouldClose(win, TRUE);
	}
	if (((action == GLFW_PRESS) || (action == GLFW_REPEAT)) && gMusic)
	{
		switch (key)
		{
			case GLFW_KEY_RIGHT:
				gMusic->setTime(gMusic->getTime() + 1.0f);
				break;
			case GLFW_KEY_LEFT:
				gMusic->setTime(gMusic->getTime() - 1.0f);
				break;
			case GLFW_KEY_DOWN:
				gMusic->setTime(gMusic->getTime() + 10.0f);
				break;
			case GLFW_KEY_UP:
				gMusic->setTime(gMusic->getTime() - 10.0f);
				break;
			case GLFW_KEY_SPACE:
				gMusic->pause(!gMusic->paused());
				break;
			case GLFW_KEY_0:
			case GLFW_KEY_1:
			case GLFW_KEY_2:
			case GLFW_KEY_3:
			case GLFW_KEY_4:
			case GLFW_KEY_5:
			case GLFW_KEY_6:
			case GLFW_KEY_7:
			case GLFW_KEY_8:
			case GLFW_KEY_9:
			{
				int number = (key == GLFW_KEY_0) ? 9 : key - GLFW_KEY_1;
				Scene* scene = demo.getScene(number);
				if (scene)
				{
					gMusic->setTime(scene->StartTime());
				}
			}
		}
	}
}

void ResizeCallback(GLFWwindow* win, int width, int height)
{
	demo.setSize(width, height);
	demo.setViewport();
}

GlslShader *loadShader, *loadSplashShader;
Texture *loadSplash;

#define c(r, g, b) vec4((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, 1.0f)

vec4 palette[] = {
	c(0x00, 0x00, 0x00), c(0xFF, 0xFF, 0xFF), c(0xD0, 0x25, 0x12), c(0x7B, 0xD7, 0xE2),
	c(0xAF, 0x61, 0xE0), c(0x70, 0xB4, 0x45), c(0x62, 0x55, 0xDC), c(0xEC, 0xEC, 0x57),
	c(0xC8, 0x5A, 0x12), c(0x80, 0x5F, 0x07), c(0xF6, 0x9B, 0x8F), c(0x55, 0x55, 0x55),
	c(0x80, 0x80, 0x80), c(0xB9, 0xF2, 0x96), c(0xB1, 0xB3, 0xFF), c(0xAB, 0xAB, 0xAB),
};

void loadCallback(void *priv, float progress)
{
	const int maxh = 240;
	static int i = 0;
	int y = 0;

	float fade = (1.0f - smoothstep(0.8f, 1.0f, progress));

	loadShader->bind();
	loadShader->uniform("u_matrix", mat4(1.0f));
	while (y < maxh) {
		int h = rand(1, 30);
		float t = (float)y / maxh * 2.0f - 1.0f;
		float b = (float)(y + h) / maxh * 2.0f - 1.0f;

		loadShader->uniform("u_color", palette[i] * fade);
		drawRect(vec2(-1.0f, b), vec2(1.0f, t), vec2(0.0f, 0.0f), vec2(0.0f, 0.0f));

		y += h;
		i = (i + 1) % ARRAY_SIZE(palette);
	}
	loadShader->unbind();

	setBlendMode(BLEND_ALPHA);
	loadSplashShader->bind();
	loadSplashShader->uniform("u_alpha", fade);
	loadSplashShader->bindTexture("tex", loadSplash, 0);
	loadSplashShader->uniform("u_matrix", mat4(1));
	setTexFilter(0, GL_NEAREST);
	drawFullscreenQuad(true);
	setTexFilter(0, GL_LINEAR);
	loadSplashShader->unbind();
	setBlendMode(NO_BLEND);

	glfwSwapBuffers((GLFWwindow *)priv);
	glfwPollEvents();
}

namespace SceneTime
{
	float      picStart =   0.0f;
	float  foreverStart =  26.7f;
	float     parkStart =  2 * SECTION + 16 * BEAT;
	float      inkStart =  79.4f;
	float   rocketStart = 106.0f;
	float outStormStart = 113.0f;
	float cubewallStart = 5.25f * SECTION + 4 * BEAT;
	float   lazorsStart = 165.6f;
	float endSceneStart = 182.0f;
	float       demoEnd = 193.0f;

	float      pic2forever  =  2.0f;
	float  forever2park     =  3.0f;
	float     park2ink      = 10.0f;
	float      ink2outStorm =  6.0f;
	float outStorm2cubewall =  8 * BEAT;
	float cubewall2lazors   =  3.0f;
	float        lazors2end =  9.0f;
}

#ifdef WIN32
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#ifdef DEBUG
	RedirectIOToConsole();
#endif
#else
int main(int argc, char *argv[])
{
#endif
	if (!glfwInit())
		return 1;

	/*	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 3);
	glfwOpenWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	*/
	const float wantAspect = 16.0f / 9.0f;

#ifdef DEBUG
	demo = Demo(1280, 768, wantAspect);
	GLFWwindow *win = glfwCreateWindow(demo.sizex, demo.sizey, "Imvitation", NULL, NULL);
#else
	GLFWmonitor *monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode *mode = glfwGetVideoMode(monitor);
	glfwWindowHint(GLFW_RED_BITS, mode->redBits);
	glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
	glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
	glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
	GLFWwindow* win = glfwCreateWindow(mode->width, mode->height, "Imvitation", monitor, NULL);
	demo = Demo(mode->width, mode->height, wantAspect);
#endif
	if (!win)
		return 1;

	glfwMakeContextCurrent(win);
	GLenum err = glewInit();
	if (GLEW_OK != err)
	  ERR("GLEW error: %s\n", glewGetErrorString(err));

	demo.setViewport();
	AntsyInit(win);
	glfwSetKeyCallback(win, KeyCallback);
	glfwSetWindowSizeCallback(win, ResizeCallback);
#ifndef DEBUG
	glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_HIDDEN); //bevor jemand heult
#endif

	gFullScreenFbo = new Fbo(demo.sizex, demo.sizey, GL_RGBA8);

	loadShader = new GlslShader("data/glsl/flat.glsl");
	loadSplashShader = new GlslShader("data/glsl/flat+t.glsl");
	loadSplash = new Texture("data/splash.png");
	loadBegin(loadCallback, win, 106);

	using namespace SceneTime;

	Scene* scenes[] = {
		new      InkScene(inkStart - park2ink, outStormStart),
		new     ParkScene(parkStart - forever2park, inkStart),
		new  ForeverScene( foreverStart -      pic2forever,      parkStart),
		new PicturesScene(     picStart,                      foreverStart),
		new OutstormScene(outStormStart -      ink2outStorm, cubewallStart),
		new   RocketScene(  rocketStart,                     outStormStart),
		new CubewallScene(cubewallStart - outStorm2cubewall,   lazorsStart),
		new      EndScene(endSceneStart -   lazors2end,          demoEnd),
		new    LazorScene(lazorsStart   - cubewall2lazors,   endSceneStart),
		NULL //Terminator
	};
	loadEnd();

	demo.setScenes(scenes);

	checkGlError();

	gMusic = new Music("data/Skyrunner-Ein Nordlicht in der Finsternis.mp3");
	gMusic->play();

	while (!(glfwWindowShouldClose(win) || gMusic->stopped()))
	{
		float time = gMusic->getTime();

		setBlendMode(NO_BLEND);
		glDepthRange(0.0f, 1.0f);
		setCullMode(NO_CULL);
		setDepthMode(DEPTH_FULL);
		glClearDepth(1.0f);
		glClear(GL_DEPTH_BUFFER_BIT);
		setDepthMode(NO_DEPTH);

		demo.draw(time);

		checkGlError();

		AntsyDraw();
		glfwSwapBuffers(win);
		glfwPollEvents();

#ifdef DEBUG
		std::ostringstream title;
		title << "imvitation ";
		title << time;
		glfwSetWindowTitle(win, title.str().c_str());
#endif
	}

#ifdef WIN32
	/* Fix exit crash on AMD >.< */
	TerminateProcess(GetCurrentProcess(), EXIT_SUCCESS);
#endif
	return 0;
}
