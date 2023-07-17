#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include <stdio.h>
#include "../habr/client_server/client.hpp"

#ifndef BUFFER_SIZE
#error size of buffer not defined!!!
#endif

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}


void Chat(bool *p_open, boost::shared_ptr<talk_to_svr> &session)
{
    static bool no_titlebar = true;
    static bool no_move = true;
    static bool no_collapse = true;
    static bool no_close = true;

    static bool no_scrollbar = false;
    static bool no_menu = true;
    static bool no_resize = true;
    static bool no_nav = false;
    static bool no_background = false;
    static bool no_bring_to_front = false;
    static bool unsaved_document = false;

    ImGuiWindowFlags window_flags = 0;
    if (no_titlebar)        window_flags |= ImGuiWindowFlags_NoTitleBar;
    if (no_scrollbar)       window_flags |= ImGuiWindowFlags_NoScrollbar;
    if (!no_menu)           window_flags |= ImGuiWindowFlags_MenuBar;
    if (no_move)            window_flags |= ImGuiWindowFlags_NoMove;
    if (no_resize)          window_flags |= ImGuiWindowFlags_NoResize;
    if (no_collapse)        window_flags |= ImGuiWindowFlags_NoCollapse;
    if (no_nav)             window_flags |= ImGuiWindowFlags_NoNav;
    if (no_background)      window_flags |= ImGuiWindowFlags_NoBackground;
    if (no_bring_to_front)  window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (unsaved_document)   window_flags |= ImGuiWindowFlags_UnsavedDocument;
    if (no_close)           p_open = NULL; // Don't pass our bool* to Begin

    // We specify a default position/size in case there's no data in the .ini file.
    // We only do it to make the demo applications a little more welcoming, but typically this isn't required.
    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 0, main_viewport->WorkPos.y + 0), ImGuiCond_FirstUseEver);//650, 20
    // ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
    // ImGui::SetNextWindowSize(ImVec2(main_viewport->WorkSize.x, main_viewport->WorkSize.y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(main_viewport->WorkSize.x, main_viewport->WorkSize.y), ImGuiCond_Always);



    if (!ImGui::Begin("MyMessanger", p_open, window_flags))
    {
        // Early out if the window is collapsed, as an optimization.
        ImGui::End();
        return;
    }
    float scale_val = 1.5;
    ImGui::SetWindowFontScale(scale_val);

    // static char outputtext[BUFFER_SIZE] = "";
    static ImGuiInputTextFlags flags_output = ImGuiInputTextFlags_ReadOnly;
    ImGui::InputTextMultiline("##output_message", session->get_read_buffer(), BUFFER_SIZE, ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 25), flags_output);

    // static char inputtext[BUFFER_SIZE] = "please, type your message\n";
    static ImGuiInputTextFlags flags_input = ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CtrlEnterForNewLine;
    session->get_mutex().lock();
    ImGui::InputTextMultiline("##input_message", session->get_write_buffer(), BUFFER_SIZE, ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 2), flags_input);
    // session->get_mutex().unlock();
    if (ImGui::Button("send_message")) {
        session->get_mutex().unlock();
    }

    ImGui::End();
}

int graphical_part(boost::shared_ptr<talk_to_svr> &client)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow *window = glfwCreateWindow(1280, 720, "My Messenger", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        bool is_opened = true;
        Chat(&is_opened, client);

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
    return 0;
}
void start_network()
{
    boost::this_thread::interruption_enabled();
    service.run();
}

int main(int argc, char const *argv[])
{
    //NETWORK SECTION
    ip::tcp::endpoint ep(ip::address::from_string("127.0.0.1"), 8001);//server_address
    const std::string name = "Artem";
    boost::shared_ptr<talk_to_svr> client = talk_to_svr::start(ep, name);

    auto thread = boost::thread(start_network);
    graphical_part(client);
    thread.interrupt();
    thread.join();

    return 0;
}