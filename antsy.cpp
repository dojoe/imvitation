#include "k.i.t.t.h"
#include <GLFW/glfw3.h>
using namespace glm;

void AntsyInit(GLFWwindow *win)
{
	TwInit(TW_OPENGL, NULL);
	TwWindowSize(demo.sizex, demo.sizey);
	glfwSetMouseButtonCallback(win, (GLFWmousebuttonfun)TwEventMouseButtonGLFW3);
	glfwSetCursorPosCallback(win, (GLFWcursorposfun)TwEventCursorPosGLFW3);
	glfwSetScrollCallback(win, (GLFWscrollfun)TwEventScrollGLFW3);
	glfwSetCharModsCallback(win, (GLFWcharmodsfun)TwEventCharModsGLFW3);
}

void AntsyShow(void *bar, bool show)
{
	int32_t val = show;
	if (bar)
		TwSetParam((TwBar *)bar, NULL, "visible", TW_PARAM_INT32, 1, &val);
}

void *AntsyCreateBar(const char *name, bool iconified)
{
	TwBar *bar;
	int32_t val = 0;

	bar = TwNewBar(name);
	TwSetParam((TwBar *)bar, NULL, "visible", TW_PARAM_INT32, 1, &val);
	val = iconified;
	TwSetParam((TwBar *)bar, NULL, "iconified", TW_PARAM_INT32, 1, &val);

	return bar;
}

static void AntsyAddGeneric(void *bar, TwType type, void *var, const char *name, const char *args, const char *defArgs)
{
	char combinedArgs[500];
	sprintf(combinedArgs, "%s %s", defArgs, args ? args : "");
	TwAddVarRW((TwBar *)bar, name, type, var, combinedArgs);
}

void AntsyAdd(void *bar, float *var, const char *name, const char *args)
{
	AntsyAddGeneric(bar, TW_TYPE_FLOAT, var, name, args, "min=0 max=1 step=0.005");
}

void AntsyAdd(void *bar, int32_t *var, const char *name, const char *args)
{
	AntsyAddGeneric(bar, TW_TYPE_INT32, var, name, args, "");
}

void AntsyAdd(void *bar, uint32_t *var, const char *name, const char *args)
{
	AntsyAddGeneric(bar, TW_TYPE_UINT32, var, name, args, "");
}

void AntsyAdd(void *bar, bool *var, const char *name, const char *args)
{
	AntsyAddGeneric(bar, TW_TYPE_BOOLCPP, var, name, args, "");
}

void AntsyAdd(void *bar, vec3 *var, const char *name, const char *args)
{
	AntsyAddGeneric(bar, TW_TYPE_DIR3F, var, name, args, "");
}

void AntsyAdd(void *bar, vec4 *var, const char *name, const char *args)
{
	AntsyAddGeneric(bar, TW_TYPE_COLOR4F, var, name, args, "");
}

