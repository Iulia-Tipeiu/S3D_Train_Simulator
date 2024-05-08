#pragma once

#include <GL/glew.h>
#include <GLM.hpp>
#include <gtc/matrix_transform.hpp>
#include <glfw3.h>

#include "Model.h"

enum MovementType
{
    V_UNKNOWN,
    V_FORWARD,
    V_BACKWARD,
    V_LEFT,
    V_RIGHT
};

class MoveableObject
{
private:
    const float zNEAR = 0.1f;
    const float zFAR = 500.f;
    const float YAW = -90.0f;
    const float FOV = 45.0f;
    glm::vec3 startPosition;
    Model objectModel;

public:
    MoveableObject() = default;
    MoveableObject(Model model, const int width, const int height, const glm::vec3& position);

    MoveableObject& operator=(const MoveableObject& othervehicle);

    void Set(const int width, const int height, const glm::vec3& position);

    const glm::vec3 GetPosition() const;

    const glm::mat4 GetProjectionMatrix() const;

    void ProcessKeyboard(MovementType direction, float deltaTime);

    Model& GetVehicleModel();

    float GetRotation();

    void SetRotation(float newRotation);

    glm::vec3 GetForward();

    float GetYaw();

private:
    void UpdateObjectVectors();

protected:
    const float vehicleSpeedFactor = 2.5f;

    // Perspective properties
    float zNear;
    float zFar;
    float FoVy;
    int width;
    int height;
    bool isPerspective;

    glm::vec3 position;
    glm::vec3 forward;
    glm::vec3 right;
    float rotation = 1.0f;
    glm::vec3 worldUp;

    // Euler Angles
    float yaw;

    float lastX = 0.f, lastY = 0.f;
};

