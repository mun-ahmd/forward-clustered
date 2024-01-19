#pragma once
#include <iostream>
//including math library to use
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/norm.hpp"

#define VULKAN_INVERT


enum Camera_Movement {
    RIGHT,
    LEFT,
    UP,
    DOWN,
    BACKWARD,
    FORWARD,
    LAST
};

class Camera
{
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    // camera options
    float movementSpeed;
    float mouseSensitivity;
    float zoom;

    Camera() = default;

    Camera(glm::vec3 position, glm::vec3 up, glm::vec3 front);

    glm::mat4 getViewMatrix();

    void changeCameraFront(glm::vec3 newFront);

    void keyboardMovementProcess(Camera_Movement direction, float deltaTime);

    void mouseLookProcess(float xOffset, float yOffset, bool constraintsActive);

    void processMouseScroll(float yOffset);

    void debugCamera();

private:
    // euler Angles
    float yaw;
    float pitch;
    float roll;

    void updateCameraVectors();
    void calcPitchYawRoll();
};

class CameraCubemap {
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 right;
    glm::vec3 up;
    glm::vec3 worldUp;

    CameraCubemap();

    void changeCamDir(Camera_Movement direction);

    glm::mat4 getViewMatrix();

private:
    glm::vec3 upFront{ 0.0f, 1.0f, 0.0f };
    glm::vec3 upRight{ 1.0f, 0.0f, 0.0f };

    glm::vec3 downFront{ 0.0f, -1.0f, 0.0f };
    glm::vec3 downRight{ -1.0f,0.0f,0.0f };

    glm::vec3 rightFront{ 1.0f, 0.0f, 0.0f };
    glm::vec3 leftFront{ -1.0f, 0.0f, 0.0f };
    glm::vec3 forwardFront{ 0.0f, 0.0f, 1.0f };
    glm::vec3 backwardFront{ 0.0f, 0.0f, -1.0f };
};

//forward declaration
class GLFWwindow;

class CamHandler
{
private:
    GLFWwindow* currentWindow;
    double mouseLastX;
    double mouseLastY;
    bool hasMouseMovedOnce;
    Camera cam;
public:
    CamHandler() = default;

    CamHandler(GLFWwindow* window)
    {
        this->cam = Camera(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0, 1, 0));
        currentWindow = window;
        hasMouseMovedOnce = 0;
        mouseLastX = 0.0;
        mouseLastY = 0.0;
        cam.movementSpeed /= 2;
    }
    void moveAround(double deltaTime);
    void lookAround();
    glm::mat4 getView()
    {
        return this->cam.getViewMatrix();
    }
    glm::vec3 get_dir() {
        return this->cam.front;
    }
    glm::vec3 get_pos() {
        return this->cam.position;
    }
};