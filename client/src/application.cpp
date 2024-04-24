#include "application.h"

#include <magic_enum.hpp>

#include <glad/glad.h>

#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/thread_pool.hpp>

#include <GLFW/glfw3.h>

#include "logger.h"

#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#include "awesome_6.h"
#include "awesome_brand.h"
#include "widget.h"

#include "util/imgui.h"

namespace ar
{
    namespace gui = ImGui;

    Application::Application(asio::io_context& ctx, Window window) noexcept
        : is_running_{false}, context_{ctx}, gui_state_{PageState::Login},
          window_{std::move(window)}, client_{ctx.get_executor(), asio::ip::make_address_v4("127.0.0.1"), 1231, this}
    {
        username_.reserve(255);
        password_.reserve(255);
    }

    Application::~Application() noexcept
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        gui::DestroyContext();

        window_.destroy();
        glfwTerminate();

        Logger::info("Application stopped!");
    }

    bool Application::init() noexcept
    {
        glfwMakeContextCurrent(window_.handle());
        glfwSwapInterval(1);
        gladLoadGL();

        gui::CreateContext();
        gui::StyleColorsDark();

        // styling
        auto& style = gui::GetStyle();
        style.WindowTitleAlign = {0.5f, 0.5f};

        // io
        auto& io = gui::GetIO();

        ImFontConfig config{};
        config.MergeMode = true;
        config.PixelSnapH = true;
        config.GlyphMinAdvanceX = 16.f * 2.f / 3.f;
        static constexpr ImWchar icon_ranges[]{ICON_MIN_FA, ICON_MAX_FA, 0};

        auto ar = "Hello";
        ImFontConfig main_config{};
        std::memcpy(main_config.Name, ar, 5);

        // add_fonts(io, "../../resource/font/FiraCodeNerdFont-SemiBold.ttf", 24.f);
        io.Fonts->AddFontFromFileTTF("../../resource/font/FiraCodeNerdFont-SemiBold.ttf", 24.f, &main_config);
        io.Fonts->AddFontFromFileTTF(
            FONT_ICON_FILE_NAME_FAS("../../resource/font/"), 16.f, &config, icon_ranges);
        io.Fonts->AddFontFromFileTTF(
            "../../resource/font/fa-brands-400.ttf", 16.f, &config, icon_ranges);
        // auto font = io.Fonts->AddFontFromFileTTF("../../resource/font/Linford.ttf", 24.f);
        // add_fonts(io, "../../resource/font/Bahila.otf", 12.f, 14.f, 24.f, 42.f, 72.f);
        // add_fonts(io, "../../resource/font/MusticaPro.otf", 12.f, 14.f, 24.f, 42.f, 72.f);
        // add_fonts(io, "../../resource/font/Linford.ttf", 12.f, 14.f, 24.f, 42.f, 72.f);

        // io.FontDefault = font;

        auto res = ImGui_ImplGlfw_InitForOpenGL(window_.handle(), true);
        if (!res)
            return false;

        res = ImGui_ImplOpenGL3_Init("#version 130");
        return res;
    }

    void Application::update() noexcept
    {
        window_.update();
        // TODO: Add message box before exiting when the server is closed
        // TODO: Check when the client is disconnected
        if (!client_.is_connected())
            gui_state_.active_overlay(OverlayState::ClientDisconnected);
    }

    void Application::start() noexcept
    {
        Logger::info("Application starting!");
        // Run the client on context's thread
        std::atomic_bool is_ready_;
        asio::co_spawn(context_, client_.start(), [&](std::exception_ptr a, bool res) {
            if (!res)
            {
                Logger::error("failed to connect to remote");
                window_.exit();
            }
            is_ready_.store(true);
        });

        while (!is_ready_.load())
            std::this_thread::sleep_for(10ms);

        window_.show();
        is_running_ = true;

        while (is_running())
        {
            update();
            draw();
            render();
        }

        Logger::trace("Application stopping...");

        window_.exit();
        if (client_.is_connected())
            client_.disconnect();
        context_.stop();
    }

    void Application::render() noexcept
    {
        gui::Render();
        auto [width, height] = window_.size();
        glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(gui::GetDrawData());
        window_.render();
    }

    bool Application::is_running() const noexcept
    {
        return !window_.is_exit() && is_running_;
    }

    void Application::exit() noexcept
    {
        is_running_ = false;
    }

    void Application::draw() noexcept
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        gui::NewFrame();

        switch (gui_state_.page_state())
        {
        case PageState::Login:
            login_page();
            break;
        case PageState::Register:
            register_page();
            break;
        case PageState::Dashboard:
            dashboard_page();
            break;
        case PageState::SendFile:
            send_file_page();
            break;
        }

        // Draw notification overlay
        draw_overlay();

        // gui::ShowDemoWindow(nullptr);
        gui::ShowMetricsWindow(nullptr);
    }

    void Application::draw_overlay() noexcept
    {
        if (auto state = gui_state_.toggle_overlay_state(); state != OverlayState::None)
            gui::OpenPopup(State::overlay_state_id(state).data());

        // Client Disconnect Popup
        ImGui::SetNextWindowPos(gui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (gui::BeginPopupModal(State::overlay_state_id(OverlayState::ClientDisconnected).data(), nullptr,
                                 ImGuiWindowFlags_AlwaysAutoResize))
        {
            gui::Text(ICON_FA_CONNECTDEVELOP"   Application couldn't make a connection with server");
            if (gui::Button("Close"))
            {
                gui::CloseCurrentPopup();
                exit();
            }
            gui::EndPopup();
        }

        // Empty Field Notification
        ImGui::SetNextWindowPos(gui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (gui::BeginPopupModal(State::overlay_state_id(OverlayState::EmptyField).data(), nullptr,
                                 ImGuiWindowFlags_AlwaysAutoResize))
        {
            gui::Text(ICON_FA_STOP"  There's some empty field");
            gui::Text("Please fill all fields and try again");
            if (gui::Button("Close"))
            {
                gui::CloseCurrentPopup();
            }
            gui::EndPopup();
        }

        // Loading Popup
        ImGui::SetNextWindowPos(gui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (gui::BeginPopupModal(State::overlay_state_id(OverlayState::Loading).data(), nullptr,
                                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration |
                                 ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove))
        {
            loading_widget("Loading", 72.f, ImVec4{0.537f, 0.341f, 0.882f, 1.f},
                           ImVec4{0.38f, 0.24f, 0.64f, 1.f}, 14, 4.0f);
            if (!gui_state_.is_loading())
                gui::CloseCurrentPopup();
            gui::EndPopup();
        }
    }

    void Application::login_page() noexcept
    {
        auto viewport = gui::GetMainViewport();
        auto& style = gui::GetStyle();

        gui::SetNextWindowSize({300, 175});
        gui::SetNextWindowPos(viewport->GetWorkCenter(), ImGuiCond_Always, {0.5f, 0.5f});
        if (!gui::Begin("Login", nullptr,
                        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
            return;

        static std::string_view login_page = "Login"sv;
        auto size = gui::CalcTextSize(login_page.data()).x + style.FramePadding.x * 2.f;
        auto avail = gui::GetContentRegionAvail().x;


        gui::SetCursorPosY(50);
        gui::SetCursorPosX(20);
        gui::Text("Username");
        gui::SameLine();
        ar::input_text("##username", username_);

        gui::Spacing();
        gui::Spacing();

        gui::SetCursorPosX(20);
        gui::Text("Password");
        gui::SameLine();
        ar::input_text("##password", password_, ImGuiInputTextFlags_Password);
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
            gui::InputText("##username", username_.data(), username_.capacity());

            gui::Spacing();
            gui::Spacing();

            gui::SetCursorPosX(20);
            gui::Text("Password");
            gui::SameLine();
            ar::input_text("##password", password_, ImGuiInputTextFlags_Password);
            auto password_field_size = gui::GetItemRectSize();

            gui::Spacing();
            gui::Spacing();

            gui::SetCursorPosX(20);
            gui::Text("Confirm\nPassword");
            gui::SameLine();
            ar::input_text("##confirm_password", password_, ImGuiInputTextFlags_Password);

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
        if (gui_state_.page_state() != PageState::Register)
        {
            gui_state_.active_page(PageState::Register);
            return;
        }

        // Handle Register
    }

    void Application::login_button_handler() noexcept
    {
        // Change state when the state is not login
        if (gui_state_.page_state() != PageState::Login)
        {
            gui_state_.active_page(PageState::Login);
            return;
        }

        // Handle login
        if (username_.empty() || password_.empty())
        {
            gui_state_.active_overlay(OverlayState::EmptyField);
            return;
        }

        LoginPayload payload{.username = username_, .password = password_};
        client_.connection().write(payload.serialize());
    }

    void Application::on_feedback_response(const FeedbackPayload& payload) noexcept
    {
        Logger::info(fmt::format("Feedback Repsonse: {}", payload.response,
                                 magic_enum::enum_name(payload.id)));
    }

    void Application::on_file_receive() noexcept
    {
    }
}
