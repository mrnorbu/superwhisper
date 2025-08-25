#include "gui_manager.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>

namespace SuperWhisper {

class ImGuiManager : public GuiManager {
public:
    ImGuiManager() : window_(nullptr), should_close_(false) {}
    
    ~ImGuiManager() override {
        shutdown();
    }
    
    bool initialize() override {
        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }
        
        // Configure GLFW for Apple Silicon optimization
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);  // Disable retina scaling for performance
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);  // Borderless window
        glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
        
        // Create window with optimal size for Apple Silicon
        window_ = glfwCreateWindow(180, 120, "SuperWhisper", nullptr, nullptr);
        if (!window_) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }
        
        // Set window position
        glfwSetWindowPos(window_, 1200, 120);
        
        // Make window draggable
        glfwSetWindowUserPointer(window_, this);
        glfwSetMouseButtonCallback(window_, &ImGuiManager::mouse_button_callback);
        glfwSetCursorPosCallback(window_, &ImGuiManager::cursor_pos_callback);
        
        // Initialize OpenGL context
        glfwMakeContextCurrent(window_);
        glfwSwapInterval(0);  // Disable vsync for better performance
        
        // Initialize Dear ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        
        // Configure ImGui for minimal memory usage
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        
        // Set up ImGui style for Apple-like appearance
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        
        // Compact, modern style
        style.WindowPadding = ImVec2(8, 8);
        style.FramePadding = ImVec2(4, 2);
        style.ItemSpacing = ImVec2(4, 4);
        style.ScrollbarSize = 8;
        style.GrabMinSize = 8;
        style.WindowRounding = 6;
        style.FrameRounding = 4;
        style.PopupRounding = 4;
        style.ScrollbarRounding = 4;
        style.GrabRounding = 4;
        style.TabRounding = 4;
        
        // Apple-like colors
        ImVec4* colors = style.Colors;
        colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.11f, 0.12f, 0.94f);
        colors[ImGuiCol_Border] = ImVec4(0.18f, 0.18f, 0.19f, 0.29f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.19f, 0.54f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.23f, 0.54f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.26f, 0.27f, 0.67f);
        colors[ImGuiCol_Button] = ImVec4(0.18f, 0.18f, 0.19f, 0.54f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.22f, 0.22f, 0.23f, 0.54f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.26f, 0.26f, 0.27f, 0.67f);
        
        // Initialize platform/renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window_, true);
        ImGui_ImplOpenGL3_Init("#version 330");
        
        return true;
    }
    
    void shutdown() override {
        if (window_) {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
            
            glfwDestroyWindow(window_);
            window_ = nullptr;
        }
        glfwTerminate();
    }
    
    void render() override {
        if (!window_) return;
        
        glfwPollEvents();
        
        // Start new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Configure window flags
        ImGuiWindowFlags window_flags = 
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse;
        
        // Set window position and size
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(180, 120));
        
        // Begin window
        ImGui::Begin("SuperWhisper", nullptr, window_flags);
        
        // Render circular button
        render_circular_button();
        
        // Render status text
        ImGui::SameLine();
        ImGui::SetCursorPos(ImVec2(76, 8));
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", status_.c_str());
        
        // Render hint text
        ImGui::SetCursorPos(ImVec2(8, 80));
        ImGui::TextColored(ImVec4(0.56f, 0.56f, 0.58f, 1.0f), "%s", hint_.c_str());
        
        ImGui::End();
        
        // Render
        int display_w, display_h;
        glfwGetFramebufferSize(window_, &display_w, &display_h);
        
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.11f, 0.11f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window_);
        
        // Check if window should close
        should_close_ = glfwWindowShouldClose(window_);
    }
    
    bool should_close() const override {
        return should_close_;
    }
    
    void set_state(AppState state) override {
        state_ = state;
        update_button_state();
    }
    
    void set_status(const std::string& status) override {
        status_ = status;
    }
    
    void set_hint(const std::string& hint) override {
        hint_ = hint;
    }
    
    void set_position(int x, int y) override {
        if (window_) {
            glfwSetWindowPos(window_, x, y);
        }
    }
    
    void get_position(int& x, int& y) const override {
        if (window_) {
            glfwGetWindowPos(window_, &x, &y);
        }
    }
    
    void set_always_on_top(bool enabled) override {
        if (window_) {
            glfwSetWindowAttrib(window_, GLFW_FLOATING, enabled ? GLFW_TRUE : GLFW_FALSE);
        }
    }
    
    // Set button click callback
    void set_button_callback(std::function<void()> callback) override {
        button_callback_ = callback;
    }
    
private:
    void render_circular_button() {
        ImGui::SetCursorPos(ImVec2(16, 12));
        
        // Button size
        const float button_size = 56.0f;
        const ImVec2 button_pos = ImGui::GetCursorScreenPos();
        const ImVec2 button_center = ImVec2(button_pos.x + button_size * 0.5f, button_pos.y + button_size * 0.5f);
        
        // Draw button background (shadow)
        ImGui::GetWindowDrawList()->AddCircleFilled(
            ImVec2(button_center.x + 1, button_center.y + 1),
            button_size * 0.5f,
            IM_COL32(44, 44, 46, 255)
        );
        
        // Draw button
        ImGui::GetWindowDrawList()->AddCircleFilled(
            button_center,
            button_size * 0.5f,
            button_color_
        );
        
        // Draw icon
        ImGui::GetWindowDrawList()->AddText(
            ImVec2(button_center.x - 4, button_center.y - 8),
            IM_COL32(255, 255, 255, 255),
            button_icon_.c_str()
        );
        
        // Handle button click
        if (ImGui::IsMouseHoveringRect(
            ImVec2(button_pos.x, button_pos.y),
            ImVec2(button_pos.x + button_size, button_pos.y + button_size)
        )) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                if (button_callback_) {
                    button_callback_();
                }
            }
        }
    }
    
    void update_button_state() {
        switch (state_) {
            case AppState::Ready:
                button_color_ = IM_COL32(48, 209, 88, 255);  // Green
                button_icon_ = "•";
                break;
            case AppState::Recording:
                button_color_ = IM_COL32(255, 69, 58, 255);  // Red
                button_icon_ = "■";
                break;
            case AppState::Transcribing:
                button_color_ = IM_COL32(255, 214, 10, 255); // Yellow
                button_icon_ = "…";
                break;
            case AppState::Error:
                button_color_ = IM_COL32(255, 107, 107, 255); // Error red
                button_icon_ = "!";
                break;
        }
    }
    
    // GLFW callbacks for window dragging
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
        auto* manager = static_cast<ImGuiManager*>(glfwGetWindowUserPointer(window));
        if (manager) {
            manager->on_mouse_button(button, action, mods);
        }
    }
    
    static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
        auto* manager = static_cast<ImGuiManager*>(glfwGetWindowUserPointer(window));
        if (manager) {
            manager->on_cursor_pos(xpos, ypos);
        }
    }
    
    void on_mouse_button(int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            dragging_ = true;
            drag_start_x_ = mouse_x_;
            drag_start_y_ = mouse_y_;
        } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
            dragging_ = false;
        }
    }
    
    void on_cursor_pos(double xpos, double ypos) {
        mouse_x_ = static_cast<int>(xpos);
        mouse_y_ = static_cast<int>(ypos);
        
        if (dragging_) {
            int window_x, window_y;
            glfwGetWindowPos(window_, &window_x, &window_y);
            
            int delta_x = mouse_x_ - drag_start_x_;
            int delta_y = mouse_y_ - drag_start_y_;
            
            glfwSetWindowPos(window_, window_x + delta_x, window_y + delta_y);
            
            drag_start_x_ = mouse_x_;
            drag_start_y_ = mouse_y_;
        }
    }
    
    GLFWwindow* window_;
    bool should_close_;
    AppState state_{AppState::Ready};
    std::string status_{"Ready"};
    std::string hint_{"Press F9 anywhere\nor click to record"};
    
    // Button state
    ImU32 button_color_{IM_COL32(48, 209, 88, 255)};
    std::string button_icon_{"•"};
    
    // Callbacks
    std::function<void()> button_callback_;
    
    // Window dragging
    bool dragging_{false};
    int mouse_x_{0}, mouse_y_{0};
    int drag_start_x_{0}, drag_start_y_{0};
};

// Factory function
std::unique_ptr<GuiManager> create_gui_manager() {
    return std::make_unique<ImGuiManager>();
}

} // namespace SuperWhisper
