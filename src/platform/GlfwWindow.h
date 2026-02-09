#pragma once

#include "app/Config.h"

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

namespace te {

class GlfwWindow {
public:
  GlfwWindow() = default;
  ~GlfwWindow();

  bool Create(int width, int height, const char* title);
  void Destroy();

  void PollEvents();
  void SwapBuffers();

  bool ShouldClose() const;
  void SetShouldClose(bool shouldClose);
  void SetVsync(bool enabled);

  GLFWwindow* GetNative() const;
  Vec2i GetFramebufferSize() const;
  Vec2i GetWindowSize() const;

  int GetMajorVersion() const { return m_glMajor; }
  int GetMinorVersion() const { return m_glMinor; }

private:
  bool CreateWithVersion(int major, int minor, int width, int height, const char* title);

  GLFWwindow* m_window = nullptr;
  int m_glMajor = 0;
  int m_glMinor = 0;
  bool m_vsyncEnabled = true;
};

} // namespace te
