#ifndef MY_RAYTRACING_H
#define MY_RAYTRACING_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <my_model.h>
#include <iostream>
#include <vector>

// Globals
GLuint triangleSSBO;
GLuint fsVAO, fsVBO;
struct GPUTriangle
{
    // Vertices
    glm::vec4 v0;
    glm::vec4 v1;
    glm::vec4 v2;

    // Vertex normals
    glm::vec4 n0;
    glm::vec4 n1;
    glm::vec4 n2;
};
std::vector<GPUTriangle> triangleBuffer;
float fullscreenQuad[] = 
{
    // Positions   // TexCoords
    -1.0f, -1.0f,  0.0f, 0.0f,
     3.0f, -1.0f,  2.0f, 0.0f,
    -1.0f,  3.0f,  0.0f, 2.0f
};

void getTriangleBuffer(Model& model)
{
    triangleBuffer.clear();
    for (const auto& mesh : model.meshes)
    {
        for (size_t i = 0; i < mesh.indices.size(); i += 3)
        {
            GPUTriangle tri;
            Vertex v0 = mesh.vertices[mesh.indices[i]];
            Vertex v1 = mesh.vertices[mesh.indices[i + 1]];
            Vertex v2 = mesh.vertices[mesh.indices[i + 2]];

            // Vertices
            tri.v0 = glm::vec4(v0.Position, 1.0f);
            tri.v1 = glm::vec4(v1.Position, 1.0f);
            tri.v2 = glm::vec4(v2.Position, 1.0f);

            // Normals
            tri.n0 = glm::vec4(glm::normalize(v0.Normal), 0.0f);
            tri.n1 = glm::vec4(glm::normalize(v1.Normal), 0.0f);
            tri.n2 = glm::vec4(glm::normalize(v2.Normal), 0.0f);
            triangleBuffer.push_back(tri);
        }
    }
}

void setupSSBO()
{
    if (triangleSSBO == 0)
        glGenBuffers(1, &triangleSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, triangleSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
        triangleBuffer.size() * sizeof(GPUTriangle),
        triangleBuffer.data(), GL_DYNAMIC_DRAW); // Use dynamic since re-uploading when changing model
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, triangleSSBO);
}

void setupFullscreenQuad()
{
    glGenVertexArrays(1, &fsVAO);
    glGenBuffers(1, &fsVBO);
    glBindVertexArray(fsVAO);
    glBindBuffer(GL_ARRAY_BUFFER, fsVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(fullscreenQuad), fullscreenQuad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void cleanupRayTracing()
{
    glDeleteBuffers(1, &triangleSSBO);
    glDeleteVertexArrays(1, &fsVAO);
    glDeleteBuffers(1, &fsVBO);
}

#endif // MY_RAYTRACING_H
