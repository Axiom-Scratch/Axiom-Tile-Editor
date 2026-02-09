#pragma once

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

namespace te {

class ImGuiLayer {
public:
  bool Init(GLFWwindow* window);
  void NewFrame();
  void Render();
  void Shutdown();

private:
  bool m_initialized = false;
};

} // namespace te
