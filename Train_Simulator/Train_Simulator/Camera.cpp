#include "Camera.h"

Camera::Camera(const int width, const int height, const glm::vec3& position)
{
    startPosition = position;
    freeCamera = true;
    Set(width, height, position);
}

void Camera::Set(const int width, const int height, const glm::vec3& position)
{
    this->isPerspective = true;
    this->yaw = YAW;
    this->pitch = PITCH;

    this->FoVy = FOV;
    this->width = width;
    this->height = height;
    this->zNear = zNEAR;
    this->zFar = zFAR;

    this->worldUp = glm::vec3(0, 1, 0);
    this->position = position;

    lastX = width / 2.0f;
    lastY = height / 2.0f;
    bFirstMouseMove = true;

    UpdateCameraVectors();
}

void Camera::Reset(const int width, const int height)
{
    Set(width, height, startPosition);
}

void Camera::Reshape(int windowWidth, int windowHeight)
{
    width = windowWidth;
    height = windowHeight;

    // define the viewport transformation
    glViewport(0, 0, windowWidth, windowHeight);
}

const glm::vec3 Camera::GetPosition() const
{
    return position;
}

void Camera::SetPosition(glm::vec3 position)
{
    this->position = position;
}

glm::mat4 Camera::GetViewMatrix(MoveableObject* model)
{
    if (freeCamera)
        return glm::lookAt(position, position + forward, up);
    float distance = 10.0f;
    this->forward.x = cos(glm::radians(model->GetYaw())) * distance;
    this->forward.z = sin(glm::radians(model->GetYaw())) * distance;
    return glm::lookAt(model->GetPosition() + glm::vec3(-forward.x, forward.y, -forward.z) + glm::vec3(0.f, 5.0f, 0.f), model->GetPosition() + glm::vec3(0.f, 3.0f, 0.f), glm::vec3(0.0f, 1.0f, 0.0f));
}

const glm::mat4 Camera::GetProjectionMatrix() const
{
    glm::mat4 Proj = glm::mat4(1);
    if (isPerspective)
    {
        float aspectRatio = ((float)(width)) / height;
        Proj = glm::perspective(glm::radians(FoVy), aspectRatio, zNear, zFar);
    } else
    {
        float scaleFactor = 2000.f;
        Proj = glm::ortho<float>(
            -width / scaleFactor, width / scaleFactor,
            -height / scaleFactor, height / scaleFactor, -zFar, zFar);
    }
    return Proj;
}

void Camera::ProcessKeyboard(ECameraMovementType direction, float deltaTime)
{
    float velocity = (float)(cameraSpeedFactor * deltaTime);
    switch (direction)
    {
        case ECameraMovementType::FORWARD:
            position += forward * velocity;
            break;
        case ECameraMovementType::BACKWARD:
            position -= forward * velocity;
            break;
        case ECameraMovementType::LEFT:
            position -= right * velocity;
            break;
        case ECameraMovementType::RIGHT:
            position += right * velocity;
            break;
        case ECameraMovementType::UP:
            position += up * velocity;
            break;
        case ECameraMovementType::DOWN:
            position -= up * velocity;
            break;
    }
}

void Camera::MouseControl(float xPos, float yPos)
{
    if (bFirstMouseMove)
    {
        lastX = xPos;
        lastY = yPos;
        bFirstMouseMove = false;
    }

    float xChange = xPos - lastX;
    float yChange = lastY - yPos;
    lastX = xPos;
    lastY = yPos;

    if (fabs(xChange) <= 1e-6 && fabs(yChange) <= 1e-6)
    {
        return;
    }
    xChange *= mouseSensitivity;
    yChange *= mouseSensitivity;

    ProcessMouseMovement(xChange, yChange);
}

void Camera::ProcessMouseScroll(float yOffset)
{
    if (FoVy >= 1.0f && FoVy <= 90.0f)
    {
        FoVy -= yOffset;
    }
    if (FoVy <= 1.0f)
        FoVy = 1.0f;
    if (FoVy >= 90.0f)
        FoVy = 90.0f;
}

void Camera::SetForwardVector(glm::vec3 forward)
{
    this->forward = forward;
}

void Camera::ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch)
{
    yaw += xOffset;
    pitch += yOffset;

    if (constrainPitch)
    {
        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;
    }

    UpdateCameraVectors();
}

void Camera::SetFreeCamera(bool freeCamera)
{
    this->freeCamera = freeCamera;
}

void Camera::UpdateCameraVectors()
{
    // Calculate the new forward vector
    this->forward.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    this->forward.y = sin(glm::radians(pitch));
    this->forward.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    this->forward = glm::normalize(this->forward);
    // Also re-calculate the Right and Up vector
    right = glm::normalize(glm::cross(forward, worldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    up = glm::normalize(glm::cross(right, forward));
}

bool Camera::GetFreeCamera()
{
    return this->freeCamera;
}
