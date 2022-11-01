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

class GLSLTableWindow : public WindowProcess {
public:
  GLSLTableWindow()
    : WindowProcess("GLSL table")
  {
  }

  void render() override
  {
    ImGui::Text("mix(x, y, a)");
    ImGui::Text("clamp(x, min, max)");
    ImGui::Text("step(e, x) := x>e");
    ImGui::Text("smoothstep(e0, e1, x)");
    ImGui::Text("texture(sampler2D, uv)");
    if (ImGui::CollapsingHeader("Vertex built-ins")) {
      ImGui::Text("in int gl_VertexID;\n"
                  "in int gl_InstanceID;");
      ImGui::Text("out gl_PerVertex {\n"
                  "  vec4 gl_Position;\n"
                  "  float gl_PointSize;\n"
                  "  float gl_ClipDistance[];\n"
                  "  float gl_CullDistance[];\n"
                  "};");
    }
    if (ImGui::CollapsingHeader("Fragment built-ins")) {
      ImGui::Text("in vec4 gl_FragCoord;\n"
                  "in int gl_PrimitiveID;\n"
                  "in int gl_SampleID;\n"
                  "in vec2 gl_SamplePosition;");
      ImGui::Text("out float gl_FragDepth;");
    }
    if (ImGui::Button("docs.gl"))
      cse::extensions::openWebPage("http://docs.gl");
    if (ImGui::Button("complete cheat sheet"))
      cse::extensions::openWebPage("https://www.opengl.org/sdk/docs/reference_card/opengl45-reference-card.pdf");
  }
};

namespace cse::extensions {

Tables::Tables()
{
  m_tables.emplace("ascii", []() { return new ASCIITableWindow; });
  m_tables.emplace("glsl", []() { return new GLSLTableWindow; });

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