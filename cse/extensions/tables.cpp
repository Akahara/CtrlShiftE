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

class ColorsSchemesWindow : public WindowProcess {
private:
  constexpr static int D3_COLOR_SCHEMES[] = {
    0xeff3ff, 0xc6dbef, 0x9ecae1, 0x6baed6, 0x3182bd, 0x08519c, 0,
    0xedf8e9,0xc7e9c0,0xa1d99b,0x74c476,0x31a354,0x006d2c, 0,
    0xfeedde,0xfdd0a2,0xfdae6b,0xfd8d3c,0xe6550d,0xa63603, 0,
    0xf2f0f7,0xdadaeb,0xbcbddc,0x9e9ac8,0x756bb1,0x54278f, 0,
    0xfee5d9,0xfcbba1,0xfc9272,0xfb6a4a,0xde2d26,0xa50f15, 0,
    0xedf8fb,0xccece6,0x99d8c9,0x66c2a4,0x2ca25f,0x006d2c, 0,
    0xf0f9e8,0xccebc5,0xa8ddb5,0x7bccc4,0x43a2ca,0x0868ac, 0,
    0xf6eff7,0xd0d1e6,0xa6bddb,0x67a9cf,0x1c9099,0x016c59, 0,
    0xffffd4,0xfee391,0xfec44f,0xfe9929,0xd95f0e,0x993404, 0,
    0x6e40aa,0x4c6edb,0x23abd8,0x1ddfa3,0x52f667,0xaff05b, 0,
    0x8c510a,0xd8b365,0xf6e8c3,0xc7eae5,0x5ab4ac,0x01665e, 0,
    0x762a83,0xaf8dc3,0xe7d4e8,0xd9f0d3,0x7fbf7b,0x1b7837, 0,
    0x1f77b4,0xff7f0e,0x2ca02c,0xd62728,0x9467bd,0x8c564b,0xe377c2,0x7f7f7f,0xbcbd22,0x17becf, 0,
    0x7fc97f,0xbeaed4,0xfdc086,0xffff99,0x386cb0,0xf0027f,0xbf5b17,0x666666, 0,
    0xa6cee3,0x1f78b4,0xb2df8a,0x33a02c,0xfb9a99,0xe31a1c,0xfdbf6f,0xff7f00,0xcab2d6,0x6a3d9a,0xffff99,0xb15928, 0,
  };
  static_assert(D3_COLOR_SCHEMES[IM_ARRAYSIZE(D3_COLOR_SCHEMES) - 1] == 0);

  constexpr static const char *ANSI_CSI[] = {
    "\\033[", "\\u001b[", "\\x1b[", "\\e["
  };

  char m_userFormat[32] = "rgb($r,$g,$b)";
  int m_ansiCSI = 0;
  
public:
  ColorsSchemesWindow()
    : WindowProcess("Color schemes")
  {
  }

  void render() override
  {
    ImGui::InputText("Copy Format", m_userFormat, sizeof(m_userFormat));
    if (ImGui::Button("#%r%g%b")) strcpy_s(m_userFormat, "#%r%g%b");
    ImGui::SameLine();
    if (ImGui::Button("rgb($r,$g,$b)")) strcpy_s(m_userFormat, "rgb($r,$g,$b)");
    ImGui::SameLine();
    if (ImGui::Button("rgb($R,$G,$B)")) strcpy_s(m_userFormat, "rgb($R,$G,$B)");

    if (ImGui::CollapsingHeader("D3 Color schemes (click to copy)", ImGuiTreeNodeFlags_DefaultOpen)) {
      for (int i = 0, s = 0; i < sizeof(D3_COLOR_SCHEMES)/sizeof(D3_COLOR_SCHEMES[0]); s++) {
        ImGui::PushID(i);
        drawColorScheme(&D3_COLOR_SCHEMES[i]);
        if (s % 2 == 0 && s < 12) ImGui::SameLine();
        ImGui::PopID();
        while (D3_COLOR_SCHEMES[i++] != 0);
      }
    }

    if (ImGui::CollapsingHeader("ANSI color codes", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Text("The CSI (control sequence introducer) is ESC followed by '['\n"
                  "The ESC (escape sequence) is the character 0x1B\n"
                  "reset                 ESC [m  or  ESC [0m\n"
                  "select foreground     ESC [38;5;<n>m\n"
                  "select background     ESC [48;5;<n>m\n"
                  "predefined fg         ESC [<n>m\n"
                  "predefined bright fg  ESC [<n>;1m\n"
                  "move up               ESC [<n>A\n"
                  "move down             ESC [<n>B\n"
                  "move foward           ESC [<n>C\n"
                  "move back             ESC [<n>D\n"
                  "move to (1-based)     ESC [<x>;<y>D\n"
      );
      if (ImGui::BeginCombo("##ansi_csi", ANSI_CSI[m_ansiCSI])) {
        for (int n = 0; n < IM_ARRAYSIZE(ANSI_CSI); n++) {
          if (ImGui::Selectable(ANSI_CSI[n], m_ansiCSI == n))
            m_ansiCSI = n;
        }
        ImGui::EndCombo();
      }
      ImGui::PushStyleColor(ImGuiCol_Button, 0);
      drawAnsiBtn("reset",     0); ImGui::SameLine();
      drawAnsiBtn("italic",    3); ImGui::SameLine(); drawAnsiBtn("italic reset",    23); ImGui::SameLine();
      drawAnsiBtn("bold",      1); ImGui::SameLine(); drawAnsiBtn("bold reset",      21); ImGui::SameLine();
      drawAnsiBtn("underline", 4);                    drawAnsiBtn("underline reset", 24); ImGui::SameLine();
      drawAnsiBtn("red",            31, IM_COL32(255,  49,  49, 255)); ImGui::SameLine();
      drawAnsiBtn("green",          32, IM_COL32( 13, 188, 121, 255)); ImGui::SameLine();
      drawAnsiBtn("yellow",         33, IM_COL32(229, 229,  16, 255)); ImGui::SameLine();
      drawAnsiBtn("blue",           34, IM_COL32( 36, 114, 200, 255)); ImGui::SameLine();
      drawAnsiBtn("magenta",        35, IM_COL32(188,  63, 188, 255)); 
      drawAnsiBtn("cyan",           36, IM_COL32( 17, 168, 205, 255)); ImGui::SameLine();
      drawAnsiBtn("white",          37, IM_COL32(229, 229, 229, 255)); ImGui::SameLine();
      drawAnsiBtn("bright red",     91, IM_COL32(241,  76,  76, 255)); ImGui::SameLine();
      drawAnsiBtn("bright green",   92, IM_COL32( 35, 209, 139, 255)); ImGui::SameLine();
      drawAnsiBtn("bright yellow",  93, IM_COL32(245, 245,  67, 255)); 
      drawAnsiBtn("bright blue",    94, IM_COL32( 59, 142, 234, 255)); ImGui::SameLine();
      drawAnsiBtn("bright magenta", 95, IM_COL32(214, 112, 214, 255)); ImGui::SameLine();
      drawAnsiBtn("bright cyan",    96, IM_COL32( 41, 184, 219, 255)); ImGui::SameLine();
      drawAnsiBtn("bright white",   97, IM_COL32(229, 229, 229, 255)); ImGui::SameLine();
      ImGui::PopStyleColor();
    }
  }

  std::string formatColorStr(int color)
  {
    std::string processed = m_userFormat;
    replaceFirst(processed, "$R", std::to_string((color>>16)&0xff));
    replaceFirst(processed, "$r", std::to_string((float)((color>>16)&0xff)/0xff));
    replaceFirst(processed, "$G", std::to_string((color>>8)&0xff));
    replaceFirst(processed, "$g", std::to_string((float)((color>>8)&0xff)/0xff));
    replaceFirst(processed, "$B", std::to_string((color>>0)&0xff));
    replaceFirst(processed, "$b", std::to_string((float)((color>>0)&0xff)/0xff));
    return processed;
  }

  inline void replaceFirst(std::string &toAffect, const char *toReplace, std::string replacement)
  {
    size_t idx = toAffect.find(toReplace);
    if (idx == std::string::npos) return;
    toAffect.replace(idx, strlen(toReplace), replacement);
  }

  inline void drawColorScheme(const int *scheme)
  {
    for (int i = 0; scheme[i]; i++) {
      ImGui::PushID(i);
      drawColorBtn(scheme[i]);
      ImGui::PopID();
      ImGui::SameLine(0, 0);
    }
    if (ImGui::Button("all")) {
      std::string drawColorScheme;
      for (int i = 0; scheme[i]; i++)
        drawColorScheme += formatColorStr(scheme[i]) + ", ";
      drawColorScheme.erase(drawColorScheme.size() - 2, 2);
      ImGui::SetClipboardText(drawColorScheme.c_str());
    }
  }

  inline void drawColorBtn(int color)
  {
    ImVec4 c{
      (float)((color>>16) & 0xff) / 0xff,
      (float)((color>> 8) & 0xff) / 0xff,
      (float)((color>> 0) & 0xff) / 0xff,
      1
    };
    if(ImGui::ColorButton("color", c))
      ImGui::SetClipboardText(formatColorStr(color).c_str());
  }

  inline void drawAnsiBtn(const char *colorName, int foregroundCode)
  {
    if (ImGui::Button(colorName))
      ImGui::SetClipboardText((ANSI_CSI[m_ansiCSI] + std::to_string(foregroundCode) + "m").c_str());
  }

  inline void drawAnsiBtn(const char *colorName, int foregroundCode, int color)
  {
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    drawAnsiBtn(colorName, foregroundCode);
    ImGui::PopStyleColor();
  }
};

namespace cse::extensions {

Tables::Tables()
{
  m_tables.emplace("ascii", []() { return new ASCIITableWindow; });
  m_tables.emplace("glsl", []() { return new GLSLTableWindow; });
  m_tables.emplace("colors", []() { return new ColorsSchemesWindow; });

  std::vector<std::string> tableNames;
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