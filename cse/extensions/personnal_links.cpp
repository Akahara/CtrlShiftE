#include "personnal_links.h"

#include <Windows.h>

static void openUrl(const char *url)
{
  auto success = ShellExecuteA(NULL, "open", url, NULL, NULL, SW_HIDE);
  cse::log(std::string("Opening url ") + url + ", got " + std::to_string((long long)success));
}

static void registerFilesLink()
{
  Command command{
    "files",
    "goto acalais.fr/files",
    { new CommandTextPart("code") },
    [](const auto &args) {
      std::string url = "https://acalais.fr/files?code=";
      url += args[0];
      openUrl(url.c_str());
    }
  };
  cse::addCommand(std::move(command));
}

static void registerSheetsLinks()
{
  Command command{
    "sheets",
    "goto acalais.fr/sheets",
    { /* no arguments */ },
    [](const auto &args) {
      openUrl("https://acalais.fr/sheets/math/");
    }
  };
  cse::addCommand(std::move(command));
}

namespace cse::extensions {

PersonnalLinks::PersonnalLinks()
{
  registerFilesLink();
  registerSheetsLinks();
}

PersonnalLinks::~PersonnalLinks()
{
}

}