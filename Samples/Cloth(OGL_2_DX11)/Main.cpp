#include "../Framework2/OpenGL/OpenGLApp.h"
#include "../Framework2/Util/SpringSystem.h"
#include "../Framework2/Math/PerlinNoise.h"

#include "../Framework2/Util/OpenGLModel.h"

#if defined(LINUX)
#include <dirent.h>
#define min(x, y) ((x < y)? x : y)
#endif

#define CLOTH_SIZE_X 54
#define CLOTH_SIZE_Y 42
#define C_SIZE (320.0f / CLOTH_SIZE_X)

#define SPHERE_LEVEL 4
#define POW4(x) (1 << (2 * (x)))
#define SPHERE_SIZE (8 * 3 * POW4(SPHERE_LEVEL))

struct Vertex {
	vec3 pos;
	vec3 normal;
	vec2 texCoord;
};


class MainApp : public OpenGLApp {
public:
	void resetCamera();

	bool init();
	bool exit();

	bool load();
	bool unload();

	void drawLight(const vec3 &lightPos);
	void drawSphere(const vec3 &lightPos, const vec3 &spherePos, const float size, const vec3 &color);
	void drawCloth(const vec3 &lightPos);
	bool drawFrame();
protected:
	SpringSystem spring;

	Vertex vertices[CLOTH_SIZE_Y][CLOTH_SIZE_X];
	unsigned short indices[CLOTH_SIZE_X * 2];
	vec3 *sphere;

	float nextTime;
	unsigned int nSpheres;
	vec3 spherePos[3];
	float sphereSize[3];
	vec3 sphereColor[3];

	ShaderID lighting, sphereShader;

	OpenGLModel *model;

	TextureID base, light;
	Set <TextureID> flags;

	bool supportsGLSL;
};

void MainApp::resetCamera(){
	position = vec3(120, 80, -190);
	wx = 0.48f;
	wy = 0.55f;
	wz = 0;
}

void subDivide(vec3 *&dest, const vec3 &v0, const vec3 &v1, const vec3 &v2, int level){
	if (level){
		vec3 v3 = normalize(v0 + v1);
		vec3 v4 = normalize(v1 + v2);
		vec3 v5 = normalize(v2 + v0);

		subDivide(dest, v0, v3, v5, level - 1);
		subDivide(dest, v3, v4, v5, level - 1);
		subDivide(dest, v3, v1, v4, level - 1);
		subDivide(dest, v5, v4, v2, level - 1);
	} else {
		*dest++ = v0;
		*dest++ = v1;
		*dest++ = v2;
	}
}

bool MainApp::init(){
	initPerlin();

	sphere = new vec3[SPHERE_SIZE];
	vec3 *dest = sphere;

	subDivide(dest, vec3(0, 1,0), vec3( 0,0, 1), vec3( 1,0, 0), SPHERE_LEVEL);
	subDivide(dest, vec3(0, 1,0), vec3( 1,0, 0), vec3( 0,0,-1), SPHERE_LEVEL);
	subDivide(dest, vec3(0, 1,0), vec3( 0,0,-1), vec3(-1,0, 0), SPHERE_LEVEL);
	subDivide(dest, vec3(0, 1,0), vec3(-1,0, 0), vec3( 0,0, 1), SPHERE_LEVEL);

	subDivide(dest, vec3(0,-1,0), vec3( 1,0, 0), vec3( 0,0, 1), SPHERE_LEVEL);
	subDivide(dest, vec3(0,-1,0), vec3( 0,0, 1), vec3(-1,0, 0), SPHERE_LEVEL);
	subDivide(dest, vec3(0,-1,0), vec3(-1,0, 0), vec3( 0,0,-1), SPHERE_LEVEL);
	subDivide(dest, vec3(0,-1,0), vec3( 0,0,-1), vec3( 1,0, 0), SPHERE_LEVEL);

	model = new OpenGLModel();
	OpenGLBatch *batch = new OpenGLBatch();

	batch->addFormat(ATT_VERTEX, ATT_FLOAT, 3, 0);
	batch->addFormat(ATT_NORMAL, ATT_FLOAT, 3, 0); // Normal = Vertex
	batch->setPrimitiveType(PRIM_TRIANGLES);
	batch->setVertices(sphere, SPHERE_SIZE, sizeof(vec3));
	batch->optimize(SPHERE_SIZE);
	model->addBatch(batch);

	for (unsigned int j = 0; j < CLOTH_SIZE_X; j++){
		indices[2 * j] = CLOTH_SIZE_X + j;
		indices[2 * j + 1] = j;
	}

	nextTime = 0;
	srand((unsigned int) cpuHz);

	return true;
}

bool MainApp::exit(){
	delete model;
	return true;
}

bool MainApp::load(){
	supportsGLSL = GL_ARB_shader_objects_supported && GL_ARB_vertex_shader_supported && GL_ARB_fragment_shader_supported && GL_ARB_shading_language_100_supported;

	if (supportsGLSL){
		if ((lighting = renderer->addShader("lighting.shd")) == SHADER_NONE) return false;
		if ((sphereShader = renderer->addShader("sphere.shd")) == SHADER_NONE) return false;
	} else {
		addToLog("No GLSL support (GL_ARB_shader_objects, GL_ARB_vertex_shader, GL_ARB_fragment_shader, GL_ARB_shading_language_100)\n");
	}
	
	setDefaultFont("../Textures/Fonts/Future.font", "../Textures/Fonts/Future.dds");

	if ((light = renderer->addTexture("../Textures/Particle.tga", TEX_CLAMP)) == TEXTURE_NONE) return false;

#if defined(_WIN32)

	int result;
	WIN32_FIND_DATA findFileData;
	HANDLE handle = FindFirstFile("../Textures/Flags/*.*", &findFileData);
	if (handle != INVALID_HANDLE_VALUE){
		do {
			if (findFileData.cFileName[0] != '.'){
				TextureID tex = renderer->addTexture(String("../Textures/Flags/") + findFileData.cFileName, TEX_CLAMP);
				if (tex != TEXTURE_NONE) flags.add(tex);
			}
			result = FindNextFile(handle, &findFileData);
		} while (result > 0);
		FindClose(handle);
	}

#elif defined(LINUX)

	DIR *dir = opendir("../Textures/Flags/");
	if (dir != NULL){
		dirent *entry;
		while ((entry = readdir(dir)) != NULL){
			if (entry->d_name[0] != '.'){
				TextureID tex = renderer->addTexture(String("../Textures/Flags/") + entry->d_name, TEX_CLAMP);
				if (tex != TEXTURE_NONE) flags.add(tex);
			}
		}
	}

#endif

	if (flags.getCount() == 0) return false;

	//if (!supportsGLSL) model->getBatch(0)->addNormals();

	((OpenGLBatch *) model->getBatch(0))->uploadToVertexBuffer();

	return true;
}

bool MainApp::unload(){
	((OpenGLBatch *) model->getBatch(0))->freeVertexBuffer();
	return true;
}

void MainApp::drawLight(const vec3 &lightPos){
	renderer->setBlending(GL_ONE, GL_ONE);
	renderer->setTextures(light);
	renderer->apply();

	glColor3f(1,1,1);

	vec3 dx(modelView.elem[0][0], modelView.elem[0][1], modelView.elem[0][2]);
	vec3 dy(modelView.elem[1][0], modelView.elem[1][1], modelView.elem[1][2]);
	glBegin(GL_QUADS);
		glTexCoord2f(0,0);
		glVertex3fv(lightPos - 15 * dx + 15 * dy);

		glTexCoord2f(1,0);
		glVertex3fv(lightPos + 15 * dx + 15 * dy);

		glTexCoord2f(1,1);
		glVertex3fv(lightPos + 15 * dx - 15 * dy);

		glTexCoord2f(0,1);
		glVertex3fv(lightPos - 15 * dx - 15 * dy);
	glEnd();
}

void MainApp::drawSphere(const vec3 &lightPos, const vec3 &spherePos, const float size, const vec3 &color){
	if (supportsGLSL){
		renderer->setShader(sphereShader);
		renderer->apply();

		renderer->changeShaderConstant3f("spherePos", spherePos);
		renderer->changeShaderConstant1f("sphereSize", size);
		renderer->changeShaderConstant3f("lightPos", lightPos);
		renderer->changeShaderConstant3f("camPos", position);
		renderer->changeShaderConstant4f("color", vec4(color, 0.0f));

		model->draw();
	} else {
		renderer->apply();

		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);

		glLightfv(GL_LIGHT0, GL_POSITION, vec4(lightPos, 1.0f));

		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  vec4(0.15f * color, 1.0f)); 
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  vec4(color, 0.0f)); 
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, vec4(0.7f, 0.7f, 0.7f, 1.0f)); 
		glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 16.0f);
		glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);

		if (GL_EXT_separate_specular_color_supported){
			glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL_EXT, GL_SEPARATE_SPECULAR_COLOR_EXT);
		}

	    glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glTranslatef(spherePos.x, spherePos.y, spherePos.z);
		glScalef(size, size, size);

		glEnable(GL_NORMALIZE);

		model->draw();

		glDisable(GL_NORMALIZE);
		glDisable(GL_LIGHTING);

		glPopMatrix();
	}
}

void MainApp::drawCloth(const vec3 &lightPos){
	if (supportsGLSL){
		renderer->setShader(lighting);
		renderer->setTexture("Base", base);
		renderer->apply();

		renderer->changeShaderConstant3f("lightPos", lightPos);
		renderer->changeShaderConstant3f("camPos", position);
	} else {
		renderer->setTextures(base);
		renderer->apply();

		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);

		glLightfv(GL_LIGHT0, GL_POSITION, vec4(lightPos, 1.0f));

		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  vec4(0.15f, 0.15f, 0.15f, 1.0f)); 
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  vec4(0.80f, 0.80f, 0.80f, 1.0f)); 
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, vec4(0.40f, 0.40f, 0.40f, 1.0f)); 
		glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 16.0f);
		glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);

		if (GL_EXT_separate_specular_color_supported){
			glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL_EXT, GL_SEPARATE_SPECULAR_COLOR_EXT);
		}
	}

	glClientActiveTextureARB(GL_TEXTURE0_ARB);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	for (unsigned int i = 0; i < CLOTH_SIZE_Y - 1; i++){
		char *base = ((char *) vertices) + i * CLOTH_SIZE_X * sizeof(Vertex);
		glVertexPointer  (3, GL_FLOAT, sizeof(Vertex), base);
		glNormalPointer  (   GL_FLOAT, sizeof(Vertex), base + sizeof(vec3));
		glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), base + sizeof(vec3) * 2);

		glDrawElements(GL_TRIANGLE_STRIP, CLOTH_SIZE_X * 2, GL_UNSIGNED_SHORT, indices);
	}

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	if (!supportsGLSL) glDisable(GL_LIGHTING);
}

bool MainApp::drawFrame(){
	static int cornerInds[] = { 0, CLOTH_SIZE_X - 1, CLOTH_SIZE_X * CLOTH_SIZE_Y - 1, (CLOTH_SIZE_Y - 1) * CLOTH_SIZE_X };
	unsigned int i, j;
	static float sum;

	renderer->changeMask(ALL);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	projection = projectionMatrixX(1.5f, float(height) / float(width), 0.1f, 6000);
	glLoadMatrixf(transpose(projection));

	modelView = rotateZXY(-wx, -wy, -wz);
	modelView.translate(-position);

    glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(transpose(modelView));

	vec3 lightPos = vec3(C_SIZE * CLOTH_SIZE_X * noise1(0.5f * time), 80 + 80 * noise1(0.3f * time + 23.37f), C_SIZE * CLOTH_SIZE_X * noise1(12.31f - 0.5f * time));

	if (time >= nextTime || ((nextTime - time < 41) && sum < 5) || sum > 9800){
		nextTime = time + 45;

		for (j = 0; j < CLOTH_SIZE_Y; j++){
			for (i = 0; i < CLOTH_SIZE_X; i++){
				vertices[j][i].pos = vec3(C_SIZE * (i - 0.5f * (CLOTH_SIZE_X - 1)), 0, C_SIZE * (0.5f * (CLOTH_SIZE_Y - 1) - j));
				vertices[j][i].texCoord = vec2(float(i) / (CLOTH_SIZE_X - 1), float(j) / (CLOTH_SIZE_Y - 1));
			}
		}

		static bool first = true;
		if (first){
			spring.addRectField(CLOTH_SIZE_X, CLOTH_SIZE_Y, vertices, ((char *) vertices) + sizeof(vec3), sizeof(Vertex));
			first = false;
		}

		for (i = 0; i < spring.getNodeCount(); i++){
			spring.getNode(i)->dir = vec3(0, 0, 0);
		}

		nSpheres = rand() & 3;
		for (i = 0; i < nSpheres; i++){
			sphereSize[i] = 60 + 40 * float(rand()) / RAND_MAX;
			spherePos[i] = vec3(0.8f * C_SIZE * CLOTH_SIZE_X * (float(rand()) / RAND_MAX - 0.5f), -sphereSize[i] - 50 * float(rand()) / RAND_MAX, 0.8f * C_SIZE * CLOTH_SIZE_Y * (float(rand()) / RAND_MAX - 0.5f));
			sphereColor[i] = vec3(float(rand()) / RAND_MAX, float(rand()) / RAND_MAX, float(rand()) / RAND_MAX);
		}

		for (i = 0; i < 4; i++) spring.getNode(cornerInds[i])->locked = false;
		if (nSpheres == 0 || (rand() > RAND_MAX / 2)){
			unsigned int pos = rand() & 3;
			unsigned int n = 2 + (rand() % 3);

			for (i = 0; i < n; i++){
				spring.getNode(cornerInds[(pos + i) & 3])->locked = true;
			}
		}
		base = flags[rand() % flags.getCount()];
	}

	spring.update(min(frameTime, 0.0125f));

	sum = 0;
	for (i = 0; i < spring.getNodeCount(); i++){
		SNode *node = spring.getNode(i);

		bool include = true;
		for (j = 0; j < nSpheres; j++){
			vec3 v = *node->pos - spherePos[j];
			float dSqr = dot(v, v);

			if (dSqr < sphereSize[j] * sphereSize[j]){
				*node->pos = spherePos[j] + sphereSize[j] * normalize(v);//rsqrtf(dSqr) * v;
				node->dir *= powf(0.015f, frameTime);
				include = false;
			}
		}
		if (include) sum += dot(node->dir, node->dir);
	}
	sum /= (CLOTH_SIZE_X * CLOTH_SIZE_Y);

	spring.evaluateNormals();


	glEnable(GL_CULL_FACE);
	for (j = 0; j < nSpheres; j++){
		drawSphere(lightPos, spherePos[j], sphereSize[j] - 1, sphereColor[j]);
	}

	for (j = 0; j < 4; j++){
		SNode *node = spring.getNode(cornerInds[j]);

		if (node->locked) drawSphere(lightPos, *node->pos, 5.0f, vec3(0.3f, 0.3f, 1.0f));
	}



	glDisable(GL_CULL_FACE);
	drawCloth(lightPos);


	drawLight(lightPos);

/*
	String str;
	str.sprintf("%d", (int) sum);

	renderer->setTextures(textTexture);
	renderer->apply();

	drawText((char *) (const char *) str, 0.3f, 0.05f, 0.03f, 0.05f);
*/
	return true;
}

App *app = new MainApp();
