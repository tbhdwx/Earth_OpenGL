#include "GL\glew.h"
#include "GL\freeglut.h"
#include <iostream>  
#include <vector>

#include "textfile.h"  
#include "FreeImage.h"

using namespace std;

GLuint vShader, fShader;//顶点着色器对象  

GLuint vaoHandle;//vertex array object  

GLint time_id;

GLint myTextureIdentityShader;
GLuint textureID;

//法线纹理
GLuint normal_texture;

GLfloat xPos = 0.0f;
GLfloat yPos = 0.0f;

GLfloat xScale = 1.0f;

int mouseX = 0, mouseY = 0;

GLuint programHandle = 0;
typedef float M3DMatrix44f[16];
typedef float	M3DMatrix33f[9];

//地球半径
double x = 6378137.0;
double y = 6378137.0;
double z = 6356752.314245;

//投影矩阵
M3DMatrix44f projMatrix;

#define M3D_PI 3.14159265358979323846

#define A(row,col)  a[(col<<2)+row]
#define B(row,col)  b[(col<<2)+row]
#define P(row,col)  product[(col<<2)+row]



//三角形个数
int NumberOfTriangle(int numberOfSlice, int numberOfStack)
{
	int AllCount = 2 * numberOfSlice;
	AllCount += 2 * ((numberOfStack - 2) * numberOfSlice);
	return AllCount;
}

//顶点个数
int NumberOfVertices(int numberOfSlice, int numberOfStack)
{
	return 2 + ((numberOfStack - 1) * numberOfSlice);
}


vector<double> positions;
vector<unsigned short> indices;
//生成球体数据
void makeSphere(int nSlice, int nStack)
{
	int numberOfVertices = NumberOfVertices(nSlice, nStack);

	vector<double> cosTheta;
	vector<double> sinTheta;
	for (int j = 0; j < nSlice; ++j)
	{
		double theta = M3D_PI * 2 * (((double)j) / nSlice);
		cosTheta.push_back(cos(theta));
		sinTheta.push_back(sin(theta));
	}

	positions.push_back(0.0);
	positions.push_back(0.0);
	positions.push_back(z);

	for (int i = 1; i < nStack; ++i)
	{
		double phi = M3D_PI * (((double)i) / nStack);
		double sinPhi = sin(phi);

		double xSinPhi = x * sinPhi;
		double ySinPhi = y * sinPhi;
		double zCosPhi = z * cos(phi);

		for (int j = 0; j < nSlice; ++j)
		{
			positions.push_back(cosTheta[j] * xSinPhi);
			positions.push_back(sinTheta[j] * ySinPhi);
			positions.push_back(zCosPhi);
		}
	}

	positions.push_back(0.0);
	positions.push_back(0.0);
	positions.push_back(-z);

	//计算顶部三角形扇形
	for (int j = 1; j < nSlice; ++j)
	{
		indices.push_back(0);
		indices.push_back(j);
		indices.push_back(j + 1);
	}
	indices.push_back(0);
	indices.push_back(nSlice);
	indices.push_back(1);


	//计算中部的三角形带
	for (int i = 0; i < nStack - 2; ++i)
	{
		int topRowOffset = (i * nSlice) + 1;
		int bottomRowOffset = ((i + 1) * nSlice) + 1;

		for (int j = 0; j < nSlice - 1; ++j)
		{
			indices.push_back(bottomRowOffset + j);
			indices.push_back(bottomRowOffset + j + 1);
			indices.push_back(topRowOffset + j + 1);
			indices.push_back(bottomRowOffset + j);
			indices.push_back(topRowOffset + j + 1);
			indices.push_back(topRowOffset + j);
		}

		indices.push_back(bottomRowOffset + nSlice - 1);
		indices.push_back(bottomRowOffset);
		indices.push_back(topRowOffset);

		indices.push_back(bottomRowOffset + nSlice - 1);
		indices.push_back(topRowOffset);
		indices.push_back(topRowOffset + nSlice - 1);
	}

	//添加底部的三角扇形
	int lastPosition = positions.size() / 3 - 1;
	for (int j = lastPosition - 1; j >= lastPosition - nSlice; --j)
	{
		indices.push_back(lastPosition);
		indices.push_back(j);
		indices.push_back(j - 1);
	}
	indices.push_back(lastPosition);
	indices.push_back(lastPosition - nSlice);
	indices.push_back(lastPosition - 1);
}

inline void m3dExtractRotationMatrix33(M3DMatrix33f dst, const M3DMatrix44f src)
{
	memcpy(dst, src, sizeof(float)* 3); // X column
	memcpy(dst + 3, src + 4, sizeof(float)* 3); // Y column
	memcpy(dst + 6, src + 8, sizeof(float)* 3); // Z column

	double sqrtLength0 = sqrtf(dst[0] * dst[0] + dst[1] * dst[1] + dst[2] * dst[2]);
	dst[0] /= sqrtLength0;
	dst[1] /= sqrtLength0;
	dst[2] /= sqrtLength0;

	double sqrtLength3 = sqrtf(dst[3] * dst[3] + dst[4] * dst[4] + dst[5] * dst[5]);
	dst[3] /= sqrtLength3;
	dst[4] /= sqrtLength3;
	dst[5] /= sqrtLength3;

	double sqrtLength6 = sqrtf(dst[6] * dst[6] + dst[7] * dst[7] + dst[8] * dst[8]);
	dst[6] /= sqrtLength6;
	dst[7] /= sqrtLength6;
	dst[8] /= sqrtLength6;


}

///////////////////////////////////////////////////////////////////////////////
void m3dLoadIdentity44(M3DMatrix44f m)
{
	// Don't be fooled, this is still column major
	static M3DMatrix44f	identity = { 1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f };

	memcpy(m, identity, sizeof(M3DMatrix44f));
}

inline void m3dTranslationMatrix44(M3DMatrix44f m, float x, float y, float z)
{
	m3dLoadIdentity44(m); m[12] = x; m[13] = y; m[14] = z;
}

inline void m3dScaleMatrix44(M3DMatrix44f m, float x, float y, float z)
{
	m3dLoadIdentity44(m); m[0] = x; m[5] = y; m[10] = z;
}



void m3dRotationMatrix44(M3DMatrix44f m, float angle, float x, float y, float z)
{
	float mag, s, c;
	float xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c;

	s = float(sin(angle));
	c = float(cos(angle));

	mag = float(sqrt(x*x + y*y + z*z));

	// Identity matrix
	if (mag == 0.0f) {
		m3dLoadIdentity44(m);
		return;
	}

	// Rotation matrix is normalized
	x /= mag;
	y /= mag;
	z /= mag;

#define M(row,col)  m[col*4+row]

	xx = x * x;
	yy = y * y;
	zz = z * z;
	xy = x * y;
	yz = y * z;
	zx = z * x;
	xs = x * s;
	ys = y * s;
	zs = z * s;
	one_c = 1.0f - c;

	M(0, 0) = (one_c * xx) + c;
	M(0, 1) = (one_c * xy) - zs;
	M(0, 2) = (one_c * zx) + ys;
	M(0, 3) = 0.0f;

	M(1, 0) = (one_c * xy) + zs;
	M(1, 1) = (one_c * yy) + c;
	M(1, 2) = (one_c * yz) - xs;
	M(1, 3) = 0.0f;

	M(2, 0) = (one_c * zx) - ys;
	M(2, 1) = (one_c * yz) + xs;
	M(2, 2) = (one_c * zz) + c;
	M(2, 3) = 0.0f;

	M(3, 0) = 0.0f;
	M(3, 1) = 0.0f;
	M(3, 2) = 0.0f;
	M(3, 3) = 1.0f;

#undef M
}

// Multiply two 4x4 matricies
void m3dMatrixMultiply44(M3DMatrix44f product, const M3DMatrix44f a, const M3DMatrix44f b)
{
	for (int i = 0; i < 4; i++) {
		float ai0 = A(i, 0), ai1 = A(i, 1), ai2 = A(i, 2), ai3 = A(i, 3);
		P(i, 0) = ai0 * B(0, 0) + ai1 * B(1, 0) + ai2 * B(2, 0) + ai3 * B(3, 0);
		P(i, 1) = ai0 * B(0, 1) + ai1 * B(1, 1) + ai2 * B(2, 1) + ai3 * B(3, 1);
		P(i, 2) = ai0 * B(0, 2) + ai1 * B(1, 2) + ai2 * B(2, 2) + ai3 * B(3, 2);
		P(i, 3) = ai0 * B(0, 3) + ai1 * B(1, 3) + ai2 * B(2, 3) + ai3 * B(3, 3);
	}
}

void SetPerspective(float fFov, float fAspect, float fNear, float fFar)
{
	float xmin, xmax, ymin, ymax;       // Dimensions of near clipping plane
	float xFmin, xFmax, yFmin, yFmax;   // Dimensions of far clipping plane

	// Do the Math for the near clipping plane
	ymax = fNear * float(tan(fFov * M3D_PI / 360.0));
	ymin = -ymax;
	xmin = ymin * fAspect;
	xmax = -xmin;

	// Construct the projection matrix
	m3dLoadIdentity44(projMatrix);
	projMatrix[0] = (2.0f * fNear) / (xmax - xmin);
	projMatrix[5] = (2.0f * fNear) / (ymax - ymin);
	projMatrix[8] = (xmax + xmin) / (xmax - xmin);
	projMatrix[9] = (ymax + ymin) / (ymax - ymin);
	projMatrix[10] = -((fFar + fNear) / (fFar - fNear));
	projMatrix[11] = -1.0f;
	projMatrix[14] = -((2.0f * fFar * fNear) / (fFar - fNear));
	projMatrix[15] = 0.0f;
}

bool LoadImageTexture(const char *szFileName, GLenum minFilter, GLenum magFilter, GLenum wrapMode)
{
	FREE_IMAGE_FORMAT format = FreeImage_GetFileType(szFileName, 0);
	FIBITMAP *bitmap = FreeImage_Load(format, szFileName);
	if (!bitmap)
	{
		return false;
	}

	bitmap = FreeImage_ConvertTo32Bits(bitmap);

	int mlWidth = FreeImage_GetWidth(bitmap);
	int mlHeight = FreeImage_GetHeight(bitmap);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mlWidth, mlHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE,
		(void*)FreeImage_GetBits(bitmap));
	FreeImage_Unload(bitmap);

	if (minFilter == GL_LINEAR_MIPMAP_LINEAR ||
		minFilter == GL_LINEAR_MIPMAP_NEAREST ||
		minFilter == GL_NEAREST_MIPMAP_LINEAR ||
		minFilter == GL_NEAREST_MIPMAP_NEAREST)
		glGenerateMipmap(GL_TEXTURE_2D);

	return true;
}

void initShader(const char *VShaderFile, const char *FShaderFile)
{
	//1、查看GLSL和OpenGL的版本  
	const GLubyte *renderer = glGetString(GL_RENDERER);
	const GLubyte *vendor = glGetString(GL_VENDOR);
	const GLubyte *version = glGetString(GL_VERSION);
	const GLubyte *glslVersion =
		glGetString(GL_SHADING_LANGUAGE_VERSION);
	GLint major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	cout << "GL Vendor    :" << vendor << endl;
	cout << "GL Renderer  : " << renderer << endl;
	cout << "GL Version (string)  : " << version << endl;
	cout << "GL Version (integer) : " << major << "." << minor << endl;
	cout << "GLSL Version : " << glslVersion << endl;

	//2、编译着色器  
	//创建着色器对象：顶点着色器  
	vShader = glCreateShader(GL_VERTEX_SHADER);
	//错误检测  
	if (0 == vShader)
	{
		cerr << "ERROR : Create vertex shader failed" << endl;
		exit(1);
	}

	//把着色器源代码和着色器对象相关联  
	const GLchar *vShaderCode = textFileRead(VShaderFile);
	glShaderSource(vShader, 1, &vShaderCode, NULL);

	//编译着色器对象  
	glCompileShader(vShader);


	//检查编译是否成功  
	GLint compileResult;
	glGetShaderiv(vShader, GL_COMPILE_STATUS, &compileResult);
	if (GL_FALSE == compileResult)
	{
		GLint logLen;
		//得到编译日志长度  
		glGetShaderiv(vShader, GL_INFO_LOG_LENGTH, &logLen);
		if (logLen > 0)
		{
			char *log = (char *)malloc(logLen);
			GLsizei written;
			//得到日志信息并输出  
			glGetShaderInfoLog(vShader, logLen, &written, log);
			cerr << "vertex shader compile log : " << endl;
			cerr << log << endl;
			free(log);//释放空间  
		}
	}

	//创建着色器对象：片断着色器  
	fShader = glCreateShader(GL_FRAGMENT_SHADER);
	//错误检测  
	if (0 == fShader)
	{
		cerr << "ERROR : Create fragment shader failed" << endl;
		exit(1);
	}

	//把着色器源代码和着色器对象相关联  
	const GLchar *fShaderCode = textFileRead(FShaderFile);
	glShaderSource(fShader, 1, &fShaderCode, NULL);

	//编译着色器对象  
	glCompileShader(fShader);

	//检查编译是否成功  
	glGetShaderiv(fShader, GL_COMPILE_STATUS, &compileResult);
	if (GL_FALSE == compileResult)
	{
		GLint logLen;
		//得到编译日志长度  
		glGetShaderiv(fShader, GL_INFO_LOG_LENGTH, &logLen);
		if (logLen > 0)
		{
			char *log = (char *)malloc(logLen);
			GLsizei written;
			//得到日志信息并输出  
			glGetShaderInfoLog(fShader, logLen, &written, log);
			cerr << "fragment shader compile log : " << endl;
			cerr << log << endl;
			free(log);//释放空间  
		}
	}

	//3、链接着色器对象  
	//创建着色器程序  
	programHandle = glCreateProgram();
	if (!programHandle)
	{
		cerr << "ERROR : create program failed" << endl;
		exit(1);
	}
	//将着色器程序链接到所创建的程序中  
	glAttachShader(programHandle, vShader);
	glAttachShader(programHandle, fShader);

	glBindAttribLocation(programHandle, 0, "vVertex");

	//将这些对象链接成一个可执行程序  
	glLinkProgram(programHandle);

	glDeleteShader(vShader);
	glDeleteShader(fShader);
	//查询链接的结果  
	GLint linkStatus;
	glGetProgramiv(programHandle, GL_LINK_STATUS, &linkStatus);
	if (GL_FALSE == linkStatus)
	{
		cerr << "ERROR : link shader program failed" << endl;
		GLint logLen;
		glGetProgramiv(programHandle, GL_INFO_LOG_LENGTH,
			&logLen);
		if (logLen > 0)
		{
			char *log = (char *)malloc(logLen);
			GLsizei written;
			glGetProgramInfoLog(programHandle, logLen,
				&written, log);
			cerr << "Program log : " << endl;
			cerr << log << endl;
		}
	}
	else//链接成功，在OpenGL管线中使用渲染程序  
	{
		glUseProgram(programHandle);
	}
}

void initVBO()
{
	glGenVertexArrays(1, &vaoHandle);
	glBindVertexArray(vaoHandle);

	// Create and populate the buffer objects  
	GLuint vboHandles[2];
	glGenBuffers(2, vboHandles);
	GLuint positionBufferHandle = vboHandles[0];
	GLuint IndexBufferHandle = vboHandles[1];

	//绑定VBO以供使用  
	glBindBuffer(GL_ARRAY_BUFFER, positionBufferHandle);
	int nVertex = positions.size();
	float *positionData = new float[nVertex];
	for (int i = 0; i != nVertex; ++i)
	{
		positionData[i] = positions[i];
	}

	//加载数据到VBO  
	glBufferData(GL_ARRAY_BUFFER, nVertex * sizeof(float), positionData, GL_DYNAMIC_DRAW);

	int nIndices = indices.size();
	unsigned short *indexData = new unsigned short[nIndices];
	for (int i = 0; i != nIndices; ++i)
	{
		indexData[i] = indices[i];
	}
	//绑定VBO以供使用  
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferHandle);
	//加载数据到VBO  
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, nIndices * sizeof(unsigned short),
		indexData, GL_DYNAMIC_DRAW);
	
	glBindVertexArray(vaoHandle);
	glEnableVertexAttribArray(0);//顶点坐标  
	//调用glVertexAttribPointer之前需要进行绑定操作  
	glBindBuffer(GL_ARRAY_BUFFER, positionBufferHandle);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLubyte *)NULL);


	glBindVertexArray(0);
}

void init()
{
	//初始化glew扩展库  
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		cout << "Error initializing GLEW: " << glewGetErrorString(err) << endl;
	}

	//开启深度测试
	glEnable(GL_DEPTH_TEST);

	makeSphere(64, 32);

	initVBO();
	initShader("basic.vert", "basic.frag");
	glShadeModel(GL_SMOOTH);
	glGenTextures(1, &textureID);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureID);
	FreeImage_Initialise(TRUE);
	LoadImageTexture("earth.bmp", GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);

	
	glGenTextures(1, &normal_texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normal_texture);
	LoadImageTexture("normalmap.jpg", GL_LINEAR, GL_LINEAR, GL_REPEAT);
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	M3DMatrix44f mScaleMatrix,  tmpTransform, mFinalTransform, mTranslationMatrix, mRotationMatrix1, mRotationMatrix2, mRotationMatrix, mModelView;

	m3dTranslationMatrix44(mTranslationMatrix, 0.0f, 0.0f, -5.0f * x);
	m3dRotationMatrix44(mRotationMatrix1, -xPos * 1.3 /* 0.017453292519943296*/, 0.0f, 1.0f, 0.0f);
	m3dRotationMatrix44(mRotationMatrix2, -yPos * 1.3 /* 0.017453292519943296*/, 1.0f, 0.0f, 0.0f);
	m3dMatrixMultiply44(mRotationMatrix, mRotationMatrix1, mRotationMatrix2);

	m3dScaleMatrix44(mScaleMatrix, xScale, xScale, xScale);
	m3dMatrixMultiply44(mRotationMatrix, mRotationMatrix, mScaleMatrix);

	m3dMatrixMultiply44(mModelView, mTranslationMatrix, mRotationMatrix);
	m3dMatrixMultiply44(mFinalTransform, projMatrix, mModelView);

	M3DMatrix33f normalMatrix;
	m3dExtractRotationMatrix33(normalMatrix, mModelView);
	glUseProgram(programHandle);
	
	GLfloat vEyeLight[] = { 0.0, 0.0,  -1.5 * z };
	GLfloat vAmbientColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	GLfloat vDiffuseColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };

	glUniform4fv(glGetUniformLocation(programHandle, "ambientColor"), 1, vAmbientColor);
	glUniform4fv(glGetUniformLocation(programHandle, "diffuseColor"), 1, vDiffuseColor);
	glUniform3fv(glGetUniformLocation(programHandle, "vLightPosition"), 1, vEyeLight);
	glUniformMatrix4fv(glGetUniformLocation(programHandle, "mvpMatrix"), 1, GL_FALSE, mFinalTransform);
	glUniformMatrix4fv(glGetUniformLocation(programHandle, "mvMatrix"), 1, GL_FALSE, mModelView);
	glUniformMatrix3fv(glGetUniformLocation(programHandle, "normalMatrix"), 1, GL_FALSE, normalMatrix);
	glUniform1i(glGetUniformLocation(programHandle, "colorMap"), 0);
	glUniform1i(glGetUniformLocation(programHandle, "normalMap"), 1);
	//使用VAO、VBO绘制  
	glBindVertexArray(vaoHandle);
	int nIndices = indices.size();
	if (0 != nIndices)
	{
		glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_SHORT, NULL);
	}
	
	glBindVertexArray(0);

	glutSwapBuffers();
	glutPostRedisplay();
}

void keyboard(int key, int x, int y)
{
	GLfloat stepSize = 0.025f;
	if (key == GLUT_KEY_UP)
	{
		yPos += stepSize;
	}

	if (key == GLUT_KEY_DOWN)
	{
		yPos -= stepSize;
	}

	if (key == GLUT_KEY_LEFT)
	{
		xPos -= stepSize;
	}

	if (key == GLUT_KEY_RIGHT)
	{
		xPos += stepSize;
	}
	glutPostRedisplay();
}

void mouse(int button, int state, int x, int y)
{

	if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN))
	{
			mouseX = x;
			mouseY = y;
	}
	else if ((button == 3) && (state == GLUT_UP))
	{
		xScale += 0.03;
	}
	else if ((button == 4) && (state == GLUT_DOWN))
	{
		xScale -= 0.03;
	}
	glutPostRedisplay();
}


void motionMouse(int x, int y)
{
	xPos += float(mouseX - x) / 800;
	yPos += float(mouseY - y) / 600;
	mouseX = x;
	mouseY = y;
}

void ChangeSize(int w, int h)
{
	if (0 == h)
	{
		h = 1;
	}

	glViewport(0, 0, w, h);
	SetPerspective(35.0f, float(w) / float(h), x * 0.01f, x * 10.f);
}

int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(800, 600);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Earth");
	init();
	glutDisplayFunc(display);
	glutReshapeFunc(ChangeSize);
	glutSpecialFunc(keyboard);
	glutMouseFunc(mouse);
	glutMotionFunc(motionMouse);

	glutMainLoop();
	return 0;
}