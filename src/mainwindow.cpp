#include "mainwindow.h"

namespace natsukashii::frontend
{
MainWindow::~MainWindow() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  
  glfwDestroyWindow(window);
  glfwTerminate();
  NFD_Quit();
}

MainWindow* g_window = nullptr;

using KeySaveState = std::pair<int, int>;

constexpr std::array<KeySaveState, 10> savestate_buttons{
  std::make_pair(GLFW_KEY_0, 0), std::make_pair(GLFW_KEY_1, 1), std::make_pair(GLFW_KEY_2, 2), std::make_pair(GLFW_KEY_3, 3), std::make_pair(GLFW_KEY_4, 4),
  std::make_pair(GLFW_KEY_5, 5), std::make_pair(GLFW_KEY_6, 6), std::make_pair(GLFW_KEY_7, 7), std::make_pair(GLFW_KEY_8, 8), std::make_pair(GLFW_KEY_9, 9)
};

constexpr std::array<KeySaveState, 10> loadstate_buttons{
  std::make_pair(GLFW_KEY_F10, 0), std::make_pair(GLFW_KEY_F1, 1), std::make_pair(GLFW_KEY_F2, 2), std::make_pair(GLFW_KEY_F3, 3), std::make_pair(GLFW_KEY_F4, 4),
  std::make_pair(GLFW_KEY_F5,  5), std::make_pair(GLFW_KEY_F6, 6), std::make_pair(GLFW_KEY_F7, 7), std::make_pair(GLFW_KEY_F8, 8), std::make_pair(GLFW_KEY_F9, 9)
};

static void glfw_error_callback(int error, const char* description)
{
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if(action == GLFW_PRESS) {
    g_window->core->key = key;
    switch(key) {
      case GLFW_KEY_O: g_window->OpenFile(); break;
      case GLFW_KEY_S: g_window->core->Stop(); break;
      case GLFW_KEY_R: g_window->core->Reset(); break;
      case GLFW_KEY_P: g_window->core->Pause(); break;
      case GLFW_KEY_Q:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        g_window->core->Stop();
        break;
    }
    
    for(int i = 0; i < 10; i++) {
      if(savestate_buttons[i].first == key) {
        g_window->core->SaveState(savestate_buttons[i].second);
      }

      if(loadstate_buttons[i].first == key) {
        g_window->core->LoadState(loadstate_buttons[i].second);
      }
    }
  } else if(action == GLFW_RELEASE) {
    g_window->core->key = 0;
  }
}

MainWindow::MainWindow(std::string title) : file("config.ini") {
  g_window = this;

  if(glfwInit() == GLFW_FALSE)
  {
    running = false;
    core.reset();
    exit(1);
  }
  
  const char* glsl_version = "#version 330";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwSetErrorCallback(glfw_error_callback);

  const GLFWvidmode *details = glfwGetVideoMode(glfwGetPrimaryMonitor());
  int w = details->width - (details->width / 4), h = details->height - (details->height / 4);
  window = glfwCreateWindow(w, h, title.c_str(), nullptr, nullptr);
  glfwSetWindowPos(window, details->width / 2 - w / 2, details->height / 2 - h / 2);

  glfwMakeContextCurrent(window);
  glfwSwapInterval(0);

  glfwSetKeyCallback(window, key_callback);

  if(!gladLoadGL()) {
    fprintf(stderr, "Failed to initialize OpenGL loader!\n");
    exit(1);
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  if (!file.read(ini)) {
    ini["emulator"]["skip"] = "false";
    ini["emulator"]["bootrom"] = "bootrom.bin";
    file.generate(ini);
  }

  bool skip = ini["emulator"]["skip"] == "true";
  std::string bootrom = ini["emulator"]["bootrom"];
  core = std::make_unique<Core>(skip, bootrom);
  
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, core->bus.ppu.pixels);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

  NFD_Init();
  emu_thread = std::thread([&] { core->RunAsync(); } ); // Wake up emulator thread
  emu_thread.detach();
}

void MainWindow::OpenFile() {
  nfdchar_t *outpath;
  nfdfilteritem_t filteritem[2] = {{ "Game Boy roms", "gb" }, { "Game Boy Color roms", "gbc" }};
  nfdresult_t result = NFD_OpenDialog(&outpath, filteritem, 2, "roms/");
  if(result == NFD_OKAY) {
    core->LoadROM(std::string(outpath));
  }
}

void MainWindow::UpdateTexture() {
  glBindTexture(GL_TEXTURE_2D, id);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, core->bus.ppu.pixels);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
}

ImVec2 image_size;

static void resize_callback(ImGuiSizeCallbackData* data) {
  float x = ImGui::GetWindowSize().x - 15, y = ImGui::GetWindowSize().y - 15;
  float current_aspect_ratio = x / y;
  
  if(aspect_ratio_gb > current_aspect_ratio) {
    y = x / aspect_ratio_gb;
  } else {
    x = y * aspect_ratio_gb;
  }

  image_size = ImVec2(x, y);
}

void MainWindow::Run() {
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  
  while(!glfwWindowShouldClose(window)) {
    u32 frameStartTicks = SDL_GetTicks();

    if(core->init && !core->pause) {
      PingEmuThread();
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    glfwPollEvents();

    if(core->bus.ppu.render) {
      core->bus.ppu.render = false;
    }
    
    UpdateTexture();

    ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, FLT_MAX), resize_callback);
    ImGui::Begin("Image", nullptr, ImGuiWindowFlags_NoTitleBar);
    ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(id)), image_size);
    ImGui::End();

    MenuBar();

    ImGui::Render();

    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);

    if(core->init && !core->pause) {
      WaitEmuThread();
    }
    
    //while ((SDL_GetTicks() - frameStartTicks) < (1000 / 60))
    SDL_Delay(1);
  }
}

void MainWindow::MenuBar()
{
  if(ImGui::BeginMainMenuBar())
  {
    if(ImGui::BeginMenu("File"))
    {
      if(ImGui::MenuItem("Open"))
      {
        OpenFile();
      }

      if(ImGui::MenuItem("Exit"))
      {
        running = false;
        core.reset();
        glfwSetWindowShouldClose(window, GLFW_TRUE);
      }
      ImGui::EndMenu();
    }
    
    if(ImGui::BeginMenu("Emulation"))
    {
      if(ImGui::MenuItem(core->pause ? "Resume" : "Pause"))
      {
        core->Pause();
      }

      if(ImGui::MenuItem("Reset"))
      {
        core->Reset();
      }

      if(ImGui::MenuItem("Stop"))
      {
        core->Stop();
        UpdateTexture();
      }

      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
}

void MainWindow::PingEmuThread() {
  std::lock_guard <std::mutex> lock (core->emu_mutex);
  
  core->run_emu_thread = true;
  core->emu_condition_variable.notify_one();
}

void MainWindow::WaitEmuThread() {
  while (core->run_emu_thread) {}
}

} // natsukashii::frontend