#include <GL/glew.h>
#include <glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <glm.hpp>

#pragma comment (lib, "freeglut.lib")
#pragma comment (lib, "glew32.lib")
#pragma comment (lib, "glfw3dll.lib")
#pragma comment (lib, "OpenGL32.lib")

// Struct to hold vertex data
struct Vertex {
    glm::vec3 position;
};

// Struct to hold face data
struct Face {
    std::vector<int> vertexIndices;
};

// Function to load .obj model
bool loadModel(const char* objFilePath, std::vector<Vertex>& vertices, std::vector<Face>& faces) {
    std::ifstream file(objFilePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << objFilePath << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            Vertex vertex;
            iss >> vertex.position.x >> vertex.position.y >> vertex.position.z;
            vertices.push_back(vertex);
        }
        else if (type == "f") {
            Face face;
            std::string indexStr;
            while (iss >> indexStr) {
                std::istringstream indexStream(indexStr);
                int index;
                char discard;
                indexStream >> index;
                face.vertexIndices.push_back(index - 1); // Indices start from 1 in .obj files
            }
            faces.push_back(face);
        }
    }

    file.close();
    return true;
}

// Function to display the model
void display(const std::vector<Vertex>& vertices, const std::vector<Face>& faces) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 1.0f, 1.0f); // Set color to white
    for (const Face& face : faces) {
        for (int index : face.vertexIndices) {
            const Vertex& vertex = vertices[index];
            glVertex3f(vertex.position.x, vertex.position.y, vertex.position.z);
        }
    }
    glEnd();

    glFlush();
}

int main(int argc, char** argv) {

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Model Viewer", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    // Load the .obj model
    std::vector<Vertex> vertices;
    std::vector<Face> faces;
    if (!loadModel("train.obj", vertices, faces)) {
        return 1;
    }

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window)) {
        // Render here
        display(vertices, faces);

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
