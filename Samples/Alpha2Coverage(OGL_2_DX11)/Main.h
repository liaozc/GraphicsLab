#include "../Framework2/OpenGL/OpenGLApp.h"
#include "../Framework2/Util/OpenGLModel.h"
#include "../Framework2/Util/CollisionSet.h"

#include "../Framework2/Util/TexturePacker.h"

struct Vertex {
	vec3 pos;
	vec2 texCoord;
	vec2 texCoord2;
};

struct Quad {
	Quad(const vec3 &v0, const vec3 &v1, const vec3 &v2, const vec3 &v3, const float s, const float t, const TextureID tex, const int texSize){
		vertex[0] = v0;
		vertex[1] = v1;
		vertex[2] = v2;
		vertex[3] = v3;
		center = 0.25f * (v0 + v1 + v2 + v3);
		scaleS = s;
		scaleT = t;
		texture = tex;
		textureSize = vec2(float(texSize), float(texSize));
	}
	vec3 vertex[4];
	vec3 center;
	float d;
	float scaleS, scaleT;
	TextureID texture;
	vec2 textureSize;
};

class MainApp : public OpenGLApp {
public:
	void selectPixelFormat(PixelFormat &pf);
	void resetCamera();

	void initMenu();
	bool init();
	bool exit();

	bool load();
	bool unload();

	void drawScene();

	bool drawFrame();
protected:
	OpenGLModel *model;

	TextureID base[3], lightMap, masked0, masked1, masked2, masked3, derv;
	ShaderID alphaTweak;
	int mode;
	bool alphaContrast;
};
