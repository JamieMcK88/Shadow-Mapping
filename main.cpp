// MD2 animation renderer
// This demo will load and render an animated MD2 model, an OBJ model and a skybox
// Most of the OpenGL code for dealing with buffer objects, etc has been moved to a 
// utility library, to make creation and display of mesh objects as simple as possible

// Windows specific: Uncomment the following line to open a console window for debug output
#if _DEBUG
#pragma comment(linker, "/subsystem:\"console\" /entry:\"WinMainCRTStartup\"")
#endif

#include "rt3d.h"
#include "rt3dObjLoader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stack>
#include "md2model.h"

#include <SDL_ttf.h>

using namespace std;

#define DEG_TO_RADIAN 0.017453293

// Globals
// Real programs don't use globals :-D

GLuint meshIndexCount = 0;
GLuint toonIndexCount = 0;
GLuint md2VertCount = 0;
GLuint meshObjects[3];

GLuint textureProgram;
GLuint shaderProgram;
GLuint skyboxProgram;
GLuint shadowProgram;
GLuint depthProgram;
GLuint quad_programID;

GLuint depthMap;
GLuint depthMapFBO;

GLuint quad_vertexbuffer;

glm::mat4 lightSpaceMatrix;
glm::mat4 depthBiasMVP;

const GLuint SHADOW_WIDTH = 1024;
const GLuint SHADOW_HEIGHT = 1024;

GLuint screenHeight = 600;
GLuint screenWidth = 800;

float counter = 4.7f; //variable used to update time for explosion effect

int shaderSelection1 = 1; //integer used to switch between programs

GLfloat r = 0.0f;

glm::vec3 eye(-4.0f, 3.0f, 8.0f);
glm::vec3 at(0.0f, 1.0f, -1.0f);
glm::vec3 up(0.0f, 1.0f, 0.0f);

stack<glm::mat4> mvStack; 

// TEXTURE STUFF
GLuint textures[3];
GLuint skybox[5];
GLuint labels[5];

rt3d::lightStruct light0 = {
	{0.4f, 0.4f, 0.4f, 1.0f}, // ambient
	{1.0f, 1.0f, 1.0f, 1.0f}, // diffuse
	{1.0f, 1.0f, 1.0f, 1.0f}, // specular
	{-5.0f, 2.0f, 2.0f, 1.0f}  // position
};
glm::vec4 lightPos(0.5f, 2.0f, 2.0f, 1.0); //light position
glm::vec4 cubePos(-5.0f, 2.0f, 2.0f, 1.0f); //cube position
glm::vec3 lightPos2(0.0f, 0.0f, 0.0f);

rt3d::materialStruct material0 = {
	{0.2f, 0.4f, 0.2f, 1.0f}, // ambient
	{0.5f, 1.0f, 0.5f, 1.0f}, // diffuse
	{0.0f, 0.1f, 0.0f, 1.0f}, // specular
	2.0f  // shininess
};
rt3d::materialStruct material1 = {
	{0.4f, 0.4f, 1.0f, 1.0f}, // ambient
	{0.8f, 0.8f, 1.0f, 1.0f}, // diffuse
	{0.8f, 0.8f, 0.8f, 1.0f}, // specular
	1.0f  // shininess
};

// light attenuation
float attConstant = 1.0f;
float attLinear = 0.0f;
float attQuadratic = 0.0f;
float theta = 0.0f;


// Set up rendering context
SDL_Window * setupRC(SDL_GLContext &context) {
	SDL_Window * window;
    if (SDL_Init(SDL_INIT_VIDEO) < 0) // Initialize video
        rt3d::exitFatalError("Unable to initialize SDL"); 
	  
    // Request an OpenGL 3.0 context.
	
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE); 

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);  // double buffering on
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8); // 8 bit alpha buffering
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4); // Turn on x4 multisampling anti-aliasing (MSAA)
 
    // Create 800x600 window
    window = SDL_CreateWindow("SDL/GLM/OpenGL Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN );
	if (!window) // Check window was created OK
        rt3d::exitFatalError("Unable to create window");
 
    context = SDL_GL_CreateContext(window); // Create opengl context and attach to window
    SDL_GL_SetSwapInterval(1); // set swap buffers to sync with monitor's vertical refresh rate
	return window;
}

// A simple texture loading function
// lots of room for improvement - and better error checking!
GLuint loadBitmap(char *fname) {
	GLuint texID;
	glGenTextures(1, &texID); // generate texture ID

	// load file - using core SDL library
 	SDL_Surface *tmpSurface;
	tmpSurface = SDL_LoadBMP(fname);
	if (!tmpSurface) {
		std::cout << "Error loading bitmap" << std::endl;
	}

	// bind texture and set parameters
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	SDL_PixelFormat *format = tmpSurface->format;
	
	GLuint externalFormat, internalFormat;
	if (format->Amask) {
		internalFormat = GL_RGBA;
		externalFormat = (format->Rmask < format-> Bmask) ? GL_RGBA : GL_BGRA;
	}
	else {
		internalFormat = GL_RGB;
		externalFormat = (format->Rmask < format-> Bmask) ? GL_RGB : GL_BGR;
	}

	glTexImage2D(GL_TEXTURE_2D,0,internalFormat,tmpSurface->w, tmpSurface->h, 0,
		externalFormat, GL_UNSIGNED_BYTE, tmpSurface->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);

	SDL_FreeSurface(tmpSurface); // texture loaded, free the temporary buffer
	return texID;	// return value of texture ID
}


// A simple cubemap loading function
// lots of room for improvement - and better error checking!
GLuint loadCubeMap(const char *fname[6], GLuint *texID)
{
	glGenTextures(1, texID); // generate texture ID
	GLenum sides[6] = { GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
						GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
						GL_TEXTURE_CUBE_MAP_POSITIVE_X,
						GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
						GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
						GL_TEXTURE_CUBE_MAP_NEGATIVE_Y	};
	SDL_Surface *tmpSurface;

	glBindTexture(GL_TEXTURE_CUBE_MAP, *texID); // bind texture and set parameters
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	GLuint externalFormat;
	for (int i=0;i<6;i++)
	{
		// load file - using core SDL library
		tmpSurface = SDL_LoadBMP(fname[i]);
		if (!tmpSurface)
		{
			std::cout << "Error loading bitmap" << std::endl;
			return *texID;
		}

		// skybox textures should not have alpha (assuming this is true!)
		SDL_PixelFormat *format = tmpSurface->format;
		externalFormat = (format->Rmask < format-> Bmask) ? GL_RGB : GL_BGR;

		glTexImage2D(sides[i],0,GL_RGB,tmpSurface->w, tmpSurface->h, 0,
							externalFormat, GL_UNSIGNED_BYTE, tmpSurface->pixels);
		// texture loaded, free the temporary buffer
		SDL_FreeSurface(tmpSurface);
	}
	return *texID;	// return value of texure ID, redundant really
}


void init(void) {

	shaderProgram = rt3d::initShaders("phong-tex.vert","phong-tex.frag");
	rt3d::setLight(shaderProgram, light0);
	rt3d::setMaterial(shaderProgram, material0);

	// set light attenuation shader uniforms
	GLuint uniformIndex = glGetUniformLocation(shaderProgram, "attConst");
	glUniform1f(uniformIndex, attConstant);
	uniformIndex = glGetUniformLocation(shaderProgram, "attLinear");
	glUniform1f(uniformIndex, attLinear);
	uniformIndex = glGetUniformLocation(shaderProgram, "attQuadratic");
	glUniform1f(uniformIndex, attQuadratic);

	textureProgram = rt3d::initShaders("textured.vert", "textured.frag");
	skyboxProgram = rt3d::initShaders("cubeMap.vert","cubeMap.frag");

	shadowProgram = rt3d::initShaders("shadowMap.vert", "shadowMap.frag");
	depthProgram = rt3d::initShaders("depth.vert", "depth.frag");
	quad_programID = rt3d::initShaders("passthrough.vert", "passthrough.frag");

	/*
	const char *cubeTexFiles[6] = {
		"Town-skybox/Town_bk.bmp", "Town-skybox/Town_ft.bmp", "Town-skybox/Town_rt.bmp", "Town-skybox/Town_lf.bmp", "Town-skybox/Town_up.bmp", "Town-skybox/Town_dn.bmp"
	};
	loadCubeMap(cubeTexFiles, &skybox[0]);
	*/

	vector<GLfloat> verts;
	vector<GLfloat> norms;
	vector<GLfloat> tex_coords;
	vector<GLuint> indices;
	rt3d::loadObj("cube.obj", verts, norms, tex_coords, indices);
	meshIndexCount = indices.size();
	//textures[0] = loadBitmap("fabric.bmp");
	meshObjects[0] = rt3d::createMesh(verts.size()/3, verts.data(), nullptr, nullptr, nullptr, /*norms.data(), tex_coords.data(),*/ meshIndexCount, indices.data());

	//textures[2] = loadBitmap("studdedmetal.bmp");

	
	//verts.clear(); norms.clear();tex_coords.clear();indices.clear();
	//rt3d::loadObj("bunny-5000.obj", verts, norms, tex_coords, indices);
	//toonIndexCount = indices.size();
	//meshObjects[2] = rt3d::createMesh(verts.size()/3, verts.data(), nullptr, norms.data(), nullptr, toonIndexCount, indices.data());


	//// set tex sampler to texture unit 0, arbitrarily
	//uniformIndex = glGetUniformLocation(geo, "texMap");
	//glUniform1i(uniformIndex, 0);
	
	// Now bind textures to texture units
	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, textures[2]);
	
	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);


	// The quad's FBO. Used only for visualizing the shadowmap.
	static const GLfloat g_quad_vertex_buffer_data[] = {
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		1.0f,  1.0f, 0.0f,
	};

	
	glGenBuffers(1, &quad_vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), g_quad_vertex_buffer_data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	///////////////////////////Shadow setup//////////////////////////////////////
	glGenFramebuffers(1, &depthMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);

	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthMap, 0);
	glDrawBuffer(GL_NONE);

	// Always check that our framebuffer is ok
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "FBO incomplete" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	///////////////////////////Shadow//////////////////////////////////////
	glClearColor(0.5f, 0.0f, 0.0f, 1.0f);
}

glm::vec3 moveForward(glm::vec3 pos, GLfloat angle, GLfloat d) {
	return glm::vec3(pos.x + d*std::sin(r*DEG_TO_RADIAN), pos.y, pos.z - d*std::cos(r*DEG_TO_RADIAN));
}

glm::vec3 moveRight(glm::vec3 pos, GLfloat angle, GLfloat d) {
	return glm::vec3(pos.x + d*std::cos(r*DEG_TO_RADIAN), pos.y, pos.z + d*std::sin(r*DEG_TO_RADIAN));
}

void update(void) {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if ( keys[SDL_SCANCODE_W] ) eye = moveForward(eye,r,0.1f);
	if ( keys[SDL_SCANCODE_S] ) eye = moveForward(eye,r,-0.1f);
	if ( keys[SDL_SCANCODE_A] ) eye = moveRight(eye,r,-0.1f);
	if ( keys[SDL_SCANCODE_D] ) eye = moveRight(eye,r,0.1f);
	if ( keys[SDL_SCANCODE_R] ) eye.y += 0.1;
	if ( keys[SDL_SCANCODE_F] ) eye.y -= 0.1;

	if ( keys[SDL_SCANCODE_I] ) lightPos[2] -= 0.1;
	if ( keys[SDL_SCANCODE_J] ) lightPos[0] -= 0.1;
	if ( keys[SDL_SCANCODE_K] ) lightPos[2] += 0.1;
	if ( keys[SDL_SCANCODE_L] ) lightPos[0] += 0.1;
	if (keys[SDL_SCANCODE_U]) lightPos[1] += 0.1;
	if (keys[SDL_SCANCODE_H]) lightPos[1] -= 0.1;

	if (keys[SDL_SCANCODE_1]) shaderSelection1 = 2;  //selects gemoetry shader program
	if (keys[SDL_SCANCODE_2]) counter += 0.05f; //increases the effect of time in explosion
	if (keys[SDL_SCANCODE_3]) counter = 4.7f;
	if (keys[SDL_SCANCODE_4]) shaderSelection1 = 3;

	if ( keys[SDL_SCANCODE_COMMA] ) r -= 1.0f;
	if ( keys[SDL_SCANCODE_PERIOD] ) r += 1.0f;
}

void ConfigureShaderAndMatrices()
{
	float near_plane = -10.0f, far_plane = 20.0f;
	glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);

	glm::mat4 lightView = glm::lookAt(glm::vec3(lightPos),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));

	glm::mat4 depthModelMatrix = glm::mat4(1.0);

	lightSpaceMatrix = lightProjection * lightView * depthModelMatrix;

	glm::mat4 biasMatrix(
		0.5, 0.0, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.0, 0.5, 0.0,
		0.5, 0.5, 0.5, 1.0
	);
	depthBiasMVP = biasMatrix*lightSpaceMatrix;

}


void RenderScene(GLuint shaderProgram, int _i_pass)
{
	glUseProgram(shaderProgram);
	glm::mat4 modelview(1.0); // set base position for scene
	mvStack.push(modelview);

	if (_i_pass == 0) {
		int lightSpaceMatrixLocation = glGetUniformLocation(shaderProgram, "depthMVP");
		glUniformMatrix4fv(lightSpaceMatrixLocation, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));


	}
	else if (_i_pass == 1) {
		glm::mat4 projection(1.0);
		projection = glm::perspective(float(60.0f), 800.0f / 600.0f, 0.01f, 150.0f);
		rt3d::setUniformMatrix4fv(shaderProgram, "projection", glm::value_ptr(projection));

		mvStack.push(modelview);
		at = moveForward(eye, r, 1.0f);
		mvStack.top() = glm::lookAt(eye, at, up);

		glActiveTexture(GL_TEXTURE1);
		GLuint TextureID = glGetUniformLocation(shaderProgram, "shadowMap");
		glUniform1i(TextureID, 1);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		

		int biasMatrixLocation = glGetUniformLocation(shaderProgram, "DepthBiasMVP");
		glUniformMatrix4fv(biasMatrixLocation, 1, GL_FALSE, glm::value_ptr(depthBiasMVP));
	}

	if (_i_pass == 1){
		; // draw light cube
		//glBindTexture(GL_TEXTURE_2D, textures[0]);
		mvStack.push(mvStack.top());
		mvStack.top() = glm::translate(mvStack.top(), glm::vec3(lightPos[0], lightPos[1], lightPos[2]));
		mvStack.top() = glm::scale(mvStack.top(), glm::vec3(0.25f, 0.25f, 0.25f));
		rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
		rt3d::setMaterial(shaderProgram, material0);
		rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
		mvStack.pop();
	}


	
	
	// draw a cube for ground plane
	mvStack.push(mvStack.top());
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(-10.0f, -0.1f, -10.0f));
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(20.0f, 0.1f, 20.0f));
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();

	// draw a small cube block at cubePos
	mvStack.push(mvStack.top());
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(cubePos[0], cubePos[1], cubePos[2]));
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(1.5f, 1.5f, 1.5f));
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();
	

	if (_i_pass == 1)
	{
		// Optionally render the shadowmap (for debug only)

		// Render only on a corner of the window (or we we won't see the real rendering...)
		glViewport(0, 0, 256, 256);

		// Use our shader
		glUseProgram(quad_programID);

		/*
		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		// Set our "renderedTexture" sampler to use Texture Unit 0
		GLuint texID = glGetUniformLocation(quad_programID, "mytexture");
		glUniform1i(texID, 0);
		*/
		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		// Draw the triangle !
		// You have to disable GL_COMPARE_R_TO_TEXTURE above in order to see anything !
		glDrawArrays(GL_TRIANGLES, 0, 6); // 2*3 indices starting at 0 -> 2 triangles
		glEnable(GL_DEPTH_TEST);
		glDisableVertexAttribArray(0);
	}

	// remember to use at least one pop operation per push...
	mvStack.pop(); // initial matrix
	//glDepthMask(GL_TRUE);
}

void draw(SDL_Window * window) {

	ConfigureShaderAndMatrices();

	// 0. first render to depth map
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	RenderScene(depthProgram, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//// 1. render scene as normal using the generated depth/shadow map  
	//// --------------------------------------------------------------
	glViewport(0, 0, screenWidth, screenHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	RenderScene(shadowProgram, 1);

    SDL_GL_SwapWindow(window); // swap buffers
}


// Program entry point - SDL manages the actual WinMain entry point for us
int main(int argc, char *argv[]) {
    SDL_Window * hWindow; // window handle
    SDL_GLContext glContext; // OpenGL context handle
    hWindow = setupRC(glContext); // Create window and render context 

	// Required on Windows *only* init GLEW to access OpenGL beyond 1.1
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err) { // glewInit failed, something is seriously wrong
		std::cout << "glewInit failed, aborting." << endl;
		exit (1);
	}
	cout << glGetString(GL_VERSION) << endl;

	init();

	bool running = true; // set running to true
	SDL_Event sdlEvent;  // variable to detect SDL events
	while (running)	{	// the event loop
		while (SDL_PollEvent(&sdlEvent)) {
			if (sdlEvent.type == SDL_QUIT)
				running = false;
		}
		update();
		
		draw(hWindow); // call the draw function
	}

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(hWindow);
    SDL_Quit();
    return 0;
}