#include "tables.h"

#include "../graphics.h"

class ASCIITableWindow : public WindowProcess {
public:
  ASCIITableWindow()
    : WindowProcess("ASCII table")
  {
  }

  void render() override
  {
    if (ImGui::BeginTable("##asciitable", 4)) {
      for (int i = 0; i < 128; i++) {
        if (i % 4 == 0)
          ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("%03d 0x%02x %c", i, i, i); // TODO add color
      }
      ImGui::EndTable();
    }
  }
};

namespace cse::extensions {

Tables::Tables()
{
  m_tables.emplace("ascii", []() { return new ASCIITableWindow; });

  std::vector<const char *> tableNames;
  for (const auto &[name, _] : m_tables)
    tableNames.push_back(name);

  cse::addCommand({
    "table",
    "sheet sheets & tables",
    { new CommandEnumPart("name", tableNames)},
    [this](auto &parts) {
      for (const auto &[name, tableProvider] : m_tables) {
        if (parts[0] == name) {
          WindowProcess *window = tableProvider();
          graphics::createWindow(std::shared_ptr<WindowProcess>{ window });
          return;
        }
      }
    }
  });
}

Tables::~Tables()
{
}

}