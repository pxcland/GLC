/*	---------------------------------------------------------------------
 *	glc.h - OpenGL Common function library
 *	Performs frequently used common tasks in OpenGL
 *	
 *
 *	Copyright 2017 Patrick Cland
 *	www.setsunasoft.com
 *
 *	Permission is hereby granted, free of charge, to any person obtaining 
 *	a copy of this software and associated documentation files (the "Software"),
 *	to deal in the Software without restriction, including without limitation
 *	the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 *	and/or sell copies of the Software, and to permit persons to whom the 
 *	Software is furnished to do so, subject to the following conditions:
 *
 *	The above copyright notice and this permission notice shall be included
 *	in all copies or substantial portions of the Software.

 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 *	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 *	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 *	THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 *	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
 *	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 *	SOFTWARE.
 *	----------------------------------------------------------------------------- */

#ifndef GLC_H
#define GLC_H

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <malloc.h>
#include <stdarg.h>

#ifndef GL_VERSION
#error OpenGL not defined. Make sure to include the OpenGL library before including glc.h
#endif

/* Defines */


/*	glcMakeShaderProgram()
	Returns: 1 (success) or -1 (failure)
	program - pointer to a to be shader program handle
	vertexShaderPath - path to vertex shader source code file
	fragmentShaderPath - path to fragment shader source code file */
int glcMakeShaderProgram(GLuint* program, const char* vertexShaderPath, const char* fragmentShaderPath)
{
	int length = 0;
	int success;
	char* source = NULL;
	char infoLog[512];
	FILE* vertexFile, *fragmentFile;
	GLuint vertexShader, fragmentShader;

	if(!(vertexFile = fopen(vertexShaderPath, "rb")))
	{
		fprintf(stderr, "Error opening vertex shader: %s\n", vertexShaderPath);
		return -1;
	}
	if(!(fragmentFile = fopen(fragmentShaderPath, "rb")))
	{
		fprintf(stderr, "Error opening fragment shader: %s\n", fragmentShaderPath);
		fclose(vertexFile);
		return -1;
	}
	/* Compile Vertex Shader */
	do { length++; } while(fgetc(vertexFile) != EOF);
	fseek(vertexFile, 0, SEEK_SET);
	source = calloc(length + 1, sizeof(char));
	fread(source, sizeof(char), length, vertexFile);
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	if(!vertexShader)
	{
		fprintf(stderr, "Error creating vertex shader object.\n");
		fclose(vertexFile); fclose(fragmentFile);
		free(source);
		return -1;
	}
	glShaderSource(vertexShader, 1, &source, NULL);
	glCompileShader(vertexShader);
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		fprintf(stderr, "Vertex shader compilation error!\n%s\n", infoLog);
		fclose(vertexFile); fclose(fragmentFile);
		free(source);
		return -1;
	}
	free(source);
	/* Compile Fragment Shader */
	length = 0;
	do{ length++; } while(fgetc(fragmentFile) != EOF);
	fseek(fragmentFile, 0, SEEK_SET);
	source = calloc(length + 1, sizeof(char));
	fread(source, sizeof(char), length, fragmentFile);
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	if(!fragmentShader)
	{
		fprintf(stderr, "Error creating fragment shader object.\n");
		fclose(vertexFile); fclose(fragmentFile);
		free(source);
		return -1;
	}
	glShaderSource(fragmentShader, 1, &source, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		fprintf(stderr, "Fragment shader compilation error!\n%s\n", infoLog);
		fclose(vertexFile); fclose(fragmentFile);
		free(source);
		return -1;
	}
	fclose(vertexFile); fclose(fragmentFile);
	free(source);
	/* Create Shader Program */
	*program = glCreateProgram();
	if(!(*program))
	{
		fprintf(stderr, "Error creating shader program object.\n");
		return -1;
	}
	glAttachShader(*program, vertexShader);
	glAttachShader(*program, fragmentShader);
	glLinkProgram(*program);
	glGetProgramiv(*program, GL_LINK_STATUS, &success);
	if(!success)
	{
		glGetProgramInfoLog(*program, 512, NULL, infoLog);
		fprintf(stderr, "Shader program linking error!\n%s\n", infoLog);
		return -1;
	}

	/* cleanup */
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	return 1;
}

/*	glcSetUniformx()
	Returns: 1 (success) or -1 (failure)
	program - shader program handle
	name - name of uniform
	type - data type to be sent to shader
		Accepted values are: GL_FLOAT, GL_INT, GL_UNSIGNED_INT
		Anything else is treated as GL_UNSIGNED_INT
	dimension - value of 1-4 to show size of uniform 
	... - 1 to 4 values for uniform */
int glcSetUniformx(GLuint program, const char* name, int type, int dimension, ...)
{
	int i;
	void* data;
	GLint location;
	va_list vl;

	if(dimension < 1 || dimension > 4)
	{
		fprintf(stderr, "Dimension invalid. Must be 1-4. Got %d.\n", dimension);
		return -1;
	}
	location = glGetUniformLocation(program, name);
	if(location == -1)
	{
		fprintf(stderr, "Uniform name invalid: %s\n", name);
		return -1;
	}
	glUseProgram(program);

	va_start(vl, dimension);
	data = malloc(	(type == GL_FLOAT) ? sizeof(GLfloat) * dimension :
					(type == GL_INT) ? sizeof(GLint) * dimension :
					sizeof(GLuint) * dimension);
	if(!data)
	{
		fprintf(stderr, "Error allocating memory when setting uniform.\n");
		va_end(vl);
		return -1;
	}
	for(i = 0; i < dimension; i++)
	{
		if(type == GL_FLOAT)
			((GLfloat*)data)[i] = (GLfloat)va_arg(vl, double);
		else if(type == GL_INT)
			((GLint*)data)[i] = (GLint)va_arg(vl, int);
		else
			((GLuint*)data)[i] = (GLuint)va_arg(vl, unsigned int);
	}
	va_end(vl);

	if(dimension == 1)
		(type == GL_FLOAT)	? glUniform1fv(location, 1, (GLfloat*)data) :
		(type == GL_INT)	? glUniform1iv(location, 1, (GLint*)data) :
		glUniform1uiv(location, 1, (GLuint*)data);
	else if(dimension == 2)
		(type == GL_FLOAT)	? glUniform2fv(location, 1, (GLfloat*)data) :
		(type == GL_INT)	? glUniform2iv(location, 1, (GLint*)data) :
		glUniform2uiv(location, 1, (GLuint*)data);
	else if(dimension == 3)
		(type == GL_FLOAT)	? glUniform3fv(location, 1, (GLfloat*)data) :
		(type == GL_INT)	? glUniform3iv(location, 1, (GLint*)data) :
		glUniform3uiv(location, 1, (GLuint*)data);
	else
		(type == GL_FLOAT)	? glUniform4fv(location, 1, (GLfloat*)data) :
		(type == GL_INT)	? glUniform4iv(location, 1, (GLint*)data) :
		glUniform4uiv(location, 1, (GLuint*)data);

	free(data);
	return 1;
}

/*	glcSetUniformxv()
	Returns: 1 (success) or -1 (failure)
	program - shader program handle
	name - name of uniform
	type - data type to be sent to shader
		Accepted values are: GL_FLOAT, GL_INT, GL_UNSIGNED_INT
		Anything else is treated as GL_UNSIGNED_INT
	dimension - value of 1-4 to show size of uniform array element
	amount - amount of elements in uniform array to set
	uniform - pointer to contiguous uniform values*/
int glcSetUniformxv(GLuint program, const char* name, int type, int dimension, int amount, const void* uniform)
{
	GLint location;

	if(dimension < 1 || dimension > 4)
	{
		fprintf(stderr, "Dimension invalid. Must be 1-4. Got %d.\n", dimension);
		return -1;
	}
	if(amount <= 0)
	{
		fprintf(stderr, "Amount invalid. Must be larger than 0. Got %d\n", amount);
		return -1;
	}
	location = glGetUniformLocation(program, name);
	if(location == -1)
	{
		fprintf(stderr, "Uniform name invalid: %s\n", name);
		return -1;
	}
	glUseProgram(program);

	if(dimension == 1)
		(type == GL_FLOAT)	? glUniform1fv(location, amount, (GLfloat*)uniform) :
		(type == GL_INT)	? glUniform1iv(location, amount, (GLint*)uniform) :
		glUniform1uiv(location, amount, (GLuint*)uniform);
	else if(dimension == 2)
		(type == GL_FLOAT)	? glUniform2fv(location, amount, (GLfloat*)uniform) :
		(type == GL_INT)	? glUniform2iv(location, amount, (GLint*)uniform) :
		glUniform2uiv(location, amount, (GLuint*)uniform);
	else if(dimension == 3)
		(type == GL_FLOAT)	? glUniform3fv(location, amount, (GLfloat*)uniform) :
		(type == GL_INT)	? glUniform3iv(location, amount, (GLint*)uniform) :
		glUniform3uiv(location, amount, (GLuint*)uniform);
	else
		(type == GL_FLOAT)	? glUniform4fv(location, amount, (GLfloat*)uniform) :
		(type == GL_INT)	? glUniform4iv(location, amount, (GLint*)uniform) :
		glUniform4uiv(location, amount, (GLuint*)uniform);

	return 1;
}

int glcSetUniformMatx(GLuint program, const char* name, int dimension, int amount, GLboolean transpose, const void* uniform)
{
	GLint location;
	if(dimension < 2 || dimension > 4)
	{
		fprintf(stderr, "Dimension invalid. Must be 2-4. Got %d.\n", dimension);
		return -1;
	}
	location = glGetUniformLocation(program, name);
	if(location == -1)
	{
		fprintf(stderr, "Uniform name invalid: %s\n", name);
		return -1;
	}
	glUseProgram(program);

	if(dimension == 2)
		glUniformMatrix2fv(location, amount, transpose, (GLfloat*)uniform);
	else if(dimension == 3)
		glUniformMatrix3fv(location, amount, transpose, (GLfloat*)uniform);
	else
		glUniformMatrix4fv(location, amount, transpose, (GLfloat*)uniform);

	return 1;
}

#endif
