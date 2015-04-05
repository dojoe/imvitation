#include "k.i.t.t.h"
#include "scenes.h"

static ObjLoader *oPark, *oInkBlotOverlay;
static ObjLoader *oCamPath, *oLookPath;
static Texture *tPark[5];
static GlslShader *sDrawPhong, *sDrawFlat, *sDrawOverlay, *sPark[ARRAY_SIZE(tPark)];

static vec3 endCamPos(-9.710229f, 12.94f, 30.271885f), endLookAt(-9.2, 7.414384, 30.6), endUp(-0.47, 0, 0.92);
static float endFov = 9.515f;
static float endBlendStart = 0.95f;
static float ovlBlendTime = 0.125f;

ParkScene::ParkScene(float aStartTime, float aEndTime) : Scene("park", aStartTime, aEndTime)
{
	oPark = new ObjLoader("data/park/park.obj", 0.1f, false, true);
	oInkBlotOverlay = new ObjLoader("data/park/inkblot.obj", 0.1f, false);
	oCamPath = new ObjLoader("data/park/campath.obj", 0.1f, false);
	oLookPath = new ObjLoader("data/park/lookpath.obj", 0.1f, false);
	tPark[0] = loadCacheTexture("data/park/skybox.jpg");
	tPark[1] = loadCacheTexture("data/park/kasten_innen.jpg");
	tPark[2] = loadCacheTexture("data/park/tonne.jpg");
	tPark[3] = loadCacheTexture("data/park/fleisch.jpg");
	tPark[4] = loadCacheTexture("data/park/grill.jpg");
	sDrawPhong = new GlslShader("data/glsl/phong+t.glsl");
	sDrawFlat = new GlslShader("data/glsl/flat+t.glsl");
	sDrawOverlay = new GlslShader("data/glsl/flat4+t.glsl");
	sPark[0] = sPark[1] = sPark[2] = sPark[3] = sPark[4] = sDrawFlat;

	/* I couldn't use blender if my life depended on it, so I just fix up the inkblot UVs in code >.> */
	for (unsigned i = 0; i < oInkBlotOverlay->getCount(); i++) {
		vec2 &uv = oInkBlotOverlay->getUVs2fv()[i];
		uv.x = 1 - uv.x;
		uv.y = 1 - uv.y;
	}

	TWEAK_INIT();
	TWEAK(endCamPos);
	TWEAK(endLookAt);
	TWEAK(endUp);
	TWEAK(endBlendStart);
	TWEAK(ovlBlendTime);
	TWEAKA(endFov, "max=60");
}

static float myx(float bla)
{
	return pow(clamp(bla - endBlendStart, 0.0f, 1.0f) / (1.0f - endBlendStart), 1.5f);
}

static inline vec3 interpolatePath(ObjLoader *path, vec3 alt, float pos)
{
	float vertIdx = pos * (path->getCount() - 1);
	vec3 *verts = path->getVerts3fv() + int(floor(vertIdx));
	return mix(mix(verts[0], verts[1], fract(vertIdx)), alt, myx(pos));
}

static void blendOverlay(ObjLoader *ovl, mat4 m, float t)
{
	vec4 pos[100];
	
	for (unsigned i = 0; i < ovl->getCount(); i++) {
		vec4 pos3D = m * vec4(ovl->getVerts3fv()[i], 1);
		vec2 uv = ovl->getUVs2fv()[i];
		vec4 pos2D = vec4(uv.x * 2 - 1, uv.y * 2 - 1, -1, 1);
		pos[i] = mix(pos3D, pos2D, t);
	}

	drawArray(GL_TRIANGLES, value_ptr(pos[0]), NULL, ovl->getUVs(), NULL, NULL, ovl->getCount(), 1, 4);
}

void ParkScene::_draw(float time)
{
	float camTime = time / Duration();
	mat4 mPersp = perspective(mix(45.0f, endFov, myx(camTime)), demo.aspect, 0.5f, 1000.0f);
	mat4 mCamera = lookAt(interpolatePath(oCamPath, endCamPos * 0.1f, camTime), interpolatePath(oLookPath, endLookAt * 0.1f, camTime), mix(vec3(0, 1, 0), endUp, myx(camTime)));

	setDepthMode(DEPTH_FULL);
	for (int i = 0; i < ARRAY_SIZE(tPark); i++) {
		GlslShader *sDraw = sPark[i];
		sDraw->bind();
		sDraw->uniform("u_matrix", mPersp * mCamera);
		sDraw->uniform("u_modelview", mCamera);
		sDraw->uniform("u_projection", mPersp);
		sDraw->uniform("u_alpha", 1.0f);
		sDraw->bindTexture("tex", tPark[i], 0);
		oPark->draw(i);
		sDraw->unbind();
	}
	if (time > Duration() - SceneTime::park2ink) {
		setBlendMode(BLEND_ALPHA);
		sDrawOverlay->bind();
		sDrawOverlay->uniform("u_matrix", mat4(1));
		sDrawOverlay->uniform("u_modelview", mCamera);
		sDrawOverlay->uniform("u_projection", mPersp);
		sDrawOverlay->bindFbo("tex", gFullScreenFbo, 0);
		blendOverlay(oInkBlotOverlay, mPersp * mCamera, pow(clamp((time - Duration() + ovlBlendTime) / ovlBlendTime, 0.0f, 1.0f), 2.0f));
		sDrawOverlay->unbind();
		setBlendMode(NO_BLEND);
	}
	setDepthMode(NO_DEPTH);
}