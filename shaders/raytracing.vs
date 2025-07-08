#version 430 core
layout(location = 0) in vec2 aPos;        // Fullscreen triangle vertex positions
layout(location = 1) in vec2 aTexCoords;  // UVs (0 to 1) NOT MODEL LAYOUT LOCATIONS, FULLSCREEN TRIANGLE

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
