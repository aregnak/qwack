#pragma once

#include <string>
#include <sstream>
#include <fstream>

namespace Util
{

inline std::string loadJsonFile(const std::string& path)
{
    std::ifstream f(path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

} // namespace Util