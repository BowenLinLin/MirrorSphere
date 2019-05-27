#include "GLTools.h"
#include "GLShaderManager.h"
#include "GLFrustum.h"
#include "GLBatch.h"
#include "GLMatrixStack.h"
#include "GLGeometryTransform.h"
#include "StopWatch.h"

#include <math.h>
#include <stdio.h>

#ifdef __APPLE__
#include <glut/glut.h>
#else
#define FREEGLUT_STATIC
#include <GL/glut.h>
#endif

//添加附加随机球
#define NUM_SPHERES 66
GLFrame spheres[NUM_SPHERES];

GLShaderManager		shaderManager;			// 着色器管理器
GLMatrixStack		modelViewMatrix;		// 模型视图矩阵
GLMatrixStack		projectionMatrix;		// 投影矩阵
GLFrustum			viewFrustum;			// 视景体
GLGeometryTransform	transformPipeline;		// 几何图形变换管道

GLTriangleBatch		bigSphereBatch;         // 大球
GLBatch				floorBatch;             // 地板批处理
GLTriangleBatch     smallSphereBatch;       //小球批处理
GLTriangleBatch     otherSphereBatch;       //非公转小球批处理

//角色帧 照相机角色帧（全局照相机实例）
GLFrame             cameraFrame;

//纹理标记数组
GLuint uiTextures[3];

//颜色值
static GLfloat vFloorColor[] = { 1.0f, 1.0f, 0.0f, 0.75f};
static GLfloat vWhite[] = { 1.0f, 1.0f, 1.0f, 1.0f };

//点光源位置
static GLfloat vLightPos[] = { 1.0f, 3.0f, 0.0f, 1.0f };

//加载指定路径的纹理
bool LoadTGATexture(const char *szFileName, GLenum minFilter, GLenum magFilter, GLenum wrapMode)
{

    GLbyte *pBits;
    int nWidth, nHeight, nComponents;
    GLenum eFormat;
    
    //1.读取纹理数据
    pBits = gltReadTGABits(szFileName, &nWidth, &nHeight, &nComponents, &eFormat);
    if(pBits == NULL)
        return false;
    
    //2、设置纹理参数
    //参数1：纹理维度
    //参数2：为S/T坐标设置模式
    //参数3：wrapMode,环绕模式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
    
    //参数1：纹理维度
    //参数2：线性过滤
    //参数3：过滤方式
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    
    //3.载入纹理
    //参数1：纹理维度
    //参数2：mip贴图层次
    //参数3：纹理单元存储的颜色成分（从读取像素图是获得）-将内部参数nComponents改为了通用压缩纹理格式GL_COMPRESSED_RGB
    //参数4：加载纹理宽
    //参数5：加载纹理高
    //参数6：加载纹理的深度
    //参数7：像素数据的数据类型（GL_UNSIGNED_BYTE，每个颜色分量都是一个8位无符号整数）
    //参数8：指向纹理图像数据的指针
    glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB, nWidth, nHeight, 0,
                 eFormat, GL_UNSIGNED_BYTE, pBits);
    
    //使用完毕释放pBits
    free(pBits);
  
    //4.设置Mip贴图,纹理生成所有的Mip层
    //参数：GL_TEXTURE_1D、GL_TEXTURE_2D、GL_TEXTURE_3D
    glGenerateMipmap(GL_TEXTURE_2D);

    return true;
}

//设置大球顶点
void setBigSphereVertex(){
    //60是径向的,80是横向的,都是越大球越圆
    gltMakeSphere(bigSphereBatch, 0.5f, 60, 80);
}
//绘制打球
void drawBigSphere(){
    static CStopWatch    rotTimer;
    float yRot = rotTimer.GetElapsedSeconds() * 60.0f;
    
    modelViewMatrix.PushMatrix();
    modelViewMatrix.Rotate(yRot, 0.0f, 1.0f, 0.0f);
    //贴上纹理
    glBindTexture(GL_TEXTURE_2D, uiTextures[1]); shaderManager.UseStockShader(GLT_SHADER_TEXTURE_POINT_LIGHT_DIFF,transformPipeline.GetModelViewMatrix(),transformPipeline.GetProjectionMatrix(),vLightPos, vWhite,0);
    bigSphereBatch.Draw();
    modelViewMatrix.PopMatrix();
}
//设置小球顶点
void setSmallSphereVertex(){
    gltMakeSphere(smallSphereBatch, 0.2, 30, 40);
}
//绘制小球
void drawSmallSphere(){
    
    static CStopWatch    rotTimer;
    float yRot = rotTimer.GetElapsedSeconds() * 60.0f;
    
    //小球需要围绕打球公转,本身也需要自传
    modelViewMatrix.PushMatrix();
    glBindTexture(GL_TEXTURE_2D, uiTextures[2]);
    //设置旋转角度
    printf("旋转角度:%f\n",yRot);
    modelViewMatrix.Rotate(-yRot * 1.5f, 0.0f, 1.0f, 0.0f);
    modelViewMatrix.Translate(0.8f, 0.0f, 0.0f);
    shaderManager.UseStockShader(GLT_SHADER_TEXTURE_POINT_LIGHT_DIFF,
                                 modelViewMatrix.GetMatrix(),
                                 transformPipeline.GetProjectionMatrix(),
                                 vLightPos,
                                 vWhite,
                                 0);
    modelViewMatrix.PopMatrix();
    smallSphereBatch.Draw();
    
    
}
//设置龙套小球的顶点
void setOtherSphereVertex(){
    gltMakeSphere(otherSphereBatch, 0.2, 20, 40);
    
    //7.随机小球球顶点坐标数据
    for (int i = 0; i < NUM_SPHERES; i++) {
        
        //y轴不变，X,Z产生随机值
        GLfloat x = ((GLfloat)((rand() % 400) - 200 ) * 0.1f);
        GLfloat y = (GLfloat)((rand() % 4));
        GLfloat z = ((GLfloat)((rand() % 400) - 200 ) * 0.1f);
        
        //在y方向，将球体设置为0.0的位置，这使得它们看起来是飘浮在眼睛的高度
        //对spheres数组中的每一个顶点，设置顶点数据
        spheres[i].SetOrigin(x, y, z);
    }
}
//绘制龙套小球
void drawOtherSphere(){
    for (int i = 0; i < NUM_SPHERES; i++) {
        modelViewMatrix.PushMatrix();
        glBindTexture(GL_TEXTURE_2D, uiTextures[2]);
        modelViewMatrix.MultMatrix(spheres[i]);
        shaderManager.UseStockShader(GLT_SHADER_TEXTURE_MODULATE,
                                     transformPipeline.GetModelViewProjectionMatrix(),
                                     vWhite,
                                     0);
        otherSphereBatch.Draw();
        modelViewMatrix.PopMatrix();
    }
}
//设置地板顶点
void setFloorVertex(){
    
    GLfloat texSize = 10.0f;
    floorBatch.Begin(GL_TRIANGLE_FAN, 4,1);
    floorBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
    floorBatch.Vertex3f(-20.f, -0.41f, 20.0f);
    
    floorBatch.MultiTexCoord2f(0, texSize, 0.0f);
    floorBatch.Vertex3f(20.0f, -0.41f, 20.f);
    
    floorBatch.MultiTexCoord2f(0, texSize, texSize);
    floorBatch.Vertex3f(20.0f, -0.41f, -20.0f);
    
    floorBatch.MultiTexCoord2f(0, 0.0f, texSize);
    floorBatch.Vertex3f(-20.0f, -0.41f, -20.0f);
    floorBatch.End();
}
//绘制地板
void drawFloor(){
    //物体不需要什么改变时不需要压栈出栈
    glBindTexture(GL_TEXTURE_2D, uiTextures[0]);
    shaderManager.UseStockShader(GLT_SHADER_TEXTURE_MODULATE,
                                 transformPipeline.GetModelViewProjectionMatrix(),
                                 vFloorColor,
                                 0);
    floorBatch.Draw();
    
}
//加载准备好纹理
void loadAllTGATexture(){
    glGenTextures(3, uiTextures);
    glBindTexture(GL_TEXTURE_2D, uiTextures[0]);
    LoadTGATexture("marble.tga", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT);
    
    glBindTexture(GL_TEXTURE_2D, uiTextures[1]);
    LoadTGATexture("marslike.tga", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT);
    
    glBindTexture(GL_TEXTURE_2D, uiTextures[2]);
    LoadTGATexture("moonlike.tga", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT);
}
void SetupRC()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    shaderManager.InitializeStockShaders();
    glEnable(GL_DEPTH_TEST);//开启深度测试
    glEnable(GL_CULL_FACE);//开启背面踢出
    //默认创建的物体在原点的位置,要设置观察者偏移一定位置或者物体本身偏移才能更好的观察
    cameraFrame.MoveForward(-3.0f);
    
    //设置顶点
    setBigSphereVertex();
    setSmallSphereVertex();
    setOtherSphereVertex();
    setFloorVertex();
    
    loadAllTGATexture();
    
}

//绘制场景
void RenderScene(void)
{
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //这次压栈是为了接收到键盘命令时,移动观察者
    //ps:这个PushMatrix()函数会自动帮我们把栈顶的元素copy一份,然后压入栈中,配合PopMatrix()函数,能让变化只在push和pop之间有效,pop之后恢复原值;这样设计能把不同物体的渲染独立开,不会相互影响.
    modelViewMatrix.PushMatrix();
    M3DMatrix44f mCamera;
    //从cameraFrame取值,然后赋值给矩阵mCamera
    cameraFrame.GetCameraMatrix(mCamera);
    modelViewMatrix.MultMatrix(mCamera);
    
    //绘制镜面开始
    modelViewMatrix.PushMatrix();
    modelViewMatrix.Scale(1.0f, -1.0f, 1.0f);
    modelViewMatrix.Translate(0.0f, 1.0f, -1.0f);
    //指定顺时针为正面,因为物体翻转了,否则看到的是背面
    glFrontFace(GL_CW);
    drawBigSphere();
    drawSmallSphere();
    drawOtherSphere();
    //恢复为逆时针为正面
    glFrontFace(GL_CCW);
    //绘制镜面结束
    modelViewMatrix.PopMatrix();
    
    //开启混合
    glEnable(GL_BLEND);
    //指定glBlendFunc 颜色混合方程式
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawFloor();
    //取消混合
    glDisable(GL_BLEND);
    
    //整体上移,否则大球会被地板挡住
    modelViewMatrix.Translate(0.0f, 0.1f, -1.0f);
    drawBigSphere();
    drawSmallSphere();
    drawOtherSphere();
    
    //不能太早pop,这个观察者是针对所有的物体的
    modelViewMatrix.PopMatrix();
    glutSwapBuffers();
    glutPostRedisplay();
}


void SpeacialKeys(int key,int x,int y)
{
    
    float linear = 0.1f;
    float angular = float(m3dDegToRad(5.0f));
    
    if (key == GLUT_KEY_UP) {
        
        //MoveForward 平移
        cameraFrame.MoveForward(linear);
    }
    
    if (key == GLUT_KEY_DOWN) {
        cameraFrame.MoveForward(-linear);
    }
    
    if (key == GLUT_KEY_LEFT) {
        //RotateWorld 旋转
        cameraFrame.RotateWorld(angular, 0.0f, 1.0f, 0.0f);
    }
    
    if (key == GLUT_KEY_RIGHT) {
        cameraFrame.RotateWorld(-angular, 0.0f, 1.0f, 0.0f);
    }
    
}


// 屏幕更改大小或已初始化
void ChangeSize(int nWidth, int nHeight)
{
    //1.设置视口
    glViewport(0, 0, nWidth, nHeight);
    
    //2.设置投影方式:透视投影(目前只支持透视投影和正视投影)
    viewFrustum.SetPerspective(35.0f, float(nWidth)/float(nHeight), 1.0f, 100.0f);
    
    //3.将投影矩阵加载到投影矩阵堆栈,
    projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
    modelViewMatrix.LoadIdentity();
    
    //4.将投影矩阵堆栈和模型视图矩阵对象设置到管道中
    transformPipeline.SetMatrixStacks(modelViewMatrix, projectionMatrix);
    
}

//删除纹理
void ShutdownRC()
{
    glDeleteTextures(3, uiTextures);
}


int main(int argc, char* argv[])
{
    gltSetWorkingDirectory(argv[0]);
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800,600);
    
    glutCreateWindow("OpenGL SphereWorld");
    
    glutReshapeFunc(ChangeSize);
    glutDisplayFunc(RenderScene);
    glutSpecialFunc(SpeacialKeys);
    
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
        return 1;
    }
    
    
    SetupRC();
    glutMainLoop();
    ShutdownRC();
    return 0;
}
