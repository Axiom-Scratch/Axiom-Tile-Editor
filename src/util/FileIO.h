#pragma once

#include <string>

namespace te::FileIO {

bool ReadTextFile(const std::string& path, std::string& outText);
bool WriteTextFile(const std::string& path, const std::string& text);

} // namespace te::FileIO
