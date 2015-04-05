#ifndef _ANTSY_H
#define _ANTSY_H

#ifdef DEBUG

#include <inttypes.h>
#include <glm/glm.hpp>

# ifdef WIN32
# define GLFW_CDECL
# endif
#include <AntTweakBar.h>

void AntsyInit(GLFWwindow *window);
#define AntsyHandleKey TwEventKeyGLFW3
#define AntsyDraw TwDraw

void AntsyShow(void *bar, bool show);
void *AntsyCreateBar(const char *name, bool iconified = true);

void AntsyAdd(void *bar, float *var, const char *name, const char *args);
void AntsyAdd(void *bar, int32_t *var, const char *name, const char *args);
void AntsyAdd(void *bar, uint32_t *var, const char *name, const char *args);
void AntsyAdd(void *bar, bool *var, const char *name, const char *args);
void AntsyAdd(void *bar, glm::vec3 *var, const char *name, const char *args);
void AntsyAdd(void *bar, glm::vec4 *var, const char *name, const char *args);

#define TWEAK_BUTTON(name, callback, param) TwAddButton((TwBar *)TweakBar, name, callback, param, NULL)
#define TWEAKSEP() TwAddSeparator((TwBar *)TweakBar, NULL, NULL)

#else

#define TW_CALL

#define AntsyInit(...)
#define AntsyHandleKey(...) 0
#define AntsyDraw(...)
#define AntsyShow(...)
#define AntsyCreateBar(...) NULL
#define AntsyAdd(...)

#define TWEAK_BUTTON(...)
#define TWEAKSEP(...)

#endif

#define TWEAK_INIT(...) TweakBar = AntsyCreateBar(name, __VA_ARGS__)
#define TWEAK(var) AntsyAdd(TweakBar, &var, STR(var), NULL)
#define TWEAKA(var, args) AntsyAdd(TweakBar, &var, STR(var), args)

#endif
