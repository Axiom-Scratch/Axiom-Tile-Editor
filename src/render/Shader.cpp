#include "render/Shader.h"

#include "render/GL.h"
#include "util/Log.h"

namespace te {

namespace {

unsigned int CompileShader(unsigned int type, const std::string& source) {
  unsigned int shader = glCreateShader(type);
  const char* src = source.c_str();
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);

  int success = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[1024] = {};
    glGetShaderInfoLog(shader, static_cast<GLsizei>(sizeof(infoLog)), nullptr, infoLog);
    Log::Error(std::string("Shader compilation failed: ") + infoLog);
    glDeleteShader(shader);
    return 0;
  }

  return shader;
}

} // namespace

Shader::~Shader() {
  if (m_program != 0) {
    glDeleteProgram(m_program);
  }
}

bool Shader::LoadFromSource(const std::string& vertexSrc, const std::string& fragmentSrc) {
  unsigned int vertex = CompileShader(GL_VERTEX_SHADER, vertexSrc);
  if (vertex == 0) {
    return false;
  }

  unsigned int fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);
  if (fragment == 0) {
    glDeleteShader(vertex);
    return false;
  }

  m_program = glCreateProgram();
  glAttachShader(m_program, vertex);
  glAttachShader(m_program, fragment);
  glLinkProgram(m_program);

  glDeleteShader(vertex);
  glDeleteShader(fragment);

  int success = 0;
  glGetProgramiv(m_program, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[1024] = {};
    glGetProgramInfoLog(m_program, static_cast<GLsizei>(sizeof(infoLog)), nullptr, infoLog);
    Log::Error(std::string("Shader link failed: ") + infoLog);
    glDeleteProgram(m_program);
    m_program = 0;
    return false;
  }

  return true;
}

void Shader::Bind() const {
  glUseProgram(m_program);
}

void Shader::Unbind() const {
  glUseProgram(0);
}

void Shader::SetMat4(const std::string& name, const Mat4& value) const {
  int location = GetUniformLocation(name);
  if (location >= 0) {
    glUniformMatrix4fv(location, 1, GL_FALSE, value.m);
  }
}

void Shader::SetVec4(const std::string& name, const Vec4& value) const {
  int location = GetUniformLocation(name);
  if (location >= 0) {
    glUniform4f(location, value.r, value.g, value.b, value.a);
  }
}

void Shader::SetInt(const std::string& name, int value) const {
  int location = GetUniformLocation(name);
  if (location >= 0) {
    glUniform1i(location, value);
  }
}

int Shader::GetUniformLocation(const std::string& name) const {
  if (m_program == 0) {
    return -1;
  }
  return glGetUniformLocation(m_program, name.c_str());
}

} // namespace te
