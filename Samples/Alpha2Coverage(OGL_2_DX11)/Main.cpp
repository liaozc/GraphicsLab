#include "Main.h"

void MainApp::selectPixelFormat(PixelFormat &pf){
	pf.fsaaLevel = 6;
}

void MainApp::resetCamera(){
	position = vec3(-500, -64, 0);
	wx = 0;
	wy = -PI / 2;
	wz = 0;
}

void MainApp::initMenu(){
	Menu *menu = menuSystem->getMainMenu();

	static char *options[] = { "Alpha blend", "Alpha to coverage", "Alpha testing" };

	mode = 1;
	alphaContrast = true;

	menu->addMenuItem("Mode: ", &mode, INPUT_INTEGER, 0, 2, options);
	menu->addMenuItem("Alpha contrast: ", &alphaContrast, INPUT_BOOL);

	App::initMenu();
}

bool MainApp::init(){
	unsigned int i, j;

	model = new OpenGLModel();
	model->loadFromFile("../Models/Corridor/Map.hmdl");

	for (i = 0; i < model->getBatchCount(); i++){
		model->getBatch(i)->insertAttribute(ATT_TEXCOORD, ATT_FLOAT, 2, 1);
	}

	TexturePacker texPacker;
	for (i = 0; i < model->getBatchCount(); i++){
		Batch *batch = model->getBatch(i);
		Vertex *src = (Vertex *) batch->getVertices();
		unsigned short *inds = (unsigned short *) batch->getIndices();

		for (j = 0; j < batch->getIndexCount(); j += 6){
			float w = length(src[inds[j + 1]].pos - src[inds[j]].pos) / 18;
			float h = length(src[inds[j + 5]].pos - src[inds[j]].pos) / 18;

			if (w < 8) w = 8;
			if (h < 8) h = 8;

			unsigned int tw = (int) w;
			unsigned int th = (int) h;

			tw = (tw + 4) & ~7;
			th = (th + 4) & ~7;

			texPacker.addRectangle(tw, th);
		}
	}

	unsigned int tw = 256;
	unsigned int th = 256;

	if (!texPacker.assignCoords(&tw, &th)){
#ifdef _WIN32
		MessageBox(NULL, "Lightmap too small", "Error", MB_OK);
#endif
		return false;
	}

	tw = getUpperPowerOfTwo(tw);
	th = getUpperPowerOfTwo(th);

	unsigned int index = 0;
	for (i = 0; i < model->getBatchCount(); i++){
		Batch *batch = model->getBatch(i);
		Vertex *src = (Vertex *) batch->getVertices();
		unsigned short *inds = (unsigned short *) batch->getIndices();

		for (j = 0; j < batch->getIndexCount(); j += 6){
			TextureRectangle *rect = texPacker.getRectangle(index);

			float x0 = float(rect->x + 0.5f) / tw;
			float x1 = float(rect->x + rect->width - 0.5f) / tw;
			float y0 = float(rect->y + 0.5f) / th;
			float y1 = float(rect->y + rect->height - 0.5f) / th;

			src[inds[j    ]].texCoord2 = vec2(x0, y0);
			src[inds[j + 1]].texCoord2 = vec2(x1, y0);
			src[inds[j + 2]].texCoord2 = vec2(x1, y1);
			src[inds[j + 5]].texCoord2 = vec2(x0, y1);

			index++;
		}
	}

	speed = 500;
	return true;
}

bool MainApp::exit(){
	delete model;
	return true;
}

bool MainApp::load(){
	if (!GL_ARB_multisample_supported){
		addToLog("No multisample support (GL_ARB_multisample)\n");
		return false;
	}
	if (!GL_ARB_shader_objects_supported || !GL_ARB_vertex_shader_supported || !GL_ARB_fragment_shader_supported || !GL_ARB_shading_language_100_supported){
		addToLog("No GLSL support (GL_ARB_shader_objects, GL_ARB_vertex_shader, GL_ARB_fragment_shader, GL_ARB_shading_language_100)\n");
		return false;
	}

	if ((alphaTweak = renderer->addShader("alphaTweak.shd")) == TEXTURE_NONE) return false;

	setDefaultFont("../Textures/Fonts/Future.font", "../Textures/Fonts/Future.dds");

	if ((base[0] = renderer->addTexture("../Textures/floor_wood_3.dds")) == TEXTURE_NONE) return false;
	if ((base[1] = renderer->addTexture("../Textures/floor6.dds"      )) == TEXTURE_NONE) return false;
	if ((base[2] = renderer->addTexture("../Textures/floor_wood_4.dds")) == TEXTURE_NONE) return false;

	if ((lightMap = renderer->addTexture("LightMap.dds", TEX_NOMIPMAPPING | TEX_NOANISO)) == TEXTURE_NONE) return false;

	if ((masked0 = renderer->addTexture("../Textures/fence.dds"  )) == TEXTURE_NONE) return false;
	if ((masked1 = renderer->addTexture("../Textures/grate1.dds" )) == TEXTURE_NONE) return false;
	if ((masked2 = renderer->addTexture("../Textures/grate4b.dds")) == TEXTURE_NONE) return false;
	if ((masked3 = renderer->addTexture("../Textures/grate.dds"  )) == TEXTURE_NONE) return false;

	model->uploadToVertexBuffer();

#define SIZE 64

	Image *img = new Image();
	unsigned char *dest, *pixels = new unsigned char[SIZE * SIZE * 4 / 3];
	img->loadFromMemory(pixels, SIZE, SIZE, FORMAT_I8, true, true);

	int level = 0;
	while (dest = img->getImage(level)){
		int size = (SIZE >> level);

		memset(dest, size, size * size);

		level++;
	}
		

	if ((derv = renderer->addTexture(img, TEX_NOANISO)) == TEXTURE_NONE) return false;


	return true;
}

bool MainApp::unload(){
	return true;
}

int quadComp(const void *elem0, const void *elem1){
	return int(((Quad *) elem1)->d - ((Quad *) elem0)->d);
}

bool MainApp::drawFrame(){
	glMatrixMode(GL_PROJECTION);
	projection = projectionMatrixX(1.5f, float(height) / float(width), 1, 4000);
	glLoadMatrixf(transpose(projection));

	glMatrixMode(GL_MODELVIEW);
	modelView = rotateXY(-wx, -wy) * translate(-position);
	glLoadMatrixf(transpose(modelView));

	renderer->changeMask(ALL);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glColor4f(1, 1, 1, 1);
	glEnable(GL_CULL_FACE);
	for (unsigned int i = 0; i < model->getBatchCount(); i++){
		renderer->setTextures(base[i], lightMap);
		renderer->apply();

		((OpenGLBatch *) model->getBatch(i))->draw();
	}
	glDisable(GL_CULL_FACE);


	Quad quads[] = {
		Quad(vec3(-384, 192,  288), vec3(-128, 192,  288), vec3(-128, -192,  288), vec3(-384, -192,  288), 3, 4, masked0, 512),
		Quad(vec3(-384, 192, -288), vec3(-128, 192, -288), vec3(-128, -192, -288), vec3(-384, -192, -288), 3, 4, masked0, 512),
		Quad(vec3( 528, 192, -128), vec3( 528, 192,  128), vec3( 528, -192,  128), vec3( 528, -192, -128), 3, 4, masked0, 512),
		Quad(vec3( 560, 192, -128), vec3( 560, 192,  128), vec3( 560, -192,  128), vec3( 560, -192, -128), 3, 4, masked0, 512),
		Quad(vec3( 384, 192,  288), vec3( 128, 192,  288), vec3( 128, -192,  288), vec3( 384, -192,  288), 3, 4, masked1, 1024),
		Quad(vec3( 384, 192, -288), vec3( 128, 192, -288), vec3( 128, -192, -288), vec3( 384, -192, -288), 3, 4, masked1, 1024),
		Quad(vec3( 960, 192,  288), vec3( 704, 192,  288), vec3( 704, -192,  288), vec3( 960, -192,  288), 3, 4, masked2, 128),
		Quad(vec3( 960, 192, -288), vec3( 704, 192, -288), vec3( 704, -192, -288), vec3( 960, -192, -288), 3, 4, masked2, 128),
		Quad(vec3(-384, 288,  128), vec3(-128, 288,  128), vec3(-128,  288, -128), vec3(-384,  288, -128), 1, 1, masked3, 256),
		Quad(vec3( 384, 288,  128), vec3( 128, 288,  128), vec3( 128,  288, -128), vec3( 384,  288, -128), 1, 1, masked3, 256),
	};
	const int nQuads = sizeof(quads) / sizeof(Quad);

	if (mode == 0){
		for (int i = 0; i < nQuads; i++){
			vec3 dir = position - quads[i].center;
			quads[i].d = dot(dir, dir);
		}
		qsort(quads, nQuads, sizeof(Quad), quadComp);
	} else if (mode == 1){
		glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB);
	} else {
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.5f);
	}

	for (int q = 0; q < nQuads; q++){
		if (alphaContrast){
			renderer->setShader(alphaTweak);
			renderer->setTexture("Derv", derv);
			renderer->setTexture("Base", quads[q].texture);
		} else {
			renderer->setTextures(quads[q].texture);
		}
		if (mode == 0){
			renderer->setBlending(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			renderer->setMask(COLOR);
		}
		renderer->apply();

		if (alphaContrast) renderer->changeShaderConstant2f("texSize", quads[q].textureSize);

		glBegin(GL_QUADS);
		for (unsigned int k = 0; k < 4; k++){
			glTexCoord2f(quads[q].scaleS * (k > 0 && k < 3), quads[q].scaleT * (k > 1));
			glVertex3fv(quads[q].vertex[k]);
		}
		glEnd();
	}

	if (mode == 1){
		glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB);
	} else if (mode == 2){
		glDisable(GL_ALPHA_TEST);
	}

	return true;
}

App *app = new MainApp();
