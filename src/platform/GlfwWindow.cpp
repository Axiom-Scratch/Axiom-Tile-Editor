#include "platform/GlfwWindow.h"

#include "util/Log.h"

#include <string>

namespace te {

namespace {

void GlfwErrorCallback(int error, const char* description) {
  Log::Error("GLFW error " + std::to_string(error) + ": " + description);
}

} // namespace

GlfwWindow::~GlfwWindow() {
  Destroy();
}

bool GlfwWindow::Create(int width, int height, const char* title) {
  if (!glfwInit()) {
    Log::Error("Failed to initialize GLFW.");
    return false;
  }

  glfwSetErrorCallback(GlfwErrorCallback);

  if (!CreateWithVersion(4, 6, width, height, title)) {
    glfwDefaultWindowHints();
    if (!CreateWithVersion(3, 3, width, height, title)) {
      Log::Error("Failed to create OpenGL 4.6 or 3.3 context.");
      glfwTerminate();
      return false;
    }
  }

  return true;
}

bool GlfwWindow::CreateWithVersion(int major, int minor, int width, int height, const char* title) {
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
#if defined(TE_DEBUG) && defined(TE_ENABLE_GL_DEBUG)
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

  m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (!m_window) {
    return false;
  }

  glfwMakeContextCurrent(m_window);
  SetVsync(true);

  m_glMajor = major;
  m_glMinor = minor;
  return true;
}

void GlfwWindow::Destroy() {
  if (m_window) {
    glfwDestroyWindow(m_window);
    m_window = nullptr;
  }
  glfwTerminate();
}

void GlfwWindow::PollEvents() {
  glfwPollEvents();
}

void GlfwWindow::SwapBuffers() {
  if (m_window) {
    glfwSwapBuffers(m_window);
  }
}

bool GlfwWindow::ShouldClose() const {
  return m_window ? glfwWindowShouldClose(m_window) != 0 : true;
}

void GlfwWindow::SetShouldClose(bool shouldClose) {
  if (m_window) {
    glfwSetWindowShouldClose(m_window, shouldClose ? GLFW_TRUE : GLFW_FALSE);
  }
}

void GlfwWindow::SetVsync(bool enabled) {
  if (m_window) {
    glfwSwapInterval(enabled ? 1 : 0);
  }
  m_vsyncEnabled = enabled;
}

GLFWwindow* GlfwWindow::GetNative() const {
  return m_window;
}

Vec2i GlfwWindow::GetFramebufferSize() const {
  Vec2i size{};
  if (m_window) {
    glfwGetFramebufferSize(m_window, &size.x, &size.y);
  }
  return size;
}

Vec2i GlfwWindow::GetWindowSize() const {
  Vec2i size{};
  if (m_window) {
    glfwGetWindowSize(m_window, &size.x, &size.y);
  }
  return size;
}

} // namespace te
