#pragma once

#include <filesystem>
#include <functional>

#define DELETE_COPY_OPERATORS(className)\
  className(const className &) = delete;\
  className &operator=(const className &) = delete;\
  className(className &&) = delete;\
  className &operator=(className &&) = delete;

namespace fs = std::filesystem;

using Executor = std::function<void(const std::vector<std::string_view> &parts)>;


namespace cse {

void log(std::string_view line);

void logm(auto &&...args)
{
  std::stringstream ss;
  (ss << ... << args);
  log(ss.str());
}

}