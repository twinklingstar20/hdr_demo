#ifndef SHADER_H_
#define SHADER_H_

class Shader
{
public:
    Shader();

    void use();

	bool init(const char* vertexPath, const char* fragmentPath);

	unsigned int getProgram()
	{
		return mProgram;
	}
protected:
	unsigned int mProgram;
    char* readShaderSource(const char *fileName);
	unsigned int genShader(unsigned int type, const char* fileName);
	unsigned int linkProgram(unsigned int* shader, int shaderNum);
};

#endif