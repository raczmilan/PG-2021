#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp> //core glm functionality
#include <glm/gtc/matrix_transform.hpp> //glm extension for generating common transformation matrices
#include <glm/gtc/matrix_inverse.hpp> //glm extension for computing inverse matrices
#include <glm/gtc/type_ptr.hpp> //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "SkyBox.hpp"

#include <iostream>

// window
gps::Window myWindow;
int windowWidth = 1920;
int windowHeight = 1080;
int state = 0;

const unsigned int SHADOW_WIDTH = 4096;
const unsigned int SHADOW_HEIGHT = 4096;

std::vector<const GLchar*> faces;
std::vector<const GLchar*> faces2;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;
glm::mat4 lightRotation;
glm::mat4 pointLightRotation;

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;
glm::vec3 pointLightPos;

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;
GLint pointLightPosLoc;

// camera
gps::Camera myCamera(
    glm::vec3(0.0f, 0.7f, 6.0f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 0.1f;

GLboolean pressedKeys[1024];

// models
gps::Model3D car;
gps::Model3D glass;
gps::Model3D road;
gps::Model3D cabin;
gps::Model3D ground;
gps::Model3D lamp;
gps::Model3D windmill;
gps::Model3D wheel;
gps::Model3D fence;
gps::Model3D trees;

GLfloat angle;
GLfloat angle2;
GLfloat lightAngle;

// shaders
gps::Shader myBasicShader;
gps::Shader depthMapShader;


gps::SkyBox mySkyBox;
gps::SkyBox mySkyBox2;
gps::Shader skyboxShader;

GLuint shadowMapFBO;
GLuint depthMapTexture;

bool wireframeMode;
bool lightOn;
bool lightMode;
bool animation;
bool firstMouse = true;

double lastTimeStamp = glfwGetTime();
float lastX = windowWidth / 2;
float lastY = windowHeight / 2;
float yaw = -90.0f, pitch = 0.0f;
const float sensitivity = 0.1f;

GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error;
		switch (errorCode) {
            case GL_INVALID_ENUM:
                error = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error = "INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                error = "INVALID_OPERATION";
                break;
            case GL_STACK_OVERFLOW:
                error = "STACK_OVERFLOW";
                break;
            case GL_STACK_UNDERFLOW:
                error = "STACK_UNDERFLOW";
                break;
            case GL_OUT_OF_MEMORY:
                error = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
	//TODO
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if (key == GLFW_KEY_T && action == GLFW_PRESS) {
        wireframeMode = !wireframeMode;
    }

    if (key == GLFW_KEY_Y && action == GLFW_PRESS) {
        lightMode = !lightMode;
    }

    if (key == GLFW_KEY_C && action == GLFW_PRESS) {
        lightOn = !lightOn;
    }

    if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
        if (animation) {
            myCamera.setPosition(glm::vec3(0.0f, 0.7f, 7.0f));
            animation = !animation;
            pitch = -5.0f;
            yaw = -90.0f;
            myCamera.rotate(pitch, yaw);
        }
        else {
            animation = !animation;
            pitch = -5.0f;
            yaw = -90.0f;
            myCamera.setPosition(glm::vec3(0.0f, 1.0f, 7.0f));
        }
    }

	if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        } else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (!animation) {
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // reversed since y-coordinates range from bottom to top
        lastX = xpos;
        lastY = ypos;


        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw += xoffset;
        pitch += yoffset;

        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        myCamera.rotate(pitch, yaw);
    }
}

void processAnimation() {
    glm::vec3 camPos = myCamera.getPosition();
    if (state == 0) {
        myCamera.move(gps::MOVE_RIGHT, cameraSpeed * 0.7f);
        yaw = yaw - 0.5f;
        myCamera.rotate(pitch, yaw);

        if (camPos.x >= 3) {
            state = 1;
        }
    }
    else if (state == 1) {

        myCamera.move(gps::MOVE_RIGHT, cameraSpeed * 0.7f);
        yaw = yaw - 0.45f;
        myCamera.rotate(pitch, yaw);

        if (camPos.z >= 4) {
            state = 2;
        }
    }
    else if (state == 2) {
        myCamera.move(gps::MOVE_RIGHT, cameraSpeed * 0.7f);
        yaw = yaw - 0.5f;
        myCamera.rotate(pitch, yaw);

        if (camPos.x <= -3) {
            state = 3;
        }
    }
    else if (state == 3) {
        myCamera.move(gps::MOVE_RIGHT, cameraSpeed * 0.7f);
        yaw = yaw - 0.45f;
        myCamera.rotate(pitch, yaw);

        if (camPos.z <= 0) {
            state = 4;
        }
    }
    else {
        myCamera.move(gps::MOVE_RIGHT, cameraSpeed * 0.7f);
        yaw = yaw - 0.5f;
        myCamera.rotate(pitch, yaw);

        if (camPos.x >= 0) {
            state = 0;
            animation = !animation;
            myCamera.setPosition(glm::vec3(camPos.x, 0.7f, camPos.z));
            pitch = -5.0f;
            myCamera.rotate(pitch, yaw);
        }
    }
}

void processMovement() {
	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
		// compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}

	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}

	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}

    if (pressedKeys[GLFW_KEY_Q]) {
        angle -= 1.0f;
        // update model matrix for teapot
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        // update normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
    }

    if (pressedKeys[GLFW_KEY_E]) {
        angle += 1.0f;
        // update model matrix for teapot
        model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
        // update normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
    }

    if (pressedKeys[GLFW_KEY_J]) {
        lightAngle -= 1.0f;
    }

    if (pressedKeys[GLFW_KEY_L]) {
        lightAngle += 1.0f;
    }
}

void initOpenGLWindow() {
    myWindow.Create(windowWidth, windowHeight, "OpenGL Project Core");
}

void setWindowCallbacks() {
	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
}

void initOpenGLState() {
	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void initModels() {
    car.LoadModel("models/car/car.obj");
    glass.LoadModel("models/car/glass.obj");
    road.LoadModel("models/road/road.obj");
    cabin.LoadModel("models/cabin/cabin.obj");
    ground.LoadModel("models/ground/ground.obj");
    lamp.LoadModel("models/lamp/lamp.obj");
    windmill.LoadModel("models/windmill/windmill.obj");
    wheel.LoadModel("models/windmill/wheel.obj");
    fence.LoadModel("models/fence/fence.obj");
    trees.LoadModel("models/trees/trees.obj");

    faces.push_back("models/skybox/right.tga");
    faces.push_back("models/skybox/left.tga");
    faces.push_back("models/skybox/top.tga");
    faces.push_back("models/skybox/bottom.tga");
    faces.push_back("models/skybox/back.tga");
    faces.push_back("models/skybox/front.tga");
    mySkyBox.Load(faces);

    faces2.push_back("models/skybox2/right.tga");
    faces2.push_back("models/skybox2/left.tga");
    faces2.push_back("models/skybox2/top.tga");
    faces2.push_back("models/skybox2/bottom.tga");
    faces2.push_back("models/skybox2/back.tga");
    faces2.push_back("models/skybox2/front.tga");

    mySkyBox2.Load(faces2);
}

void initShaders() {
	myBasicShader.loadShader("shaders/basic.vert", "shaders/basic.frag");
    depthMapShader.loadShader("shaders/depthMap.vert", "shaders/depthMap.frag");
    skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
}

void initUniforms() {
    myBasicShader.useShaderProgram();

    // create model matrix for teapot
    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

    // get view matrix for current camera
    view = myCamera.getViewMatrix();
    viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");
    // send view matrix to shader
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    view = myCamera.getViewMatrix();
    glCheckError();
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "view"), 1, GL_FALSE,
        glm::value_ptr(view));

    // compute normal matrix for teapot
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

    // create projection matrix
    projection = glm::perspective(glm::radians(45.0f),
        (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
        0.1f, 200.0f);
    projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
    // send projection matrix to shader
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    
    projection = glm::perspective(glm::radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 1000.0f);
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "projection"), 1, GL_FALSE,
        glm::value_ptr(projection));


    //set the light direction (direction towards the light)
    lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
    lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
    // send light dir to shader
    glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));
    
    //set light color
    lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
    lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
    // send light color to shader
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
    
    pointLightPos = glm::vec3(1.3f, 0.96f, 0.65f);
    pointLightPosLoc = glGetUniformLocation(myBasicShader.shaderProgram, "pointLightPos");
    glUniform3fv(pointLightPosLoc, 1, glm::value_ptr(pointLightPos));

}

void initFBO() {
    //TODO - Create the FBO, the depth texture and attach the depth texture to the FBO
    glGenFramebuffers(1, &shadowMapFBO);

    //create depth texture for FBO
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    //attach texture to FBO
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

glm::mat4 computeLightSpaceTrMatrix() {
    //TODO - Return the light-space transformation matrix
    lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightView = glm::lookAt(lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    const GLfloat near_plane = -15.0f, far_plane = 15.0f;
    glm::mat4 lightProjection = glm::ortho(-15.0f, 15.0f, -15.0f, 15.0f, near_plane, far_plane);

    glm::mat4 lightSpaceTrMatrix = lightProjection * lightView * lightRotation;

    return lightSpaceTrMatrix;
}

float movementSpeed = 100; // units per second 
void updateAngle(double elapsedSeconds) { 
    angle2 = angle2 + movementSpeed * elapsedSeconds; 
} 

void renderObjects(gps::Shader shader, bool pass) {
    // select active shader program
    shader.useShaderProgram();
    
    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    if (!pass) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
        glUniform3fv(pointLightPosLoc, 1, glm::value_ptr(glm::vec3(model * glm::vec4(pointLightPos, 1.0f))));
        glUniform1i(glGetUniformLocation(shader.shaderProgram, "lightOn"), lightOn);
        glUniform1i(glGetUniformLocation(shader.shaderProgram, "refl"), false);
        glUniform1i(glGetUniformLocation(shader.shaderProgram, "transparent"), false);
    }

    if (lightMode)
        mySkyBox2.Draw(skyboxShader, view, projection);
    else
        mySkyBox.Draw(skyboxShader, view, projection);

    road.Draw(shader);
    ground.Draw(shader);
    cabin.Draw(shader);
    fence.Draw(shader);
    trees.Draw(shader);
    
    if (!pass) {
        glUniform1i(glGetUniformLocation(shader.shaderProgram, "refl"), true);
    }

    lamp.Draw(shader);
    windmill.Draw(shader);

    double currentTimeStamp = glfwGetTime(); 
    updateAngle(currentTimeStamp - lastTimeStamp); 
    lastTimeStamp = currentTimeStamp;

    glm::mat4 model1 = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    model1 = glm::translate(model1, glm::vec3(0.0f, 2.528f, -5.237f));
    model1 = glm::rotate(model1, glm::radians(angle2), glm::vec3(1.0f, 0.0f, 0.0f));
    model1 = glm::translate(model1, glm::vec3(0.0f, -2.528f, 5.237f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model1));
    wheel.Draw(shader);
    
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glDisable(GL_CULL_FACE);
    car.Draw(shader);

    if (!pass) {
        glUniform1i(glGetUniformLocation(shader.shaderProgram, "transparent"), true);
    }

    glass.Draw(shader);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
}

void renderObjects2(gps::Shader shader) {
    shader.useShaderProgram();

    model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    mySkyBox.Draw(skyboxShader, view, projection);
    ground.Draw(shader);
    road.Draw(shader);
    cabin.Draw(shader);
    fence.Draw(shader);
    trees.Draw(shader);
    lamp.Draw(shader);
    windmill.Draw(shader);

    double currentTimeStamp = glfwGetTime();
    updateAngle(currentTimeStamp - lastTimeStamp);
    lastTimeStamp = currentTimeStamp;
    glm::mat4 model1 = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    model1 = glm::translate(model1, glm::vec3(0.0f, 2.528f, -5.237f));
    model1 = glm::rotate(model1, glm::radians(angle2), glm::vec3(1.0f, 0.0f, 0.0f));
    model1 = glm::translate(model1, glm::vec3(0.0f, -2.528f, 5.237f));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model1));
    wheel.Draw(shader);
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    car.Draw(shader);
    glass.Draw(shader);
}

void renderScene() {
    if (wireframeMode) {
        glViewport(0, 0, windowWidth, windowHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        myBasicShader.useShaderProgram();
        view = myCamera.getViewMatrix();
        glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        renderObjects2(myBasicShader);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    else {

        if(lightMode)
            glUniform3fv(lightColorLoc, 1, glm::value_ptr(glm::vec3(0.003f, 0.003f, 0.003f)));
        else
            glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
        lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        lightDir = glm::vec3(glm::mat3(lightRotation) * lightDir);
        lightAngle = 0;

        depthMapShader.useShaderProgram();
        glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
            1,
            GL_FALSE,
            glm::value_ptr(computeLightSpaceTrMatrix()));
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        renderObjects(depthMapShader, true);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glViewport(0, 0, windowWidth, windowHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        myBasicShader.useShaderProgram();
        view = myCamera.getViewMatrix();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, depthMapTexture);
        glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap"), 3);

        glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceTrMatrix"),
            1,
            GL_FALSE,
            glm::value_ptr(computeLightSpaceTrMatrix()));

        renderObjects(myBasicShader, false);
    }
}

void cleanup() {
    myWindow.Delete();
    //cleanup code for your own data
}

int main(int argc, const char * argv[]) {

    try {
        initOpenGLWindow();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
	initModels();
	initShaders();
	initUniforms();
    initFBO();
    setWindowCallbacks();
    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);


	glCheckError();
	// application loop
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
        if (animation)
            processAnimation();
        else
            processMovement();
	    renderScene();

		glfwPollEvents();
		glfwSwapBuffers(myWindow.getWindow());

		//glCheckError();
	}

	cleanup();

    return EXIT_SUCCESS;
}
