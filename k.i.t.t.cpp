#include "k.i.t.t.h"
#include "scenes.h"

Demo demo(1, 1, 1.0f);
unsigned long seed = 1;

Fbo *gFullScreenFbo;
Music *gMusic;

void MESSAGE(bool exitOnError, const char* fmt, va_list args)
{
#if !defined(DEBUG) && defined(WIN32)
	char str[1000];
	vsnprintf(str, sizeof(str), fmt, args);
	MessageBoxA(NULL, str, "Error", MB_ICONEXCLAMATION | MB_OK);
#else
	printf("%7.3f ", (float)glfwGetTime());
	vprintf(fmt, args);
#endif
	if (exitOnError)
	{
#if defined(WIN32) && defined(DEBUG)
		PutConsoleToFront();
		printf("Press Return to exit...");
		getchar();
#endif
		exit(1);
	}
}

void ERR(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	MESSAGE(true, fmt, args);
	va_end(args);
}

const char *getGlErrorName(GLenum error)
{
	switch (error)
	{
	case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
	case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
	case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
	case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
	case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
	case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
	case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
	default: return "???";
	}
}

void _checkGlError(const char *file, int line)
{
	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
		ERR("%s:%d:GL error %d (%s)\n", file, line, error, getGlErrorName(error));
}

void Demo::draw(float time)
{
	Scene** scene = scenes;
	while (*scene)
	{
		(*scene)->draw(time);
		++scene;
	}
}

Scene* Demo::getScene(int number)
{
	for (int idx = 0; scenes[idx]; idx++)
	{
		if (idx == number)
		{
			return scenes[idx];
		}
	}
	return NULL;
}

void Demo::setSize(int width, int height)
{
	sizey = height;
	sizex = int(height * aspect);

	if (width < sizex) {
		sizex = width;
		sizey = int(width * inv_aspect);
		xborder = 0;
		yborder = (height - sizey) / 2;
	}
	else {
		xborder = (width - sizex) / 2;
		yborder = 0;
	}
}

Fbo::Fbo(int aWidth, int aHeight, GLenum format, bool depthBuffer) : width(aWidth), height(aHeight), hasDepth(depthBuffer)
{
	// color texture
	glGenTextures(1, &cTex);
	glBindTexture(GL_TEXTURE_2D, cTex);
	glTexStorage2D(GL_TEXTURE_2D, 1, format, width, height);
	glBindTexture(GL_TEXTURE_2D, 0);

	checkGlError();

	if (depthBuffer) {
		// depth buffer
		glGenRenderbuffers(1, &dTex);
		glBindRenderbuffer(GL_RENDERBUFFER, dTex);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		checkGlError();
	}

	// framebuffer object
	glGenFramebuffers(1, &handle);
	glBindFramebuffer(GL_FRAMEBUFFER, handle);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, cTex, 0);
	if (depthBuffer)
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, dTex);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	checkGlError();
}

Fbo::~Fbo()
{
	glDeleteFramebuffers(1, &handle);
	if (hasDepth)
		glDeleteRenderbuffers(1, &dTex);
	glDeleteTextures(1, &cTex);
}

void Fbo::bind()
{
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
	glPushAttrib(GL_VIEWPORT_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, handle);
	glViewport(0, 0, width, height);
}

void Fbo::unbind()
{
	glPopAttrib();
	glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);
}

File::File(const char *fname, bool loadAsString) : data(NULL), size(0)
{
	FILE *f = fopen(fname, "rb");
	if (!f)
		ERR("open failed: %s\n", strerror(errno));

	fseek(f, 0, SEEK_END);
	size_t fsize = ftell(f);
	fseek(f, 0, SEEK_SET);

	size = fsize + (loadAsString ? 1 : 0);

	data = (uint8_t *)malloc(size);
	if (!data)
		return;

	if (fsize != fread(data, 1, fsize, f))
		ERR("wat?\n");

	if (loadAsString)
		data[size-1] = 0;

	fclose(f);
}

File::~File()
{
	if (data)
		free(data);
}

Scene::Scene(const char *aName, float aStartTime, float aEndTime) :
	startTime(aStartTime), endTime(aEndTime), name(aName), BarVisible(false)
{
	LOG("Init %s scene\n", name);
}

void Scene::draw(float time)
{
	if ((time > startTime) && (time < endTime)) {
		setBlendMode(NO_BLEND);
		setDepthMode(NO_DEPTH);
		setCullMode(NO_CULL);

		setTexFilter(0, GL_LINEAR);
		_draw(time - startTime);
		if (!BarVisible) {
			AntsyShow(TweakBar, true);
			BarVisible = true;
		}
	}
	else {
		if (BarVisible) {
			AntsyShow(TweakBar, false);
			BarVisible = false;
		}
	}
}

static inline const char *getShaderType(GLenum type)
{
	switch (type)
	{
	case GL_VERTEX_SHADER:   return "vertex";
	case GL_FRAGMENT_SHADER: return "fragment";
	case GL_GEOMETRY_SHADER: return "geometry";
	default:                 return "??";
	}
}

void GlslShader::compile(GLenum shader, const char *source, GLenum type)
{
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (success != GL_TRUE)
	{
		GLint errlen;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &errlen);
		char *error = (char *)malloc(errlen);
		glGetShaderInfoLog(shader, errlen, &errlen, error);
		ERR("Error compiling %s shader:\n%s\n", getShaderType(type), error);
	}
}

GlslShader::GlslShader(const char *sourceFile)
{
	LOG("Loading shader %s\n", sourceFile);

	File source(sourceFile, true);
	if (!source.getData())
		abort();

	char *vsSource = (char *)source.getData();
	char *gsSource = strstr(vsSource, "//__GEOMETRY_SHADER");
	if (gsSource)
		ERR("No Geometry Shader support yet, poo.\n");

	char *fsSource = strstr(vsSource, "//__FRAGMENT_SHADER__");
	if (!fsSource)
		ERR("No Fragment Shader found\n");

	*fsSource = 0;
	fsSource += 21;

	prog = glCreateProgram();
	vs   = glCreateShader(GL_VERTEX_SHADER);
	gs   = 0;
	fs   = glCreateShader(GL_FRAGMENT_SHADER);

	compile(vs, vsSource, GL_VERTEX_SHADER);
	compile(fs, fsSource, GL_FRAGMENT_SHADER);

	glAttachShader(prog, vs);
	if (gs)
		glAttachShader(prog, gs);
	glAttachShader(prog, fs);
	glLinkProgram(prog);

	GLint success;
	glGetProgramiv(prog, GL_LINK_STATUS, &success);
	if (success != GL_TRUE)
	{
		GLint errlen;
		glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &errlen);
		char *error = (char *)malloc(errlen);
		glGetProgramInfoLog(prog, errlen, &errlen, error);
		ERR("Error linking program:\n%s\n", error);
	}
}


GlslShader::~GlslShader()
{
	glDeleteProgram(prog);
	glDeleteShader(fs);
	if (gs)
		glDeleteShader(gs);
	glDeleteShader(vs);
}

void GlslShader::bindTexture(const char *name, GLuint tex, int texUnit)
{
	glActiveTexture(GL_TEXTURE0 + texUnit);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex);
	uniform(name, texUnit);
}

void GlslShader::bind()
{
	glUseProgram(prog);
}

void GlslShader::unbind()
{
	glUseProgram(0);
}

GLint GlslShader::uloc(const GLchar *name)
{
	GLint loc = glGetUniformLocation(prog, name);
	if (loc < 0) {
		std::string ns(name);
		if (!warnedUniforms.count(ns)) {
			warnedUniforms.insert(ns);
			WRN("Uniform '%s' not in program\n", name);
		}
	}
	return loc;
}

Music *Music::instance = NULL;

Music::Music(const char *fname)
{
	pausedTime = 0;
	startTime  = 0;

	if (instance)
		ERR("Can only init Music once\n");

	instance = this;

	if (!BASS_Init(-1, 44100, 0, NULL, NULL))
		ERR("Failed to initialize BASS\n");

	stream = BASS_StreamCreateFile(0, fname, 0, 0, BASS_MP3_SETPOS);
	if (!stream)
		WRN("Failed to open music %s\n", fname);
}

void Music::play()
{
	BASS_ChannelPlay(instance->stream, false);
	pausedTime = 0;
}

void Music::pause(bool pause)
{
	if ((!pause) && (pausedTime != 0))
	{
		setTime(pausedTime);
		pausedTime = 0;
	}
	else
	{
		pausedTime = getTime();
	}

	if (pause)
	{
		BASS_ChannelPause(instance->stream);
	}
	else
	{
		BASS_ChannelPlay(instance->stream, false);
	}
}

bool Music::paused()
{
	return stream ? BASS_ChannelIsActive(instance->stream) == BASS_ACTIVE_PAUSED : (pausedTime != 0);
}

bool Music::stopped()
{
	return stream ? BASS_ChannelIsActive(instance->stream) == BASS_ACTIVE_STOPPED : (getTime() > 500 * 60); //TODO: Echte Länge angeben
}

float Music::getTime()
{
	if (!instance->stream)
	{
		if (pausedTime != 0)
		{
			return pausedTime;
		}
		return ((float)glfwGetTime() - StartTime());
	}
	return (float)BASS_ChannelBytes2Seconds(instance->stream, BASS_ChannelGetPosition(stream, BASS_POS_BYTE));
}

float Music::getBeat()
{
	return getTime() / BEAT;
}

void Music::setTime(float time)
{
	if (time <= 0)
	{
		time = 0;
	}
	if (pausedTime != 0)
	{
		pausedTime = time;
	}
	if (!instance->stream)
	{
		StartTime() = ((float)glfwGetTime() - time);
	}
	BASS_ChannelSetPosition(stream, BASS_ChannelSeconds2Bytes(stream, time), BASS_POS_BYTE);
}

float& Music::StartTime()
{
	if (!startTime)
	{
		startTime = (float)glfwGetTime();
	}
	return startTime;
}

ObjLoader::ObjLoader(const char *fname, float scale, bool blender, bool splitMaterials)
{
	LOG("Loading object %s\n", fname);

	FILE *f = fopen(fname, "r");
	if (!f)
		ERR("Failed to open %s: %s\n", fname, strerror(errno));

	vec4 curColor(1.0f);
	float x, y, z;
	GLuint v1, t1, n1, v2, t2, n2, v3, t3, n3;

	vert_t vert_tmp;
	color_t col_tmp;
	uv_t uv_tmp;
	normal_t normal_tmp;
	bool color = false;

#define addVert(v, t, n) \
	verts.push_back(vert_tmp[v - 1]); \
	if (color) \
		colors.push_back(col_tmp[v - 1]); \
	if (t) \
		uvs.push_back(uv_tmp[t - 1]); \
	if (n) \
		normals.push_back(normal_tmp[n - 1]);

#define addFace(v1, t1, n1, v2, t2, n2, v3, t3, n3) \
	addVert(v1, t1, n1); addVert(v2, t2, n2); addVert(v3, t3, n3);

	int lineno = 0;
	int range_start = 0;
	char str[1000], mtl[100];
	while (fgets(str, sizeof(str), f)) {
		lineno++;

		if (sscanf(str, "v %f %f %f", &x, &y, &z) == 3) {
			if (blender)
				vert_tmp.push_back(vec3(x * scale, z * -scale, y * scale));
			else
				vert_tmp.push_back(vec3(x * scale, y * scale, z * scale));
			if (color)
				col_tmp.push_back(curColor);
		}
		else if (sscanf(str, "vt %f %f", &x, &y) == 2) {
			uv_tmp.push_back(vec2(x, y));
		}
		else if (sscanf(str, "vn %f %f %f", &x, &y, &z) == 3) {
			if (blender)
				normal_tmp.push_back(vec3(x, -z, y));
			else
				normal_tmp.push_back(vec3(x, y, z));
		}
		else if (sscanf(str, "Kd %f %f %f", &x, &y, &z) == 3) {
			color = true;
			curColor = vec4(x, y, z, 1.0f);
		}
		else if (sscanf(str, "f %u %u %u", &v1, &v2, &v3) == 3) {
			addFace(v1, 0, 0, v2, 0, 0, v3, 0, 0);
		}
		else if (sscanf(str, "f %u//%u  %u//%u  %u//%u",
						&v1, &n1, &v2, &n2, &v3, &n3) == 6) {
			addFace(v1, 0, n1, v2, 0, n2, v3, 0, n3);
		}
		else if (sscanf(str, "f %u/%u  %u/%u  %u/%u",
						&v1, &t1, &v2, &t2, &v3, &t3) == 6) {
			addFace(v1, t1, 0, v2, t2, 0, v3, t3, 0);
		}
		else if (sscanf(str, "f %u/%u/%u  %u/%u/%u  %u/%u/%u",
						&v1, &t1, &n1, &v2, &t2, &n2, &v3, &t3, &n3) == 9) {
			addFace(v1, t1, n1, v2, t2, n2, v3, t3, n3);
		}
		else if (splitMaterials && (sscanf(str, "usemtl %s", mtl) == 1) && (verts.size() != range_start)) {
			range_t range = { range_start, verts.size() - range_start };
			ranges.push_back(range);
			range_start = verts.size();
		}
	}

	if (!verts.size() && vert_tmp.size()) {
		/* No faces -- use vertices as they are */
		verts = vert_tmp;
		normals = normal_tmp;
		uvs = uv_tmp;
		colors = col_tmp;
	}

	if (verts.size() != range_start) {
		range_t range = { range_start, verts.size() - range_start };
		ranges.push_back(range);
		range_start = verts.size();
	}

	if (colors.size() && colors.size() != verts.size() ||
		uvs.size() && uvs.size() != verts.size() ||
		normals.size() && normals.size() != verts.size())
		ERR("OBJ count mismatch - %u verts, %u colors, %u uvs, %u normals\n",
		verts.size(), colors.size(), uvs.size(), normals.size());

	fclose(f);
}

Texture::Texture(const char *fname, int force_channels, unsigned int flags)
{
	LOG("Loading texture %s\n", fname);

	uint8_t *data = SOIL_load_image(fname, &width, &height, &channels, force_channels);
	if (!data) {
		WRN("Failed to load %s: %s\n", fname, SOIL_last_result());
		return;
	}

	aspect = (float)width / height;

	tex = SOIL_create_OGL_texture(data, width, height, channels, SOIL_CREATE_NEW_ID, flags);
	SOIL_free_image_data(data);
	if (!tex)
		WRN("Failed to create texture for %s: %s\n", fname, SOIL_last_result());
}

Texture *loadCacheTexture(std::string fname)
{
	typedef std::map<std::string, Texture *> cache_t;
	static cache_t texCache;
	cache_t::const_iterator it = texCache.find(fname);
	if (it == texCache.end()) {
		Texture *tex = new Texture(fname.c_str(), SOIL_LOAD_AUTO, SOIL_FLAG_INVERT_Y);
		texCache.insert(cache_t::value_type(fname, tex));
		return tex;
	}
	else
		return it->second;
}

void setBlendMode(enum blendMode mode)
{
	switch (mode) {
	case NO_BLEND:
		glDisable(GL_BLEND);
		break;
	case BLEND_ALPHA:
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	case BLEND_ADD_ALPHA:
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		break;
	case BLEND_INVERT_ALPHA:
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
		break;
	case BLEND_ADD_AND_DIM:
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	}
}

void setDepthMode(enum depthMode mode)
{
	switch (mode) {
	case NO_DEPTH:
		glDisable(GL_DEPTH_TEST);
		break;
	case DEPTH_READ:
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_FALSE);
		break;
	case DEPTH_FULL:
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		break;
	}
}

void setCullMode(enum cullMode mode)
{
	switch (mode) {
	case NO_CULL:
		glDisable(GL_CULL_FACE);
		break;
	case CULL_FRONT:
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		break;
	case CULL_BACK:
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		break;
	}
}

void setTexFilter(int unit, int filter)
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
}

void drawArray(GLenum mode, float *verts, float *normals, float *texCoords, float *colors, GLuint *indices, size_t count, GLsizei numInstances, int vertDims)
{

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, vertDims, GL_FLOAT, GL_FALSE, 0, verts);

	if (normals)
	{
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, normals);
	}

	if (texCoords)
	{
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, texCoords);
	}

	if (colors)
	{
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, colors);
	}

	if (indices) glDrawElements(mode, count, GL_UNSIGNED_INT, indices);
	else if (numInstances == 1)
	{
		glDrawArrays(mode, 0, count);
	}
	else
	{
		glDrawArraysInstanced(mode, 0, count, numInstances);
	}

	glDisableVertexAttribArray(0);
	if (normals) glDisableVertexAttribArray(1);
	if (texCoords) glDisableVertexAttribArray(2);
	if (colors) glDisableVertexAttribArray(3);
}

void drawQuad(vec2 bl, vec2 br, vec2 tl, vec2 tr, vec2 tbl, vec2 ttr)
{
	const vec2 EPSILON(0.001f, -0.001f);
	tbl += EPSILON; ttr -= EPSILON;
	vec3 verts[4] = { vec3(bl, 0.0f), vec3(br, 0.0f), vec3(tr, 0.0f), vec3(tl, 0.0f) };
	vec2 texCoords[4] = { tbl, vec2(ttr.x, tbl.y), ttr, vec2(tbl.x, ttr.y) };
	drawArray(GL_QUADS, value_ptr(verts[0]), NULL, value_ptr(texCoords[0]), NULL, NULL, 4);
}

void drawParallelogram(vec2 bl, vec2 br, vec2 tl, vec2 tbl, vec2 ttr)
{
	drawQuad(bl, br, tl, br + tl - bl, tbl, ttr);
}

void drawRect(vec2 bl, vec2 tr, vec2 tbl, vec2 ttr)
{
	drawQuad(bl, vec2(tr.x, bl.y), vec2(bl.x, tr.y), tr, tbl, ttr);
}

void drawFullscreenQuad(bool flipY)
{
	if (flipY)
		drawRect(vec2(-1.0f, -1.0f), vec2(1.0f, 1.0f), vec2(0.0f, 1.0f), vec2(1.0f, 0.0f));
	else
		drawRect(vec2(-1.0f, -1.0f), vec2(1.0f, 1.0f), vec2(0.0f, 0.0f), vec2(1.0f, 1.0f));
}

void drawUnitQuad()
{
	drawRect(vec2(-0.5f, -0.5f), vec2(0.5f, 0.5f), vec2(0.0f, 0.0f), vec2(1.0f, 1.0f));
}

static loaderDrawFunc loadDrawFunc = NULL;
static void *loadFuncPriv;
static bool isLoading = false;
static int numProgressCalls, maxProgressCalls;

void loadBegin(loaderDrawFunc callback, void *priv, int expectedProgressCalls)
{
	isLoading = true;
	numProgressCalls = 0;
	maxProgressCalls = expectedProgressCalls;
	loadDrawFunc = callback;
	loadFuncPriv = priv;

	loadDrawFunc(loadFuncPriv, 0.0f);
}

void loadProgress()
{
	if (!isLoading)
		return;
	numProgressCalls++;
	loadDrawFunc(loadFuncPriv, min(1.0f, (float)numProgressCalls / maxProgressCalls));
}

void loadEnd()
{
	isLoading = false;
	LOG("got %u progress calls\n", numProgressCalls);
}
