#pragma once

#include "cse_utils.h"

/*
 * CSE extensions are instanciated after core functions and are destroyed before them.
 * It is safe to put initialization and cleanup code in constructors/destructors (RAII)
 */
class CSEExtension {
public:
  CSEExtension() = default;
  virtual ~CSEExtension() = default;

  CSEExtension(const CSEExtension &) = delete;
  CSEExtension &operator=(const CSEExtension &) = delete;
  CSEExtension(CSEExtension &&) = delete;
  CSEExtension &operator=(CSEExtension &&) = delete;

  virtual void reload() {}
};


namespace cse::extensions
{

const fs::path &getUserFilesPath();

// run in separate thread to avoid stalling the main thread until the command starts
Executor runLater(Executor executor);

/*
 * Utility function that opens a web browser to the given url.
 * Beware! This function is fundamentally unsafe when used with
 * unsanitized user input, it may open files or run executables
 * if the url looks like a path.
 * url must start with http:// or https://, otherwise behavior
 * is undefined.
 */
void openWebPage(const char *url);
void openFileDir(const char *path);

}
