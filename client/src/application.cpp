#include "application.h"

#include <filesystem>

#include <magic_enum.hpp>

#include <glad/glad.h>

#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/thread_pool.hpp>

#include <GLFW/glfw3.h>

#include "logger.h"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "awesome_6.h"
#include "awesome_brand.h"
#include "widget.h"

#include "util/imgui.h"
#include <user.h>

#include <fmt/ostream.h>

#include <tinyfiledialogs.h>

namespace ar
{
  namespace gui = ImGui;

  static bool is_send_file_modal_open = false;

  Application::Application(asio::io_context& ctx, Window window) noexcept
    : is_running_{false}, context_{ctx}, gui_state_{PageState::Login},
      window_{std::move(window)},
      client_{ctx.get_executor(), asio::ip::make_address_v4("127.0.0.1"), 1231, this}
  {
    username_.reserve(255);
    password_.reserve(255);
    confirm_password_.reserve(255);

    users_.emplace_back(1, "mizhan", "");
    users_.emplace_back(2, "nahzim", "");
    users_.emplace_back(3, "nahzim", "");
    users_.emplace_back(4, "nahzim", "");
    users_.emplace_back(5, "nahzim", "");
    users_.emplace_back(6, "nahzim", "");
    users_.emplace_back(7, "nahzim", "");
    users_.emplace_back(8, "nahzim", "");
    users_.emplace_back(9, "nahzim", "");
    users_.emplace_back(10, "nahzim", "");
    users_.emplace_back(11, "nahzim", "");
    users_.emplace_back(12, "nahzim", "");
    users_.emplace_back(13, "nahzim", "");
    users_.emplace_back(14, "nahzim", "");
    users_.emplace_back(15, "nahzim", "");
    users_.emplace_back(16, "nahzim", "");
    users_.emplace_back(17, "nahzim", "");
    users_.emplace_back(18, "nahzim", "");
    users_.emplace_back(19, "nahzim", "");
    users_.emplace_back(20, "nahzim", "");
    users_.emplace_back(21, "nahzim", "");

    files_.emplace_back("file 1");
    files_.emplace_back("file 2");
    files_.emplace_back("file 3");
    files_.emplace_back("file 4");
    files_.emplace_back("file 5");
    files_.emplace_back("file 6");
    files_.emplace_back("file 7");
    files_.emplace_back("file 8");
    files_.emplace_back("file 9");
    files_.emplace_back("file 10");
    files_.emplace_back("file 11");
    files_.emplace_back("file 12");
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

    // GLFWCallback
    glfwSetWindowUserPointer(window_.handle(), this);
    glfwSetDropCallback(window_.handle(), [](GLFWwindow* window, int path_count, const char* paths[]) {
      auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
      // get last item (doesn't allow dropping multipple items)
      app->on_file_drop(paths[path_count - 1]);
    });

    gui::CreateContext();
    // gui::StyleColorsDark();
    gui::StyleColorsLight();

    // styling
    auto& style = gui::GetStyle();
    style.WindowTitleAlign = {0.5f, 0.5f};
    style.WindowRounding = 5.f;
    style.ChildRounding = 5.f;
    style.TabRounding = 5.f;
    style.FrameRounding = 5.f;

    // io
    auto& io = gui::GetIO();

    ImFontConfig config{};
    config.MergeMode = true;
    config.PixelSnapH = true;
    config.GlyphMinAdvanceX = 16.f * 2.f / 3.f;
    static constexpr ImWchar icon_ranges[]{ICON_MIN_FA, ICON_MAX_FA, 0};

    ImFontConfig main_config{};
    config.MergeMode = true;
    config.SizePixels = 12.f;

    // add_fonts(io, "../../resource/font/FiraCodeNerdFont-SemiBold.ttf", 24.f);
    io.Fonts->AddFontFromFileTTF("../../resource/font/FiraCodeNerdFont-SemiBold.ttf", 24.f, nullptr);
    io.Fonts->AddFontFromFileTTF(
      FONT_ICON_FILE_NAME_FAS("../../resource/font/"), 16.f, &config, icon_ranges);
    io.Fonts->AddFontFromFileTTF(
      "../../resource/font/fa-brands-400.ttf", 16.f, &config, icon_ranges);
    // add_fonts(io, "../../resource/font/FiraCodeNerdFont-SemiBold.ttf", 6.f, 8.f, 12.f);
    // add_fonts(io, "../../resource/font/Bahila.otf", 12.f, 14.f, 24.f, 42.f, 72.f);
    // add_fonts(io, "../../resource/font/MusticaPro.otf", 12.f, 14.f, 24.f, 42.f, 72.f);
    // add_fonts(io, "../../resource/font/Linford.ttf", 12.f, 14.f, 24.f, 42.f, 72.f);
    if (!resource_manager_.load_font("FiraCodeNerdFont-SemiBold.ttf", 6.f, 8.f, 12.f, 14.f, 16.f))
      Logger::warn("Failed on reading fonts FiraCodeNerdFont-SemiBold.ttf");

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
        Logger::error("failed to connect to remote");
      // window_.exit();
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
    }

    // Draw notification overlay
    draw_overlay();

    // gui::ShowMetricsWindow(nullptr);
  }

  void Application::draw_overlay() noexcept
  {
    if (auto state = gui_state_.toggle_overlay_state(); state != OverlayState::None)
      gui::OpenPopup(State::overlay_state_id(state).data());
    // else
    //   return;

    // TODO: The current problem when the overlay state is toggled, it will become None afterward
    // so it makes next frame unable to show the expected overlay

    // Send file modal
    send_file_modal();

    // Client Disconnect Popup
    notification_overlay(State::overlay_state_id(OverlayState::ClientDisconnected),
                         ICON_FA_CONNECTDEVELOP" Application couldn't make a connection with server"sv,
                         "Please fill all fields and try again"sv, [this] {
                           window_.exit();
                         });

    // Empty Field Notification
    notification_overlay(State::overlay_state_id(OverlayState::EmptyField),
                         ICON_FA_STOP" There's some empty field"sv,
                         "Please fill all fields and try again"sv);

    // Password Different
    notification_overlay(State::overlay_state_id(OverlayState::PasswordDifferent),
                         ICON_FA_STOP" Password and Confirm Password fields has different value"sv,
                         "Please check and try again"sv);

    // Loading
    loading_overlay(!gui_state_.is_loading());

    // Login Failed
    notification_overlay(State::overlay_state_id(OverlayState::LoginFailed),
                         "Username/Password doesn't match"sv, "Please try again");

    // Register failed
    notification_overlay(State::overlay_state_id(OverlayState::RegisterFailed),
                         "User with those username already exist"sv, "Please provide different username");
  }

  void Application::login_page() noexcept
  {
    auto viewport = gui::GetMainViewport();
    auto& style = gui::GetStyle();

    gui::SetNextWindowSize({400, 260});
    gui::SetNextWindowPos(viewport->GetWorkCenter(), ImGuiCond_Always, {0.5f, 0.5f});
    if (!gui::Begin("Login", nullptr,
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
      return;

    static std::string_view login_page = "Login"sv;
    auto size = gui::CalcTextSize(login_page.data()).x + style.FramePadding.x * 2.f;
    auto avail = gui::GetContentRegionAvail().x;

    // static auto image = resource_manager_.image("login.png").value();
    // gui::Image((ImTextureID)(intptr_t)image.id, {128.f, 128.f});

    gui::SetCursorPosY(120);
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

    gui::SetCursorPosX(gui::GetCursorPosX() + 154 + style.FramePadding.y * 2.f);
    if (gui::Button("Register", {password_field_size.x / 2.5f, 0.f}))
      register_button_handler();
    gui::SameLine();
    if (gui::Button("Login", {password_field_size.x / 2.5f, 0.f}))
      login_button_handler();

    gui::End();
  }

  void Application::register_page() noexcept
  {
    auto viewport = gui::GetMainViewport();
    auto& style = gui::GetStyle();

    gui::SetNextWindowSize({400, 315});
    gui::SetNextWindowPos(viewport->GetWorkCenter(), ImGuiCond_Always, {0.5f, 0.5f});
    if (!gui::Begin("Register", nullptr,
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse
    ))
      return;

    static std::string_view login_page = "Login"sv;
    auto size = gui::CalcTextSize(login_page.data()).x + style.FramePadding.x * 2.f;
    auto avail = gui::GetContentRegionAvail().x;

    // static auto image = resource_manager_.image("signup.png").value();
    // gui::Image((ImTextureID)(intptr_t)image.id, {128.f, 128.f});

    gui::SetCursorPosY(120);
    gui::SetCursorPosX(20);
    gui::Text("Username");
    gui::SameLine();
    ar::input_text("##username", username_);

    gui::Spacing();
    gui::Spacing();
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
    gui::SetCursorPosY(gui::GetCursorPosY() + 8.f);
    ar::input_text("##confirm_password", confirm_password_, ImGuiInputTextFlags_Password);

    gui::Spacing();
    // gui::Spacing();
    // gui::Spacing();
    // gui::Spacing();

    gui::SetCursorPosX(gui::GetCursorPosX() + 154 + style.FramePadding.y * 2.f);
    if (gui::Button("Login", {password_field_size.x / 2.5f, 0.f}))
      login_button_handler();
    gui::SameLine();
    if (gui::Button("Register", {password_field_size.x / 2.5f, 0.f}))
      register_button_handler();

    gui::End();
  }

  void Application::dashboard_page() noexcept
  {
    auto viewport = gui::GetMainViewport();
    auto& style = gui::GetStyle();

    gui::SetNextWindowPos(viewport->WorkPos);
    gui::SetNextWindowSize(viewport->WorkSize);
    gui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
    if (gui::Begin("Dashboard", nullptr,
                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                   ImGuiWindowFlags_NoTitleBar))
    {
      gui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.f);
      if (gui::BeginChild("user-online", {viewport->WorkSize.x * 0.25f, 0}, ImGuiChildFlags_Border,
                          ImGuiWindowFlags_MenuBar))
      {
        if (gui::BeginMenuBar())
        {
          center_text_unformatted("Users");
          gui::EndMenuBar();
        }

        std::ranges::for_each(users_, [this](const User& user) {
          user_widget(std::to_string(user.id), user.name, resource_manager_);
        });
      }
      gui::EndChild();

      gui::SameLine(0, 0);
      if (gui::BeginChild("file list", {
                            viewport->WorkSize.x * 0.75f - 15.f,
                            viewport->WorkSize.y - style.WindowPadding.y - 45.f
                          },
                          ImGuiChildFlags_Border))
      {
        std::ranges::for_each(files_, [this](const FileProperty& file) {
          file_widget(file, resource_manager_);
        });
      }
      gui::EndChild();
      gui::SetCursorPosY(viewport->WorkSize.y - style.WindowPadding.y - 30.f);
      static constexpr std::string_view send_file_text = "Send File"sv;
      gui::SetCursorPosX(viewport->WorkSize.x - gui::CalcTextSize(send_file_text.data()).x - 15.f);
      if (gui::Button(send_file_text.data()))
      {
        send_file_button_handler();
        is_send_file_modal_open = true;
      }

      gui::PopStyleVar();
    }

    gui::End();
    gui::PopStyleVar();
  }

  void Application::send_file_modal() noexcept
  {
    auto viewport = gui::GetMainViewport();
    auto& style = gui::GetStyle();

    ImGui::SetNextWindowSize({viewport->WorkSize.x * 0.7f, viewport->WorkSize.y * 0.5f});
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (gui::BeginPopupModal(State::overlay_state_id(OverlayState::SendFile).data(), &is_send_file_modal_open,
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
    {
      if (gui::BeginChild("user-online-select", {viewport->WorkSize.x * 0.25f, 0}, ImGuiChildFlags_Border,
                          ImGuiWindowFlags_MenuBar))
      {
        if (gui::BeginMenuBar())
        {
          center_text_unformatted("Users");
          gui::EndMenuBar();
        }

        std::ranges::for_each(users_ | std::views::enumerate, [this](const std::tuple<usize, User>& data) {
          auto n = std::get<0>(data);
          auto& user = std::get<1>(data);
          selected_user_ = selectable_user_widget(std::to_string(user.id), user.name, n, resource_manager_);
        });
      }
      gui::EndChild();

      gui::SameLine();

      if (gui::BeginChild("send_file-right_side", {0, 0}, ImGuiChildFlags_Border))
      {
        if (gui::BeginChild("send_file-right_side",
                            {0, ImGui::GetContentRegionAvail().y - 34.f},
                            ImGuiChildFlags_Border))
        {
          auto child_size = gui::GetWindowSize();
          if (!dropped_file_path_.empty())
          {
            // TODO: Show the file information

            // Image
            gui::SetCursorPosY(50.f);

            auto [format, format_str] = get_file_format(dropped_file_path_);
            ImageProperties image{};
            switch (format)
            {
            case FileFormat::None:
              image = resource_manager_.image("none.png").value();
              break;
            case FileFormat::Image:
              image = resource_manager_.image("picture.png").value();
              break;
            case FileFormat::Archive:
              image = resource_manager_.image("zip.png").value();
              break;
            case FileFormat::Document:
              image = resource_manager_.image("docs.png").value();
              break;
            case FileFormat::Other:
              image = resource_manager_.image("other.png").value();
              break;
            }
            gui::SetCursorPosX((child_size.x - 128.f) * 0.5f);
            gui::Image((ImTextureID)(intptr_t)image.id, {128.f, 128.f});

            // File Name
            const auto filename = get_filename(dropped_file_path_);
            std::string filename_copy = std::string{filename};
            auto filename_size = gui::CalcTextSize(filename_copy.data());
            gui::SetCursorPosX((child_size.x - filename_size.x) * 0.5f);
            gui::TextUnformatted(filename_copy.data());

            // Fullpath
            auto fullpath_size = gui::CalcTextSize(dropped_file_path_.data());
            gui::SetCursorPosX((child_size.x - fullpath_size.x) * 0.5f);
            gui::TextUnformatted(dropped_file_path_.data());

            // File size
            auto size = std::filesystem::file_size(dropped_file_path_);
            // MB for more than 10KB and KB otherwise
            float good_size;
            std::string_view type{};
            if (size < 10000)
            {
              good_size = static_cast<float>(size) / 1000.f;
              type = "KB";
            }
            else
            {
              good_size = static_cast<float>(size) / 1'000'000.f;
              type = "MB";
            }

            auto filesize = fmt::format("{} {}", good_size, type);
            auto filesize_size = gui::CalcTextSize(filesize.data());

            gui::SetCursorPosX((child_size.x - filesize_size.x) * 0.5f);
            gui::TextUnformatted(filesize.data());
          }
          else
          {
            constexpr static std::string_view drag_here = "Drag and Drop File Here"sv;
            constexpr static std::string_view or_text = "OR"sv;
            constexpr static std::string_view open_dialog_text = "Press Open Dialog Button"sv;

            auto drag_here_size = gui::CalcTextSize(drag_here.data());
            auto or_text_size = gui::CalcTextSize(or_text.data());
            auto open_dialog_text_size = gui::CalcTextSize(open_dialog_text.data());

            gui::SetCursorPosX((child_size.x - drag_here_size.x) * 0.5f);
            gui::SetCursorPosY((child_size.y - drag_here_size.y - or_text_size.y - open_dialog_text_size.y) * 0.5f);
            gui::TextUnformatted(drag_here.data());

            gui::Spacing();
            gui::SetCursorPosX((child_size.x - or_text_size.x) * 0.5f);
            gui::TextUnformatted(or_text.data());

            gui::Spacing();
            gui::SetCursorPosX((child_size.x - open_dialog_text_size.x) * 0.5f);
            gui::TextUnformatted(open_dialog_text.data());
          }
          gui::EndChild();
        }

        constexpr static std::string_view open_file_text = "Open File"sv;
        constexpr static usize send_text_x_size = 95.f;
        auto open_file_text_size = gui::CalcTextSize(open_file_text.data());
        gui::SetCursorPosX(
          gui::GetContentRegionAvail().x - open_file_text_size.x - send_text_x_size - style.WindowPadding.x);

        if (gui::Button(open_file_text.data()))
        {
          auto path = tinyfd_openFileDialog(nullptr, nullptr, 0, nullptr, nullptr, 0);
          Logger::trace(fmt::format("Path: {}", path));
          dropped_file_path_ = path;
        }

        gui::SameLine();
        if (gui::Button("Send", {send_text_x_size, 0.f}))
        {
          send_file();
        }
        gui::EndChild();
      }
      gui::EndPopup();
    }
  }

  void Application::register_button_handler() noexcept
  {
    // change state
    if (gui_state_.page_state() != PageState::Register)
    {
      gui_state_.active_page(PageState::Register);
      return;
    }

    // empty field check
    if (username_.empty() || password_.empty() || confirm_password_.empty())
    {
      gui_state_.active_overlay(OverlayState::EmptyField);
      return;
    }

    // check both password equality
    if (password_ != confirm_password_)
    {
      gui_state_.active_overlay(OverlayState::PasswordDifferent);
      return;
    }

    // handle register
    RegisterPayload payload{username_, password_};
    client_.connection().write(payload.serialize());

    gui_state_.active_overlay(OverlayState::Loading);
  }

  void Application::login_button_handler() noexcept
  {
    // change state
    if (gui_state_.page_state() != PageState::Login)
    {
      gui_state_.active_page(PageState::Login);
      return;
    }

    // empty field check
    if (username_.empty() || password_.empty())
    {
      gui_state_.active_overlay(OverlayState::EmptyField);
      return;
    }

    // handle login
    LoginPayload payload{.username = username_, .password = password_};
    client_.connection().write(payload.serialize());

    gui_state_.active_overlay(OverlayState::Loading);
  }

  void Application::send_file_button_handler() noexcept
  {
    dropped_file_path_.clear();
    gui_state_.active_overlay(OverlayState::SendFile);
  }

  void Application::send_file() noexcept
  {
    // TODO: Implement
  }

  void Application::on_file_drop(std::string_view paths) noexcept
  {
    if (gui_state_.overlay_state() != OverlayState::SendFile)
    {
      Logger::trace("trying to drop files when the state is not Send File yet");
      return;
    }

    dropped_file_path_ = std::string{paths};
  }

  void Application::on_feedback_response(const FeedbackPayload& payload) noexcept
  {
    std::this_thread::sleep_for(1s);
    gui_state_.active_overlay(OverlayState::None);

    switch (payload.id)
    {
    case PayloadId::Login: {
      // malformed request
      if (gui_state_.page_state() != PageState::Login)
        break;

      if (!payload.response)
      {
        gui_state_.active_overlay(OverlayState::LoginFailed);
        break;
      }

      gui_state_.active_page(PageState::Dashboard);
      break;
    }
    case PayloadId::Register: {
      // malformed request
      if (gui_state_.page_state() != PageState::Register)
        break;

      if (!payload.response)
      {
        gui_state_.active_overlay(OverlayState::RegisterFailed);
        break;
      }

      gui_state_.active_page(PageState::Login);
      break;
    }
    case PayloadId::UserCheck:
      break;
    case PayloadId::Unauthenticated:
      break;
    case PayloadId::Authenticated:
      break;
    case PayloadId::UserDetail:
      break;
    case PayloadId::UserOnlineList:
      break;
    }

    if constexpr (_DEBUG)
    {
      Logger::info(fmt::format("Feedback Repsonse: {}", payload.response,
                               magic_enum::enum_name(payload.id)));
      if (payload.message)
        Logger::trace(fmt::format("Feedback Message: {}", *payload.message));
    }
  }

  void Application::on_file_receive() noexcept {}
}
