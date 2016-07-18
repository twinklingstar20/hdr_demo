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


// The width and height of the window.
const GLuint SCR_W = 800;
const GLuint SCR_H = 200;

#define MODE_DEFAULT		0x00
#define MODE_GAUSS_NORMAL	0x01
#define MODE_GAUSS_VAR1		0x02

#define PI 3.141592653589

int gMode = MODE_DEFAULT;
int gPass = 1;

// About key messages.
bool	gKeys[1024];

Camera gCamera(glm::vec3(0.0f, 0.0f, 5.0f));

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


void printMode()
{

	printf("**************************************************************\n");
	if(gMode == MODE_DEFAULT)
		printf("无任何模糊处理\n");
	else if(gMode == MODE_GAUSS_NORMAL)
		printf("普通的高斯滤波\n");
	else if(gMode == MODE_GAUSS_VAR1)
		printf("高斯滤波变种1\n");
	printf("模糊处理次数%d\n", gPass);
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
		gMode = (gMode + 1) % 3;
	if(gKeys[GLFW_KEY_W])
		gPass = gPass % 10 + 1;
	if(gKeys[GLFW_KEY_S])
		gPass = (gPass - 2 + 10) % 10 + 1;

	printMode();
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

	// Assign texture to ID
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, gammaCorrection ? GL_SRGB : GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, bits);
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

void renderToQuad(Shader& shader, RenderTexture* rt, Quad* quad)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, rt->getWidth(), rt->getHeight());
	shader.use();
	rt->bind();
	quad->draw();
}

Quad* genQuad(float lu, float lv, float ru, float rv)
{
	GLfloat quadVertices[] = 
	{
		-1.0f,  1.0f, 0.0f, lu, rv,
		-1.0f, -1.0f, 0.0f, lu, lv,
		 1.0f,  1.0f, 0.0f, ru, rv,
		 1.0f, -1.0f, 0.0f, ru, lv,
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


void renderDefault(GLuint texture, Shader& defaultShader, Quad* defaultQuad)
{
	// render to the default frame buffer.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, SCR_W, SCR_H);
	glClear(GL_COLOR_BUFFER_BIT);
	defaultShader.use();
	glBindTexture(GL_TEXTURE_2D, texture);
	defaultQuad->draw();
}

void renderGaussBlurDefault(GLuint texture, RenderTexture* pingpong1, RenderTexture* pingpong2, Shader& defalutGaussShader, Shader& defaultShader, Quad* defaultQuad)
{
	RenderTexture* renderBuffer		= pingpong1;
	RenderTexture* textureBuffer	= pingpong2;
	RenderTexture* swapBuffer;
	int pass = gPass;

	while(pass > 0)
	{
		renderBuffer->activateFB();
		glViewport(0, 0, SCR_W, SCR_H);
		glClear(GL_COLOR_BUFFER_BIT);
		defalutGaussShader.use();
		glUniform1i(glGetUniformLocation(defalutGaussShader.getProgram(), "width"), SCR_W);
		glUniform1i(glGetUniformLocation(defalutGaussShader.getProgram(), "height"), SCR_H);
		glUniform1i(glGetUniformLocation(defalutGaussShader.getProgram(), "sampleWidth"), 5);
		glUniform1i(glGetUniformLocation(defalutGaussShader.getProgram(), "sampleHeight"), 5);
		glUniform1i(glGetUniformLocation(defalutGaussShader.getProgram(), "sampleType"), 0);

		if(pass == gPass)
		{
			glBindTexture(GL_TEXTURE_2D, texture); 
		}
		else
		{
			textureBuffer->bind();
		}
		defaultQuad->draw();
		pass --;

		swapBuffer = renderBuffer;
		renderBuffer = textureBuffer;
		textureBuffer = swapBuffer;
	}

	// render to the default frame buffer.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, SCR_W, SCR_H);
	glClear(GL_COLOR_BUFFER_BIT);
	defaultShader.use();

	textureBuffer->bind();
	defaultQuad->draw();
}

void renderGaussBlurVar1(GLuint texture, RenderTexture* pingpong1, RenderTexture* pingpong2, Shader& defalutGaussShader, Shader& defaultShader, Quad* defaultQuad)
{
	RenderTexture* renderBuffer		= pingpong1;
	RenderTexture* textureBuffer	= pingpong2;
	RenderTexture* swapBuffer;
	int pass = gPass;
	while(pass > 0)
	{
		// horizonal pass.
		renderBuffer->activateFB();
		glViewport(0, 0, SCR_W, SCR_H);
		glClear(GL_COLOR_BUFFER_BIT);
		defalutGaussShader.use();
		glUniform1i(glGetUniformLocation(defalutGaussShader.getProgram(), "width"), SCR_W);
		glUniform1i(glGetUniformLocation(defalutGaussShader.getProgram(), "sampleWidth"), 5);
		glUniform1i(glGetUniformLocation(defalutGaussShader.getProgram(), "sampleType"), 1);
		if(pass == gPass)
		{
			glBindTexture(GL_TEXTURE_2D, texture); 
		}
		else
		{
			textureBuffer->bind();
		}
		defaultQuad->draw();

		// vertical pass. 
		textureBuffer->activateFB();
		glViewport(0, 0, SCR_W, SCR_H);
		glClear(GL_COLOR_BUFFER_BIT);
		defalutGaussShader.use();
		glUniform1i(glGetUniformLocation(defalutGaussShader.getProgram(), "height"), SCR_H);
		glUniform1i(glGetUniformLocation(defalutGaussShader.getProgram(), "sampleHeight"), 5);
		glUniform1i(glGetUniformLocation(defalutGaussShader.getProgram(), "sampleType"), 2);
		renderBuffer->bind();
		defaultQuad->draw();

		pass --;
	}

	// render to the default frame buffer.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, SCR_W, SCR_H);
	glClear(GL_COLOR_BUFFER_BIT);
	defaultShader.use();

	textureBuffer->bind();
	defaultQuad->draw();
}

float gaussianDistribution( float x, float y, float rho)
{
	float g = 1.0f / sqrt( 2.0f * PI * rho * rho );
	g *= exp( -( x * x + y * y ) / ( 2 * rho * rho));
	return g;
}

int main()
{
	// Init GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWwindow* window = glfwCreateWindow(SCR_W, SCR_H, "Gaussian", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Set the required callback functions
	glfwSetKeyCallback(window, keyCallback);

	// Options
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Initialize GLEW to setup the OpenGL Function pointers
	glewExperimental = GL_TRUE;
	if(glewInit() != GLEW_OK)
	{
		printf("Initializing flew fails!\n");
		exit(0);
	}
	// Setup some OpenGL options
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

	// set up the frame info.
	GLfloat currentFrame = glfwGetTime();
	GLfloat lastFrame = currentFrame;
	GLfloat deltaTime;
	GLuint	imageTexture = loadTexture("Data/cballs.png");
	Quad*	defaultQuad  = genQuad(0, 0, 1.0f, 1.0f);

	Shader drawQuad = Shader();
	assert(drawQuad.init("Shader/DrawQuad.vs", "Shader/DrawQuad.frag"));
	Shader gaussBlur = Shader();
	assert(gaussBlur.init("Shader/DrawQuad.vs", "Shader/Blur5x5.frag"));

	int x, y;
	float total = 0.0;
	float gaus[5][5];
	for(y = -2; y <= 2; y++)
	{
		for(x = -2; x <= 2; x++)
		{
			gaus[y + 2][x + 2] = gaussianDistribution(x, y, 1.0f);
			total += gaus[y + 2][x + 2];
		}
	}
	for(y = -2; y <= 2; y++)
	{
		for(x = -2; x <= 2; x++)
		{
			printf("%f, ", gaus[y + 2][x + 2] / total);
		}
		printf("\n");
	}
	printf("\n");
	float rowTotal = 0.0f;
	float colTotal = 0.0f;
	int i;
	for(i = 0; i < 5; i ++)
	{
		rowTotal += gaus[2][i];
		colTotal += gaus[i][2];
	}
	for(i = 0; i < 5; i ++)
	{
		printf("%f ", gaus[2][i] / rowTotal);
	}
	printf("\n");
	for(i = 0; i < 5; i ++)
	{
		printf("%f ", gaus[i][2] / colTotal);
	}

	RenderTexture* pingpong1 = new RenderTexture();
	pingpong1->init(SCR_W, SCR_H, RenderTexture::RGBA16F, RenderTexture::NoDepth);
	RenderTexture* pingpong2 = new RenderTexture();
	pingpong2->init(SCR_W, SCR_H, RenderTexture::RGBA16F, RenderTexture::NoDepth);

	printMode();
	// Game loop
	while (!glfwWindowShouldClose(window))
	{
		// Set frame time
		currentFrame = glfwGetTime();
		deltaTime	 = currentFrame - lastFrame;
		lastFrame	 = currentFrame;
		updateFrameDelta(deltaTime);

		glfwPollEvents();

		switch (gMode)
		{
		case MODE_DEFAULT:
			{
				renderDefault(imageTexture, drawQuad, defaultQuad);
			}
			break;
		case MODE_GAUSS_NORMAL:
			{
				renderGaussBlurDefault(imageTexture, pingpong1, pingpong2, gaussBlur, drawQuad, defaultQuad);
			}
			break;
		case MODE_GAUSS_VAR1:
			{
				renderGaussBlurVar1(imageTexture, pingpong1, pingpong2, gaussBlur, drawQuad, defaultQuad);
			}
			break;
		default:
			break;
		}
		// Swap the buffers
		glfwSwapBuffers(window);
	}
	//delete defaultQuad;
	glfwTerminate();
	return 0;
}