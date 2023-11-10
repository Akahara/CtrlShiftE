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
fs::path getUserConfigFilePath(const char *fileName, const char *defaultFileContents);


// run in between frames, when no extensions are active, to avoid stalling the main thread during display (still synchronous, use with care)
void runLater(std::function<void()> &&executor);
// run in separate thread to avoid stalling the main thread until the command starts
void runDetached(std::function<void()> &&call);

#ifdef CSE_EXPOSE_INTERNALS
void runDelayedTasks();
#endif

/*
 * Utility function that opens a web browser to the given url.
 * Beware! This function is fundamentally unsafe when used with
 * unsanitized user input, it may open files or run executables
 * if the url looks like a path.
 * url must start with http:// or https://, otherwise behavior
 * is undefined.
 */
void openWebPage(const char *url);
/*
 * Opens the given file path in an new windows explorer window.
 */
void openFileDir(const char *path);
void executeShellCommand(const std::string &command, bool interactive=false);

}
