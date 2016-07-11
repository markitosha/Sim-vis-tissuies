/////////////////////////////////////////////////////////////////
// main.cpp Author: Vladimir Frolov, 2011, Graphics & Media Lab.
/////////////////////////////////////////////////////////////////

#include <GL/glew.h>

#include "GL/glus.h"
#include "../vsgl3/glHelper.h"

#include "ClothSim.h"

#include <iostream>
#include <fstream>
#include <string>
#include <map>

ShaderProgram g_program;
ShaderProgram g_programRed;
ShaderProgram g_programBox;
ShaderProgram g_programMount;
ShaderProgram g_programMeshes;

struct MyInput
{
  MyInput() 
  {
    cam_rot[0] = cam_rot[1] = cam_rot[2] = cam_rot[3] = 0.f;
    mx = my = 0;
    rdown = ldown = false;
    cam_dist = 40.0f;
  }

  int mx;
  int my;
  bool rdown;
  bool ldown;
  float cam_rot[4];
  float cam_pos[4];

  float cam_dist;

}input;

FullScreenQuad* g_pFullScreenQuad = nullptr;
int g_width  = 0;
int g_height = 0;


SimpleMesh* g_pLandMesh = nullptr;
SimpleMesh* g_pBoxMesh = nullptr;
std::vector<SimpleMesh*> g_pMeshesMesh;
std::shared_ptr<SimpleMesh> g_pCarpMesh = nullptr;
std::shared_ptr<SimpleMesh> g_pMountMesh = nullptr;
Texture2D*  g_pClothTex = nullptr;
Texture2D*  g_pFloorTex = nullptr;
Texture2D*  g_pBoxTex = nullptr;
Texture2D*  g_pMountTex = nullptr;
Texture2D*  g_pMeshesTex = nullptr;

float4x4    g_projectionMatrix;
float3      g_camPos(0,20,20);
float3      g_sunDir = normalize(float3(-5.0f, 20.0f, -20.0f));

// cloth sim
//
ClothMeshData g_cloth;
ClothMeshData g_mount;

int  g_clothShaderId = 3;
bool g_drawShadowMap = false;

void RequreExtentions() // check custome extentions here
{
  CHECK_GL_ERRORS;

  std::cout << "GPU Vendor: " << glGetString(GL_VENDOR) << std::endl;
  std::cout << "GPU Name  : " << glGetString(GL_RENDERER) << std::endl;
  std::cout << "GL_VER    : " << glGetString(GL_VERSION) << std::endl;
  std::cout << "GLSL_VER  : " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

  GlusHelperRequireExt h;
  h.require("GL_EXT_texture_filter_anisotropic");
}

/**
* Function for initialization.
*/
GLUSboolean init(GLUSvoid)
{
  try 
  {
    RequreExtentions();
    // Load the source of the vertex shader. GLUS loader corrupts shaders.
    // Thats why we do that with our class ShaderProgram

    g_program       = ShaderProgram("../src/Vertex.vert", "../src/Fragment.frag"); // shaders to draw land
    g_programRed    = ShaderProgram("../src/Vertex.vert", "../src/Fragment.frag");
	g_programBox	= ShaderProgram("../src/Vertex.vert", "../src/Red.frag");  // shaders to draw cloth in wire frame mode
	g_programMount  = ShaderProgram("../src/Vertex.vert", "../src/Red.frag");
	g_programMeshes = ShaderProgram("../src/Vertex.vert", "../src/Fragment.frag");


    g_pFullScreenQuad  = new FullScreenQuad();

    g_pLandMesh        = new SimpleMesh(g_program.program, 20, SimpleMesh::PLANE);
	g_pBoxMesh		   = new SimpleMesh(g_programBox.program, 20, SimpleMesh::SPHERE);
	for (int i = 0; i < 5; ++i)
		g_pMeshesMesh.push_back(new SimpleMesh(g_programMeshes.program, 20, SimpleMesh::CUBE));

    g_pClothTex        = new Texture2D("../bin/data/cmc.tga");
	g_pFloorTex		   = new Texture2D("../bin/data/texture1.jpg");
	g_pMeshesTex	   = new Texture2D("../bin/data/wood.jpg");

    g_cloth = CreateTest2Vertices();

	g_mount = CreateMountVertices();


    // copy geometry data to GPU; use shader program to create vao inside; 
    // !!!!!!!!!!!!!!!!!!!!!!!!!!! DEBUG THIS CAREFULLY !!!!!!!!!!!!!!!!!!!!!!! <<============= !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // please remember about vertex shader attributes opt and invalid locations.
    //
    g_cloth.initGPUData(g_programRed.program); 

	g_mount.initGPUData(g_programMount.program);

    // g_cloth = CreateQuad(16, 16, 1.0f, g_programRed.program, g_programCloth2.program);

    return GLUS_TRUE;
  }
  catch(std::runtime_error e)
  {
    std::cerr << e.what() << std::endl;
    exit(-1);
  }
  catch(...)
  {
    std::cerr << "Unexpected Exception (init)!" << std::endl;
    exit(-1);
  }
}

GLUSvoid reshape(GLUSuint width, GLUSuint height)
{
	glViewport(0, 0, width, height);
  g_width  = width;
  g_height = height;

  glusPerspectivef(g_projectionMatrix.L(), 45.0f, (GLfloat) width / (GLfloat) height, 1.0f, 100.0f);  // Calculate the projection matrix
}

GLUSvoid mouse(GLUSboolean pressed, GLUSuint button, GLUSuint x, GLUSuint y)
{
  if(!pressed)
    return;

  if (button & 1)// left button
  {
    input.ldown=true;		
    input.mx=x;			
    input.my=y;
  }

  if (button & 4)	// right button
  {
    input.rdown=true;
    input.mx=x;
    input.my=y;
  }
}

GLUSvoid mouseMove(GLUSuint button, GLUSint x, GLUSint y)
{
  if(button & 1)		// left button
  {
    int x1 = x;
    int y1 = y;

    input.cam_rot[0] += 0.25f*(y1-input.my);	// change rotation
    input.cam_rot[1] += 0.25f*(x1-input.mx);

    input.mx=x;
    input.my=y;
  }
}

GLUSvoid keyboard(GLUSboolean pressed, GLUSuint key)
{

  switch(key)
  {
  case 'w':
  case 'W':
    input.cam_dist -= 0.1f;
    break;

  case 's':
  case 'S':
    input.cam_dist += 0.1f;
    break;

  case 'a':
  case 'A':
   
    break;

  case 'd':
  case 'D':

    break;


  case '1':
  case '!':
    g_clothShaderId = 1;
    g_drawShadowMap = false;
    break;

  case '2':
  case '@':
    g_clothShaderId = 2;
    g_drawShadowMap = false;
    break;

  case '3':
  case '#':
    g_clothShaderId = 3;
    g_drawShadowMap = false;
    break;

  case '4':
  case '$':
	g_clothShaderId = 4;
    g_drawShadowMap = false;
    break;

  case '5':
  case '%':
	  g_clothShaderId = 5;
	  g_drawShadowMap = false;
	  break;

  }

}

GLUSboolean update(GLUSfloat time)
{
	try
	{
		static float elaspedTimeFromStart = 0;
		elaspedTimeFromStart += 10 * time;

		g_camPos.z = input.cam_dist;


		float4x4 model;
		float4x4 modelView;
		glusLoadIdentityf(model.L());
		glusRotateRzRyRxf(model.L(), input.cam_rot[0], input.cam_rot[1], 0.0f);
		glusLookAtf(modelView.L(), g_camPos.x, g_camPos.y, g_camPos.z,
			0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f);                           // ... and the view matrix ...

		glusMultMatrixf(modelView.L(), modelView.L(), model.L()); 	            // ... to get the final model view matrix

		float4x4 rotationMatrix, scaleMatrix, translateMatrix;
		float4x4 transformMatrix1, transformMatrix2;

		// make our program current
		//
		glViewport(0, 0, g_width, g_height);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		// cloth
		//

		SimStep(&g_cloth, 0.01f);
		g_cloth.updatePositionsGPU(g_clothShaderId);

		RecalculateNormals(&g_cloth);
		g_cloth.updateNormalsGPU();

		SimStep2(&g_mount, 0.01f);
		g_mount.updatePositionsGPU2();

		RecalculateNormals2(&g_mount);
		g_mount.updateNormalsGPU2();


		// cloth object space transform
		//
		const float  clothScale = 9.0f;
		const float  clothTranslateY = 2.5f;

		g_pCarpMesh = g_cloth.pMesh;

		rotationMatrix.identity();
		scaleMatrix.identity();
		translateMatrix.identity();
		transformMatrix1.identity();
		transformMatrix2.identity();

		glusTranslatef(translateMatrix.L(), 0, clothTranslateY, 0);
		glusScalef(scaleMatrix.L(), clothScale, clothScale, clothScale);
		glusMultMatrixf(transformMatrix1.L(), rotationMatrix.L(), scaleMatrix.L());
		glusMultMatrixf(transformMatrix2.L(), translateMatrix.L(), transformMatrix1.L());

		GLuint clothProgramId = g_programRed.program;

		glUseProgram(clothProgramId);

		setUniform(clothProgramId, "modelViewMatrix", modelView);
		setUniform(clothProgramId, "projectionMatrix", g_projectionMatrix);  // set matrix we have calculated in "reshape" funtion
		setUniform(clothProgramId, "objectMatrix", transformMatrix2);

		setUniform(clothProgramId, "g_sunDir", g_sunDir);

		setUniform(clothProgramId, "eyePos", g_camPos);

		if (g_clothShaderId == 2){
			setUniform(clothProgramId, "Norm", 1);
		}
		else{
			setUniform(clothProgramId, "Norm", 0);
		}

		if (g_clothShaderId == 4){
			setUniform(clothProgramId, "Blinn", 1);
		}
		else{
			setUniform(clothProgramId, "Blinn", 0);
		}

		bindTexture(clothProgramId, 1, "diffuseTexture", g_pClothTex->GetColorTexId());

		if (g_clothShaderId != 1)
		{
			setUniform(clothProgramId, "Color", 0);
			if (g_clothShaderId == 5){
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				g_pCarpMesh->Draw();
				glDisable(GL_BLEND);
			}
			g_pCarpMesh->Draw();
			//   g_cloth.pTris->Draw();
		}
		else
		{
			setUniform(clothProgramId, "Color", 1);
			glPointSize(4.0f); // well, we want to draw vertices as bold points )
			g_cloth.pMesh->Draw(GL_POINTS); // draw vertices
			g_cloth.pMesh->Draw(GL_LINES);  // draw connections
		}

		// mount

		const float  mountScale = 1.0f;
		const float  mountTranslateY = - 15.0f;

		g_pMountMesh = g_mount.pMesh;

		rotationMatrix.identity();
		scaleMatrix.identity();
		translateMatrix.identity();
		transformMatrix1.identity();
		transformMatrix2.identity();

		glusTranslatef(translateMatrix.L(), 4.5, mountTranslateY, 0);
		glusScalef(scaleMatrix.L(), mountScale, mountScale, mountScale);
		glusMultMatrixf(transformMatrix1.L(), rotationMatrix.L(), scaleMatrix.L());
		glusMultMatrixf(transformMatrix2.L(), translateMatrix.L(), transformMatrix1.L());

		GLuint mountProgramId = g_programMount.program;

		glUseProgram(mountProgramId);

		setUniform(mountProgramId, "modelViewMatrix", modelView);
		setUniform(mountProgramId, "projectionMatrix", g_projectionMatrix);  // set matrix we have calculated in "reshape" funtion
		setUniform(mountProgramId, "objectMatrix", transformMatrix2);

		setUniform(mountProgramId, "g_sunDir", g_sunDir);

		setUniform(mountProgramId, "eyePos", g_camPos);

		if (g_clothShaderId == 4){
			setUniform(mountProgramId, "Blinn", 1);
		}
		else{
			setUniform(mountProgramId, "Blinn", 0);
		}

		g_pMountMesh->Draw();
		
		// draw land
		//
		rotationMatrix.identity();
		scaleMatrix.identity();
		translateMatrix.identity();
		transformMatrix1.identity();
		transformMatrix2.identity();

		glusScalef(scaleMatrix.L(),	15, 15, 15);
		glusTranslatef(translateMatrix.L(), 0, -15, 0);
		glusMultMatrixf(transformMatrix1.L(), rotationMatrix.L(), scaleMatrix.L());
		glusMultMatrixf(transformMatrix2.L(), translateMatrix.L(), transformMatrix1.L());

		glUseProgram(g_program.program);

		setUniform(g_program.program, "modelViewMatrix", modelView);
		setUniform(g_program.program, "projectionMatrix", g_projectionMatrix);  // set matrix we have calculated in "reshape" funtion
		setUniform(g_program.program, "objectMatrix", transformMatrix2);
		setUniform(g_program.program, "g_sunDir", g_sunDir);

		setUniform(g_program.program, "Norm", 0);
		setUniform(g_program.program, "Color", 0);

		setUniform(g_program.program, "eyePos", g_camPos);

		bindTexture(g_program.program, 1, "diffuseTexture", g_pFloorTex->GetColorTexId());

		if (g_clothShaderId == 4){
			setUniform(g_program.program, "Blinn", 1);
		}
		else{
			setUniform(g_program.program, "Blinn", 0);
		}


		g_pLandMesh->Draw();

		for (int i = 0; i < 5; ++i){
			rotationMatrix.identity();
			scaleMatrix.identity();
			translateMatrix.identity();
			transformMatrix1.identity();
			transformMatrix2.identity();

			glusScalef(scaleMatrix.L(), 0.5, 0.5, 0.5);
			if (i % 4 == 0)
				glusTranslatef(translateMatrix.L(), 4.5, -14.5, i*0.5 - 1);
			else
				glusTranslatef(translateMatrix.L(), i+2.5, -14.5, 0);
			glusMultMatrixf(transformMatrix1.L(), rotationMatrix.L(), scaleMatrix.L());
			glusMultMatrixf(transformMatrix2.L(), translateMatrix.L(), transformMatrix1.L());

			glUseProgram(g_programMeshes.program);

			setUniform(g_programMeshes.program, "modelViewMatrix", modelView);
			setUniform(g_programMeshes.program, "projectionMatrix", g_projectionMatrix);  // set matrix we have calculated in "reshape" funtion
			setUniform(g_programMeshes.program, "objectMatrix", transformMatrix2);
			setUniform(g_programMeshes.program, "g_sunDir", g_sunDir);

			setUniform(g_programMeshes.program, "Norm", 0);
			setUniform(g_programMeshes.program, "Color", 0);

			setUniform(g_programMeshes.program, "eyePos", g_camPos);

			bindTexture(g_program.program, 1, "diffuseTexture", g_pMeshesTex->GetColorTexId());

			if (g_clothShaderId == 4){
				setUniform(g_program.program, "Blinn", 1);
			}
			else{
				setUniform(g_program.program, "Blinn", 0);
			}


			g_pMeshesMesh[i]->Draw();
		}

		//draw sphere
		rotationMatrix.identity();
		scaleMatrix.identity();
		translateMatrix.identity();
		transformMatrix1.identity();
		transformMatrix2.identity();

		glusScalef(scaleMatrix.L(), 0.5, 0.5, 0.5);
		glusTranslatef(translateMatrix.L(), 4.5, 8, 0);
		glusMultMatrixf(transformMatrix1.L(), rotationMatrix.L(), scaleMatrix.L());
		glusMultMatrixf(transformMatrix2.L(), translateMatrix.L(), transformMatrix1.L());

		glUseProgram(g_programBox.program);

		setUniform(g_programBox.program, "modelViewMatrix", modelView);
		setUniform(g_programBox.program, "projectionMatrix", g_projectionMatrix);  // set matrix we have calculated in "reshape" funtion
		setUniform(g_programBox.program, "objectMatrix", transformMatrix2);
		setUniform(g_programBox.program, "g_sunDir", g_sunDir);

		setUniform(g_programBox.program, "eyePos", g_camPos);

		if (g_clothShaderId == 4){
			setUniform(g_programBox.program, "Blinn", 1);
		}
		else{
			setUniform(g_programBox.program, "Blinn", 0);
		}

		g_pBoxMesh->Draw();

    return GLUS_TRUE;
  }
  catch(std::runtime_error e)
  {
    std::cerr << e.what() << std::endl;
    exit(-1);
  }
  catch(...)
  {
    std::cerr << "Unexpected Exception(render)!" << std::endl;
    exit(-1);
  }
}

/**
 * Function to clean up things.
 */
void shutdown(void)
{
  delete g_pFullScreenQuad; g_pFullScreenQuad = nullptr;
  delete g_pLandMesh;       g_pLandMesh = nullptr;
  delete g_pClothTex;       g_pClothTex = nullptr;

}

/**
 * Main entry point.
 */
int main(int argc, char* argv[])
{
	glusInitFunc(init);
	glusReshapeFunc(reshape);
	glusUpdateFunc(update);
	glusTerminateFunc(shutdown);
  glusMouseFunc(mouse);
  glusMouseMoveFunc(mouseMove);
  glusKeyFunc(keyboard);

	glusPrepareContext(3, 0, GLUS_FORWARD_COMPATIBLE_BIT);

	if (!glusCreateWindow("cloth sim", 1024, 768, GLUS_FALSE))
	{
		printf("Could not create window!");
		return -1;
	}

	// Init GLEW
	glewExperimental = GL_TRUE;
  GLenum err=glewInit();
  if(err!=GLEW_OK)
  {
    sprintf("glewInitError", "Error: %s\n", glewGetErrorString(err));
    return -1;
  }
  glGetError(); // flush error state variable, caused by glew errors
  

	// Only continue, if OpenGL 3.3 is supported.
	if (!glewIsSupported("GL_VERSION_3_0"))
	{
		printf("OpenGL 3.0 not supported.");

		glusDestroyWindow();
		return -1;
	}

	glusRun();

	return 0;
}

