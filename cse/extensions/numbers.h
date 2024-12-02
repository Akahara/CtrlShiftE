#pragma once

#include "cse_extensions.h"
#include "cse_graphics.h"

namespace cse::extensions
{

class NumbersWindow : public WindowProcess
{
private:
  using int_type = int64_t;
  using uint_type = uint64_t;
  using float_type = double_t;
  static constexpr size_t BIT_COUNT = sizeof(int_type) * CHAR_BIT;
  static constexpr size_t TEXTS_LENGTH = 80;
  static_assert(TEXTS_LENGTH >= BIT_COUNT+2);
public:
  static std::shared_ptr<NumbersWindow> make_window()
  {
    std::shared_ptr<NumbersWindow> win{ new NumbersWindow(s_windowCount++) };
    s_allWindows.push_back(std::weak_ptr(win));
    return win;
  }

  void render() override;

  int_type getValueAsInt() const;

private:
  explicit NumbersWindow(size_t winId);
  template<class smallint_type, class smallunsigned_type>
  void updateTexts(smallint_type value, smallunsigned_type uvalue);
  void updateTexts();
  void parseAndEvalExpression(const std::string &expression, bool asFloat=false);

  static size_t s_windowCount;
  static std::vector<std::weak_ptr<NumbersWindow>> s_allWindows;

  size_t      m_enabledByteCount = 4;
  int         m_textFocus = 1;
  std::string m_winVarName;
  int_type    m_intValue = 0;
  float_type  m_floatValue = 0.;
  char m_unsignedString[TEXTS_LENGTH] = "0u0";
  char m_signedString  [TEXTS_LENGTH] = "0";
  char m_hexString     [TEXTS_LENGTH] = "0x0";
  char m_binString     [TEXTS_LENGTH] = "0b0";
  char m_floatString   [TEXTS_LENGTH] = "0.0";
  std::string m_evalErrorString;
};

class Numbers : public CSEExtension
{
public:
  static constexpr const char *EXTENSION_NAME = "Numbers";

  Numbers();
};

}
