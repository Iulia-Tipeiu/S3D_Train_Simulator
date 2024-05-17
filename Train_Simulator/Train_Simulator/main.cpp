#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <GL/glew.h>

#define GLM_FORCE_CTOR_INIT 
#include <GLM.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include <glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include <vector>

#include <chrono>
#include <thread>

#define STB_IMAGE_IMPLEMENTATION

#include "Camera.h"
#include "Shader.h"
#include "Model.h"
#include "MoveableObject.h"

#pragma comment (lib, "glfw3dll.lib")
#pragma comment (lib, "glew32.lib")
#pragma comment (lib, "OpenGL32.lib")


const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;
bool isDayTime = true;
bool gameRunning = false;
bool movementAllowed = false;
bool cameraMovementAllowed = true;

enum railType {
    STRAIGHT,
	TURN_LEFT,
	TURN_RIGHT,
	SWITCH_LEFT,
	SWITCH_RIGHT
};
struct rail {
    glm::vec3 position;
	float rotation;
	railType type;

	rail(float x, float y, float z, float rotationAngle, railType type) : position(glm::vec3(x, y, z)), type(type), rotation(rotationAngle) {}
};

// The amount of ms between light changes
const unsigned int DAY_NIGHT_CYCLE_SPEED_MS = 20; // lower = faster
const float TRAIN_SPEED = 4.0f;
const float CAMERA_SPEED = 5.0f;

glm::vec3 trainScale = glm::vec3(1.0f);
glm::vec3 mountainScale = glm::vec3(.7f);
glm::vec3 railsScale = glm::vec3(0.5f);

Camera* pCamera = nullptr;

MoveableObject* currentObject;

unsigned int CreateTexture(const std::string& strTexturePath)
{
    unsigned int textureId = -1;

    // load image, create texture and generate mipmaps
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(strTexturePath.c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
    {
        std::cout << "Failed to load texture: " << strTexturePath << std::endl;
    }
    stbi_image_free(data);

    return textureId;
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

void process_day_night();
void Start();
void Update();

void renderScene(const Shader& shader);
void renderFloor();
void renderModel(Shader& ourShader, Model& ourModel, const glm::vec3& position, float rotationAngle, const glm::vec3& scale);

double deltaTime = 0.0f; // time between current frame and last frame
double lastFrame = 0.0f;

void updateLightPosition(float& lightX, float& lightY, float& lightZ, float elapsedTime) {
    // Calculate new light position based on elapsed time
    float radius = 20.0f; // Distance from the center of the scene
    float angularSpeed = 0.1f; // Speed of the sun's movement

    lightX = radius * cos(angularSpeed * elapsedTime);
    lightZ = radius * sin(angularSpeed * elapsedTime);
    lightY = radius * sin(angularSpeed * elapsedTime / 2.0f); // Adjust the vertical movement
}



float skyboxVertices[] = {
    -1.0f,  -1.0f, 1.0f,
    1.0f, -1.0f, 1.0f,
     1.0f, -1.0f, -1.0f,
     -1.0f, -1.0f, -1.0f,
     -1.0f, 1.0f, 1.0f,
     1.0f,  1.0f, 1.0f,
    1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f
};

unsigned int skyboxIndices[] =
{
    1,2,6,
    6,5,1,
    0,4,7,
    7,3,0,
    4,5,6,
    6,7,4,
    0,3,2,
    2,1,0,
    0,1,5,
    5,4,0,
    3,7,6,
    6,2,3
};

Model trainModel;

MoveableObject trainVehicle;

std::vector<glm::vec3> mountainsPositions =
{
    glm::vec3(-20.5f, -1.55f, 10.0f),
    glm::vec3(50.0f, -1.55f, -25.0f),
    glm::vec3(20.0f, -1.55f, 20.0f),
    glm::vec3(-35.0f, -1.55f, -15.0f),
    glm::vec3(-25.0f, -1.55f, -30.0f),
    glm::vec3(-50.0f, -1.55f, -35.0f),
    //far
    glm::vec3(-30.0f, -1.55f, -150.0f),
    glm::vec3(-15.0f, -1.55f, -145.0f),
    glm::vec3(15.0f, -1.55f, -155.0f),

    //far far
    glm::vec3(70.0f, -1.55f, 200.0f),
    glm::vec3(-70.0f, -1.55f, 200.0f),
    glm::vec3(90.0f, -1.55f, 160.0f),

    glm::vec3(160.0f, -1.55f, 100.0f),
    glm::vec3(-160.0f, -1.55f, 100.0f)
};

std::vector<glm::vec3> mountainsScales =
{
    glm::vec3(0.05f),
    glm::vec3(0.8f),
    glm::vec3(0.05f),
    glm::vec3(0.1f),
    glm::vec3(0.1f),
    glm::vec3(0.6f),
    //far
    glm::vec3(0.6f),
    glm::vec3(0.8f),
    glm::vec3(0.7f),
    //far far
    glm::vec3(0.6f),
    glm::vec3(0.6f),
    glm::vec3(0.8f),

    glm::vec3(0.4f),
    glm::vec3(0.4f)
};


const std::vector<std::string> facesDay
{
    ".\\Assets\\skybox_images\\skybox_right.jpg",
    ".\\Assets\\skybox_images\\skybox_left.jpg",
    ".\\Assets\\skybox_images\\skybox_top.jpg",
    ".\\Assets\\skybox_images\\skybox_bottom.jpg",
    ".\\Assets\\skybox_images\\skybox_back.jpg",
    ".\\Assets\\skybox_images\\skybox_front.jpg"
};

const std::vector<std::string>facesNight
{

    ".\\Assets\\skybox_images_night\\skybox_night_front.jpg",
    ".\\Assets\\skybox_images_night\\skybox_night_back.jpg",
    ".\\Assets\\skybox_images_night\\skybox_night_left.jpg",
    ".\\Assets\\skybox_images_night\\skybox_night_right.jpg",
    ".\\Assets\\skybox_images_night\\skybox_night_top.jpg",
    ".\\Assets\\skybox_images_night\\skybox_night_bottom.jpg"
};

// x-2.42,  y,  z+9.7,  r-15.8, TURN_RIGHT

std::vector<rail> rails = {
	
    rail(123, -1.5F, -195,  0,  STRAIGHT),
    rail(123, -1.5F, -184.9,  0,  STRAIGHT),
    rail(123, -1.5F, -174.8,  0,  STRAIGHT),
	rail(123, -1.5F, -164.7,  180,  TURN_RIGHT),
	rail(120.58, -1.5F, -155,  164.2,  TURN_RIGHT),
	rail(115.5, -1.5F, -146.1,  148.85,  TURN_RIGHT),
	rail(109, -1.5F, -139.4,  134.51,  STRAIGHT),
    rail(101.9, -1.5F, -132.429,  134.51,  STRAIGHT),
    rail(94.8, -1.5F, -125.45,  134.51,  STRAIGHT),
    rail(87.7, -1.5F, -118.47,  134.51,  STRAIGHT),
    rail(80.6, -1.5F, -111.49,  134.51,  STRAIGHT),
    rail(73.5, -1.5F, -104.51,  134.51,  STRAIGHT),
    rail(66.4, -1.5F, -97.53,  134.51,  STRAIGHT),
    rail(59.3, -1.5F, -90.55,  134.51,  STRAIGHT),
    rail(52.2, -1.5F, -83.57,  134.51,  STRAIGHT),
    rail(45.1, -1.5F, -76.59,  134.51,  STRAIGHT),
    rail(38, -1.5F, -69.61,  134.51,  STRAIGHT),
    rail(30.9, -1.5F, -62.63,  134.51,  STRAIGHT),
    rail(23.8, -1.5F, -55.65,  134.51,  STRAIGHT),
    rail(16.7, -1.5F, -48.67,  134.51,  STRAIGHT),
    rail(9.6, -1.5F, -41.69,  134.51,  TURN_LEFT),
    rail(4.43, -1.5F, -32.9,  152.2,  TURN_LEFT),
    rail(3.04, -1.5F, -23.05,  -0.5,  STRAIGHT),
    rail(2.95, -1.5F, -13,  -0.5,  STRAIGHT),
    rail(2.86, -1.5F, -2.95,  -0.5,  STRAIGHT),
    rail(2.77, -1.5F, 7.1,  -0.5,  STRAIGHT),
    rail(2.68, -1.5F, 17.15,  -0.5,  STRAIGHT),
    rail(2.59, -1.5F, 27.2,  -0.5,  STRAIGHT),
    rail(2.5, -1.5F, 37.25,  -0.5,  STRAIGHT),
    rail(2.41, -1.5F, 47.3,  -0.5,  STRAIGHT),
    rail(2.32, -1.5F, 57.35,  -0.5,  STRAIGHT),
    rail(2.23, -1.5F, 67.4,  -0.5,  STRAIGHT),
    rail(2.14, -1.5F, 77.45,  -0.5,  STRAIGHT),
    rail(2.05, -1.5F, 87.5,  -0.5,  STRAIGHT),
    rail(1.96, -1.5F, 97.55,  -0.5,  STRAIGHT),
    rail(1.87, -1.5F, 107.6,  -0.5,  STRAIGHT),
    rail(1.78, -1.5F, 117.65,  -0.5,  STRAIGHT),
    rail(1.69, -1.5F, 127.7,  -0.5,  STRAIGHT),
    rail(1.6, -1.5F, 137.75,  -0.5,  STRAIGHT),
    rail(1.51, -1.5F, 147.8,  -0.5,  STRAIGHT),
    rail(1.42, -1.5F, 157.85,  -0.5,  STRAIGHT),
    rail(1.33, -1.5F, 167.9,  -0.5,  STRAIGHT),
    rail(1.24, -1.5F, 177.95,  -0.5,  STRAIGHT),
    rail(1.15, -1.5F, 188,  -0.5,  STRAIGHT),
    rail(1.06, -1.5F, 198.05,  -0.5,  STRAIGHT),

};


float blendFactor = 0;
float ambientFactor = 0.9;

int main(int argc, char** argv)
{
    std::string strFullExeFileName = argv[0];
    std::string strExePath;
    const size_t last_slash_idx = strFullExeFileName.rfind('\\');
    if (std::string::npos != last_slash_idx)
    {
        strExePath = strFullExeFileName.substr(0, last_slash_idx);
    }

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "TrainSim", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glewInit();

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glEnable(GL_DEPTH_TEST);


    // build and compile shaders
    // -------------------------
    Shader shadowMappingShader("ShadowMapping.vs", "ShadowMapping.fs");
    Shader shadowMappingDepthShader("ShadowMappingDepth.vs", "ShadowMappingDepth.fs");
    Shader ModelShader("ModelShader.vs", "ModelShader.fs");
    Shader skyboxShader("skybox.vs", "skybox.fs");
    Shader textShader("text.vs", "text.fs");

    // load textures
    // -------------
    unsigned int floorTexture = CreateTexture("Assets\\grass1.jpg");

    // configure depth map FBO
    // -----------------------
    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    // create depth texture
    unsigned int depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // shader configuration
    // --------------------
    shadowMappingShader.Use();
    shadowMappingShader.SetInt("diffuseTexture", 0);
    shadowMappingShader.SetInt("shadowMap", 1);

    // lighting info
    // -------------
    glm::vec3 lightPos(-2.0f, 4.0f, -1.0f);

    glEnable(GL_CULL_FACE);

    unsigned int skyboxVAO, skyboxVBO, skyboxEBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glGenBuffers(1, &skyboxEBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), &skyboxIndices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    unsigned int cubemapTexture;

    unsigned int cubemapTextureNight;

    glGenTextures(1, &cubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < facesDay.size(); i++)
    {
        unsigned char* data = stbi_load(facesDay[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            stbi_set_flip_vertically_on_load(false);
            glTexImage2D
            (
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0,
                GL_RGB,
                width,
                height,
                0,
                GL_RGB,
                GL_UNSIGNED_BYTE,
                data
            );
            glGetError();
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << facesDay[i] << std::endl;
            stbi_image_free(data);
        }
    }

    glGenTextures(1, &cubemapTextureNight);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTextureNight);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    //night skybox
    for (unsigned int i = 0; i < facesNight.size(); i++) {
        unsigned char* data = stbi_load(facesNight[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            stbi_set_flip_vertically_on_load(false);
            glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            glGetError();
            stbi_image_free(data);
        }
        else {
            std::cout << "Cubemap texture failed to load at path: " << facesNight[i] << std::endl;
        }
    }

    trainModel = Model("Assets\\Models\\Train\\2te116.obj");
    
    trainVehicle = MoveableObject(trainModel, SCR_WIDTH, SCR_HEIGHT, glm::vec3(2.5f, -1.47f, 17.15f));

    currentObject = &trainVehicle;

    Model mountainModel("Assets\\Models\\Mountain\\mountain.obj");
	Model trainStation("Assets\\Models\\TrainStation\\milwaukeeroaddepot.obj");
    Model railStraight("Assets\\Models\\tracks\\RailStraight.obj");
	Model railSwitchLeft("Assets\\Models\\tracks\\RailSwitchLeft.obj");
	Model railSwitchRight("Assets\\Models\\tracks\\RailSwitchRight.obj");
	Model railTurnLeft("Assets\\Models\\tracks\\RailTurnLeft.obj");
	Model railTurnRight("Assets\\Models\\tracks\\RailTurnRight.obj");

	
	MoveableObject trainStationObject(trainStation, SCR_WIDTH, SCR_HEIGHT, glm::vec3(-1.0f, -1.55f, 20.0f));
    trainStationObject.SetRotation(90);

    pCamera = new Camera(SCR_WIDTH, SCR_HEIGHT, glm::vec3(0, 0, -20));
    //pCamera = new Camera(SCR_WIDTH, SCR_HEIGHT, glm::vec3(trainVehicle.GetPosition().x, trainVehicle.GetPosition().y + 2.4f, trainVehicle.GetPosition().z));

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    int refreshRate = mode->refreshRate;

    gameRunning = true;

    // start in fullscreen

    //glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    
	Start();

    // run the day night thread
    std::thread dayNightThread(process_day_night);

    
    while (!glfwWindowShouldClose(window))
    {

        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTextureNight);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default
        glDepthMask(GL_TRUE);
        double currentFrame = glfwGetTime();

        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

		glm::vec3 cameraPosition = pCamera->GetPosition();
		std::string title = "X:" + std::to_string(cameraPosition.x) + " Y:" + std::to_string(cameraPosition.y) + " Z:" + std::to_string(cameraPosition.z) + " R:" + std::to_string(pCamera->GetYaw()) + " Train_Yaw:" + std::to_string(currentObject->GetYaw());
		glfwSetWindowTitle(window, title.c_str());

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        

        glm::mat4 lightProjection, lightView;
        glm::mat4 lightSpaceMatrix;

        glm::mat4 lightRotationMatrix = glm::mat4(
            glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
            glm::vec4(sin(lastFrame), 0.0f, cos(lastFrame), 1.0f)
        );

        glm::vec4 rotatedLightPos = lightRotationMatrix * glm::vec4(lightPos, 1.0f);

        float near_plane = 1.0f, far_plane = 10.5f;
        lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, near_plane, far_plane);
        lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        lightSpaceMatrix = lightProjection * lightView;

        // render scene from light's point of view
        shadowMappingDepthShader.Use();
        shadowMappingDepthShader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        renderScene(shadowMappingDepthShader);

        float trainRotation = -0.5f;

        renderModel(shadowMappingDepthShader, trainVehicle.GetVehicleModel(), trainVehicle.GetPosition(), trainVehicle.GetRotation(), trainScale);

        float mountainRotation = 0.0f;
        

        for (int i = 0; i < mountainsPositions.size(); i++)
        {
            renderModel(shadowMappingDepthShader, mountainModel, mountainsPositions[i] - glm::vec3(0.0f, 0.0f, 0.0f), mountainRotation, mountainsScales[i] * mountainScale);
        }


        glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // reset viewport
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shadowMappingShader.Use();
        glm::mat4 projection = pCamera->GetProjectionMatrix();
        glm::mat4 view = pCamera->GetViewMatrix(currentObject);
        shadowMappingShader.SetMat4("projection", projection);
        shadowMappingShader.SetMat4("view", view);
        shadowMappingShader.SetFloat("ambientFactor", ambientFactor);
        // set light uniforms
        shadowMappingShader.SetVec3("viewPos", pCamera->GetPosition());
        shadowMappingShader.SetVec3("lightPos", lightPos);
        shadowMappingShader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDisable(GL_CULL_FACE);
        renderScene(shadowMappingShader);

        renderModel(ModelShader, trainVehicle.GetVehicleModel(), trainVehicle.GetPosition(), trainVehicle.GetRotation(), trainScale);
		renderModel(ModelShader, trainStation, trainStationObject.GetPosition(), trainStationObject.GetRotation(), glm::vec3(.0025f));
        for (int i = 0; i < mountainsPositions.size(); i++)
        {
            renderModel(ModelShader, mountainModel, mountainsPositions[i] - glm::vec3(0.0f, 0.0f, 0.0f), mountainRotation, mountainsScales[i] * mountainScale);
        }

        for (auto rail : rails)
        {
			switch (rail.type)
			{
			case STRAIGHT:
				renderModel(ModelShader, railStraight, rail.position, rail.rotation, railsScale);
				break;
			case TURN_LEFT:
				renderModel(ModelShader, railTurnLeft, rail.position, rail.rotation, railsScale);
				break;
			case TURN_RIGHT:
				renderModel(ModelShader, railTurnRight, rail.position, rail.rotation, railsScale);
				break;
			case SWITCH_LEFT:
				renderModel(ModelShader, railSwitchLeft, rail.position, rail.rotation, railsScale);
				break;
			case SWITCH_RIGHT:
				renderModel(ModelShader, railSwitchRight, rail.position, rail.rotation, railsScale);
				break;
			}
        }


        glm::vec3 sunPos(-2.0f, 4.0f, -1.0f);
        glm::vec3 moonPos(2.0f, -4.0f, 1.0f);

        glm::vec3 sunColor(1.0f, 0.95f, 0.8f);
        glm::vec3 moonColor(0.6f, 0.7f, 1.0f);

        float sunMoonBlendFactor = 0.0f;

        glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f); // White light
        glm::vec3 lightDir = glm::normalize(glm::vec3(-0.2f, -1.0f, -0.3f)); // Example direction
        glm::vec3 objectColor = glm::vec3(1.0f, 0.5f, 0.31f); // Example color (rust)

        ModelShader.Use();
        ModelShader.SetVec3("lightColor", lightColor);
        ModelShader.SetVec3("lightDir", lightDir);
        ModelShader.SetVec3("objectColor", objectColor);

        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        skyboxShader.Use();

        skyboxShader.SetInt("skybox1", 0);
        skyboxShader.SetInt("skybox2", 1);
        skyboxShader.SetFloat("blendFactor", blendFactor);

        glm::mat4 viewSB = glm::mat4(glm::mat3(pCamera->GetViewMatrix(currentObject)));

        glm::mat4 projectionSB = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        skyboxShader.SetMat4("view", viewSB);
        skyboxShader.SetMat4("projection", projectionSB);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTextureNight);

        glBindVertexArray(skyboxVAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glDepthFunc(GL_LESS);

        glfwSwapBuffers(window);
        glfwPollEvents();
        Update();
        movementAllowed = true;
    }

    gameRunning = false;
    dayNightThread.join();
    delete pCamera;
    glfwTerminate();
    return 0;
}

// renders the 3D scene
// --------------------
void renderScene(const Shader& shader)
{
    // floor
    glm::mat4 model;
    float deltaY = -1.0f;
    model = glm::translate(model, glm::vec3(0.0f, deltaY, 0.0f));
    shader.SetMat4("model", model);
    renderFloor();
}


unsigned int planeVAO = 0;
void renderFloor()
{
    unsigned int planeVBO;

    if (planeVAO == 0)
    {
        float planeVertices[] = {
            200.0f, -0.5f,  200.0f,  0.0f, 1.0f, 0.0f,  200.0f,  0.0f,
            -200.0f, -0.5f,  200.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
            -200.0f, -0.5f, -200.0f,  0.0f, 1.0f, 0.0f,   0.0f, 200.0f,

            200.0f, -0.5f,  200.0f,  0.0f, 1.0f, 0.0f,  200.0f,  0.0f,
            -200.0f, -0.5f, -200.0f,  0.0f, 1.0f, 0.0f,   0.0f, 200.0f,
            200.0f, -0.5f, -200.0f,  0.0f, 1.0f, 0.0f,  200.0f, 200.0f
        };
        // plane VAO
        glGenVertexArrays(1, &planeVAO);
        glGenBuffers(1, &planeVBO);
        glBindVertexArray(planeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindVertexArray(0);
    }

    glBindVertexArray(planeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

unsigned int modelVAO = 0;
unsigned int modelVBO = 0;
unsigned int modelEBO;
void renderModel(Shader& ourShader, Model& ourModel, const glm::vec3& position, float rotationAngle, const glm::vec3& scale)
{
    ourShader.Use();

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, scale);

    glm::mat4 viewMatrix = pCamera->GetViewMatrix(currentObject);
    glm::mat4 projectionMatrix = pCamera->GetProjectionMatrix();

    ourShader.SetMat4("model", model);
    ourShader.SetMat4("view", viewMatrix);
    ourShader.SetMat4("projection", projectionMatrix);

    ourModel.Draw(ourShader);
}


void blendNight()
{
    blendFactor = std::min(blendFactor + 0.001, 1.0);
    ambientFactor = std::max(ambientFactor - 0.001, 0.34);
}

void blendDay()
{
    blendFactor = std::max(blendFactor - 0.001, 0.0);
    ambientFactor = std::min(ambientFactor + 0.001, 0.9);

}

void process_day_night()
{
    while (gameRunning)
    {
        if (isDayTime)
        {
            blendNight();
            if(blendFactor > 0.9f)
				isDayTime = false;
        }
        else
        {

            blendDay();
            if (blendFactor < 0.1f)
				isDayTime = true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(DAY_NIGHT_CYCLE_SPEED_MS));
    }

}

float speedFactor = 1.0f;
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);


#ifdef _DEBUG
    if (cameraMovementAllowed)
    {
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            speedFactor = CAMERA_SPEED;
        else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE)
            speedFactor = 1.0f;

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            pCamera->ProcessKeyboard(LEFT, (float)deltaTime * speedFactor);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            pCamera->ProcessKeyboard(RIGHT, (float)deltaTime * speedFactor);

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            pCamera->ProcessKeyboard(FORWARD, (float)deltaTime * speedFactor);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            pCamera->ProcessKeyboard(BACKWARD, (float)deltaTime * speedFactor);
        if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
            pCamera->ProcessKeyboard(UP, (float)deltaTime * speedFactor);
        if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
            pCamera->ProcessKeyboard(DOWN, (float)deltaTime * speedFactor);

		//if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		//	pCamera->LookAt(trainVehicle.GetPosition(), trainVehicle.GetRotation());
    }

    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
    {
        cameraMovementAllowed = true;
    }

	if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS)
	{
		cameraMovementAllowed = false;
	}

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        pCamera->Reset(width, height);

    }
#endif // DEBUG



}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    pCamera->Reshape(width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (pCamera->GetFreeCamera())
        pCamera->MouseControl((float)xpos, (float)ypos);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yOffset)
{
    if (pCamera->GetFreeCamera())
        pCamera->ProcessMouseScroll((float)yOffset);
}

/// <summary>
/// This is called before the first frame update
/// </summary>
void Start()
{
    //prepare train
    
    
	pCamera->SetPosition(trainVehicle.GetPosition() + glm::vec3(0,5,0));
    trainVehicle.SetPosition(rails[0].position);
	pCamera->SetForwardVector(trainVehicle.GetForward());
    pCamera->SetRotation(trainVehicle.GetRotation());
	currentObject = &trainVehicle;
	cameraMovementAllowed = false;


   // std::cout << "Start yaw:" << trainVehicle.GetRotation() << " " << pCamera->GetYaw() << std::endl;
}

int railIndex = 1;

/// <summary>
/// This is called once per frame
/// </summary>

float stopDuration = 5.0f; 
bool isStopped = false;
std::chrono::time_point<std::chrono::steady_clock> stopStartTime;

void Update()
{
    if (!movementAllowed) 
        return;

    if (!cameraMovementAllowed)
    {
		pCamera->SetPosition(trainVehicle.GetPosition() + glm::vec3(0, 5, 0));
        float trainYaw = trainVehicle.GetRotation();

        pCamera->SetRotation(abs(trainYaw)+90);
    }

    if (isStopped)
    {
        auto currentTime = std::chrono::steady_clock::now();
        float elapsedTime = std::chrono::duration<float>(currentTime - stopStartTime).count();

        if (elapsedTime >= stopDuration)
        {
            isStopped = false;
            std::cout << "Stop completed. Resuming movement." << std::endl;
        }
        else
        {
            return;
        }
    }

    if (trainVehicle.MoveTo(rails[railIndex].position, deltaTime * TRAIN_SPEED))
    {
        if (railIndex == 26)
        {
            isStopped = true;
            stopStartTime = std::chrono::steady_clock::now();
            std::cout << "Train stopped at index " << 26 << ". Waiting for " << stopDuration << " seconds." << std::endl;
            
        }
        if (railIndex < rails.size() - 1)
        {            
            railIndex++;
            std::cout << "Next rail: " << railIndex << std::endl;
        }
    }
}

