#ifndef MY_IMGUI
#define MY_IMGUI

// <includes>
#include <iostream>
#include <sstream> // Requires C++17
#include <iomanip> // Requires C++17
#include <vector>
#include <filesystem> // Requires C++17
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stb_image_write.h>
// </includes>

// <Screenshot>
void saveScreenshot(const std::string& filename, int width, int height)
{
    std::vector<unsigned char> pixels(width * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // Flip vertically for correct orientation
    for (int j = 0; j < height / 2; ++j)
    {
        for (int i = 0; i < width * 3; ++i)
        {
            std::swap(pixels[j * width * 3 + i], pixels[(height - 1 - j) * width * 3 + i]);
        }
    }

    // Ensure "screenshots" folder exists
    std::filesystem::path screenshotDir = "screenshots";
    if (!std::filesystem::exists(screenshotDir))
        std::filesystem::create_directory(screenshotDir);

    std::string fullPath = (screenshotDir / filename).string();
    stbi_write_png(fullPath.c_str(), width, height, 3, pixels.data(), width * 3);
    std::cout << "Saved screenshot to " << fullPath << "\n";
}
// </Screenshot>

// <FPS Tester>
struct FPSTracker 
{
    float minFPS = std::numeric_limits<float>::max();
    float maxFPS = 0.0f;
    float totalFPS = 0.0f;
    int frameCount = 0;
    int frameLimit = 1000;
    bool active = false;

    void start(int frames = 1000) 
    {
        minFPS = std::numeric_limits<float>::max();
        maxFPS = 0.0f;
        totalFPS = 0.0f;
        frameCount = 0;
        frameLimit = frames;
        active = true;
    }

    void update(float deltaTime) 
    {
        if (!active) 
            return;

        float fps = 1.0f / deltaTime;
        minFPS = std::min(minFPS, fps);
        maxFPS = std::max(maxFPS, fps);
        totalFPS += fps;
        frameCount++;

        if (frameCount >= frameLimit) 
        {
            active = false;
            float avg = totalFPS / static_cast<float>(frameCount);

            std::cout << "FPS Test Results:\n";
            std::cout << "> Frames: " << frameCount << "\n";
            std::cout << "> Min FPS: " << minFPS << "\n";
            std::cout << "> Max FPS: " << maxFPS << "\n";
            std::cout << "> Avg FPS: " << avg << "\n";
            std::cout << "****************************\n\n";
        }
    }

    float averageFPS() const 
    {
        return (frameCount > 0) ? (totalFPS / frameCount) : 0.0f;
    }
};
// </FPS Tester>

// Global instance
FPSTracker fpsTracker;

enum ModelTypes
{
    TeaPot = 0,
    Donut = 1,
    Sphere = 2,
    Monkey = 3,
    Buddha = 4
};

enum Skyboxes
{
    Graffiti = 0,
    NightSky = 1,
    Museum = 2
};

float IOR = 1.5f;
const char* modelOptions[5] = { "Teapot", "Donut", "Sphere", "Monkey", "Buddha"};
const char* skyboxOptions[3] = { "Graffiti", "Night Sky", "Museum" };
ModelTypes selectedModel = Monkey;
Skyboxes selectedSkybox = Museum;
bool enableReflect = true;
bool ImGuiUseMouse = true;
bool modelChanged = false;
bool takeScreenshot = false;
bool zoomIn = false;

void ImGuiSetup(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Set font
    io.Fonts->Clear();
    std::string openSansPath = "C:\\fonts\\Open_Sans\\static\\OpenSans_Condensed-Regular.ttf";
    ImFont* myFont = io.Fonts->AddFontFromFileTTF(openSansPath.c_str(), 30.0f);

    // Rebuild the font atlas
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
}

void ImGuiNewFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiDrawWindow()
{
    ImGui::SetNextWindowCollapsed(!ImGuiUseMouse);
    ImGui::SetNextWindowPos(ImVec2(50, 50));
    ImGui::SetNextWindowSize(ImVec2(500, 550));
    ImGui::Begin("Raytracer");

    ImGui::Text("Index of Refraction (IOR):");
    ImGui::SliderFloat("IOR", &IOR, 1.0f, 2.5f);

    ImGui::Text("Disable Reflectance:");
    ImGui::Checkbox("Reflect:", &enableReflect);

    // Dropdown menu for model selection
    ImGui::Text("Select Model:");
    if (ImGui::Combo("Model", reinterpret_cast<int*>(&selectedModel), modelOptions, IM_ARRAYSIZE(modelOptions)))
        modelChanged = true;

    // Dropdown menu for skybox selection
    ImGui::Text("Select Skybox:");
    ImGui::Combo("Skybox", reinterpret_cast<int*>(&selectedSkybox), skyboxOptions, IM_ARRAYSIZE(skyboxOptions));

    // FPS test
    ImGui::Text("Run FPS Test:");
    if (ImGui::Button("Start FPS Test", ImVec2(150, 36)))
    {
        std::cout << "****************************\n";
        std::cout << "Starting FPS Test:\n";
        std::cout << "> Active Model: " << modelOptions[selectedModel] << "\n";
        std::cout << "> Active Skybox: " << skyboxOptions[selectedSkybox] << "\n";
        std::cout << "> Reflection Active: " << enableReflect << "\n";
        std::cout << "> IOR: " << IOR << "\n";
        std::cout << "****************************\n";
        fpsTracker.start(50);
    }

    // Zoom in
    ImGui::Text("Zoom Camera In:");
    ImGui::Checkbox("Zoom In:", &zoomIn);

    // Screenshot
    ImGui::Text("Take Screenshot:");
    if (ImGui::Button("Screenshot", ImVec2(150, 36)))
        takeScreenshot = true;

    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

#endif // MY_IMGUI