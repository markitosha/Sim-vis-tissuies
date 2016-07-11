#include "ClothSim.h"
#include <cstdint>

#define WIDTH 22
#define HEIGHT 24
#define NUM_POINT WIDTH*HEIGHT
#define HEIGHT_F 23
#define EDGE_LEN 0.05
#define INIT_HARD 150.0f
#define N_STR 11
#define N_COL 12
#define K 0.05f
#define MASS 0.1f
#define GR_CONST 9.8f
#define WIND_TIME 500
#define PI 3.141592
#define PHI 15 * PI / 180
#define RAD 0.3f

int time = 0;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ClothMeshData CreateMountVertices()
{
	ClothMeshData mdata;

	for (int i = 0; i * PHI < 2*PI; ++i){
		mdata.vertPos0.push_back(float4(RAD*cos(i*PHI), 0, RAD*sin(i*PHI), 1));
		mdata.vertPos1.push_back(float4(RAD*cos(i*PHI), 0, RAD*sin(i*PHI), 1));
		mdata.texCoords.push_back(float2(0.0f, float(i) / (2 * PI / (PHI))));
	}

	for (int i = 0; i * PHI < 2*PI; ++i){
		mdata.vertPos0.push_back(float4(RAD*cos(i*PHI), HEIGHT_F, RAD*sin(i*PHI), 1));
		mdata.vertPos1.push_back(float4(RAD*cos(i*PHI), HEIGHT_F, RAD*sin(i*PHI), 1));
		mdata.texCoords.push_back(float2(1.0f, float(i) / (2 * PI / (PHI))));
	}

	int num_pos = mdata.vertPos0.size();

	mdata.faceNormals.resize(num_pos);
	mdata.vertNormals.resize(num_pos);

	for (int i = 0; i < num_pos / 2; ++i)
	{
		mdata.edgeIndices.push_back(i);
		if (i == num_pos / 2 - 1)
			mdata.edgeIndices.push_back(0);
		else
			mdata.edgeIndices.push_back(i + 1);
		mdata.edgeIndices.push_back(i + num_pos / 2);
	}

	for (int i = num_pos / 2; i < num_pos; ++i)
	{
		mdata.edgeIndices.push_back(i);
		if (i == num_pos / 2)
			mdata.edgeIndices.push_back(num_pos - 1);
		else
			mdata.edgeIndices.push_back(i - 1);
		mdata.edgeIndices.push_back(i - num_pos / 2);
	}

	mdata.pMesh = std::make_shared<SimpleMesh>();

	GLUSshape& shape = mdata.pMesh->m_glusShape;

	shape.numberVertices = mdata.vertPos0.size();
	shape.numberIndices = mdata.edgeIndices.size();

	shape.vertices = (GLUSfloat*)malloc(4 * shape.numberVertices * sizeof(GLUSfloat));
	shape.indices = (GLUSuint*)malloc(shape.numberIndices * sizeof(GLUSuint));

	memcpy(shape.vertices, &mdata.vertPos0[0], sizeof(float)* 4 * shape.numberVertices);
	memcpy(shape.indices, &mdata.edgeIndices[0], sizeof(int)* shape.numberIndices);

	shape.normals = (GLUSfloat*)malloc(3 * shape.numberVertices * sizeof(GLUSfloat));
	memcpy(shape.normals, &mdata.vertNormals[0], sizeof(float)* 3 * shape.numberVertices);

	shape.texCoords = (GLUSfloat*)malloc(2 * shape.numberVertices * sizeof(GLUSfloat));
	memcpy(shape.texCoords, &mdata.texCoords[0], sizeof(float)* 2 * shape.numberVertices);

	return mdata;
}

ClothMeshData CreateTest2Vertices()
{
	ClothMeshData mdata;

	mdata.vertPos0.resize(NUM_POINT);
	mdata.vertVel0.resize(NUM_POINT);
	mdata.vertPos1.resize(NUM_POINT);
	mdata.vertVel1.resize(NUM_POINT);

	mdata.vertForces.resize(NUM_POINT);
	mdata.vertMassInv.resize(NUM_POINT);
	mdata.faceNormals.resize(NUM_POINT);
	mdata.vertNormals.resize(NUM_POINT);
	mdata.texCoords.resize(NUM_POINT);

	for (int i = 0; i < WIDTH; ++i)
	for (int j = 0; j < HEIGHT; ++j){
		mdata.vertPos0[i*HEIGHT + j] = float4((i - N_STR)*EDGE_LEN, (j - N_COL)*EDGE_LEN, 0, 1);
		mdata.vertVel0[i*HEIGHT + j] = float4(0, 0, 0, 0);
		mdata.vertPos1[i*HEIGHT + j] = mdata.vertPos0[i*HEIGHT + j];
		mdata.vertVel1[i*HEIGHT + j] = mdata.vertVel0[i*HEIGHT + j];
		mdata.texCoords[i*HEIGHT + j] = float2(float(i) / float(WIDTH), float(j) / float(HEIGHT));
		if (i*HEIGHT + j >= NUM_POINT - HEIGHT)
			mdata.vertMassInv[i*HEIGHT + j] = MASS / 10e20;
		else
			mdata.vertMassInv[i*HEIGHT + j] = MASS;

	}

	for (int i = 0; i < NUM_POINT; ++i)
	for (int j = i + 1; j < NUM_POINT; ++j){
		float4 vA = mdata.vertPos0[i];
		float4 vB = mdata.vertPos0[j];

		float dist = length(vB - vA);

		if (dist < 2.0*sqrtf(2.0f)*EDGE_LEN){
			float hardness = INIT_HARD * (EDGE_LEN / dist);

			mdata.edgeIndices.push_back(i);
			mdata.edgeIndices.push_back(j);
			mdata.edgeHardness.push_back(hardness);
			mdata.edgeInitialLen.push_back(dist);
		}
	}

	for (int i = 0; i < NUM_POINT; ++i){
		if ((i + 1) % HEIGHT != 0 && i < NUM_POINT - HEIGHT){
			mdata.edgeIndicesTri.push_back(i);
			mdata.edgeIndicesTri.push_back(i + 1);
			mdata.edgeIndicesTri.push_back(i + HEIGHT);
		}
		if (i % HEIGHT != 0 && i > HEIGHT - 1){
			mdata.edgeIndicesTri.push_back(i);
			mdata.edgeIndicesTri.push_back(i - 1);
			mdata.edgeIndicesTri.push_back(i - HEIGHT);
		}
	  }

  mdata.g_wind = float4(0, 0, 0, 0);

  // you can use any intermediate mesh representation or load data to GPU (in VBOs) here immediately.                              <<===== !!!!!!!!!!!!!!!!!!

  // create graphics mesh; SimpleMesh uses GLUS Shape to store geometry; 
  // we copy data to GLUS Shape, and then these data will be copyed later from GLUS shape to GPU 
  //
  mdata.pMesh = std::make_shared<SimpleMesh>();
 
  GLUSshape& shape = mdata.pMesh->m_glusShape;

  shape.numberVertices = mdata.vertPos0.size();
  shape.numberIndices  = mdata.edgeIndices.size();

  shape.vertices  = (GLUSfloat*)malloc(4 * shape.numberVertices * sizeof(GLUSfloat));
  shape.indices   = (GLUSuint*) malloc(shape.numberIndices * sizeof(GLUSuint));

  memcpy(shape.vertices, &mdata.vertPos0[0], sizeof(float) * 4 * shape.numberVertices);
  memcpy(shape.indices, &mdata.edgeIndices[0], sizeof(int) * shape.numberIndices);

  // for tri mesh you will need normals, texCoords and different indices
  // 

  shape.normals = (GLUSfloat*)malloc(3 * shape.numberVertices * sizeof(GLUSfloat));
  memcpy(shape.normals, &mdata.vertNormals[0], sizeof(float) * 3 * shape.numberVertices);

  shape.texCoords = (GLUSfloat*)malloc(2 * shape.numberVertices * sizeof(GLUSfloat));
  memcpy(shape.texCoords, &mdata.texCoords[0], sizeof(float)* 2 * shape.numberVertices);

  return mdata;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ClothMeshData::updatePositionsGPU2()
{
	if (pMesh == nullptr)
		return;
	GLUSshape& shape = pMesh->m_glusShape;

	shape.numberVertices = vertPos0.size();
	shape.vertices = (GLUSfloat*)malloc(4 * shape.numberVertices * sizeof(GLUSfloat));
	memcpy(shape.vertices, &vertPos0[0], sizeof(float)* 4 * shape.numberVertices);

	shape.numberIndices = edgeIndices.size();
	shape.indices = (GLUSuint*)malloc(shape.numberIndices * sizeof(GLUSuint));
	memcpy(shape.indices, &edgeIndices[0], sizeof(int)* shape.numberIndices);

	shape.texCoords = (GLUSfloat*)malloc(2 * shape.numberVertices * sizeof(GLUSfloat));
	memcpy(shape.texCoords, &texCoords[0], sizeof(float)* 2 * shape.numberVertices);

	glBindBuffer(GL_ARRAY_BUFFER, pMesh->m_vertexTexCoordsBufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLUSfloat)* 2 * shape.numberVertices, (GLUSfloat*)shape.texCoords, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, pMesh->m_vertexPosBufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLUSfloat)* 4 * shape.numberVertices, (GLUSfloat*)shape.vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, pMesh->m_indexBufferObject);
	glBufferData(GL_ARRAY_BUFFER, shape.numberIndices * sizeof(GLUSuint), (GLUSuint*)shape.indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ClothMeshData::updatePositionsGPU(int g_clothShaderId)
{
  if (pMesh == nullptr)
    return;
  else{
	  GLUSshape& shape = pMesh->m_glusShape;

	  shape.numberVertices = vertPos0.size();

	  shape.vertices = (GLUSfloat*)malloc(4 * shape.numberVertices * sizeof(GLUSfloat));
	  memcpy(shape.vertices, &vertPos0[0], sizeof(float)* 4 * shape.numberVertices);

	  shape.texCoords = (GLUSfloat*)malloc(2 * shape.numberVertices * sizeof(GLUSfloat));
	  memcpy(shape.texCoords, &texCoords[0], sizeof(float)* 2 * shape.numberVertices);

	  glBindBuffer(GL_ARRAY_BUFFER, pMesh->m_vertexTexCoordsBufferObject);
	  glBufferData(GL_ARRAY_BUFFER, sizeof(GLUSfloat)* 2 * shape.numberVertices, (GLUSfloat*)shape.texCoords, GL_DYNAMIC_DRAW);
	  glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	  glBindBuffer(GL_ARRAY_BUFFER, pMesh->m_vertexPosBufferObject);
	  glBufferData(GL_ARRAY_BUFFER, sizeof(GLUSfloat)* 4 * shape.numberVertices, (GLUSfloat*)shape.vertices, GL_STATIC_DRAW);
	  glBindBuffer(GL_ARRAY_BUFFER, 0);

	  if (g_clothShaderId != 1){
		  shape.numberIndices = edgeIndicesTri.size();
		  shape.indices = (GLUSuint*)malloc(shape.numberIndices * sizeof(GLUSuint));
		  memcpy(shape.indices, &edgeIndicesTri[0], sizeof(int)* shape.numberIndices);
	  }
	  else{
		  shape.numberIndices = edgeIndices.size();
		  shape.indices = (GLUSuint*)malloc(shape.numberIndices * sizeof(GLUSuint));
		  memcpy(shape.indices, &edgeIndices[0], sizeof(int)* shape.numberIndices);
	  }
	  glBindBuffer(GL_ARRAY_BUFFER, pMesh->m_indexBufferObject);
	  glBufferData(GL_ARRAY_BUFFER, shape.numberIndices * sizeof(GLUSuint), (GLUSuint*)shape.indices, GL_STATIC_DRAW);
	  glBindBuffer(GL_ARRAY_BUFFER, 0);
  }
  
  // copy current vertex positions to positions VBO
 
}

void ClothMeshData::updateNormalsGPU2()
{
	if (pMesh == nullptr || this->vertNormals.size() == 0)
		return;

	GLUSshape& shape = pMesh->m_glusShape;

	shape.numberVertices = vertNormals.size();
	shape.normals = (GLUSfloat*)malloc(3 * shape.numberVertices * sizeof(GLUSfloat));

	memcpy(shape.normals, &vertNormals[0], sizeof(float)* 3 * shape.numberVertices);

	glBindBuffer(GL_ARRAY_BUFFER, pMesh->m_vertexNormBufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLUSfloat)* 3 * shape.numberVertices, (GLUSfloat*)shape.normals, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ClothMeshData::updateNormalsGPU()
{
  if (pMesh == nullptr || this->vertNormals.size() == 0)
    return;

  GLUSshape& shape = pMesh->m_glusShape;

  shape.numberVertices = vertNormals.size();
  shape.normals = (GLUSfloat*)malloc(3 * shape.numberVertices * sizeof(GLUSfloat));

  memcpy(shape.normals, &vertNormals[0], sizeof(float) * 3 * shape.numberVertices);

  glBindBuffer(GL_ARRAY_BUFFER, pMesh->m_vertexNormBufferObject);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLUSfloat)* 3 * shape.numberVertices, (GLUSfloat*)shape.normals, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // copy current recalculated normals to appropriate VBO on GPU
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SimStep2(ClothMeshData* pMesh, float delta_t)
{
	float4* inVertPos = pMesh->pinPong ? &pMesh->vertPos1[0] : &pMesh->vertPos0[0];
	float4* outVertPos = pMesh->pinPong ? &pMesh->vertPos0[0] : &pMesh->vertPos1[0];

	for (int i = 0; i < pMesh->vertexNumber(); ++i)
		outVertPos[i] = inVertPos[i];

	pMesh->pinPong = !pMesh->pinPong;
}

void SimStep(ClothMeshData* pMesh, float delta_t)
{
  // get in and out pointers
  //
  float4* inVertPos  = pMesh->pinPong ? &pMesh->vertPos1[0] : &pMesh->vertPos0[0];
  float4* inVertVel  = pMesh->pinPong ? &pMesh->vertVel1[0] : &pMesh->vertVel0[0];

  float4* outVertPos = pMesh->pinPong ? &pMesh->vertPos0[0] : &pMesh->vertPos1[0];
  float4* outVertVel = pMesh->pinPong ? &pMesh->vertVel0[0] : &pMesh->vertVel1[0];

  if (time++ % WIND_TIME == 0){
	  float x = (float)(rand() % 20 - 10) / 10.0f;
	  float y = (float)(rand() % 20 - 10) / 10.0f;
	  float z = (float)(rand() % 20 - 10) / 10.0f;
	  pMesh->g_wind = float4(x, y, z + 0.1, 0);
  }

  for (size_t i = 0; i < pMesh->vertForces.size(); i++){
	  pMesh->vertForces[i] = float4(0, - GR_CONST * pMesh->vertMassInv[i], 0, 0);
	  pMesh->vertForces[i] += - K * inVertVel[i];
	  pMesh->vertForces[i] += pMesh->g_wind;
  }

  for (int connectId = 0; connectId < pMesh->connectionNumber(); connectId++)
  {
	  int i = pMesh->edgeIndices[2 * connectId];
	  int j = pMesh->edgeIndices[2 * connectId + 1];
	  float4 vec = inVertPos[j] - inVertPos[i];
	  float dl = pMesh->edgeInitialLen[connectId] - length(vec);
	  pMesh->vertForces[i] += normalize(-vec) * pMesh->edgeHardness[connectId] * dl;
	  pMesh->vertForces[j] += normalize(vec) * pMesh->edgeHardness[connectId] * dl;
  }

  // update positions and velocity
  //
  for (size_t i = 0; i < pMesh->vertForces.size(); ++i){
	  if (i < NUM_POINT - HEIGHT){
		  float4 a = pMesh->vertForces[i] / pMesh->vertMassInv[i];
		  float4 dVel = a*delta_t;
		  outVertVel[i] = inVertVel[i] + dVel;
		  float4 dPos = outVertVel[i] * delta_t + a * pow(delta_t, 2) / 2;
		  outVertPos[i] = inVertPos[i] + dPos;
	  }else{
		  outVertPos[i] = inVertPos[i];
		  outVertVel[i] = float4(0, 0, 0, 0);
	  }
  }

  pMesh->pinPong = !pMesh->pinPong; // swap pointers for next sim step
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float4 CalRightUpNormal(ClothMeshData* pMesh, int i)
{
	float4 vec1 = pMesh->vertPos0[i + 1] - pMesh->vertPos0[i];
	float4 vec2 = pMesh->vertPos0[i + HEIGHT] - pMesh->vertPos0[i];
	float4 vec_cr = cross(vec1, vec2);
	return normalize(vec_cr);
}

float4 CalRightDownNormal(ClothMeshData* pMesh, int i)
{
	float4 vec1 = - pMesh->vertPos0[i - 1] + pMesh->vertPos0[i];
	float4 vec2 = pMesh->vertPos0[i + HEIGHT] - pMesh->vertPos0[i];
	float4 vec_cr = cross(vec1, vec2);
	return normalize(vec_cr);
}

float4 CalLeftUpNormal(ClothMeshData* pMesh, int i)
{
	float4 vec1 = pMesh->vertPos0[i + 1] - pMesh->vertPos0[i];
	float4 vec2 = - pMesh->vertPos0[i - HEIGHT] + pMesh->vertPos0[i];
	float4 vec_cr = cross(vec1, vec2);
	return normalize(vec_cr);
}

float4 CalLeftDownNormal(ClothMeshData* pMesh, int i)
{
	float4 vec1 = - pMesh->vertPos0[i - 1] + pMesh->vertPos0[i];
	float4 vec2 = - pMesh->vertPos0[i - HEIGHT] + pMesh->vertPos0[i];
	float4 vec_cr = cross(vec1, vec2);
	return normalize(vec_cr);
}

void RecalculateNormals2(ClothMeshData* pMesh)
{
	int num_p = pMesh->vertexNumber();
	pMesh->vertNormals.resize(num_p);
	pMesh->faceNormals.resize(num_p);
	float4 vec1, vec2, vec3, vec_cr;
	for (int i = 0; i < num_p / 2; ++i){
		if (i == num_p / 2 - 1)
			vec1 = pMesh->vertPos0[0] - pMesh->vertPos0[i];
		else
			vec1 = pMesh->vertPos0[i + 1] - pMesh->vertPos0[i];
		vec2 = pMesh->vertPos0[i + num_p / 2] - pMesh->vertPos0[i];
		if (i == 0)
			vec3 = pMesh->vertPos0[i] - pMesh->vertPos0[num_p / 2 - 1];
		else
			vec3 = pMesh->vertPos0[i] - pMesh->vertPos0[i - 1];
		vec_cr = normalize(cross(vec1, vec2) + cross(vec3, vec2));
		pMesh->faceNormals[i] = vec_cr;
		pMesh->vertNormals[i] = float3(vec_cr.x, vec_cr.y, vec_cr.z);
	}
	for (int i = num_p / 2; i < num_p; ++i){
		if (i == num_p - 1)
			vec1 = pMesh->vertPos0[num_p / 2] - pMesh->vertPos0[i];
		else
			vec1 = pMesh->vertPos0[i + 1] - pMesh->vertPos0[i];
		vec2 = pMesh->vertPos0[i] - pMesh->vertPos0[i - num_p / 2];
		if (i == num_p / 2)
			vec3 = pMesh->vertPos0[i] - pMesh->vertPos0[num_p - 1];
		else
			vec3 = pMesh->vertPos0[i] - pMesh->vertPos0[i - 1];
		vec_cr = normalize(cross(vec1, vec2) + cross(vec3, vec2));
		pMesh->faceNormals[i] = vec_cr;
		pMesh->vertNormals[i] = float3(vec_cr.x, vec_cr.y, vec_cr.z);
	}
	std::cout << std::endl << std::endl;
}

void RecalculateNormals(ClothMeshData* pMesh)
{
	pMesh->vertNormals.resize(NUM_POINT);
	for (int i = 0; i < NUM_POINT; ++i){
		if (i == 0){
			pMesh->faceNormals[i] = CalRightUpNormal(pMesh, i);
		}else if (i == HEIGHT - 1){
			pMesh->faceNormals[i] = CalRightDownNormal(pMesh, i);
		}else if (i == NUM_POINT - 1){
			pMesh->faceNormals[i] = CalLeftDownNormal(pMesh, i);
		}else if (i == NUM_POINT - HEIGHT){
			pMesh->faceNormals[i] = CalLeftUpNormal(pMesh, i);
		}else if (i % HEIGHT == 0){
			pMesh->faceNormals[i] = normalize(CalRightUpNormal(pMesh, i) + CalLeftUpNormal(pMesh, i));
		}else if ((i + 1) % HEIGHT == 0){
			pMesh->faceNormals[i] = normalize(CalRightDownNormal(pMesh, i) + CalLeftDownNormal(pMesh, i));
		}else if (i < HEIGHT){
			pMesh->faceNormals[i] = normalize(CalRightDownNormal(pMesh, i) + CalRightUpNormal(pMesh, i));
		}else if (i > NUM_POINT - HEIGHT){
			pMesh->faceNormals[i] = normalize(CalLeftDownNormal(pMesh, i) + CalLeftUpNormal(pMesh, i));
		}else{
			pMesh->faceNormals[i] = normalize(CalLeftDownNormal(pMesh, i) + CalLeftUpNormal(pMesh, i) + CalRightDownNormal(pMesh, i) + CalRightUpNormal(pMesh, i));
		}
		pMesh->vertNormals[i] = float3(pMesh->faceNormals[i].x, pMesh->faceNormals[i].y, pMesh->faceNormals[i].z);
	}
}

