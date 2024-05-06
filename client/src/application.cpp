#include "application.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fmt/ostream.h>
#include <tinyfiledialogs.h>
#include <user.h>

#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/thread_pool.hpp>
#include <magic_enum.hpp>

#include "awesome_6.h"
#include "awesome_brand.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "core.h"
#include "crypto/camellia.h"
#include "crypto/dm_rsa.h"
#include "logger.h"
#include "util/algorithm.h"
#include "util/convert.h"
#include "util/file.h"
#include "util/imgui.h"
#include "util/time.h"
#include "util/tinyfd.h"
#include "widget.h"

namespace ar
{
  namespace gui = ImGui;

  static bool is_send_file_modal_open = false;

  Application::Application(asio::io_context& ctx, Window window, std::string_view ip, u16 port,
                           std::string_view save_dir) noexcept
      : is_running_{false},
        context_{ctx},
        state_{PageState::Login},
        window_{std::move(window)},
        save_dir_{save_dir},
        selected_user_{-1},
        client_{ctx.get_executor(), asio::ip::make_address_v4(ip), port, this}
  {
    users_.reserve(1024);
    username_.reserve(255);
    password_.reserve(255);
    confirm_password_.reserve(255);

    // create directory for saved files
    std::error_code ec;
    std::filesystem::create_directory(save_dir_, ec);
  }

  Application::~Application() noexcept
  {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    gui::DestroyContext();

    // Clear
    for (usize i = 0; i < send_file_datas_.size(); ++i)
      send_file_datas_.pop();

    send_file_cv_.notify_all();
    if (send_file_thread_.joinable())
      send_file_thread_.join();

    // send file thread needs the query window, so it should be destroyed after
    // the thread
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
    glfwSetDropCallback(window_.handle(),
                        [](GLFWwindow* window, int path_count, const char* paths[]) {
                          auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
                          // get last item (doesn't allow dropping multipple items)
                          app->on_file_drop(paths[path_count - 1]);
                        });

    gui::CreateContext();
    // gui::StyleColorsDark();
    gui::StyleColorsLight();

    // styling
    auto& style = gui::GetStyle();
    styling(style);
    // Override
    style.WindowTitleAlign = {0.5f, 0.5f};
    style.WindowRounding = 5.f;
    style.ChildRounding = 5.f;
    style.TabRounding = 5.f;
    style.FrameRounding = 5.f;

    ImFontConfig config{};
    config.MergeMode = true;
    config.PixelSnapH = true;
    config.GlyphMinAdvanceX = 16.f * 2.f / 3.f;
    static constexpr ImWchar icon_ranges[]{ICON_MIN_FA, ICON_MAX_FA, 0};

    resource_manager_.load_font("FiraCodeNerdFont-SemiBold.ttf", 24.f);
    resource_manager_.load_font(ICON_FONT_FAS, &config, icon_ranges, 16.f);
    resource_manager_.load_font(ICON_FONT_FAR, &config, icon_ranges, 16.f);
    resource_manager_.load_font(ICON_FONT_FAB, &config, icon_ranges, 16.f);
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
    {
      state_.disable_loading_overlay();
      state_.active_overlay(OverlayState::ClientDisconnected);
    }

    // delete files
    {
      std::unique_lock lock{file_mutex_};
      for (const auto filename : deleted_file_names_)
        std::erase_if(files_, [&](const FileProperty& prop) { return prop.fullpath == filename; });
      deleted_file_names_.clear();
    }
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

  void Application::styling(ImGuiStyle& style) noexcept
  {
    auto& colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4{0.1f, 0.1f, 0.13f, 1.0f};
    colors[ImGuiCol_MenuBarBg] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};

    // Border
    colors[ImGuiCol_Border] = ImVec4{0.44f, 0.37f, 0.61f, 0.29f};
    colors[ImGuiCol_BorderShadow] = ImVec4{0.0f, 0.0f, 0.0f, 0.24f};

    // Text
    colors[ImGuiCol_Text] = ImVec4{1.0f, 1.0f, 1.0f, 1.0f};
    colors[ImGuiCol_TextDisabled] = ImVec4{0.5f, 0.5f, 0.5f, 1.0f};

    // Headers
    colors[ImGuiCol_Header] = ImVec4{0.13f, 0.13f, 0.17, 1.0f};
    colors[ImGuiCol_HeaderHovered] = ImVec4{0.19f, 0.2f, 0.25f, 1.0f};
    colors[ImGuiCol_HeaderActive] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};

    // Buttons
    colors[ImGuiCol_Button] = ImVec4{0.13f, 0.13f, 0.17, 1.0f};
    colors[ImGuiCol_ButtonHovered] = ImVec4{0.19f, 0.2f, 0.25f, 1.0f};
    colors[ImGuiCol_ButtonActive] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
    colors[ImGuiCol_CheckMark] = ImVec4{0.74f, 0.58f, 0.98f, 1.0f};

    // Popups
    colors[ImGuiCol_PopupBg] = ImVec4{0.1f, 0.1f, 0.13f, 0.92f};

    // Slider
    colors[ImGuiCol_SliderGrab] = ImVec4{0.44f, 0.37f, 0.61f, 0.54f};
    colors[ImGuiCol_SliderGrabActive] = ImVec4{0.74f, 0.58f, 0.98f, 0.54f};

    // Frame BG
    colors[ImGuiCol_FrameBg] = ImVec4{0.13f, 0.13, 0.17, 1.0f};
    colors[ImGuiCol_FrameBgHovered] = ImVec4{0.19f, 0.2f, 0.25f, 1.0f};
    colors[ImGuiCol_FrameBgActive] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};

    // Tabs
    colors[ImGuiCol_Tab] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
    colors[ImGuiCol_TabHovered] = ImVec4{0.24, 0.24f, 0.32f, 1.0f};
    colors[ImGuiCol_TabActive] = ImVec4{0.2f, 0.22f, 0.27f, 1.0f};
    colors[ImGuiCol_TabUnfocused] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};

    // Title
    colors[ImGuiCol_TitleBg] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
    colors[ImGuiCol_TitleBgActive] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg] = ImVec4{0.1f, 0.1f, 0.13f, 1.0f};
    colors[ImGuiCol_ScrollbarGrab] = ImVec4{0.16f, 0.16f, 0.21f, 1.0f};
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4{0.19f, 0.2f, 0.25f, 1.0f};
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4{0.24f, 0.24f, 0.32f, 1.0f};

    // Seperator
    colors[ImGuiCol_Separator] = ImVec4{0.44f, 0.37f, 0.61f, 1.0f};
    colors[ImGuiCol_SeparatorHovered] = ImVec4{0.74f, 0.58f, 0.98f, 1.0f};
    colors[ImGuiCol_SeparatorActive] = ImVec4{0.84f, 0.58f, 1.0f, 1.0f};

    // Resize Grip
    colors[ImGuiCol_ResizeGrip] = ImVec4{0.44f, 0.37f, 0.61f, 0.29f};
    colors[ImGuiCol_ResizeGripHovered] = ImVec4{0.74f, 0.58f, 0.98f, 0.29f};
    colors[ImGuiCol_ResizeGripActive] = ImVec4{0.84f, 0.58f, 1.0f, 0.29f};

    // Docking
    style.TabRounding = 4;
    style.ScrollbarRounding = 9;
    style.WindowRounding = 7;
    style.GrabRounding = 3;
    style.FrameRounding = 3;
    style.PopupRounding = 4;
    style.ChildRounding = 4;
  }

  void Application::send_file_thread() noexcept
  {
    while (is_running())
    {
      // when the work is empty, then wait or continue until there's no work
      // left
      if (send_file_datas_.empty())
      {
        // wait until there is a work
        {
          std::unique_lock lock{send_file_data_mutex_};
          send_file_cv_.wait(lock);

          // exit thread and handle spurious wake-up
          if (send_file_datas_.empty())
            continue;
        }
      }

      auto data = std::move(send_file_datas_.front());
      send_file_datas_.pop();

      std::string_view filepath = data.file_fullpath;
      std::string_view filename = get_filename_with_format(filepath);

      UserClient* user{};
      {
        std::unique_lock lock{user_mutex_};

        auto it = std::ranges::find_if(users_,
                                       [&](const UserClient& uc) { return uc.id == data.user_id; });
        if (it == users_.end())
        {
          Logger::error("trying to send file to user that doesn't exists");
          state_.disable_loading_overlay();
          state_.active_overlay(OverlayState::SendFileFailed);
          continue;
        }

        user = &*it;
      }

      // FIX: not needed, because the condition variable will only signaled when
      // the user detail is ready
      if (!user->public_key.is_valid())
      {
        // wait until user detail received
        if (state_.expected_operation_state() != OperationState::GetUserDetails)
        {
          Logger::error(
              "trying to send file to user with no public key and not "
              "getting the public key");
          state_.disable_loading_overlay();
          state_.active_overlay(OverlayState::InternalError);
          continue;
        }
        while (state_.operation_state() != OperationState::GetUserDetails)
          std::this_thread::sleep_for(10ms);
      }

      state_.disable_loading_overlay();
      state_.expect_operation_state(OperationState::SendFile);

      // Read file
      auto result = read_file_as_bytes(filepath);
      if (!result.has_value())
      {
        Logger::error(result.error());
        state_.active_overlay(OverlayState::FileNotExist);
        continue;
      }

      // Encrypt file
      auto symmetric_key = random_bytes<16>();
      Camellia symmetric{symmetric_key};
      auto [file_filler, enc_files] = symmetric.encrypts(result.value());

      // Encrypt symmetric key
      DMRSA rsa{user->public_key};
      auto [key_filler, enc_key] = rsa.encrypts(symmetric_key);

      // PERF: asymmetric returning vector<u8> instead of vector<u64> so it can
      // be moved instead of copying
      auto enc_key_bytes = ar::as_byte_span<usize>(enc_key);

      auto [format, format_str] = get_file_format(filename);
      // Send payload
      SendFilePayload payload{
          .file_filler = static_cast<u8>(file_filler),
          .key_filler = static_cast<u8>(key_filler),
          .file_size = result->size(),
          .filename = std::string{filename},
          .timestamp = get_current_time(),
          .symmetric_key = std::vector<u8>{enc_key_bytes.begin(), enc_key_bytes.end()},
          .files = std::move(enc_files)
      };
      client_.connection().write(payload.serialize(user->id));

      {
        std::unique_lock lock{file_mutex_};
        files_.emplace_back(format, result->size(), this_user_.get(), std::move(data.file_fullpath),
                            std::move(payload.timestamp));
      }
    }
  }

  void Application::draw() noexcept
  {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    gui::NewFrame();

    switch (state_.page_state())
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
    if (auto state = state_.toggle_overlay_state(); state != OverlayState::None)
      gui::OpenPopup(State::overlay_state_id(state).data());
    // else
    //   return;
    if (state_.is_loading())
      gui::OpenPopup(State::loading_overlay_id().data());

    // TODO: The current problem when the overlay state is toggled, it will
    // become None afterward so it makes next frame unable to show the expected
    // overlay

    // Send file modal
    send_file_modal();

    // Client Disconnect Popup
    notification_overlay(State::overlay_state_id(OverlayState::ClientDisconnected),
                         ICON_FA_CONNECTDEVELOP
                         " Application couldn't make a connection with server"sv,
                         "Please fill all fields and try again"sv, [this] { window_.exit(); });

    // Empty Field Notification
    notification_overlay(State::overlay_state_id(OverlayState::EmptyField),
                         ICON_FA_STOP " There's some empty field"sv,
                         "Please fill all fields and try again"sv);

    // Internal Error
    notification_overlay(State::overlay_state_id(OverlayState::InternalError),
                         ICON_FA_STOP " Some error happens"sv,
                         "Please close and open the application again"sv);

    notification_overlay(State::overlay_state_id(OverlayState::SendFileFailed),
                         ICON_FA_STOP " failed to send error on server"sv,
                         "Please send the file again"sv);

    // Password Different
    notification_overlay(State::overlay_state_id(OverlayState::PasswordDifferent),
                         ICON_FA_STOP " Password and Confirm Password fields has different value"sv,
                         "Please check and try again"sv);

    // Login Failed
    notification_overlay(State::overlay_state_id(OverlayState::LoginFailed),
                         "Username/Password doesn't match"sv, "Please try again");

    // Register failed
    notification_overlay(State::overlay_state_id(OverlayState::RegisterFailed),
                         "User with those username already exist"sv,
                         "Please provide different username");

    // File Doesn't  Exists
    notification_overlay(State::overlay_state_id(OverlayState::FileNotExist),
                         "File provided is not exists or broken"sv,
                         "Please select another file and try again");

    // Loading
    loading_overlay(!state_.is_loading());
  }

  void Application::login_page() noexcept
  {
    auto viewport = gui::GetMainViewport();
    auto& style = gui::GetStyle();

    gui::SetNextWindowSize({400, 260});
    gui::SetNextWindowPos(viewport->GetWorkCenter(), ImGuiCond_Always, {0.5f, 0.5f});
    if (!gui::Begin(
            "Login", nullptr,
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
    if (!gui::Begin(
            "Register", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
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
                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse
                       | ImGuiWindowFlags_NoTitleBar))
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

        std::ranges::for_each(
            users_, [this](const UserClient& user) { user_widget(user, resource_manager_); });
      }
      gui::EndChild();

      gui::SameLine(0, 0);
      if (gui::BeginChild("file list",
                          {viewport->WorkSize.x * 0.75f - 15.f,
                           viewport->WorkSize.y - style.WindowPadding.y - 45.f},
                          ImGuiChildFlags_Border))
      {
        std::ranges::for_each(
            files_ | std::views::enumerate,
            [this](const std::tuple<usize, const FileProperty&>& file) {
              const auto& fl = std::get<1>(file);
              file_widget(
                  fl, resource_manager_,
                  std::bind(&Application::delete_file_on_dashboard, this, std::get<0>(file)),
                  std::bind(&Application::open_file_on_dashboard, this, std::get<0>(file)));
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
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetWorkCenter(), ImGuiCond_Always,
                            ImVec2(0.5f, 0.5f));
    if (gui::BeginPopupModal(State::overlay_state_id(OverlayState::SendFile).data(),
                             &is_send_file_modal_open,
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
    {
      if (state_.overlay_state() != OverlayState::SendFile)
      {
        gui::CloseCurrentPopup();
      }

      if (gui::BeginChild("user-online-select", {viewport->WorkSize.x * 0.25f, 0},
                          ImGuiChildFlags_Border, ImGuiWindowFlags_MenuBar))
      {
        if (gui::BeginMenuBar())
        {
          center_text_unformatted("Users");
          gui::EndMenuBar();
        }

        // filter out offline user
        auto filtered_users
            = users_ | std::views::filter([](const UserClient& user) { return user.is_online; })
              | std::views::enumerate;
        std::ranges::for_each(filtered_users, [this](const std::tuple<usize, UserClient>& data) {
          auto n = std::get<0>(data);
          auto& user = std::get<1>(data);
          selected_user_
              = selectable_user_widget(std::to_string(user.id), user.name, n, resource_manager_);
        });
      }
      gui::EndChild();

      gui::SameLine();

      if (gui::BeginChild("send_file-right_side", {0, 0}, ImGuiChildFlags_Border))
      {
        if (gui::BeginChild("send_file-right_side", {0, ImGui::GetContentRegionAvail().y - 34.f},
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
            usize size = 0;
            size = std::filesystem::file_size(dropped_file_path_);
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
            gui::SetCursorPosY(
                (child_size.y - drag_here_size.y - or_text_size.y - open_dialog_text_size.y)
                * 0.5f);
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
        gui::SetCursorPosX(gui::GetContentRegionAvail().x - open_file_text_size.x - send_text_x_size
                           - style.WindowPadding.x);

        if (gui::Button(open_file_text.data()))
        {
          const auto path = tinyfd_openFileDialog(nullptr, nullptr, 0, nullptr, nullptr, 0);
          if (path)
          {
            Logger::trace(fmt::format("Path: {}", path));
            dropped_file_path_ = path;
          }
        }

        gui::SameLine();
        // prevent when no user selected
        gui::BeginDisabled(selected_user_ < 0 || dropped_file_path_.empty());
        if (gui::Button("Send", {send_text_x_size, 0.f}))
        {
          // allow only single file encryption at a moment
          send_file_handler();
        }
        gui::EndDisabled();
        gui::EndChild();
      }
      gui::EndPopup();
    }
  }

  void Application::register_button_handler() noexcept
  {
    // change state
    if (state_.page_state() != PageState::Register)
    {
      state_.active_page(PageState::Register);
      return;
    }

    // empty field check
    if (username_.empty() || password_.empty() || confirm_password_.empty())
    {
      state_.active_overlay(OverlayState::EmptyField);
      return;
    }

    // check both password equality
    if (password_ != confirm_password_)
    {
      state_.active_overlay(OverlayState::PasswordDifferent);
      return;
    }

    // handle register
    RegisterPayload payload{username_, password_};
    client_.connection().write(payload.serialize());

    state_.active_loading_overlay();
    state_.expect_operation_state(OperationState::Register);
  }

  void Application::login_button_handler() noexcept
  {
    // change state
    if (state_.page_state() != PageState::Login)
    {
      state_.active_page(PageState::Login);
      return;
    }

    // empty field check
    if (username_.empty() || password_.empty())
    {
      state_.active_overlay(OverlayState::EmptyField);
      return;
    }

    // handle login
    LoginPayload payload{.username = username_, .password = password_};
    client_.connection().write(payload.serialize());

    state_.active_loading_overlay();
    state_.expect_operation_state(OperationState::Login);
  }

  void Application::send_file_button_handler() noexcept
  {
    dropped_file_path_.clear();
    state_.active_overlay(OverlayState::SendFile);
  }

  void Application::send_file_handler() noexcept
  {
    // Escape from send file modal
    state_.active_overlay(OverlayState::None);

    auto& user = users_[selected_user_];
    // Get the public key
    {
      std::unique_lock lock{send_file_data_mutex_};
      send_file_datas_.emplace(user.id, dropped_file_path_);
    }

    if (!user.public_key.is_valid())
    {
      GetUserDetailsPayload user_details_payload{.id = user.id};
      client_.connection().write(user_details_payload.serialize());

      state_.expect_operation_state(OperationState::GetUserDetails);
      state_.active_loading_overlay();
      return;
    }

    // signal the thread to start working
    send_file_cv_.notify_one();
  }

  void Application::login_feedback_handler(const FeedbackPayload& payload) noexcept
  {
    state_.disable_loading_overlay();
    // malformed request
    if (state_.expected_operation_state() != OperationState::Login)
    {
      Logger::warn(fmt::format("got unexpected response"));
      return;
    }

    state_.operation_state_complete();

    if (!payload.response)
    {
      state_.active_overlay(OverlayState::LoginFailed);
      return;
    }

    // serialize public key and send it to server
    auto public_key = asymmetric_encryptor_.public_key();
    auto public_key_bytes = ar::serialize(public_key);

    StorePublicKeyPayload key_payload{.public_key = std::move(public_key_bytes)};

    // set current user
    // TODO: get self user detail from server
    this_user_
        = std::make_unique<UserClient>(true, std::numeric_limits<User::id_type>::max(), "me");
    // start thread
    send_file_thread_ = std::thread{&Application::send_file_thread, this};

    auto* ucc = new UserClient{true, 123, "mirna"};
    // TODO: Delete this, it is for debug purpose
    // files_.emplace_back(FileFormat::Archive, 12345, this_user_.get(),
    // "File 1.png", get_current_time());
    // files_.emplace_back(FileFormat::Archive, 12345, ucc, "File 2.png",
    // get_current_time()); files_.emplace_back(FileFormat::Archive, 12345,
    // this_user_.get(), "File 3.png", get_current_time());
    // files_.emplace_back(FileFormat::Archive, 12345, this_user_.get(),
    // "File 4.png", get_current_time());
    // files_.emplace_back(FileFormat::Archive, 12345, this_user_.get(),
    // "File 5.png", get_current_time());
    // files_.emplace_back(FileFormat::Archive, 12345, this_user_.get(),
    //                     "File File File File File File File File File 6.png",
    //                     get_current_time());

    client_.connection().write(key_payload.serialize());

    state_.expect_operation_state(OperationState::StorePublicKey);
    state_.active_loading_overlay();  // make it loading on login page
  }

  void Application::register_feedback_handler(const FeedbackPayload& payload) noexcept
  {
    state_.disable_loading_overlay();

    // malformed request
    if (state_.expected_operation_state() != OperationState::Register)
    {
      Logger::warn(fmt::format("got unexpected reponse"));
      return;
    }

    state_.operation_state_complete();

    if (!payload.response)
    {
      state_.active_overlay(OverlayState::RegisterFailed);
      return;
    }

    state_.active_page(PageState::Login);
  }

  void Application::send_file_feedback_handler(const FeedbackPayload& payload) noexcept
  {
    if (state_.expected_operation_state() != OperationState::SendFile)
    {
      Logger::warn(fmt::format("got unexpected reponse"));
      return;
    }
    state_.disable_loading_overlay();
    state_.operation_state_complete();

    if (!payload.response)
    {
      state_.active_overlay(OverlayState::SendFileFailed);
      return;
    }

    state_.active_page(PageState::Dashboard);
  }

  void Application::get_user_details_feedback_handler(const FeedbackPayload& payload) noexcept
  {
    if (state_.expected_operation_state() != OperationState::GetUserDetails)
    {
      Logger::warn(fmt::format("got unexpected reponse"));
      return;
    }

    Logger::error(fmt::format("error on getting user details: {}", payload.message.value()));
    state_.operation_state_complete();
    state_.disable_loading_overlay();
    state_.active_overlay(OverlayState::InternalError);
  }

  void Application::get_user_online_feedback_handler(const FeedbackPayload& payload) noexcept
  {
    if (state_.expected_operation_state() != OperationState::GetUserOnline)
    {
      Logger::warn(fmt::format("got unexpected reponse"));
      return;
    }

    Logger::error(fmt::format("error on getting user onlines: {}", payload.message.value()));
    state_.operation_state_complete();
    state_.disable_loading_overlay();
    state_.active_overlay(OverlayState::InternalError);
  }

  void Application::store_public_key_feedback_handler(const FeedbackPayload& payload) noexcept
  {
    if (state_.expected_operation_state() != OperationState::StorePublicKey)
    {
      Logger::warn(fmt::format("got unexpected reponse"));
      return;
    }

    state_.operation_state_complete();

    // failed to store
    if (!payload.response)
    {
      state_.disable_loading_overlay();
      state_.active_overlay(OverlayState::InternalError);
      return;
    }

    // Get user onlines
    GetUserOnlinePayload user_online_payload{};
    client_.connection().write(user_online_payload.serialize());

    state_.active_loading_overlay();  // make it loading and still on loading page
    state_.expect_operation_state(OperationState::GetUserOnline);
  }

  void Application::on_file_drop(std::string_view paths) noexcept
  {
    if (state_.overlay_state() != OverlayState::SendFile)
    {
      Logger::trace("trying to drop files when the state is not Send File yet");
      return;
    }

    dropped_file_path_ = std::string{paths};
  }

  void Application::delete_file_on_dashboard(usize file_index) noexcept
  {
    std::unique_lock lock{file_mutex_};
    auto& file = files_[file_index];

    if (!delete_file(file.fullpath))
    {
      Logger::warn(fmt::format("failed to delete file: {}", file.fullpath));
      return;
    }
    Logger::info(fmt::format("file {} deleted successfully", file.fullpath));

    // should not modify files in here, because it is still used
    deleted_file_names_.emplace_back(file.fullpath);
  }

  void Application::open_file_on_dashboard(usize file_index) noexcept
  {
    std::unique_lock lock{file_mutex_};
    auto& file = files_[file_index];
    Logger::trace(fmt::format("open file: {}", file.fullpath));

    execute_file(file.fullpath);
  }

  void Application::on_feedback_response(const FeedbackPayload& payload) noexcept
  {
    switch (payload.id)
    {
    case PayloadId::Login: {
      login_feedback_handler(payload);
      break;
    }
    case PayloadId::Register: {
      register_feedback_handler(payload);
      break;
    }
    case PayloadId::SendFile: {
      send_file_feedback_handler(payload);
      break;
    }
    case PayloadId::GetUserDetails: {
      // GetUserDetails payload will only response Feedback when there is
      // error happens
      get_user_details_feedback_handler(payload);
      break;
    }
    case PayloadId::GetUserOnline: {
      // GetUserOnline payload will only response Feedback when there is error
      // happens
      get_user_online_feedback_handler(payload);
      break;
    }
    case PayloadId::StorePublicKey: {
      store_public_key_feedback_handler(payload);
      break;
    }
    }

    if constexpr (AR_DEBUG)
    {
      Logger::info(fmt::format("Feedback Repsonse: {}", payload.response,
                               magic_enum::enum_name(payload.id)));
      if (payload.message)
        Logger::trace(fmt::format("Feedback Message: {}", *payload.message));
    }
  }

  void Application::on_file_receive(const Message::Header& header,
                                    const SendFilePayload& payload) noexcept
  {
    // Decrypt key
    auto decipher_key_result = asymmetric_encryptor_.decrypts(payload.symmetric_key);
    if (!decipher_key_result.has_value())
    {
      Logger::error(
          fmt::format("failed to decrypt symmetric key: {}", decipher_key_result.error()));
      return;
    }
    auto decipher_key_bytes
        = as_byte_span<DMRSA::block_type>(decipher_key_result.value(), payload.key_filler);
    if (decipher_key_bytes.size() != KEY_BYTE)
    {
      Logger::error("decrypted key is malformed");
      return;
    }

    // Decrypt files
    Camellia::key_type key{decipher_key_bytes};
    Camellia symmetric_encryptor{key};
    auto decipher_file_result = symmetric_encryptor.decrypts(payload.files, payload.file_filler);
    if (!decipher_file_result.has_value())
    {
      Logger::error(
          fmt::format("failed to decrypt incoming file: {}", decipher_file_result.error()));
      return;
    }
    if (decipher_file_result->size() != payload.file_size)
    {
      Logger::error("decipher file has different size than the original file");
      return;
    }

    // Save file (overwrite)
    auto dest_path = save_dir_ / payload.filename;
    if (!save_bytes_as_file<true>(dest_path.string(), decipher_file_result.value()))
    {
      Logger::error("failed to save incoming file to disk");
      return;
    }

    // Show to dashboard
    {
      std::unique_lock lock{file_mutex_};
      auto [format, format_str] = get_file_format(payload.filename);
      {
        // get user opponent
        std::unique_lock lock2{user_mutex_};  // FIX: use shared_mutex for user so
        // it can be read by multiple thread

        auto it = std::ranges::find_if(
            users_, [&](const UserClient& uc) { return uc.id == header.opponent_id; });
        if (it == users_.end())
        {
          // WARN: it should be UB here
          Logger::error("got files from ghost, scaryyyyyyyyyyyy");
          return;
        }

        files_.emplace_back(format, payload.file_size, &*it, dest_path.string(),
                            get_current_time());
      }
    }
  }

  void Application::on_user_login(const UserLoginPayload& payload) noexcept
  {
    std::unique_lock lock{user_mutex_};
    // search if user already on database (offline)
    auto offline_users
        = users_ | std::views::filter([](const UserClient& uc) { return !uc.is_online; });
    if (offline_users.empty())
    {
      users_.emplace_back(true, payload.id, payload.username);
      return;
    }
    // toggle is_online
    auto it = std::ranges::find_if(offline_users,
                                   [&](const UserClient& uc) { return uc.id == payload.id; });
    if (it == offline_users.end())
    {
      users_.emplace_back(true, payload.id, payload.username);
      return;
    }
    it->is_online = true;
  }

  void Application::on_user_logout(const UserLogoutPayload& payload) noexcept
  {
    std::unique_lock lock{user_mutex_};
    // std::erase_if(users_, [&](const UserClient& user) { return user.id ==
    // payload.id; });
    auto it = std::ranges::find_if(users_,
                                   [&](const UserClient& user) { return user.id == payload.id; });
    if (it == users_.end())
    {
      Logger::warn(
          "got signal user logout, but the user doesn't exists on "
          "client database");
      return;
    }
    // set to offline
    it->is_online = false;
  }

  void Application::on_user_detail_response(const UserDetailPayload& payload) noexcept
  {
    if (state_.expected_operation_state() != OperationState::GetUserDetails)
    {
      Logger::warn(fmt::format("got unexpected reponse"));
      return;
    }

    {
      std::unique_lock lock{user_mutex_};
      auto it = std::ranges::find_if(users_,
                                     [&](const UserClient& user) { return user.id == payload.id; });
      if (it == users_.end())
      {
        Logger::error(fmt::format("got user detail, but the user is not on the list: {}|{}",
                                  payload.id, payload.username));

        state_.operation_state_complete();
        state_.disable_loading_overlay();
        state_.active_overlay(OverlayState::InternalError);
        return;
      }

      // deserialize public key
      auto result = ar::deserialize(payload.public_key);
      if (!result.has_value())
      {
        state_.operation_state_complete();
        state_.disable_loading_overlay();
        state_.active_overlay(OverlayState::InternalError);
        return;
      }

      it->name = payload.username;
      it->public_key = result.value();
    }

    state_.operation_state_complete();
    state_.disable_loading_overlay();
    state_.active_overlay(OverlayState::None);
    // notify the send file thread to start working
    if (!send_file_datas_.empty())
      send_file_cv_.notify_one();
  }

  void Application::on_user_online_response(const UserOnlinePayload& payload) noexcept
  {
    if (state_.expected_operation_state() != OperationState::GetUserOnline)
    {
      Logger::warn(fmt::format("got unexpected reponse"));
      return;
    }

    {
      std::unique_lock lock{user_mutex_};
      for (const auto& user : payload.users)
      {
        users_.emplace_back(true, user.id, user.name);
      }
    }

    state_.operation_state_complete();
    state_.disable_loading_overlay();
    state_.active_page(PageState::Dashboard);
  }
}  // namespace ar
