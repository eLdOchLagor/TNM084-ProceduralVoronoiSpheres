﻿#pragma once

#define _USE_MATH_DEFINES

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Quickhull for creating convex hull
#include <quickhull/QuickHull.hpp>
#include <quickhull/Structs/Vector3.hpp>

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <random>
#include <math.h>
#include <vector>
#include <chrono>
#include <functional>

#include "Utility.h"

std::vector<quickhull::Vector3<float>> generateRandomPointsOnSphere(int n, float r);
void GenerateAnchors(std::vector<float>& points, int n);

glm::vec3 RayIntersectsRoom(const glm::vec3& point_ray);

std::vector<unsigned int> generateAndUploadBuffers(unsigned int& VAO, unsigned int& VBO, unsigned int& EBO);
std::vector<std::pair<glm::vec3, glm::vec3>> computeVoronoiEdges(const std::vector<float>& vertices, const std::vector<glm::vec3>& circumcenters, const std::vector<unsigned int>& indices);
std::vector<glm::vec3> computeCircumcenters(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);
std::vector<float> linesToTriangles(const std::vector<float>& vertices, float width);

#define drawRoomBoundaryAndAnchors 0
// A flattened vector of floats representing the cube's lines
std::vector<float> cubeLines = {
    // Bottom face (-1.5 z)
    -1.5f, -1.5f, -1.5f,  1.5f, -1.5f, -1.5f, // Line 1
     1.5f, -1.5f, -1.5f,  1.5f,  1.5f, -1.5f, // Line 2
     1.5f,  1.5f, -1.5f, -1.5f,  1.5f, -1.5f, // Line 3
    -1.5f,  1.5f, -1.5f, -1.5f, -1.5f, -1.5f, // Line 4

    // Top face (1.5 z)
    -1.5f, -1.5f,  1.5f,  1.5f, -1.5f,  1.5f, // Line 5
     1.5f, -1.5f,  1.5f,  1.5f,  1.5f,  1.5f, // Line 6
     1.5f,  1.5f,  1.5f, -1.5f,  1.5f,  1.5f, // Line 7
    -1.5f,  1.5f,  1.5f, -1.5f, -1.5f,  1.5f, // Line 8

    // Vertical edges
    -1.5f, -1.5f, -1.5f, -1.5f, -1.5f,  1.5f, // Line 9
     1.5f, -1.5f, -1.5f,  1.5f, -1.5f,  1.5f, // Line 10
     1.5f,  1.5f, -1.5f,  1.5f,  1.5f,  1.5f, // Line 11
    -1.5f,  1.5f, -1.5f, -1.5f,  1.5f,  1.5f  // Line 12
};


float viewportWidth = 1500.0;
float viewportHeight = 1500.0;

float randomness = 0.5f;
float width = 0.005f;

struct hash_pair {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& pair) const {
        auto hash1 = std::hash<T1>()(pair.first);  // Hash the first element
        auto hash2 = std::hash<T2>()(pair.second); // Hash the second element
        // Combine the two hashes using bitwise XOR and a shift
        return hash1 ^ (hash2 << 1);
    }
};

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   
    
    // 
    GLFWwindow* window = glfwCreateWindow(viewportWidth, viewportHeight, "Voronoi Spheres", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // UNCOMMENT for mouse movement
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    // Read the vertex shader source from the file
    std::string vertexShaderSource = Utility::readShaderFile("..\\src\\VertexShader.vert");
    if (vertexShaderSource.empty())
    {
        return -1; // Exit if the file couldn't be read
    }

    // Read the fragment shader source from the file
    std::string fragmentShaderSource = Utility::readShaderFile("..\\src\\FragmentShader.frag");
    if (fragmentShaderSource.empty())
    {
        return -1; // Exit if the file couldn't be read
    }

    // Read the geometry shader source from the file
    std::string geometryShaderSource = Utility::readShaderFile("..\\src\\GeometryShader.geom");
    if (geometryShaderSource.empty())
    {
        return -1; // Exit if the file couldn't be read
    }

    unsigned int vertexShader = Utility::compileShader(vertexShaderSource.c_str(), GL_VERTEX_SHADER);
    unsigned int fragmentShader = Utility::compileShader(fragmentShaderSource.c_str(), GL_FRAGMENT_SHADER);
    unsigned int geometryShader = Utility::compileShader(geometryShaderSource.c_str(), GL_GEOMETRY_SHADER);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
   // glAttachShader(shaderProgram, geometryShader);
    glLinkProgram(shaderProgram);

    // Check for linking errors
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    // Delete the shaders as it is no longer needed
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(geometryShader);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    glm::mat4 view = glm::mat4(1.0f);

    glm::mat4 projection;
    projection = glm::perspective(glm::radians(45.0f), viewportWidth / viewportHeight, 0.1f, 100.0f);

    float color[3] = {0.2f, 0.2f, 0.2f};

    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
    unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
    unsigned int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    unsigned int cameraPosLoc = glGetUniformLocation(shaderProgram, "cameraPos");
    unsigned int numberOfPointsLoc = glGetUniformLocation(shaderProgram, "numberOfPoints");
    unsigned int colorLoc = glGetUniformLocation(shaderProgram, "objectColor");

    glViewport(0, 0, viewportWidth, viewportHeight);

    // Updates glViewport when window is resized
    glfwSetFramebufferSizeCallback(window, Utility::framebuffer_size_callback);

    // UNCOMMENT for mouse movement
    //glfwSetCursorPosCallback(window, Utility::mouse_callback);

    // OpenGL buffers
    unsigned int VAO, VBO, EBO;

    // Initialize OpenGL buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    std::vector<unsigned int> castedIndexBuffer = generateAndUploadBuffers(VAO, VBO, EBO);
    
    #if drawRoomBoundaryAndAnchors
    unsigned int VAO_lines, VBO_lines;
    glGenVertexArrays(1, &VAO_lines);
    glGenBuffers(1, &VBO_lines);

    glBindVertexArray(VAO_lines);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_lines);
    glBufferData(GL_ARRAY_BUFFER, cubeLines.size() * sizeof(float), cubeLines.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    #endif

    glBindVertexArray(0);


    glEnable(GL_DEPTH_TEST);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");



    // Rendering loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        // Process user input
        Utility::processInput(window);

        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        // Clear the screen
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Pass uniforms
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3f(cameraPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);
        glUniform3f(colorLoc, color[0], color[1], color[2]);
        glUniform1i(numberOfPointsLoc, numberOfPoints);

        //numberOfPoints = abs(sin(currentFrame)) * 100;
        
        // Detect change in numberOfPoints
        if (numberOfPoints != previousNumberOfPoints) {
            previousNumberOfPoints = numberOfPoints;
            castedIndexBuffer = generateAndUploadBuffers(VAO, VBO, EBO); // Regenerate points and upload buffers
        }

        

        // Use the shader program
        glUseProgram(shaderProgram);
        
        glLineWidth(5.0f);
        glPointSize(5.0f);

        // Bind the VAO and draw
        glBindVertexArray(VAO);

        // For drawing triangulation
        if (renderTriangles)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawElements(GL_TRIANGLES, castedIndexBuffer.size(), GL_UNSIGNED_INT, 0);
        }
        else if (renderPoints)
        {
            glDrawArrays(GL_POINTS, 0, 1000000);
        }
        else
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDrawArrays(GL_TRIANGLES, 0, 1000000);

            #if drawRoomBoundaryAndAnchors
            glBindVertexArray(VAO_lines);
            glDrawArrays(GL_LINES, 0, 1000);
            #endif
        }
        

        // Unbind VAO
        glBindVertexArray(0);

        ImGui::Begin("User interface");
        bool regenerateMesh = false;

        // Check if any of the sliders changed
        if (ImGui::SliderFloat("Randomness", &randomness, 0.0f, 1.0f)) {
            regenerateMesh = true;
        }
        if (ImGui::SliderInt("Number of seeds", &numberOfPoints, 5, 2000)) {
            regenerateMesh = true;
        }
        if (ImGui::SliderFloat("Edge width", &width, 0.001f, 0.03f)) {
            regenerateMesh = true;
        }

        ImGui::ColorEdit3("Color", color);

        if (ImGui::Checkbox("Display Delaunay triangles", &renderTriangles))
        {
            regenerateMesh = true;
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Display seed points", &renderPoints))
        {
            regenerateMesh = true;
        }

        if (ImGui::Button("Export"))
        {
            Utility::exportToOBJ(triangleStripVertices, "mesh.obj");
        }

        if (regenerateMesh) {
            castedIndexBuffer = generateAndUploadBuffers(VAO, VBO, EBO); // Regenerate points and upload buffers
        }

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap the buffers
        glfwSwapBuffers(window);

        // Poll for events
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}

// https://extremelearning.com.au/how-to-evenly-distribute-points-on-a-sphere-more-effectively-than-the-canonical-fibonacci-lattice/
std::vector<quickhull::Vector3<float>> generateRandomPointsOnSphere(int n, float r) {
    std::vector<quickhull::Vector3<float>> points;

    float goldenRatio = (1.0f + sqrt(5.0f)) / 2.0f; // Golden ratio

    for (int i = 0; i < n; i++) {
        float inclinationAngle = 2.0f * M_PI * i / goldenRatio;
        float azimuthAngle = std::acos(1.0f - 2.0f * (i + 0.5f) / n);

        // Convert spherical to Cartesian coordinates
        float x = std::cos(inclinationAngle) * std::sin(azimuthAngle);
        float y = std::sin(inclinationAngle) * std::sin(azimuthAngle);
        float z = std::cos(azimuthAngle);

        glm::vec3 pos{ x, y, z };

        // Generate a random direction vector for offset
        glm::vec3 randomOffset{
            Utility::generateRandomValue(-1.0, 1.0),
            Utility::generateRandomValue(-1.0, 1.0),
            Utility::generateRandomValue(-1.0, 1.0)
        };
        randomOffset = glm::normalize(randomOffset);

        double offsetMagnitude = Utility::generateRandomValue(0.0, randomness); // Magnitude determined by how random
        randomOffset *= offsetMagnitude;

        pos += randomOffset;
        pos = glm::normalize(pos);

        points.push_back(quickhull::Vector3<float>{pos.x, pos.y, pos.z});   
    }

    return points;
}

std::vector<unsigned int> generateAndUploadBuffers(unsigned int& VAO, unsigned int& VBO, unsigned int& EBO) {
    auto start = std::chrono::high_resolution_clock::now(); // TIMER START

    // Generate grid points
    std::vector<quickhull::Vector3<float>> points = generateRandomPointsOnSphere(numberOfPoints, radius);

    quickhull::QuickHull<float> qh;
    auto hull = qh.getConvexHull(points, true, false);
    const auto& indexBuffer = hull.getIndexBuffer();
    const auto& vertexBuffer = hull.getVertexBuffer();


    // Flatten the vertex buffer
    std::vector<float> flattenedVertexBuffer;
    flattenedVertexBuffer.reserve(vertexBuffer.size() * 3);
    for (const auto& vertex : vertexBuffer) {
        flattenedVertexBuffer.push_back(vertex.x);
        flattenedVertexBuffer.push_back(vertex.y);
        flattenedVertexBuffer.push_back(vertex.z);
    }

    std::vector<unsigned int> castedIndexBuffer(indexBuffer.begin(), indexBuffer.end());

    // Compute voronoi edges
    std::vector<glm::vec3> circumcenters = computeCircumcenters(flattenedVertexBuffer, castedIndexBuffer);
    std::vector<std::pair<glm::vec3, glm::vec3>> voronoiEdges = computeVoronoiEdges(flattenedVertexBuffer, circumcenters, castedIndexBuffer);

    // Flatten Voronoi edges for OpenGL
    std::vector<float> voronoiEdgeVertices;

    for (const auto& edge : voronoiEdges) {
        voronoiEdgeVertices.push_back(edge.first.x);
        voronoiEdgeVertices.push_back(edge.first.y);
        voronoiEdgeVertices.push_back(edge.first.z);

        voronoiEdgeVertices.push_back(edge.second.x);
        voronoiEdgeVertices.push_back(edge.second.y);
        voronoiEdgeVertices.push_back(edge.second.z);
    }

#if drawRoomBoundaryAndAnchors
    GenerateAnchors(voronoiEdgeVertices, 100);
#endif

    triangleStripVertices = linesToTriangles(voronoiEdgeVertices, width);

    auto end = std::chrono::high_resolution_clock::now(); // TIMER STOP
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "The generation took " << duration << " milliseconds.\n";

    if (renderTriangles)
    {
        // Update OpenGL buffers
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, flattenedVertexBuffer.size() * sizeof(float), flattenedVertexBuffer.data(), GL_DYNAMIC_DRAW);

        // For drawing the triangulation
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, castedIndexBuffer.size() * sizeof(unsigned int), castedIndexBuffer.data(), GL_DYNAMIC_DRAW);
        
        // configure vertex attributes
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }
    else if (renderPoints)
    {
        std::vector<float> flattened;
        flattened.reserve(points.size() * 3);
        for (const auto& vertex : points) {
            flattened.push_back(vertex.x);
            flattened.push_back(vertex.y);
            flattened.push_back(vertex.z);
        }

        // Update OpenGL buffers
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, flattened.size() * sizeof(float), flattened.data(), GL_DYNAMIC_DRAW);

        // configure vertex attributes
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }
    else
    {
        // Update OpenGL buffers
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, triangleStripVertices.size() * sizeof(float), triangleStripVertices.data(), GL_DYNAMIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }


    return castedIndexBuffer;
}

std::vector<glm::vec3> computeCircumcenters(const std::vector<float>& vertices,
    const std::vector<unsigned int>& indices) {
    std::vector<glm::vec3> circumcenters;

    for (size_t i = 0; i < indices.size(); i += 3) {
        // Get the three vertices of the triangle
        glm::vec3 A{ vertices[indices[i] * 3], vertices[indices[i] * 3 + 1], vertices[indices[i] * 3 + 2] };
        glm::vec3 B{ vertices[indices[i + 1] * 3], vertices[indices[i + 1] * 3 + 1], vertices[indices[i + 1] * 3 + 2] };
        glm::vec3 C{ vertices[indices[i + 2] * 3], vertices[indices[i + 2] * 3 + 1], vertices[indices[i + 2] * 3 + 2] };
        
        glm::vec3 a = A - C;
        glm::vec3 b = B - C;

        glm::vec3 circumcenter = glm::cross((glm::length(a) * glm::length(a) * b - glm::length(b) * glm::length(b) * a), glm::cross(a, b)) / (2.0f * glm::length(glm::cross(a, b)) * glm::length(glm::cross(a, b))) + C;

        // Project the circumcenter onto the sphere surface
        circumcenter = glm::normalize(circumcenter);

        circumcenters.push_back(circumcenter);
    }

    return circumcenters;
}

std::vector<std::pair<glm::vec3, glm::vec3>> computeVoronoiEdges(const std::vector<float>& vertices,
    const std::vector<glm::vec3>& circumcenters,
    const std::vector<unsigned int>& indices) {

    std::unordered_map<std::pair<int, int>, glm::vec3, hash_pair> edgeToCircumcenter;
    std::vector<std::pair<glm::vec3, glm::vec3>> voronoiEdges;

    for (size_t i = 0; i < indices.size(); i += 3) { // loop through every triangle
        int a = indices[i];
        int b = indices[i + 1];
        int c = indices[i + 2];

        glm::vec3 A{ vertices[a], vertices[a + 1], vertices[a + 2] };
        glm::vec3 B{ vertices[b], vertices[b + 1], vertices[b + 2] };
        glm::vec3 C{ vertices[c], vertices[c + 1], vertices[c + 2] };

        glm::vec3 AB = B - A;
        glm::vec3 AC = C - A;
        glm::vec3 normal = glm::cross(AB, AC);

        glm::vec3 circumcenter = circumcenters[i / 3];

        auto insertEdge = [&](int u, int v) {
            if (u > v) std::swap(u, v);
            auto edge = std::make_pair(u, v);

            if (edgeToCircumcenter.find(edge) != edgeToCircumcenter.end()) { // If already added
                voronoiEdges.emplace_back(edgeToCircumcenter[edge], circumcenter);
                edgeToCircumcenter.erase(edge);
            }
            else {
                edgeToCircumcenter[edge] = circumcenter;
            }
            };

        insertEdge(a, b);
        insertEdge(b, c);
        insertEdge(c, a);
    }

    return voronoiEdges;
}

std::vector<float> linesToTriangles(const std::vector<float>& vertices, float width) {
    
    std::vector<float> triangles;

    for (size_t i = 0; i < vertices.size(); i += 6)
    {
        glm::vec3 p1{ vertices[i], vertices[i + 1], vertices[i + 2] };
        glm::vec3 p2{ vertices[i+3], vertices[i + 4], vertices[i + 5] };

        glm::vec3 midPoint = (p1 + p2) / 2.0f;

        glm::vec3 offset = glm::normalize(glm::cross(glm::normalize(midPoint), p2 - p1)) * width;

        glm::vec3 newPoint1 = p1 - offset;
        glm::vec3 newPoint2 = p2 - offset;
        glm::vec3 newPoint3 = p1 + offset;
        glm::vec3 newPoint4 = p2 + offset;

        glm::vec3 newPoint5 = newPoint1 * (1.0f - 2.0f * width);
        glm::vec3 newPoint6 = newPoint2 * (1.0f - 2.0f * width);
        glm::vec3 newPoint7 = newPoint3 * (1.0f - 2.0f * width);
        glm::vec3 newPoint8 = newPoint4 * (1.0f - 2.0f * width);

        glm::vec3 normal = glm::normalize(midPoint);
        glm::vec3 normalSide = -glm::normalize(offset);

        // ----------------- Outside faces
        {
            // Triangle 1
            triangles.push_back(newPoint1.x);
            triangles.push_back(newPoint1.y);
            triangles.push_back(newPoint1.z);
            // Normal
            triangles.push_back(normal.x);
            triangles.push_back(normal.y);
            triangles.push_back(normal.z);

            triangles.push_back(newPoint3.x);
            triangles.push_back(newPoint3.y);
            triangles.push_back(newPoint3.z);
            // Normal
            triangles.push_back(normal.x);
            triangles.push_back(normal.y);
            triangles.push_back(normal.z);

            triangles.push_back(newPoint2.x);
            triangles.push_back(newPoint2.y);
            triangles.push_back(newPoint2.z);
            // Normal
            triangles.push_back(normal.x);
            triangles.push_back(normal.y);
            triangles.push_back(normal.z);

            // Triangle 2
            triangles.push_back(newPoint2.x);
            triangles.push_back(newPoint2.y);
            triangles.push_back(newPoint2.z);
            // Normal
            triangles.push_back(normal.x);
            triangles.push_back(normal.y);
            triangles.push_back(normal.z);

            triangles.push_back(newPoint3.x);
            triangles.push_back(newPoint3.y);
            triangles.push_back(newPoint3.z);
            // Normal
            triangles.push_back(normal.x);
            triangles.push_back(normal.y);
            triangles.push_back(normal.z);

            triangles.push_back(newPoint4.x);
            triangles.push_back(newPoint4.y);
            triangles.push_back(newPoint4.z);
            // Normal
            triangles.push_back(normal.x);
            triangles.push_back(normal.y);
            triangles.push_back(normal.z);
        }

        // ------------------ Inside faces
        {
            // Triangle 1
            triangles.push_back(newPoint5.x);
            triangles.push_back(newPoint5.y);
            triangles.push_back(newPoint5.z);
            // Normal
            triangles.push_back(normal.x);
            triangles.push_back(normal.y);
            triangles.push_back(normal.z);

            triangles.push_back(newPoint7.x);
            triangles.push_back(newPoint7.y);
            triangles.push_back(newPoint7.z);
            // Normal
            triangles.push_back(normal.x);
            triangles.push_back(normal.y);
            triangles.push_back(normal.z);

            triangles.push_back(newPoint6.x);
            triangles.push_back(newPoint6.y);
            triangles.push_back(newPoint6.z);
            // Normal
            triangles.push_back(normal.x);
            triangles.push_back(normal.y);
            triangles.push_back(normal.z);

            // Triangle 2
            triangles.push_back(newPoint6.x);
            triangles.push_back(newPoint6.y);
            triangles.push_back(newPoint6.z);
            // Normal
            triangles.push_back(normal.x);
            triangles.push_back(normal.y);
            triangles.push_back(normal.z);

            triangles.push_back(newPoint7.x);
            triangles.push_back(newPoint7.y);
            triangles.push_back(newPoint7.z);
            // Normal
            triangles.push_back(normal.x);
            triangles.push_back(normal.y);
            triangles.push_back(normal.z);

            triangles.push_back(newPoint8.x);
            triangles.push_back(newPoint8.y);
            triangles.push_back(newPoint8.z);
            // Normal
            triangles.push_back(normal.x);
            triangles.push_back(normal.y);
            triangles.push_back(normal.z);
        }

        // ------------------ Side 1 faces
        {
            // Triangle 1
            triangles.push_back(newPoint1.x);
            triangles.push_back(newPoint1.y);
            triangles.push_back(newPoint1.z);
            // Normal
            triangles.push_back(normalSide.x);
            triangles.push_back(normalSide.y);
            triangles.push_back(normalSide.z);

            triangles.push_back(newPoint2.x);
            triangles.push_back(newPoint2.y);
            triangles.push_back(newPoint2.z);
            // Normal
            triangles.push_back(normalSide.x);
            triangles.push_back(normalSide.y);
            triangles.push_back(normalSide.z);

            triangles.push_back(newPoint5.x);
            triangles.push_back(newPoint5.y);
            triangles.push_back(newPoint5.z);
            // Normal
            triangles.push_back(normalSide.x);
            triangles.push_back(normalSide.y);
            triangles.push_back(normalSide.z);

            // Triangle 2
            triangles.push_back(newPoint5.x);
            triangles.push_back(newPoint5.y);
            triangles.push_back(newPoint5.z);
            // Normal
            triangles.push_back(normalSide.x);
            triangles.push_back(normalSide.y);
            triangles.push_back(normalSide.z);

            triangles.push_back(newPoint2.x);
            triangles.push_back(newPoint2.y);
            triangles.push_back(newPoint2.z);
            // Normal
            triangles.push_back(normalSide.x);
            triangles.push_back(normalSide.y);
            triangles.push_back(normalSide.z);

            triangles.push_back(newPoint6.x);
            triangles.push_back(newPoint6.y);
            triangles.push_back(newPoint6.z);
            // Normal
            triangles.push_back(normalSide.x);
            triangles.push_back(normalSide.y);
            triangles.push_back(normalSide.z);
        }

        // ------------------ Side 2 faces
        {
            // Triangle 1
            triangles.push_back(newPoint3.x);
            triangles.push_back(newPoint3.y);
            triangles.push_back(newPoint3.z);
            // Normal
            triangles.push_back(-normalSide.x);
            triangles.push_back(-normalSide.y);
            triangles.push_back(-normalSide.z);

            triangles.push_back(newPoint7.x);
            triangles.push_back(newPoint7.y);
            triangles.push_back(newPoint7.z);
            // Normal
            triangles.push_back(-normalSide.x);
            triangles.push_back(-normalSide.y);
            triangles.push_back(-normalSide.z);

            triangles.push_back(newPoint4.x);
            triangles.push_back(newPoint4.y);
            triangles.push_back(newPoint4.z);
            // Normal
            triangles.push_back(-normalSide.x);
            triangles.push_back(-normalSide.y);
            triangles.push_back(-normalSide.z);

            // Triangle 2
            triangles.push_back(newPoint4.x);
            triangles.push_back(newPoint4.y);
            triangles.push_back(newPoint4.z);
            // Normal
            triangles.push_back(-normalSide.x);
            triangles.push_back(-normalSide.y);
            triangles.push_back(-normalSide.z);

            triangles.push_back(newPoint3.x);
            triangles.push_back(newPoint3.y);
            triangles.push_back(newPoint3.z);
            // Normal
            triangles.push_back(-normalSide.x);
            triangles.push_back(-normalSide.y);
            triangles.push_back(-normalSide.z);

            triangles.push_back(newPoint8.x);
            triangles.push_back(newPoint8.y);
            triangles.push_back(newPoint8.z);
            // Normal
            triangles.push_back(-normalSide.x);
            triangles.push_back(-normalSide.y);
            triangles.push_back(-normalSide.z);
        }
        
    }

    return triangles;
}

void GenerateAnchors(std::vector<float>& points, int n) {
    std::vector<glm::vec3> randomPoints;

    // Select n random points
    for (int i = 0; i < n; i++)
    {
        int randomIndex = Utility::generateRandomValue(0.0, 1.0) * ((points.size()/3) - 1);

        // If I want to move the point out aswell, brute force fix since there are duplicate points
        for (size_t i = 0; i < points.size(); i++)
        {
            if (points[i] == points[randomIndex])
            {
                //points[randomIndex] *= 1.2;
                //points[randomIndex + 1] *= 1.2;
                //points[randomIndex + 2] *= 1.2;
            }
        }

        randomPoints.push_back(glm::vec3{points[randomIndex], points[randomIndex+1], points[randomIndex+2]});
    }

    for (size_t i = 0; i < randomPoints.size(); i++)
    {
        glm::vec3 currentPoint = randomPoints[i];
        glm::vec3 newPoint = currentPoint * 2.0f;

        glm::vec3 intersectionPoint = RayIntersectsRoom(currentPoint);

        std::cout << intersectionPoint.x << " " << intersectionPoint.y << " " << intersectionPoint.z << "\n";

        points.push_back(currentPoint.x);
        points.push_back(currentPoint.y);
        points.push_back(currentPoint.z);

        points.push_back(intersectionPoint.x);
        points.push_back(intersectionPoint.y);
        points.push_back(intersectionPoint.z);
        
    }
    
}

glm::vec3 RayIntersectsRoom(const glm::vec3& point_ray) {
    float t_min = std::numeric_limits<float>::infinity();

    float tx1 = (-1.5f - point_ray.x) / point_ray.x;
    float tx2 = (1.5f - point_ray.x) / point_ray.x;
    t_min = glm::min(t_min, glm::max(tx1, tx2));

    float ty1 = (-1.5f - point_ray.y) / point_ray.y;
    float ty2 = (1.5f - point_ray.y) / point_ray.y;
    t_min = glm::min(t_min, glm::max(ty1, ty2));

    float tz1 = (-1.5f - point_ray.z) / point_ray.z;
    float tz2 = (1.5f - point_ray.z) / point_ray.z;
    t_min = glm::min(t_min, glm::max(tz1, tz2));

    glm::vec3 intersection_point = point_ray + t_min * point_ray;

    return intersection_point;
}

