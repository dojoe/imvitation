#ifndef _SCENES_H
#define _SCENES_H

#include "k.i.t.t.h"

static const float BPM = 145.0f;
static const float BEAT = 60.0f / BPM;
static const float SECTION = 64 * BEAT;

namespace SceneTime
{
	extern float      picStart;
	extern float  foreverStart;
	extern float     parkStart;
	extern float      inkStart;
	extern float   rocketStart;
	extern float outStormStart;
	extern float cubewallStart;
	extern float   lazorsStart;

	extern float      pic2forever;
	extern float  forever2park;
	extern float     park2ink;
	extern float      ink2outStorm;
	extern float outStorm2cubewall;
	extern float cubewall2lazors;
}

class PicturesScene : public Scene {
public:
	PicturesScene(float aStartTime, float aEndTime);
protected:
	virtual void _draw(float time);
};

class ForeverScene: public Scene {
public:
	ForeverScene(float aStartTime, float aEndTime);
protected:
	virtual void _draw(float time);
};

class InkScene : public Scene {
public:
	InkScene(float aStartTime, float aEndTime);
protected:
	virtual void _draw(float time);
};

class RocketScene : public Scene {
public:
	RocketScene(float aStartTime, float aEndTime);
protected:
	virtual void _draw(float time);
};

class CubewallScene : public Scene {
public:
	CubewallScene(float aStartTime, float aEndTime);
protected:
	virtual void _draw(float time);
};

class OutstormScene : public Scene {
public:
	OutstormScene(float aStartTime, float aEndTime);
protected:
	virtual void _draw(float time);
};

class ParkScene : public Scene {
public:
	ParkScene(float aStartTime, float aEndTime);
protected:
	virtual void _draw(float time);
};

class LazorScene : public Scene {
public:
	LazorScene(float aStartTime, float aEndTime);
protected:
	virtual void _draw(float time);
};

class EndScene : public Scene {
public:
	EndScene(float aStartTime, float aEndTime);
protected:
	virtual void _draw(float time);
};

#endif