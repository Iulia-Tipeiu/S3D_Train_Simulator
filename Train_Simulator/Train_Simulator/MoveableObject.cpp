#include "MoveableObject.h"

MoveableObject::MoveableObject(Model model, const int width, const int height, const glm::vec3& position)
{
    objectModel = model;
	startPosition = position;
	Set(width, height, position);
}

MoveableObject& MoveableObject::operator=(const MoveableObject& otherObject)
{
    objectModel = otherObject.objectModel;
    startPosition = otherObject.startPosition;
    Set(otherObject.width, otherObject.height, otherObject.position);
    return *this;
}

void MoveableObject::Set(const int width, const int height, const glm::vec3 & position)
{
    this->isPerspective = true;
    this->yaw = YAW;

    this->FoVy = FOV;
    this->width = width;
    this->height = height;
    this->zNear = zNEAR;
    this->zFar = zFAR;

    this->worldUp = glm::vec3(0, 1, 0);
    this->position = position;

    lastX = width / 2.0f;
    lastY = height / 2.0f;

    UpdateObjectVectors();
}

const glm::vec3 MoveableObject::GetPosition() const
{
	return position;
}

const glm::mat4 MoveableObject::GetProjectionMatrix() const
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

void MoveableObject::ProcessKeyboard(MovementType direction, float deltaTime)
{
    float velocity = (float)(vehicleSpeedFactor * deltaTime);
    switch (direction)
    {
    case MovementType::V_FORWARD:
        position += forward * velocity;
        break;
    case MovementType::V_BACKWARD:
        position -= forward * velocity;
        break;
    case MovementType::V_LEFT:
        rotation += 10.5f * velocity;
        yaw -= 10.5f * velocity;
        UpdateObjectVectors();
        break;
    case MovementType::V_RIGHT:
        rotation -= 10.5f * velocity;
        yaw += 10.5f * velocity;
        UpdateObjectVectors();
        break;
    }
}

Model& MoveableObject::GetVehicleModel()
{
    return objectModel;
}

float MoveableObject::GetRotation()
{
    return rotation;
}

void MoveableObject::SetRotation(float newRotation)
{
    rotation = newRotation;
}

glm::vec3 MoveableObject::GetForward()
{
    return forward;
}

float MoveableObject::GetYaw()
{
    return yaw;
}

void MoveableObject::UpdateObjectVectors()
{
    // Calculate the new forward vector
    this->forward.x = cos(glm::radians(yaw));
    this->forward.z = sin(glm::radians(yaw));
    this->forward = glm::normalize(this->forward);
    
    //See if right is needed

    right = glm::normalize(glm::cross(forward, worldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
}
