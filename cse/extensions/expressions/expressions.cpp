#include "expressions.h"

#include <stack>
#include <stdexcept>

namespace expressions
{

static Operator OPERATOR_TOKENS_TABLE[TokenType::TOKEN_TYPE_COUNT];
static auto _1 = []
{
  OPERATOR_TOKENS_TABLE[TokenType::TK_ADD] = Operator::ADD;
  OPERATOR_TOKENS_TABLE[TokenType::TK_SUBTRACT] = Operator::SUBTRACT;
  OPERATOR_TOKENS_TABLE[TokenType::TK_MULTIPLY] = Operator::MULTIPLY;
  OPERATOR_TOKENS_TABLE[TokenType::TK_DIVIDE] = Operator::DIVIDE;
  OPERATOR_TOKENS_TABLE[TokenType::TK_MODULO] = Operator::MODULO;
  OPERATOR_TOKENS_TABLE[TokenType::TK_EXPONENCIATE] = Operator::EXPONENCIATE;
  OPERATOR_TOKENS_TABLE[TokenType::TK_SHIFT_LEFT] = Operator::SHIFT_LEFT;
  OPERATOR_TOKENS_TABLE[TokenType::TK_SHIFT_RIGHT] = Operator::SHIFT_RIGHT;
  OPERATOR_TOKENS_TABLE[TokenType::TK_LOGICAL_XOR] = Operator::LOGICAL_XOR;
  OPERATOR_TOKENS_TABLE[TokenType::TK_LOGICAL_AND] = Operator::LOGICAL_AND;
  OPERATOR_TOKENS_TABLE[TokenType::TK_LOGICAL_OR] = Operator::LOGICAL_OR;
  return 0;
}();
static int OPERTATOR_PRIORITY_TABLE[Operator::OPERATOR_COUNT];
static auto _2 = []
{
  OPERTATOR_PRIORITY_TABLE[Operator::ADD] = 0;
  OPERTATOR_PRIORITY_TABLE[Operator::SUBTRACT] = 0;
  OPERTATOR_PRIORITY_TABLE[Operator::MULTIPLY] = 0;
  OPERTATOR_PRIORITY_TABLE[Operator::DIVIDE] = 0;
  OPERTATOR_PRIORITY_TABLE[Operator::MODULO] = 0;
  OPERTATOR_PRIORITY_TABLE[Operator::EXPONENCIATE] = 0;
  OPERTATOR_PRIORITY_TABLE[Operator::SHIFT_LEFT] = 0;
  OPERTATOR_PRIORITY_TABLE[Operator::SHIFT_RIGHT] = 0;
  OPERTATOR_PRIORITY_TABLE[Operator::LOGICAL_XOR] = 0;
  OPERTATOR_PRIORITY_TABLE[Operator::LOGICAL_AND] = 0;
  OPERTATOR_PRIORITY_TABLE[Operator::LOGICAL_OR] = 0;
  return 0;
}();
static bool OPERTATOR_UNARY_CAPABLE_TABLE[Operator::OPERATOR_COUNT];
static auto _3 = []
{
  OPERTATOR_UNARY_CAPABLE_TABLE[Operator::ADD] = true;
  OPERTATOR_UNARY_CAPABLE_TABLE[Operator::SUBTRACT] = true;
  return 0;
}();
static constexpr std::pair<TokenType, const char *> TOKEN_TEXTS[] = {
  { TokenType::TK_PAREN_OPEN,   "("  },
  { TokenType::TK_PAREN_CLOSE,  ")"  },
  { TokenType::TK_ADD,          "+"  },
  { TokenType::TK_SUBTRACT,     "-"  },
  { TokenType::TK_EXPONENCIATE, "**" },
  { TokenType::TK_MULTIPLY,     "*"  },
  { TokenType::TK_DIVIDE,       "/"  },
  { TokenType::TK_MODULO,       "%"  },
  { TokenType::TK_SHIFT_LEFT,   "<<" },
  { TokenType::TK_SHIFT_RIGHT,  ">>" },
  { TokenType::TK_LOGICAL_XOR,  "^"  },
  { TokenType::TK_LOGICAL_AND,  "&"  },
  { TokenType::TK_LOGICAL_OR,   "|"  },
};

static constexpr SectionToken SECTION_TOKENS[] = {
  SectionToken{ TokenType::TK_PAREN_OPEN, TokenType::TK_PAREN_CLOSE }
};

eval_type VarExp::eval(EvaluationContext &context) const
{
  if (context.variables.contains(variableName))
    return std::visit([](auto x) { return static_cast<eval_type>(x); }, context.variables[variableName]);
  context.evaluationError = "Unbound variable: " + variableName;
  return 0;
}

inteval_type VarExp::evalInt(EvaluationContext &context) const
{
  if (context.variables.contains(variableName))
    return std::visit([](auto x) { return static_cast<inteval_type>(x); }, context.variables[variableName]);
  context.evaluationError = "Unbound variable: " + variableName;
  return 0;
}

template<class T>
T evalCommonUnaryOperation(EvaluationContext &context, T operand, Operator operation)
{
  switch (operation) {
  case Operator::ADD:
    return operand;
  case Operator::SUBTRACT:
    return -operand;
  default:;
  }
  context.evaluationError = "Invalid unary operator";
  return 0;
}

eval_type UnaryOpExp::eval(EvaluationContext& context) const
{
  return evalCommonUnaryOperation(context, operand->eval(context), operation);
}

inteval_type UnaryOpExp::evalInt(EvaluationContext &context) const
{
  return evalCommonUnaryOperation(context, operand->evalInt(context), operation);
}

template<class T>
T evalCommonBinaryOperation(EvaluationContext &context, T leftOp, T rightOp, Operator operation)
{
  switch (operation) {
  case Operator::ADD:
    return leftOp + rightOp;
  case Operator::SUBTRACT:
    return leftOp - rightOp;
  case Operator::MULTIPLY:
    return leftOp * rightOp;
  case Operator::DIVIDE:
    return leftOp / rightOp;
  default: break;
  }
  context.evaluationError = "Invalid binary operator";
  return 0;
}

eval_type BinaryOpExp::eval(EvaluationContext& context) const
{
  eval_type leftOp = leftOperand->eval(context);
  if (context.evaluationError) return 0;
  eval_type rightOp = rightOperand->eval(context);
  if (context.evaluationError) return 0;
  switch (operation) {
  case Operator::MODULO:
    return std::fmod(leftOp, rightOp);
  case Operator::EXPONENCIATE:
    return std::pow(leftOp, rightOp);
  default:
    return evalCommonBinaryOperation(context, leftOp, rightOp, operation);
  }
}

inteval_type BinaryOpExp::evalInt(EvaluationContext& context) const
{
  inteval_type leftOp = leftOperand->evalInt(context);
  if (context.evaluationError) return 0;
  inteval_type rightOp = rightOperand->evalInt(context);
  if (context.evaluationError) return 0;
  switch (operation) {
  case Operator::SHIFT_LEFT:
    return leftOp << rightOp;
  case Operator::SHIFT_RIGHT:
    return leftOp >> rightOp;
  case Operator::LOGICAL_XOR:
    return leftOp ^ rightOp;
  case Operator::LOGICAL_AND:
    return leftOp & rightOp;
  case Operator::LOGICAL_OR:
    return leftOp | rightOp;
  case Operator::MODULO:
    return leftOp % rightOp;
  case Operator::EXPONENCIATE:
    return static_cast<inteval_type>(std::pow(leftOp, rightOp));
  default:
    return evalCommonBinaryOperation(context, leftOp, rightOp, operation);
  }
}

Section Section::getSubSection(size_t begin, size_t end) const
{
  Section subsection{ begin, end };
  for (const Section &sec : subsections) {
    if (sec.start == begin && sec.stop == end)
      return sec;
    if (sec.start < end && sec.stop >= begin)
      subsection.subsections.push_back(sec);
    if ((sec.start < end && sec.stop > end) || (sec.stop > begin && sec.start < begin))
      throw std::runtime_error("Unexpected section overlap");
  }
  for (auto &[op, pos] : operators) {
    if (begin <= pos && pos < end)
      subsection.operators.emplace_back(op, pos);
  }
  return subsection;
}

Section getVisibleSection(ParsingContext& context)
{
  std::stack<Section> sections;
  Section current{ 0, context.tokens.size() };

  for (size_t i = 0; i < context.tokens.size(); i++) {
    TokenType t = context.tokens[i].type;

    if (OPERATOR_TOKENS_TABLE[t] != Operator::INVALID_OPERATOR) {
      current.operators.emplace_back(OPERATOR_TOKENS_TABLE[t], i);
      continue;
    }

    for (const SectionToken &sec : SECTION_TOKENS) {
      if (t == sec.start) {
        Section subsection{ i + 1, 0 };
        current.subsections.push_back(subsection);
        sections.push(std::exchange(current, std::move(subsection)));
        break;
      }
      if (t == sec.end) {
        current.stop = i;
        current = sections.top();
        sections.pop();
        break;
      }
    }
  }
  if (!sections.empty()) {
    context.parsingError = "A section was left open";
    return {};
  }
  return current;
}

std::unique_ptr<Expression> parse(ParsingContext& context)
{
  Section section = getVisibleSection(context);
  if (context.parsingError) return nullptr;
  return parse(context, section);
}

std::pair<eval_type, inteval_type> parseNumber(ParsingContext &context, const std::string& string)
{
  if(string.size() > 2 && string[0] == '0') {
    std::string substr = string.substr(2);
    inteval_type i{};
    size_t s = 0;
    try {
      switch(string[1])
      {
      static_assert(sizeof(long long) == sizeof(inteval_type));
      case 'b': i = std::stoll(substr, &s, 2);  break;
      case 'x': i = std::stoll(substr, &s, 16); break;
      case 'u': i = std::stoll(substr, &s, 10); break;
      default: s = -1; break;
      }
    } catch (...) {
      context.parsingError = "Invalid marked string literal: " + string;
      return {};
    }
    if(s != -1) {
      if(s != substr.size()) {
        context.parsingError = "Invalid marked string literal: " + string;
        return {};
      }
      return { static_cast<eval_type>(i), i};
    }
  }

  size_t s;
  eval_type e = std::stod(string, &s);
  if(s != string.size()) {
    context.parsingError = "Invalid string literal: " + string;
    return {};
  }
  return { e,static_cast<inteval_type>(e) };
}

std::unique_ptr<Expression> parse(ParsingContext& context, const Section& section)
{
  if (section.stop == section.start) {
    context.parsingError = "Empty expression";
    return nullptr;
  }

  // remove extra parenthesis
  if (section.subsections.size() == 1
    && section.subsections.front().start == section.start + 1
    && section.subsections.front().stop == section.stop - 1) {
    return parse(context, section.subsections.front());
  }

  // parse single token expression
  if (section.stop - section.start == 1) {
    const Token &tk = context.tokens[section.start];
    if (tk.type == TokenType::VAR_VARIABLE)
      return std::make_unique<VarExp>(context.getText(tk));
    if (tk.type == TokenType::VAR_NUMBER) {
      auto [num, intNum] = parseNumber(context, context.getText(tk));
      if (context.parsingError) return nullptr;
      return std::make_unique<NumberExp>(num, intNum);
    }
    context.parsingError = "Unknown literal";
    return nullptr;
  }

  // parse operation
  if (!section.operators.empty())
    return parseOperationExpression(context, section);

  context.parsingError = "Unknown expression";
  return nullptr;
}

std::unique_ptr<Expression> parseOperationExpression(ParsingContext& context, const Section& section)
{
  auto &[op, pos] = *std::ranges::min_element(section.operators, std::less{}, [](auto &o) { return OPERTATOR_PRIORITY_TABLE[o.first]; });
  std::unique_ptr<Expression> leftOperand = nullptr;
  if (pos == section.start) {
    if (!OPERTATOR_UNARY_CAPABLE_TABLE[op]) {
      context.parsingError = "Binary operator used as unary operator";
      return nullptr;
    }
  } else {
    leftOperand = parse(context, section.getSubSection(section.start, pos));
    if (context.parsingError) return nullptr;
  }
  
  if (pos + 1 == section.stop) {
    context.parsingError = "Operator does not have a right operand";
    return nullptr;
  }
  std::unique_ptr<Expression> rightOperand = parse(context, section.getSubSection(pos + 1, section.stop));
  if (context.parsingError) return nullptr;
  return leftOperand == nullptr ?
           static_cast<std::unique_ptr<Expression>>(std::make_unique<UnaryOpExp>(op, std::move(rightOperand)))
           : static_cast<std::unique_ptr<Expression>>(std::make_unique<BinaryOpExp>(op, std::move(leftOperand), std::move(rightOperand)));
}

TokenType readNonSectionToken(LexingContext& context, size_t begin, size_t end)
{
  std::string_view text{ context.line.c_str() + begin, end-begin };

  //if (std::ranges::all_of(text, [](char c) { return (c >= '0' && c <= '9') || c == '.'; }))
  //  return TokenType::VAR_NUMBER;
  if (text[0] >= '0' && text[0] <= '9')
    return TokenType::VAR_NUMBER;
  if (std::ranges::all_of(text, [](char c) { return true; }))
    return TokenType::VAR_VARIABLE;
  context.lexingError = "Could not parse text: " + context.line.substr(begin, end-begin);
  return TokenType::INVALID_TOKEN;
}

std::pair<TokenType, size_t> getDelimiter(const LexingContext& context, size_t start)
{
  if(context.line[start] == ' ') {
    size_t stop = 1;
    while (stop < context.line.size() && context.line[stop] == ' ')
      stop++;
    return { TokenType::TK_SPACE, stop };
  }
  for(auto &[tk, txt] : TOKEN_TEXTS) {
    if (std::string_view{ context.line.c_str() + start, context.line.size() - start }.starts_with(txt))
      return { tk, start + std::strlen(txt) };
  }
  return { TokenType::INVALID_TOKEN, 0 };
}

std::vector<Token> lex(LexingContext& context)
{
  std::vector<Token> tokens;
  size_t currentTokenBegin = 0;

  for (size_t i = 0; i < context.line.length(); ) {
    auto [delimiter, stop] = getDelimiter(context, i);
    if (delimiter == TokenType::INVALID_TOKEN) {
      // no token could be read, advance by 1
      i++;
      continue;
    }

    if(currentTokenBegin != i) {
      tokens.emplace_back(currentTokenBegin, i, readNonSectionToken(context, currentTokenBegin, i));
      if (context.lexingError) return {};
    }

    tokens.emplace_back(i, stop, delimiter);
    i = stop;
    currentTokenBegin = i;
  }

  // read the last non section token
  if(currentTokenBegin != context.line.size())
    tokens.emplace_back(currentTokenBegin, context.line.size(), readNonSectionToken(context, currentTokenBegin, context.line.length()));

  return tokens;
}
}
