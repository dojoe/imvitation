#ifndef _K_I_T_T_H
#define _K_I_T_T_H

#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS 1
#include <Windows.h>
#else
#include <errno.h>
#endif

#include <algorithm>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include <SOIL/SOIL.h>
#include <bass.h>
#include "antsy.h"

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <vector>
#include <set>
#include <map>
#include <string>

#ifdef _MSC_VER
inline int snprintf(char *str, size_t size, const char *format, ...)
{
	int ret;
	va_list va;

	va_start(va, format);
	ret = _vsnprintf_s(str, size, _TRUNCATE, format, va);
	va_end(va);

	return ret;
}
#endif

#if defined(WIN32) && defined(DEBUG)
#include "guicon.h"
#endif

using namespace glm;

typedef void(*loaderDrawFunc)(void *priv, float progress);
void loadBegin(loaderDrawFunc callback, void *priv, int expectedProgressCalls);
void loadProgress();
void loadEnd();

#ifdef DEBUG
#define LOG(msg, ...)  { loadProgress(); printf("%7.3f " msg, (float)glfwGetTime(), __VA_ARGS__); }
#else
#define LOG(...) loadProgress()
#endif

#define WRN LOG

void ERR(const char* fmt, ...);
const char *getGlErrorName(GLenum error);
void _checkGlError(const char *file, int line);
#define checkGlError() _checkGlError(__FILE__, __LINE__)

#define __STR(x) #x
#define STR(x) __STR(x)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))

const float PI = 3.14159265358979323846f;

extern unsigned long seed;

static void skrand(unsigned int s)
{
	seed = s;
}

static inline int krand()
{
	seed = seed * 214013L + 2531011L;
	return ((seed >> 16) & 0x7fff);
}

#define KRAND_MAX 0x7FFF

static inline float rand(float min, float max)
{
	return min + (float)krand() / KRAND_MAX * (max - min);
}

static inline int rand(int min, int max)
{
	return min + krand() % (max - min + 1);
}

static inline int randsign()
{
	return rand(0, 1) * 2 - 1;
}

static inline mat2 rotate(mat2 m, float angle)
{
	float a = angle * PI / 180, c = cos(a), s = sin(a);
	return mat2(c, -s, s, c) * m;
}

static inline mat2 scale(mat2 m, vec2 s)
{
	return mat2(s.x, 0.0f, 0.0f, s.y) * m;
}

class Scene {
public:
	Scene(const char *aName, float aStartTime, float aEndTime);
	virtual void draw(float time);
	float StartTime() { return startTime; };
	float EndTime() { return endTime;  }
	float Duration() { return endTime - startTime; }
protected:
	const char *name;
	void *TweakBar;
	virtual void _draw(float time) = 0;
private:
	float startTime, endTime;
	bool BarVisible;
};

class Demo {
public:
	int sizex, sizey;
	float aspect;     /* sizex / sizey */
	float inv_aspect; /* sizey / sizex */
	int xborder, yborder;

	Demo(int sx, int sy, float wantAspect) : scenes(NULL), aspect(wantAspect), inv_aspect(1.0f / wantAspect) { setSize(sx, sy);  }
	mat4 fix_aspect(mat4 m) { return scale(m, vec3(inv_aspect, 1.0f, 1.0f)); }
	void setScenes(Scene** demoScenes) { scenes = demoScenes;  }
	void draw(float time);
	Scene* getScene(int number);
	void setSize(int width, int height);
	void setViewport() { glViewport(xborder, yborder, sizex, sizey); }

private:
	Scene** scenes;
};

extern Demo demo;

enum blendMode { NO_BLEND, BLEND_ALPHA, BLEND_INVERT_ALPHA, BLEND_ADD_ALPHA, BLEND_ADD_AND_DIM };
enum depthMode { NO_DEPTH, DEPTH_READ, DEPTH_FULL };
enum cullMode  { NO_CULL, CULL_FRONT, CULL_BACK };

void setBlendMode(enum blendMode mode);
void setDepthMode(enum depthMode mode);
void setCullMode(enum cullMode mode);
void setTexFilter(int unit, int filter);
void drawArray(GLenum mode, float *verts, float *normals, float *texCoords, float *colors, GLuint *indices, size_t count, GLsizei numInstances = 1, int vertDims = 3);
void drawQuad(vec2 bl, vec2 br, vec2 tl, vec2 tr, vec2 tbl, vec2 ttr);
void drawParallelogram(vec2 bl, vec2 br, vec2 tl, vec2 tbl, vec2 ttr);
void drawRect(vec2 bl, vec2 tr, vec2 tbl, vec2 ttr);
void drawFullscreenQuad(bool flipY = true);
void drawUnitQuad();

class Fbo
{
public:
	Fbo(int width, int height, GLenum format, bool depthBuffer = true);
	~Fbo();

	GLuint getHandle() { return handle; }
	GLuint getColorTexture() { return cTex; }

	void bind();
	void unbind();

private:
	GLuint handle, cTex, dTex;
	GLsizei width, height;
	bool hasDepth;
	GLint prevFbo;
};

extern Fbo *gFullScreenFbo;

class File
{
public:
	File(const char *fname, bool loadAsString = false);
	~File();

	uint8_t *getData() { return data; }
	size_t getSize() { return size; }

private:
	uint8_t *data;
	size_t size;
};

class Texture {
public:
	Texture(const char *fname, int force_channels = SOIL_LOAD_AUTO, unsigned int flags = 0);
	GLuint tex;
	int width;
	int height;
	int channels;
	float aspect; /* width / height */
};

Texture *loadCacheTexture(std::string fname);

class GlslShader
{
public:
	GlslShader(const char *sourceFile);
	~GlslShader();

	void bindTexture(const char *name, GLuint tex, int texUnit);
	void bindFbo(const char *name, Fbo *fbo, int texUnit) {
		bindTexture(name, fbo->getColorTexture(), texUnit);
	}
	void bindTexture(const char *name, Texture *tex, int texUnit) {
		bindTexture(name, tex->tex, texUnit);
	}

	void uniform(const char *name, int value) {
		glUniform1i(uloc(name), value);
	}

	void uniform(const char *name, float value) {
		glUniform1f(uloc(name), value);
	}

	void uniform(const char *name, vec2 value) {
		glUniform2f(uloc(name), value.x, value.y);
	}

	void uniform(const char *name, vec3 value) {
		glUniform3f(uloc(name), value.x, value.y, value.z);
	}

	void uniform(const char *name, vec4 value) {
		glUniform4f(uloc(name), value.x, value.y, value.z, value.w);
	}

	void uniform(const char *name, mat4 value) {
		glUniformMatrix4fv(uloc(name), 1, GL_FALSE, &value[0][0]);
	}

	void uniform(const char *name, mat3 value) {
		glUniformMatrix3fv(uloc(name), 1, GL_FALSE, &value[0][0]);
	}

	void uniform(const char *name, mat2 value) {
		glUniformMatrix2fv(uloc(name), 1, GL_FALSE, &value[0][0]);
	}

	void uniform(const char *name, int *value, GLsizei count) {
		glUniform1iv(uloc(name), count, value);
	}

	void uniform(const char *name, float *value, GLsizei count) {
		glUniform1fv(uloc(name), count, value);
	}

	void uniform(const char *name, vec2 *value, GLsizei count) {
		glUniform2fv(uloc(name), count, &value->x);
	}

	void uniform(const char *name, vec3 *value, GLsizei count) {
		glUniform3fv(uloc(name), count, &value->x);
	}

	void uniform(const char *name, vec4 *value, GLsizei count) {
		glUniform4fv(uloc(name), count, &value->x);
	}

	void uniform(const char *name, mat4 *value, GLsizei count) {
		glUniformMatrix4fv(uloc(name), count, GL_FALSE, &(*value)[0][0]);
	}

	void uniform(const char *name, mat3 *value, GLsizei count) {
		glUniformMatrix3fv(uloc(name), count, GL_FALSE, &(*value)[0][0]);
	}

	void uniform(const char *name, mat2 *value, GLsizei count) {
		glUniformMatrix2fv(uloc(name), count, GL_FALSE, &(*value)[0][0]);
	}

	void bind();
	void unbind();

private:
	GLuint vs, gs, fs, prog;
	std::set<std::string> warnedUniforms;

	void compile(GLenum shader, const char *source, GLenum type);
	GLint uloc(const GLchar *name);
};

class Music {
public:
	Music(const char *fname);

	void play();
	void pause(bool pause);
	bool paused();
	bool stopped();
	float getTime();
	float getBeat();
	void setTime(float time);

private:
	static Music *instance;
	HSTREAM stream;
	float    startTime;
	float 	pausedTime;

	float& StartTime();
};

extern Music *gMusic;

class ObjLoader {
public:
	ObjLoader(const char *fname, float scale = 1.0f, bool blender = true, bool splitMaterials = false);
	vec3 *getVerts3fv(int oid = 0) {
		return &verts[ranges[oid].start];
	}
	float *getVerts(int oid = 0) {
		return value_ptr(*getVerts3fv(oid));
	}
	float *getColors(int oid = 0) {
		return colors.size() ? value_ptr(colors[ranges[oid].start]) : NULL;
	}
	vec2 *getUVs2fv(int oid = 0) {
		return uvs.size() ? &uvs[ranges[oid].start] : NULL;
	}
	float *getUVs(int oid = 0) {
		return uvs.size() ? value_ptr(uvs[ranges[oid].start]) : NULL;
	}
	float *getNormals(int oid = 0) {
		return normals.size() ? value_ptr(normals[ranges[oid].start]) : NULL;
	}
	size_t getCount(int oid = 0) {
		return ranges[oid].count;
	}
	void draw(int oid = 0) {
		drawArray(GL_TRIANGLES, getVerts(oid), getNormals(oid), getUVs(oid), getColors(oid), NULL, getCount(oid));
	}

private:
	typedef std::vector<vec3> vert_t;
	typedef std::vector<vec4> color_t;
	typedef std::vector<vec2> uv_t;
	typedef std::vector<vec3> normal_t;

	typedef struct { int start, count; } range_t;
	typedef std::vector<range_t> ranges_t;

	vert_t verts;
	color_t colors;
	uv_t uvs;
	normal_t normals;
	ranges_t ranges;
};

#endif
