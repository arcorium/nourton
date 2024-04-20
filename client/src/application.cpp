#include "application.h"

#include <thread>

#include <glad/glad.h>

#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/thread_pool.hpp>

#include <GLFW/glfw3.h>

#include "logger.h"

#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"


namespace ar
{
    namespace gui = ImGui;

    Application::Application(asio::io_context& ctx, Window window) noexcept
        : m_context{ctx}, m_gui_state{PageState::Login}, m_window{std::move(window)},
          m_client{ctx.get_executor(), asio::ip::make_address_v4("127.0.0.1"), 1231}
    {
        m_username.reserve(255);
        m_password.reserve(255);
    }

    Application::~Application() noexcept
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        gui::DestroyContext();

        m_window.destroy();
        glfwTerminate();
    }

    bool Application::init() noexcept
    {
        glfwMakeContextCurrent(m_window.handle());
        glfwSwapInterval(1);
        gladLoadGL();

        gui::CreateContext();
        gui::StyleColorsDark();

        auto& style = gui::GetStyle();
        style.WindowTitleAlign = {0.5f, 0.5f};

        auto res = ImGui_ImplGlfw_InitForOpenGL(m_window.handle(), true);
        if (!res)
            return false;

        res = ImGui_ImplOpenGL3_Init("#version 130");
        return res;
    }

    void Application::start() noexcept
    {
        Logger::info("Application starting!");
        // Run the client on context's thread
        asio::co_spawn(m_client.executor(), m_client.start(), [this](std::exception_ptr a, bool res) {
            if (!res)
            {
                Logger::critical("failed to connect to remote");
                m_window.exit();
            }
            m_is_ready.store(true);
        });

        while (!m_is_ready.load())
            std::this_thread::sleep_for(10ms);

        bool show_demo_window = true;
        // TODO: Add message box before exiting when the server is closed
        while (!m_window.is_exit() && m_client.is_connected())
        {
            m_window.update();
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            gui::NewFrame();

            draw();

            gui::Render();
            auto [width, height] = m_window.size();
            glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(gui::GetDrawData());
            m_window.render();
        }

        m_window.exit();
        m_client.disconnect();
        m_context.stop();
    }

    void Application::draw() noexcept
    {
        // switch (m_gui_state.state())
        // {
        // case PageState::Login:
        //     login_page();
        //     break;
        // case PageState::Register:
        //     register_page();
        //     break;
        // case PageState::Dashboard:
        //     dashboard_page();
        //     break;
        // case PageState::SendFile:
        //     send_file_page();
        //     break;
        // }


        // Draw notification modal
        if (gui::Button("Hello"))
        {
            gui::OpenPopup("Notification");
        }

        ImGui::SetNextWindowPos(gui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (gui::BeginPopupModal("Notification", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            gui::Text("Hello");
            if (gui::Button("Ok"))
            {
                gui::CloseCurrentPopup();
            }
            gui::EndPopup();
        }

        // Draw notification overlay
    }

    void Application::login_page() noexcept
    {
        auto viewport = gui::GetMainViewport();
        auto& style = gui::GetStyle();

        gui::SetNextWindowSize({300, 175});
        gui::SetNextWindowPos(viewport->GetWorkCenter(), ImGuiCond_Always, {0.5f, 0.5f});
        if (gui::Begin("Login", nullptr,
                       ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse
        ))
        {
            static std::string_view login_page = "Login"sv;
            auto size = gui::CalcTextSize(login_page.data()).x + style.FramePadding.x * 2.f;
            auto avail = gui::GetContentRegionAvail().x;


            gui::SetCursorPosY(50);
            gui::SetCursorPosX(20);
            gui::Text("Username");
            gui::SameLine();
            gui::InputText("##username", m_username.data(), m_username.capacity());

            gui::Spacing();
            gui::Spacing();

            gui::SetCursorPosX(20);
            gui::Text("Password");
            gui::SameLine();
            gui::InputText("##password", m_password.data(), m_password.capacity(), ImGuiInputTextFlags_Password);
            auto password_field_size = gui::GetItemRectSize();

            gui::Spacing();
            gui::Spacing();
            gui::Spacing();
            gui::Spacing();

            gui::SetCursorPosX(gui::GetCursorPosX() + 100 + style.FramePadding.y * 2.f);
            if (gui::Button("Register", {password_field_size.x / 2.5f, 0.f}))
            {
                register_button_handler();
            }
            gui::SameLine();
            if (gui::Button("Login", {password_field_size.x / 2.5f, 0.f}))
            {
                login_button_handler();
            }

            gui::End();
        }
    }

    void Application::register_page() noexcept
    {
        auto viewport = gui::GetMainViewport();
        auto& style = gui::GetStyle();

        gui::SetNextWindowSize({300, 225});
        gui::SetNextWindowPos(viewport->GetWorkCenter(), ImGuiCond_Always, {0.5f, 0.5f});
        if (gui::Begin("Register", nullptr,
                       ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse
        ))
        {
            static std::string_view login_page = "Login"sv;
            auto size = gui::CalcTextSize(login_page.data()).x + style.FramePadding.x * 2.f;
            auto avail = gui::GetContentRegionAvail().x;


            gui::SetCursorPosY(50);
            gui::SetCursorPosX(20);
            gui::Text("Username");
            gui::SameLine();
            gui::InputText("##username", m_username.data(), m_username.capacity());

            gui::Spacing();
            gui::Spacing();

            gui::SetCursorPosX(20);
            gui::Text("Password");
            gui::SameLine();
            gui::InputText("##password", m_password.data(), m_password.capacity(), ImGuiInputTextFlags_Password);
            auto password_field_size = gui::GetItemRectSize();

            gui::Spacing();
            gui::Spacing();

            gui::SetCursorPosX(20);
            gui::Text("Confirm\nPassword");
            gui::SameLine();
            gui::InputText("##confirm_password", m_password.data(), m_password.capacity(),
                           ImGuiInputTextFlags_Password);

            gui::Spacing();
            gui::Spacing();
            gui::Spacing();
            gui::Spacing();

            gui::SetCursorPosX(gui::GetCursorPosX() + 100 + style.FramePadding.y * 2.f);
            if (gui::Button("Login", {password_field_size.x / 2.5f, 0.f}))
            {
                login_button_handler();
            }
            gui::SameLine();
            if (gui::Button("Register", {password_field_size.x / 2.5f, 0.f}))
            {
                register_button_handler();
            }

            gui::End();
        }
    }

    void Application::dashboard_page() noexcept
    {
        auto viewport = gui::GetMainViewport();
        gui::SetNextWindowPos(viewport->WorkPos);
        gui::SetNextWindowSize(viewport->WorkSize);
        if (gui::Begin("Dashboard", nullptr))
        {
            gui::End();
        }
    }

    void Application::send_file_page() noexcept
    {
    }

    void Application::register_button_handler() noexcept
    {
        if (m_gui_state.state() != PageState::Register)
        {
            m_gui_state.active(PageState::Register);
            return;
        }

        // Handle Register
    }

    void Application::login_button_handler() noexcept
    {
        // Change state when the state is not login
        if (m_gui_state.state() != PageState::Login)
        {
            m_gui_state.active(PageState::Login);
            return;
        }

        // Handle login
        if (m_username.empty() || m_password.empty())
        {
            gui::OpenPopup("Notification");
        }
        // LoginPayload payload{.username = m_username, .password = m_password};
        // m_client.connection().write(payload.serialize());
    }
}
