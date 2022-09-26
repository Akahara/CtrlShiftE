#include "recorded_commands.h"

void IOTRecordedCommands::getCommands(const std::string &text, std::vector<IOTCommand *> &out_commands)
{

  out_commands.push_back(new NormalCommand("python", 6.f, []() {
    //system("\"C:\\Program Files\\Python310\\python.exe\"");
    system("\"C:\\WINDOWS\\system32\\cmd.exe\"");
  }));
  out_commands.push_back(new NormalCommand("vscode", 6.f, []() {
    system("code");
  }));
}
