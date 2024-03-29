// this code is based on example "libs\imgui\examples\example_glfw_opengl3\main.cpp"


#include <iostream>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <filesystem>
#include <memory>
#include <imnodes.h>
#include "app.hpp"



static void GLFW_error_callback(int err,const char* msg){
    std::cout << "GLFW error "<<err<< " : " << msg;
}


int main(int argc, char** argv){


    glfwSetErrorCallback(GLFW_error_callback);
    if(!glfwInit()){
        std::cout << "Could not init GLFW lib \n";
        return -1;
    }


    // // GL ES 2.0 + GLSL 100
    // const char* glsl_version = "#version 100";
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif


    const char* window_title = PLC_EDITIO_WINDOW_NAME;
    GLFWwindow* window = glfwCreateWindow(500, 500, window_title, nullptr, nullptr );
    if(!window){
        std::cout << "Cound not create window object\n";
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImNodes::SetCurrentContext(nullptr); // <- prevent creating editor without its own context
    

    ImGuiIO& io = ImGui::GetIO();

    // change imgui.ini location
    
    std::filesystem::path exe = argv[0];
    std::filesystem::path imgui_ini = exe.parent_path().append("imgui.ini");
    
    int count = 0;
    while (imgui_ini.c_str()[count++]); // find end of string;

    // this might me unnecessary because memory leak in this place would leak 4kb max 
    // and only once
    std::unique_ptr<char[]> ini_file_name = std::make_unique<char[]>(count);

    for (int i = 0; i < count; i++)
        ini_file_name[i] = imgui_ini.c_str()[i];

    io.IniFilename = ini_file_name.get();
    

    // set viewport properties
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;


    ImGui::StyleColorsDark();


    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);


    App app(argc, argv);



    while(!glfwWindowShouldClose(window)){


        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        ImGui::NewFrame();

        // user app there
        app.update();


        ImGui::Render();
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0,0,0,0);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // without this code crashes, for some reason
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }


    std::cout << "Closing app\n";

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    
    // this line might be unnecessary
    if(ImNodes::GetCurrentContext()) ImNodes::DestroyContext();
    
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;


}