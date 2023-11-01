#pragma once

#include <algorithm>
#include <memory>
#include <optional>
#include <variant>
#include <string>
#include <unordered_map>

namespace expressions
{

using eval_type = double;
using inteval_type = int64_t;


namespace tokens {
enum TokenType {
  INVALID_TOKEN,

  TK_SPACE,

  VAR_NUMBER, VAR_VARIABLE,

  TK_PAREN_OPEN, TK_PAREN_CLOSE,
  TK_ADD, TK_SUBTRACT, TK_MULTIPLY, TK_DIVIDE,
  TK_MODULO, TK_EXPONENCIATE, TK_SHIFT_LEFT, TK_SHIFT_RIGHT,
  TK_LOGICAL_XOR, TK_LOGICAL_AND, TK_LOGICAL_OR,

  TOKEN_TYPE_COUNT
};
}

using TokenType = tokens::TokenType;
namespace operators
{
enum Operator {
  INVALID_OPERATOR,
  ADD, SUBTRACT, MULTIPLY, DIVIDE,
  MODULO, EXPONENCIATE,
  SHIFT_LEFT, SHIFT_RIGHT,
  LOGICAL_XOR, LOGICAL_AND, LOGICAL_OR,

  OPERATOR_COUNT
};
}
using Operator = operators::Operator;

struct EvaluationContext
{
  std::optional<std::string> evaluationError;
  std::unordered_map<std::string, std::variant<eval_type, inteval_type>> variables;
};

struct Expression
{
  virtual ~Expression() = default;
  virtual eval_type eval(EvaluationContext &context) const = 0;
  virtual inteval_type evalInt(EvaluationContext &context) const = 0;
};

struct VarExp : Expression
{
  explicit VarExp(std::string variableName) : variableName(std::move(variableName)) {}

  std::string variableName;

  eval_type eval(EvaluationContext &context) const override;
  inteval_type evalInt(EvaluationContext &context) const override;
};

struct NumberExp : Expression
{
  NumberExp(eval_type value, inteval_type intValue) : value(value), intValue(intValue) {}

  eval_type value;
  inteval_type intValue;

  eval_type eval(EvaluationContext &context) const override { return value; }
  inteval_type evalInt(EvaluationContext &context) const override { return intValue; }
};

struct UnaryOpExp : Expression {
  UnaryOpExp(Operator op, std::unique_ptr<Expression> &&operand)
    : operation(op), operand(std::move(operand)) {}

  Operator operation;
  std::unique_ptr<Expression> operand;

  eval_type eval(EvaluationContext &context) const override;
  inteval_type evalInt(EvaluationContext &context) const override;
};

struct BinaryOpExp : Expression {

  BinaryOpExp(Operator op, std::unique_ptr<Expression> &&leftOperand, std::unique_ptr<Expression> &&rightOperand)
    : operation(op), leftOperand(std::move(leftOperand)), rightOperand(std::move(rightOperand)) {}

  Operator operation;
  std::unique_ptr<Expression> leftOperand;
  std::unique_ptr<Expression> rightOperand;

  eval_type eval(EvaluationContext &context) const override;
  inteval_type evalInt(EvaluationContext &context) const override;
};

struct Token
{
  size_t begin, end;
  TokenType type;
};

struct SectionToken
{
  TokenType start, end;
};

struct Section
{
  size_t start, stop;
  std::vector<Section> subsections;
  std::vector<std::pair<Operator, size_t>> operators;

  Section getSubSection(size_t begin, size_t end) const;
};

struct LexingContext
{
  std::string line;
  std::optional<std::string> lexingError;
};

struct ParsingContext {
  std::vector<Token> tokens;
  std::string line;
  std::optional<std::string> parsingError;

  std::string getText(const Token &token) const { return line.substr(token.begin, token.end - token.begin); }
};


std::vector<Token> lex(LexingContext &context);
TokenType readNonSectionToken(LexingContext &context, size_t begin, size_t end);
std::pair<TokenType, size_t> getDelimiter(const LexingContext &context, size_t start);

std::unique_ptr<Expression> parse(ParsingContext &context);
std::pair<eval_type, inteval_type> parseNumber(ParsingContext &context, const std::string &string);
Section getVisibleSection(ParsingContext &context);
std::unique_ptr<Expression> parse(ParsingContext &context, const Section &section);
std::unique_ptr<Expression> parseOperationExpression(ParsingContext &context, const Section &section);

inline eval_type eval(EvaluationContext &context, const Expression &exp) { return exp.eval(context); }
inline inteval_type evalInt(EvaluationContext &context, const Expression &exp) { return exp.evalInt(context); }

}
