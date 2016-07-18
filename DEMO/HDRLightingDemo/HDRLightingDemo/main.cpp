#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glew32.lib")

#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "FreeImage.lib")

// GLEW
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM Mathemtics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Other Libs
#include <FreeImage.h>

#include <stdio.h>


// downloaded from https://github.com/JoeyDeVries/LearnOpenGL
#include "Camera.h"
#include "Shader.h"

#include "RenderTexture.h"

#include <assert.h>

#define NUM_LIGHTS 2

typedef struct tagMeshMeta
{
	GLfloat*	data;
	GLuint		numQuads;
	GLshort*	indexData; 
	GLuint		vao;
	GLuint		vbo;
	GLuint		ibo;
	GLuint		tex;
}MeshMeta;

typedef struct tagRect
{
	LONG    left;
	LONG    top;
	LONG    right;
	LONG    bottom;
}Rect;

typedef struct tagCoordRect
{
	float lu, ru;
	float lv, rv;
}CoordRect;

typedef struct tagQuad
{
	GLuint vao;
	GLuint vbo;
	void draw()
	{
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);	
	}
}Quad;

#define PI 3.14159265358979323846264338327950288

// The width and height of the window.
const GLuint SCR_W = 800;
const GLuint SCR_H = 600;

const GLuint SCR_CROP_W = SCR_W - SCR_W % 8;
const GLuint SCR_CROP_H = SCR_H - SCR_H % 8;

const GLuint MAX_SAMPLES = 16;
const GLuint NUM_TONEMAP_TEXTURES = 4;

// About key messages.
bool	gKeys[1024];
int		gUseToneMapping = 0;
int		gMode = 1;
int		gQuadIndex = 0;
int		gUseBloom = 1;
float	gMuti = 0.7;

MeshMeta*	gRoomData = NULL;
MeshMeta*	gFloorData = NULL;
MeshMeta*	gCeilData = NULL;
MeshMeta*	PaintData[2];


GLfloat gLightIntensity[NUM_LIGHTS][4];

glm::vec4 gLightPosition[NUM_LIGHTS]= {
	glm::vec4(4.0f, 2.0f, 18.0f, 1.0f),
	glm::vec4(11.0f, 2.0f, 18.0f, 1.0f),
};

// Light intensities on a log scale
int gLightLogIntensity[NUM_LIGHTS] = {-4 + 4, -4 + 4};
// Mantissa of the light intensity
int gLightMantissa[NUM_LIGHTS] = {1 + 4, 1 + 4};


// Camera
//Camera gCamera(glm::vec3(0.0f, 0.0f, 5.0f));
//Camera gCamera(glm::vec3(0.0f, 0.0f, 5.0f));

Camera gCamera(glm::vec3(0.0f, 0.0f, 5.0f));

void printMode()
{
	if(gMode)
	{
		printf("*************************场景渲染模式*************************\n");
	}
	else
	{
		printf("*************************面片渲染模式*************************\n");
		switch (gQuadIndex)
		{
		case 0:
			printf("场景纹理\n");
			break;
		case 1:
			printf("缩放纹理\n");
			break;
		case 2:
			printf("高光纹理\n");
			break;
		case 3:
			printf("Bloom纹理\n");
			break;
		}
	}
	if(gUseBloom)
		printf("Bloom效果：开启\n");
	else
		printf("Bloom效果：关闭\n");

	if(gUseToneMapping)
		printf("HDR效果：开启\n");
	else
		printf("HDR效果：关闭\n");
	printf("**************************************************************\n");
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
		{
			gKeys[key] = true;
		}
		else if (action == GLFW_RELEASE)
		{
			gKeys[key] = false;
		}
	}
	if(gKeys[GLFW_KEY_SPACE])
		gUseToneMapping = !gUseToneMapping;
	if(gKeys[GLFW_KEY_C])
		gMode = !gMode;
	if(gKeys[GLFW_KEY_M])
		gQuadIndex = (gQuadIndex + 1) % 8;
	if(gKeys[GLFW_KEY_N])
		gUseBloom = !gUseBloom;

	printMode();
}


void refreshLights()
{
	int i;
	for(i = 0; i < NUM_LIGHTS; i++ )
	{
		gLightIntensity[i][0] = gLightMantissa[i] * ( float )pow( 10.0f, gLightLogIntensity[i] );
		gLightIntensity[i][1] = gLightMantissa[i] * ( float )pow( 10.0f, gLightLogIntensity[i] );
		gLightIntensity[i][2] = gLightMantissa[i] * ( float )pow( 10.0f, gLightLogIntensity[i] );
		gLightIntensity[i][3] = 1.0f;
	}
}

void adjustLight(int iLight, bool bIncrement)
{
	if( iLight >= NUM_LIGHTS )
		return;

	if( bIncrement && gLightLogIntensity[iLight] < 7 )
	{
		gLightMantissa[iLight]++;
		if( gLightMantissa[iLight] > 9 )
		{
			gLightMantissa[iLight] = 1;
			gLightLogIntensity[iLight]++;
		}
	}

	if( !bIncrement && gLightLogIntensity[iLight] > -4 )
	{
		gLightMantissa[iLight]--;
		if( gLightMantissa[iLight] < 1 )
		{
			gLightMantissa[iLight] = 9;
			gLightLogIntensity[iLight]--;
		}
	}

	refreshLights();
	return;
}

void updateFrameDelta(GLfloat deltaTime)
{
	if (gKeys[GLFW_KEY_W])
		gCamera.processKeyboard(Camera::FORWARD, deltaTime);
	if (gKeys[GLFW_KEY_S])
		gCamera.processKeyboard(Camera::BACKWARD, deltaTime);
	if (gKeys[GLFW_KEY_A])
		gCamera.processKeyboard(Camera::LEFT, deltaTime);
	if (gKeys[GLFW_KEY_D])
		gCamera.processKeyboard(Camera::RIGHT, deltaTime);
	if (gKeys[GLFW_KEY_UP])
		gCamera.processKeyboard(Camera::UP, deltaTime);
	if (gKeys[GLFW_KEY_DOWN])
		gCamera.processKeyboard(Camera::DOWN, deltaTime);

	// Change exposure of the scene's HDR camera
	if (gKeys[GLFW_KEY_Q])
	{
		adjustLight(0, true);
		adjustLight(1, true);
	}
	else if (gKeys[GLFW_KEY_E])
	{
		adjustLight(0, false);
		adjustLight(1, false);
	}

	// Rotate the camera.
	if(gKeys[GLFW_KEY_J])
		gCamera.processMouseMovement(250.0 * deltaTime, 0);
	if(gKeys[GLFW_KEY_K])
		gCamera.processMouseMovement(-250.0 * deltaTime, 0);

}

void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{

}

GLuint loadTexture(GLchar const * filename, bool gammaCorrection = false)
{
	// Generate texture ID and load texture data 
	GLuint textureID;
	glGenTextures(1, &textureID);
	//image format
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
	//pointer to the image, once loaded
	FIBITMAP *dib(0);
	//pointer to the image data
	BYTE* bits(0);
	//image width and height
	unsigned int width(0), height(0);

	//check the file signature and deduce its format
	fif = FreeImage_GetFileType(filename, 0);
	//if still unknown, try to guess the file format from the file extension
	if(fif == FIF_UNKNOWN) 
		fif = FreeImage_GetFIFFromFilename(filename);

	//if still unkown, return failure
	if(fif == FIF_UNKNOWN)
		return false;

	//check that the plugin has reading capabilities and load the file
	if(FreeImage_FIFSupportsReading(fif))
		dib = FreeImage_Load(fif, filename);
	//if the image failed to load, return failure
	if(!dib)
		return false;

	//retrieve the image data
	dib = FreeImage_ConvertTo24Bits(dib);
	bits = FreeImage_GetBits(dib);
	//get the image width and height
	width = FreeImage_GetWidth(dib);
	height = FreeImage_GetHeight(dib);
	//if this somehow one of these failed (they shouldn't), return failure
	if((bits == 0) || (width == 0) || (height == 0))
		return false;
	if(FreeImage_IsLittleEndian())
	{
		int i, j;
		for(i = 0; i < height; i ++)
		{
			for(j = 0; j < width; j ++)
			{
				BYTE r = bits[3 * (i * width + j) + 0];
				BYTE g = bits[3 * (i * width + j) + 1];
				BYTE b = bits[3 * (i * width + j) + 2];

				bits[3 * (i * width + j) + 0] = b;
				bits[3 * (i * width + j) + 2] = r;
			}
		}
	}

	// Assign texture to ID
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, gammaCorrection ? GL_SRGB : GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, bits);
	glGenerateMipmap(GL_TEXTURE_2D);

	// Parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	//Free FreeImage's copy of the data
	FreeImage_Unload(dib);
	return textureID;
}

int VertexBride = (3 + 3 + 2);

void setSingleVertex(int index, GLfloat* data, float x, float y, float z)
{
	*(data + 0 + index * 8) = x;
	*(data + 1 + index * 8) = y;
	*(data + 2 + index * 8) = z;

	//printf("vertex %d %f %f %f\n",(int)data, *(data + 0), *(data + 1), *(data + 2));
}

void setSingleNormal(GLfloat * data, float nx, float ny, float nz)
{
	*(data + 3) = nx;
	*(data + 4) = ny;
	*(data + 5) = nz;
	//printf("normal %d %f %f %f\n",(int)data, *(data + 3), *(data + 4), *(data + 5));
}

void setSingleTex(GLfloat* data, float u, float v)
{
	*(data + 6) = u;
	*(data + 7) = v;
	//printf("tex %d %f %f\n",(int)data, *(data + 6), *(data + 7));
}

GLfloat* nextVertex(GLfloat *data)
{
	return data + 8;
}

GLfloat* nextSide(GLfloat* data, int sides = 1)
{
	return data + 8 * 4 * sides;
}

void setTextureCoords(GLfloat* data, float u, float v )
{
	setSingleTex(data, 0, 0);

	data = nextVertex(data);
	setSingleTex(data, u, 0.0f);

	data = nextVertex(data);
	setSingleTex(data, u, v);

	data = nextVertex(data);
	setSingleTex(data, 0.0f, v);
}

void setNormals(GLfloat* data, float nx, float ny, float nz)
{
	setSingleNormal(data, nx, ny, nz);

	data = nextVertex(data);
	setSingleNormal(data, nx, ny, nz);

	data = nextVertex(data);
	setSingleNormal(data, nx, ny, nz);

	data = nextVertex(data);
	setSingleNormal(data, nx, ny, nz);
}

void buildIndex(MeshMeta* mesh)
{
	GLshort*	indexData = new GLshort[mesh->numQuads * 6];
	int i;
	for(i = 0; i < mesh->numQuads; i ++)
	{
		*(indexData + i * 6 + 0) = i * 4 + 0;
		*(indexData + i * 6 + 1) = i * 4 + 1;
		*(indexData + i * 6 + 2) = i * 4 + 2;

		*(indexData + i * 6 + 3) = i * 4 + 0;
		*(indexData + i * 6 + 4) = i * 4 + 2;
		*(indexData + i * 6 + 5) = i * 4 + 3;
	}
	mesh->indexData = indexData;
}

void buildColum(GLfloat* data, float x, float y, float z, float width)
{
	float w = width * 0.5f;

	// Back side
	setTextureCoords(data, 1.0f, 2.0f);
	setNormals(data, 0.0f, 0.0f, -1.0f);
	setSingleVertex(0, data, x - w, y, z - w);
	setSingleVertex(1, data, x + w, y, z - w);
	setSingleVertex(2, data, x + w, 0.0f, z - w);
	setSingleVertex(3, data, x - w, 0.0f, z - w);

	
	// Right side
	data = nextSide(data);
	setTextureCoords(data, 1.0f, 2.0f);
	setNormals(data, 1.0f, 0.0f, 0.0f);
	setSingleVertex(0, data, x + w, y, z - w);
	setSingleVertex(1, data, x + w, y , z + w);
	setSingleVertex(2, data, x + w, 0.0f, z + w);
	setSingleVertex(3, data, x + w, 0.0f, z - w);
	
	// Front side.
	data = nextSide(data);
	setTextureCoords(data, 1.0f, 2.0f);
	setNormals(data, 0.0f, 0.0f, 1.0f);
	setSingleVertex(0, data, x + w, y, z + w);
	setSingleVertex(1, data, x - w, y, z + w);
	setSingleVertex(2, data, x - w, 0.0f, z + w);
	setSingleVertex(3, data, x + w, 0.0f, z + w);

	// Left side.
	data = nextSide(data);
	setTextureCoords(data, 1.0f, 2.0f);
	setNormals(data, -1.0f, 0.0f, 0.0f);
	setSingleVertex(0, data, x - w, y, z + w);
	setSingleVertex(1, data, x - w, y, z - w);
	setSingleVertex(2, data, x - w, 0.0f, z - w);
	setSingleVertex(3, data, x - w, 0.0f, z + w);
}


void buildGLObject(MeshMeta* mesh)
{
	GLuint vao = 0;
	GLuint vbo = 0;
	GLuint ibo = 0;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * VertexBride * mesh->numQuads * 4, mesh->data, GL_STATIC_DRAW);

	glBindVertexArray(vao);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VertexBride * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, VertexBride * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, VertexBride * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));

	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(mesh->indexData[0]) * mesh->numQuads * 6, (GLvoid*)mesh->indexData,GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	mesh->vao = vao;
	mesh->vbo = vbo;
	mesh->ibo = ibo;
}

void buildRoomVert(MeshMeta* mesh, float width, float height, float depth)
{
	GLfloat *data = mesh->data;
	// Front wall.
	setTextureCoords(data, 7.0f, 2.0f);
	setNormals(data, 0.0f, 0.0f, -1.0);
	setSingleVertex(0, data, 0.0f, height, depth);
	setSingleVertex(1, data, width, height, depth);
	setSingleVertex(2, data, width, 0.0f, depth);
	setSingleVertex(3, data, 0.0f, 0.0f, depth);

	// Back wall.
	data = nextSide(data);
	setTextureCoords(data, 7.0f, 2.0f);
	setNormals(data, 0.0f, 0.0f, 1.0f);
	setSingleVertex(0, data, width, height, 0.0f);
	setSingleVertex(1, data, 0.0f, height, 0.0f);
	setSingleVertex(2, data, 0.0f, 0.0f, 0.0f);
	setSingleVertex(3, data, width, 0.0f, 0.0f);

	// Right wall
	data = nextSide(data);
	setTextureCoords(data, 10.5f, 2.0f);
	setNormals(data, -1.0f, 0.0f, 0.0f);
	setSingleVertex(0, data, width, height, depth);
	setSingleVertex(1, data, width, height, 0.0f);
	setSingleVertex(2, data, width, 0.0f, 0.0f);
	setSingleVertex(3, data, width, 0.0f, depth);

	// Left wall
	data = nextSide(data);
	setTextureCoords(data, 10.5, 2.0f);
	setNormals(data, 1.0f, 0.0f, 0.0f);
	setSingleVertex(0, data, 0.0f, height, 0.0f);
	setSingleVertex(1, data, 0.0f, height, depth);
	setSingleVertex(2, data, 0.0f, 0.0f, depth);
	setSingleVertex(3, data, 0.0f, 0.0f, 0.0f);

	data = nextSide(data);
	buildColum(data, 4.0f, height, 7.0f, 0.75f);

	data = nextSide(data, 4);
	buildColum(data, 4.0f, height, 13.0f, 0.75f);

	data = nextSide(data, 4);
	buildColum(data, 11.0f, height, 7.0f, 0.75f);

	data = nextSide(data, 4);
	buildColum(data, 11.0f, height, 13.0f, 0.75f);
}

MeshMeta* buildRoom(float width, float height, float depth)
{
	GLfloat *data = new GLfloat[VertexBride * 20 * 4];
	MeshMeta* room = new MeshMeta;
	room->data		= data;
	room->numQuads	= 20;

	buildRoomVert(room, width, height, depth);

	buildIndex(room);

	buildGLObject(room);

	room->tex = loadTexture("Data/env2.bmp", false);

	return room;
}

void buildFloorVert(MeshMeta* mesh, float width, float height, float depth)
{
	GLfloat *data = mesh->data;

	setTextureCoords(data, 7.0f, 7.0f);
	setNormals(data, 0.0f, 1.0f, 0.0);
	setSingleVertex(0, data, 0.0f, 0.0f, depth);
	setSingleVertex(1, data, width, 0.0f, depth);
	setSingleVertex(2, data, width, 0.0f, 0.0f);
	setSingleVertex(3, data, 0.0f, 0.0f, 0.0f);
}

MeshMeta* buildFloor(float width, float height, float depth)
{
	GLfloat*data = new GLfloat[VertexBride * 4];
	MeshMeta* floor = new MeshMeta;
	floor->data	= data;
	floor->numQuads = 1;

	buildFloorVert(floor, width, height, depth);
	buildIndex(floor);
	buildGLObject(floor);

	floor->tex = loadTexture("Data/ground2.bmp", false);
	return floor;
}

void buildCeilVert(MeshMeta* mesh, float width, float height, float depth)
{
	GLfloat *data = mesh->data;
	setTextureCoords(data, 7.0f, 2.0f);
	setNormals(data, 0.0f, -1.0f, 0.0f);
	setSingleVertex(0, data, 0.0f, height, 0.0f);
	setSingleVertex(1, data, width, height, 0.0f);
	setSingleVertex(2, data, width, height, depth);
	setSingleVertex(3, data, 0.0f, height, depth);
}

MeshMeta* buildCeil(float width, float height, float depth)
{
	GLfloat *data = new GLfloat[VertexBride * 4];
	MeshMeta* ceildata = new MeshMeta;
	ceildata->data	= data;
	ceildata->numQuads = 1;

	buildCeilVert(ceildata, width, height, depth);
	buildIndex(ceildata);
	buildGLObject(ceildata);

	ceildata->tex = loadTexture("Data/seafloor.bmp", false);
	return ceildata;
}

MeshMeta* buildPaint1(float width, float height, float depth)
{
	GLfloat *data = new GLfloat[VertexBride * 4];
	MeshMeta* paint = new MeshMeta;
	paint->data	= data;
	paint->numQuads = 1;

	setTextureCoords(data, 1.0f, 1.0f);
	setNormals(data, 0.0f, 0.0f, -1.0f);
	setSingleVertex(0, data, 2.0f, height - 0.5f, depth - 0.01f);
	setSingleVertex(1, data, 6.0f, height - 0.5f, depth - 0.01f);
	setSingleVertex(2, data, 6.0f, height - 2.5f, depth - 0.01f);
	setSingleVertex(3, data, 2.0f, height - 2.5f, depth - 0.01f);

	buildIndex(paint);
	buildGLObject(paint);

	paint->tex = loadTexture("Data/env3.bmp", false);
	return paint;
}

MeshMeta* buildPaint2(float width, float height, float depth)
{
	GLfloat *data = new GLfloat[VertexBride * 4];
	MeshMeta* paint = new MeshMeta;
	paint->data	= data;
	paint->numQuads = 1;

	setTextureCoords(data, 1.0f, 1.0f);
	setNormals(data, 0.0f, 0.0f, -1.0f);
	setSingleVertex(0, data, 9.0f, height - 0.5f, depth - 0.01f);
	setSingleVertex(1, data, 13.0f, height - 0.5f, depth - 0.01f);
	setSingleVertex(2, data, 13.0f, height - 2.5f, depth - 0.01f);
	setSingleVertex(3, data, 9.0f, height - 2.5f, depth - 0.01f);

	buildIndex(paint);
	buildGLObject(paint);

	paint->tex = loadTexture("Data/env3.bmp", false);
	return paint;
}

void buildMesh()
{
	const float width = 15.0f;
	const float depth = 20.0f;
	const float height = 3.0f;
	
	gRoomData = buildRoom(width, height, depth);
	gFloorData = buildFloor(width, height, depth);
	gCeilData = buildCeil(width, height, depth);
	PaintData[0] = buildPaint1(width, height, depth);
	PaintData[1] = buildPaint2(width, height, depth);
}

void freeMesh()
{
	MeshMeta* meshes[5];
	meshes[0] = gRoomData;
	meshes[1] = gCeilData;
	meshes[2] = gFloorData;
	meshes[3] = PaintData[0];
	meshes[4] = PaintData[1];
	int i;
	for( i = 0 ; i < 5; i ++ )
	{
		delete []meshes[i]->data;
		delete meshes[i];
	}
	gRoomData = NULL;
	gCeilData = NULL;
	gFloorData = NULL;
	PaintData[0] = PaintData[1] = NULL;
}

void drawMesh(MeshMeta* mesh, Shader& shader)
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mesh->tex);
	glUniform1i(glGetUniformLocation(shader.getProgram(), "diffuseTexture"), 0);

	glBindVertexArray(mesh->vao);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ibo);

	glDrawElements(GL_TRIANGLES, mesh->numQuads * 6, GL_UNSIGNED_SHORT, 0);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

Quad* genQuad(CoordRect* rect)
{
	GLfloat quadVertices[] = 
	{
		-1.0f,  1.0f, 0.0f, rect->lu, rect->rv,
		-1.0f, -1.0f, 0.0f, rect->lu, rect->lv,
		 1.0f,  1.0f, 0.0f, rect->ru, rect->rv,
		 1.0f, -1.0f, 0.0f, rect->ru, rect->lv,
	};
	GLuint quadVAO, quadVBO;
	// Setup plane VAO
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));

	glBindVertexArray(0);

	Quad *quad = new Quad;
	quad->vao = quadVAO;
	quad->vbo = quadVBO;

	return quad;
}

Quad* genDefaultQuad()
{
	CoordRect coordRect;
	coordRect.lu = 0.0f;
	coordRect.lv = 0.0f;
	coordRect.ru = 1.0f;
	coordRect.rv = 1.0f;
	return genQuad(&coordRect);
}

void scaledScene(Shader& shader, RenderTexture* scene, RenderTexture* scaled, Quad* quad)
{
	scaled->activateFB();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	shader.use();
	scene->bind();
	glUniform1i(glGetUniformLocation(shader.getProgram(), "width"), scene->getWidth());
	glUniform1i(glGetUniformLocation(shader.getProgram(), "height"), scene->getHeight());

	quad->draw();
}

void renderToScene(Shader& shader, RenderTexture* rt)
{
	rt->activateFB();
	shader.use();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glm::mat4 projection = glm::perspective(gCamera.zoom(), (GLfloat)SCR_W / (GLfloat)SCR_H, 0.1f, 1000.0f);
	glm::mat4 view       = gCamera.getViewMatrix();
	glm::mat4 model		 = glm::mat4();
	model = glm::rotate(model, (float)PI, glm::vec3(0, 1.0f, 0.0f));
	model = glm::translate(model, glm::vec3(-7.0f, -2.0f, -11.0));

	glUniformMatrix4fv(glGetUniformLocation(shader.getProgram(), "projMatrix"),	1, GL_FALSE, glm::value_ptr(projection));
	glUniformMatrix4fv(glGetUniformLocation(shader.getProgram(), "viewMatrix"),	1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(shader.getProgram(), "modelMatrix"),	1, GL_FALSE, glm::value_ptr(model));
	int i;
	for (i = 0; i < 2; i++)
	{
		glm::vec4 newpos = model * gLightPosition[i];

		char strPos[256], strColor[256];
		sprintf(strPos, "lights[%d].position", i);
		sprintf(strColor, "lights[%d].color", i);
		glUniform3fv(glGetUniformLocation(shader.getProgram(), strPos), 1, glm::value_ptr(newpos));
		glUniform3fv(glGetUniformLocation(shader.getProgram(), strColor), 1, &gLightIntensity[i][0]);
	}

	glUniform1f(glGetUniformLocation(shader.getProgram(), "phongCoeff"), 5.0f);
	glUniform1f(glGetUniformLocation(shader.getProgram(), "phongExp"),	1.0f);
	glUniform1f(glGetUniformLocation(shader.getProgram(), "diffCoeff"), 0.5f);
	drawMesh(gRoomData, shader);

	glUniform1f(glGetUniformLocation(shader.getProgram(), "phongCoeff"), 50.0f);
	glUniform1f(glGetUniformLocation(shader.getProgram(), "phongExp"),	3.0f);
	glUniform1f(glGetUniformLocation(shader.getProgram(), "diffCoeff"), 1.0f);
	drawMesh(gFloorData, shader);

	glUniform1f(glGetUniformLocation(shader.getProgram(), "phongCoeff"), 5.0f);
	glUniform1f(glGetUniformLocation(shader.getProgram(), "phongExp"),	0.3f);
	glUniform1f(glGetUniformLocation(shader.getProgram(), "diffCoeff"), 0.3f);
	drawMesh(gCeilData, shader);

	glUniform1f(glGetUniformLocation(shader.getProgram(), "phongCoeff"), 5.0f);
	glUniform1f(glGetUniformLocation(shader.getProgram(), "phongExp"),	0.3f);
	glUniform1f(glGetUniformLocation(shader.getProgram(), "diffCoeff"), 1.0f);
	drawMesh(PaintData[0], shader);
	drawMesh(PaintData[1], shader);
}

void measureLuminance(Shader& iniShader,Shader& sampleShader, Shader& avlumexpShader, RenderTexture* scene, RenderTexture** samples, Quad* quad)
{
	int current = NUM_TONEMAP_TEXTURES - 1;
	samples[current]->activateFB();
	iniShader.use();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	scene->bind();
	glUniform1i(glGetUniformLocation(iniShader.getProgram(), "width"), samples[current]->getWidth());
	glUniform1i(glGetUniformLocation(iniShader.getProgram(), "height"), samples[current]->getHeight());
	quad->draw();

	current --;
	while(current > 0)
	{
		samples[current]->activateFB();
		sampleShader.use();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		samples[current + 1]->bind();
		glUniform1i(glGetUniformLocation(sampleShader.getProgram(), "width"), samples[current + 1]->getWidth());
		glUniform1i(glGetUniformLocation(sampleShader.getProgram(), "height"), samples[current + 1]->getHeight());
		quad->draw();
		current --;
	}

	avlumexpShader.use();
	samples[0]->activateFB();
	avlumexpShader.use();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	samples[1]->bind();
	glUniform1i(glGetUniformLocation(avlumexpShader.getProgram(), "width"), samples[1]->getWidth());
	glUniform1i(glGetUniformLocation(avlumexpShader.getProgram(), "height"), samples[1]->getHeight());
	quad->draw();
}

void brightPassToBloom(Shader& gaussBlurShader, RenderTexture* bright, RenderTexture* pingpong1, RenderTexture* pingpong2, RenderTexture*& bloomRT, Quad* quad)
{
	RenderTexture* renderBuffer		= pingpong1;
	RenderTexture* textureBuffer	= pingpong2;
	RenderTexture* swapBuffer;
	
	int numPass = 10;
	int pass = numPass;
	while(pass > 0)
	{
		// horizonal pass.
		renderBuffer->activateFB();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		gaussBlurShader.use();
		glUniform1i(glGetUniformLocation(gaussBlurShader.getProgram(), "width"), SCR_W);
		glUniform1i(glGetUniformLocation(gaussBlurShader.getProgram(), "sampleWidth"), 5);
		glUniform1i(glGetUniformLocation(gaussBlurShader.getProgram(), "sampleType"), 1);
		if(pass == numPass)
		{
			bright->bind();
		}
		else
		{
			textureBuffer->bind();
		}
		quad->draw();

		// vertical pass. 
		textureBuffer->activateFB();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		gaussBlurShader.use();
		glUniform1i(glGetUniformLocation(gaussBlurShader.getProgram(), "height"), SCR_H);
		glUniform1i(glGetUniformLocation(gaussBlurShader.getProgram(), "sampleHeight"), 5);
		glUniform1i(glGetUniformLocation(gaussBlurShader.getProgram(), "sampleType"), 2);
		renderBuffer->bind();
		quad->draw();

		pass --;
	}

	bloomRT = textureBuffer;
}

void brightPass(Shader& brightShader, RenderTexture* bright, RenderTexture* scaled, Quad* quad)
{
	bright->activateFB();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	brightShader.use();
	scaled->bind();
	quad->draw();
}

void combinePass(Shader& combine, RenderTexture* sceneRT, RenderTexture* bloomRT, RenderTexture* luminanceRT, Quad* quad)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SCR_W, SCR_H);
	combine.use();

	glActiveTexture(GL_TEXTURE0);
	sceneRT->bind();
	glUniform1i(glGetUniformLocation(combine.getProgram(), "sceneBuffer"), 0);

	glActiveTexture(GL_TEXTURE1);
	bloomRT->bind();
	glUniform1i(glGetUniformLocation(combine.getProgram(), "bloomBuffer"), 1);

	if(gUseToneMapping)
	{
		glActiveTexture(GL_TEXTURE2);
		luminanceRT->bind();
		glUniform1i(glGetUniformLocation(combine.getProgram(), "hdrBuffer"), 2);
	}
	glUniform1i(glGetUniformLocation(combine.getProgram(), "useHdr"), gUseToneMapping);
	glUniform1i(glGetUniformLocation(combine.getProgram(), "useBloom"), gUseBloom);
	
	quad->draw();
}

void renderToQuad(Shader& shader, RenderTexture* rt, Quad* quad)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, rt->getWidth(), rt->getHeight());
	shader.use();
	rt->bind();
	quad->draw();
}
int main()
{
	// Init GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWwindow* window = glfwCreateWindow(SCR_W, SCR_H, "HDR Lighting", nullptr, nullptr); // Windowed
	glfwMakeContextCurrent(window);

	// Set the required callback functions
	glfwSetKeyCallback(window, keyCallback);
	glfwSetCursorPosCallback(window, mouseCallback);
	glfwSetScrollCallback(window, scrollCallback);

	// Options
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Initialize GLEW to setup the OpenGL Function pointers
	glewExperimental = GL_TRUE;
	if(glewInit() != GLEW_OK)
	{
		printf("Initializing flew fails!\n");
		exit(0);
	}
	// Define the viewport dimensions
	glViewport(0, 0, SCR_W, SCR_H);
	// Setup some OpenGL options
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

	RenderTexture* sceneRT = new RenderTexture();
	sceneRT->init(SCR_W, SCR_H, RenderTexture::RGBA16F, RenderTexture::Depth24);

	RenderTexture* scaledRT = new RenderTexture();
	scaledRT->init(SCR_CROP_W / 4, SCR_CROP_H / 4, RenderTexture::RGBA16F, RenderTexture::NoDepth);

	RenderTexture* brightRT = new RenderTexture();
	brightRT->init(SCR_CROP_W / 4, SCR_CROP_H / 4, RenderTexture::RGBA16F, RenderTexture::NoDepth);

	RenderTexture* pingpongRT[2];
	pingpongRT[0] = new RenderTexture();
	pingpongRT[0]->init(SCR_CROP_W / 4, SCR_CROP_H / 4, RenderTexture::RGBA16F, RenderTexture::NoDepth);

	pingpongRT[1] = new RenderTexture();
	pingpongRT[1]->init(SCR_CROP_W / 4, SCR_CROP_H / 4, RenderTexture::RGBA16F, RenderTexture::NoDepth);

	RenderTexture* bloomRT;

	int i;
	RenderTexture* luminanceRT[NUM_TONEMAP_TEXTURES];
	for(i = 0 ; i < NUM_TONEMAP_TEXTURES ; i ++ )
	{
		int iSampleLen = 1 << (2 * i);
		luminanceRT[i] = new RenderTexture();
		luminanceRT[i]->init(iSampleLen, iSampleLen, RenderTexture::RGBA16F, RenderTexture::NoDepth);
	}

	Shader scene;
	assert(scene.init("Shader/RenderScene.vs", "Shader/RenderScene.frag"));
	Shader drawQuad;
	assert(drawQuad.init("Shader/DrawQuad.vs", "Shader/DrawQuad.frag"));
	Shader downScale4x4;
	assert(downScale4x4.init("Shader/DrawQuad.vs", "Shader/DownScale4x4.frag"));
	Shader avluminit;
	assert(avluminit.init("Shader/DrawQuad.vs", "Shader/SampleAvLumInit.frag"));
	Shader avlumpass;
	assert(avlumpass.init("Shader/DrawQuad.vs", "Shader/SampleAvLumPass.frag"));
	Shader avlumexp;
	assert(avlumexp.init("Shader/DrawQuad.vs", "Shader/SampleAvLumExp.frag"));
	Shader bright;
	assert(bright.init("Shader/DrawQuad.vs", "Shader/BrightPassFilter.frag"));
	Shader gaussBlur5x5;
	assert(gaussBlur5x5.init("Shader/DrawQuad.vs", "Shader/GaussBlur5x5.frag"));
	Shader combine;
	assert(combine.init("Shader/DrawQuad.vs", "Shader/CombinePass.frag"));

	buildMesh();

	Quad* defaultQuad	= genDefaultQuad();

	refreshLights();

	// set up the frame info.
	GLfloat currentFrame = glfwGetTime();
	GLfloat lastFrame = currentFrame;

	printMode();
	// Game loop
	while (!glfwWindowShouldClose(window))
	{
		// Set frame time
		currentFrame = glfwGetTime();
		GLfloat deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		updateFrameDelta(deltaTime);

		// Check and call events
		glfwPollEvents();

		renderToScene(scene, sceneRT);
		
		scaledScene(downScale4x4, sceneRT, scaledRT, defaultQuad);

		if(gUseToneMapping)
			measureLuminance(avluminit, avlumpass, avlumexp, scaledRT, luminanceRT, defaultQuad);

		brightPass(bright, brightRT, scaledRT, defaultQuad);

		brightPassToBloom(gaussBlur5x5, brightRT, pingpongRT[0], pingpongRT[1], bloomRT, defaultQuad);

		if(gMode)
		{
			combinePass(combine, sceneRT, bloomRT, luminanceRT[0], defaultQuad);
		}
		else
		{
			switch(gQuadIndex)
			{
			case 0:
				renderToQuad(drawQuad, sceneRT, defaultQuad);
				break;
			case 1:
				renderToQuad(drawQuad, scaledRT, defaultQuad);
				break;
			case 2:
				renderToQuad(drawQuad, brightRT, defaultQuad);
				break;
			case 3:
				renderToQuad(drawQuad, bloomRT, defaultQuad);
				break;
			case 4:
				renderToQuad(drawQuad, luminanceRT[3], defaultQuad);
				break;
			case 5:
				renderToQuad(drawQuad, luminanceRT[2], defaultQuad);
				break;
			case 6:
				renderToQuad(drawQuad, luminanceRT[1], defaultQuad);
				break;
			case 7:
				renderToQuad(drawQuad, luminanceRT[0], defaultQuad);
				break;
			}
		}
		// Swap the buffers
		glfwSwapBuffers(window);
	}
	delete sceneRT;
	delete scaledRT;
	for(i = 0 ; i < NUM_TONEMAP_TEXTURES ; i ++ )
	{
		delete luminanceRT[i];
	}
	delete defaultQuad;

	freeMesh();
	glfwTerminate();
	return 0;
}