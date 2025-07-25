#ifndef MY_MODEL_H
#define MY_MODEL_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <my_mesh.h>
#include <my_shader.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

class Model
{
public:
    // Public for wall constraints
    std::vector<Mesh> meshes;

    // Constructor (expects a filepath to a 3D model)
    Model(std::string const& objPath, const GLenum& minFilterType)
        : minFilterMethod(minFilterType)
    {
        loadModel(objPath);
    }

    // Draw the model (all its meshes)
    void draw(Shader& shader)
    {
        for (unsigned int i = 0; i < static_cast<unsigned int>(meshes.size()); i++)
            meshes[i].draw(shader);
    }

private:
    // For mipmaps
    GLenum minFilterMethod;

    // All textures already loaded
    std::vector<Texture> loadedTextures;

    // Load a 3D model specified by path
    void loadModel(std::string const& path)
    {
        // Read file
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        
        // Check for errors
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            return;
        }

        // Process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene);
    }

    // Processes a node recursively
    void processNode(aiNode* node, const aiScene* scene)
    {
        // Process each mesh located at current node
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        // Recursively process children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++)
            processNode(node->mChildren[i], scene);
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene)
    {
        // Data to fill
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture> textures;

        // Loop through mesh's vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector;

            // Positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;

            // Normals
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }

            // Texture 
            if (mesh->mTextureCoords[0])
            {
                glm::vec2 vec;
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }

        // Loop through mesh's faces and retrieve the corresponding vertex indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];

            // Retrieve all indices of the face and store them in the indices vector
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        // Process materials
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        // Diffuse textures
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "diffuseMap");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        // Specular textures
        std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "specularMap");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

        // Normal map textures
        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "normalMap");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

        // Bump map textures
        std::vector<Texture> bumpMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "bumpMap");
        textures.insert(textures.end(), bumpMaps.begin(), bumpMaps.end());

        // Return a mesh object created from the extracted mesh data
        return Mesh(vertices, indices, textures, std::string(mesh->mName.C_Str()));
    }

    // Load materials
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
    {
        std::vector<Texture> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);

            // Check if texture already loaded and if so, continue
            bool skip = false;
            for (int j = 0; j < static_cast<int>(loadedTextures.size()); j++)
            {
                if (std::strcmp(loadedTextures[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(loadedTextures[j]);
                    skip = true;
                    break;
                }
            }
            if (!skip)
            {
                Texture texture;
                texture.id = loadTexture(str.C_Str());
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                loadedTextures.push_back(texture);
            }
        }
        return textures;
    }

    // Load texture
    unsigned int loadTexture(const char* texturePath)
    {
        unsigned int textureID;
        glGenTextures(1, &textureID);

        stbi_set_flip_vertically_on_load(false);
        int width, height, numChannels;
        unsigned char* data = stbi_load(texturePath, &width, &height, &numChannels, 0);
        if (data)
        {
            GLenum format = GL_RGB;
            if (numChannels == 1)
                format = GL_RED;
            else if (numChannels == 3)
                format = GL_RGB;
            else if (numChannels == 4)
                format = GL_RGBA;

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            // If one channel, "swizzle" G and B channels to R (colour diffuse shader expects RGB vec3)
            if (format == GL_RED)
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
            }

            // If using mipmaps
            if (minFilterMethod == GL_NEAREST_MIPMAP_NEAREST ||
                minFilterMethod == GL_NEAREST_MIPMAP_LINEAR ||
                minFilterMethod == GL_LINEAR_MIPMAP_NEAREST ||
                minFilterMethod == GL_LINEAR_MIPMAP_LINEAR)
            {
                glGenerateMipmap(GL_TEXTURE_2D);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilterMethod);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
            // If nearest or linear (no mipmap)
            else if (minFilterMethod == GL_NEAREST ||
                minFilterMethod == GL_LINEAR)
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilterMethod);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
            // Invalid, use linear
            else
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
        }
        else
            std::cout << "Texture failed to load at path: " << texturePath << std::endl;

        stbi_image_free(data);

        return textureID;
    }
};

#endif // MY_MODEL_H
