// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/page/cpdf_psengine.h"

#include <utility>

#include "core/fpdfapi/parser/cpdf_simple_parser.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/fx_string.h"
#include "third_party/base/logging.h"
#include "third_party/base/ptr_util.h"

namespace {

struct PDF_PSOpName {
  const char* name;
  PDF_PSOP op;
};

const PDF_PSOpName kPsOpNames[] = {
    {"add", PSOP_ADD},         {"sub", PSOP_SUB},
    {"mul", PSOP_MUL},         {"div", PSOP_DIV},
    {"idiv", PSOP_IDIV},       {"mod", PSOP_MOD},
    {"neg", PSOP_NEG},         {"abs", PSOP_ABS},
    {"ceiling", PSOP_CEILING}, {"floor", PSOP_FLOOR},
    {"round", PSOP_ROUND},     {"truncate", PSOP_TRUNCATE},
    {"sqrt", PSOP_SQRT},       {"sin", PSOP_SIN},
    {"cos", PSOP_COS},         {"atan", PSOP_ATAN},
    {"exp", PSOP_EXP},         {"ln", PSOP_LN},
    {"log", PSOP_LOG},         {"cvi", PSOP_CVI},
    {"cvr", PSOP_CVR},         {"eq", PSOP_EQ},
    {"ne", PSOP_NE},           {"gt", PSOP_GT},
    {"ge", PSOP_GE},           {"lt", PSOP_LT},
    {"le", PSOP_LE},           {"and", PSOP_AND},
    {"or", PSOP_OR},           {"xor", PSOP_XOR},
    {"not", PSOP_NOT},         {"bitshift", PSOP_BITSHIFT},
    {"true", PSOP_TRUE},       {"false", PSOP_FALSE},
    {"if", PSOP_IF},           {"ifelse", PSOP_IFELSE},
    {"pop", PSOP_POP},         {"exch", PSOP_EXCH},
    {"dup", PSOP_DUP},         {"copy", PSOP_COPY},
    {"index", PSOP_INDEX},     {"roll", PSOP_ROLL}};

}  // namespace

class CPDF_PSOP {
 public:
  explicit CPDF_PSOP(PDF_PSOP op) : m_op(op), m_value(0) {
    ASSERT(m_op != PSOP_CONST);
    ASSERT(m_op != PSOP_PROC);
  }
  explicit CPDF_PSOP(float value) : m_op(PSOP_CONST), m_value(value) {}
  CPDF_PSOP()
      : m_op(PSOP_PROC),
        m_value(0),
        m_proc(pdfium::MakeUnique<CPDF_PSProc>()) {}

  float GetFloatValue() const {
    if (m_op == PSOP_CONST)
      return m_value;

    NOTREACHED();
    return 0;
  }
  CPDF_PSProc* GetProc() const {
    if (m_op == PSOP_PROC)
      return m_proc.get();
    NOTREACHED();
    return nullptr;
  }

  PDF_PSOP GetOp() const { return m_op; }

 private:
  const PDF_PSOP m_op;
  const float m_value;
  std::unique_ptr<CPDF_PSProc> m_proc;
};

bool CPDF_PSEngine::Execute() {
  return m_MainProc.Execute(this);
}

CPDF_PSProc::CPDF_PSProc() {}
CPDF_PSProc::~CPDF_PSProc() {}

bool CPDF_PSProc::Execute(CPDF_PSEngine* pEngine) {
  for (size_t i = 0; i < m_Operators.size(); ++i) {
    const PDF_PSOP op = m_Operators[i]->GetOp();
    if (op == PSOP_PROC)
      continue;

    if (op == PSOP_CONST) {
      pEngine->Push(m_Operators[i]->GetFloatValue());
      continue;
    }

    if (op == PSOP_IF) {
      if (i == 0 || m_Operators[i - 1]->GetOp() != PSOP_PROC)
        return false;

      if (static_cast<int>(pEngine->Pop()))
        m_Operators[i - 1]->GetProc()->Execute(pEngine);
    } else if (op == PSOP_IFELSE) {
      if (i < 2 || m_Operators[i - 1]->GetOp() != PSOP_PROC ||
          m_Operators[i - 2]->GetOp() != PSOP_PROC) {
        return false;
      }
      size_t offset = static_cast<int>(pEngine->Pop()) ? 2 : 1;
      m_Operators[i - offset]->GetProc()->Execute(pEngine);
    } else {
      pEngine->DoOperator(op);
    }
  }
  return true;
}

CPDF_PSEngine::CPDF_PSEngine() : m_StackCount(0) {}

CPDF_PSEngine::~CPDF_PSEngine() {}

void CPDF_PSEngine::Push(float v) {
  if (m_StackCount < PSENGINE_STACKSIZE)
    m_Stack[m_StackCount++] = v;
}

float CPDF_PSEngine::Pop() {
  return m_StackCount > 0 ? m_Stack[--m_StackCount] : 0;
}

bool CPDF_PSEngine::Parse(const char* str, int size) {
  CPDF_SimpleParser parser(reinterpret_cast<const uint8_t*>(str), size);
  CFX_ByteStringC word = parser.GetWord();
  return word == "{" ? m_MainProc.Parse(&parser, 0) : false;
}

bool CPDF_PSProc::Parse(CPDF_SimpleParser* parser, int depth) {
  if (depth > kMaxDepth)
    return false;

  while (1) {
    CFX_ByteStringC word = parser->GetWord();
    if (word.IsEmpty())
      return false;

    if (word == "}")
      return true;

    if (word == "{") {
      m_Operators.push_back(pdfium::MakeUnique<CPDF_PSOP>());
      if (!m_Operators.back()->GetProc()->Parse(parser, depth + 1))
        return false;
      continue;
    }

    std::unique_ptr<CPDF_PSOP> op;
    for (const PDF_PSOpName& op_name : kPsOpNames) {
      if (word == CFX_ByteStringC(op_name.name)) {
        op = pdfium::MakeUnique<CPDF_PSOP>(op_name.op);
        break;
      }
    }
    if (!op)
      op = pdfium::MakeUnique<CPDF_PSOP>(FX_atof(word));
    m_Operators.push_back(std::move(op));
  }
}

bool CPDF_PSEngine::DoOperator(PDF_PSOP op) {
  int i1;
  int i2;
  float d1;
  float d2;
  FX_SAFE_INT32 result;
  switch (op) {
    case PSOP_ADD:
      d1 = Pop();
      d2 = Pop();
      Push(d1 + d2);
      break;
    case PSOP_SUB:
      d2 = Pop();
      d1 = Pop();
      Push(d1 - d2);
      break;
    case PSOP_MUL:
      d1 = Pop();
      d2 = Pop();
      Push(d1 * d2);
      break;
    case PSOP_DIV:
      d2 = Pop();
      d1 = Pop();
      Push(d1 / d2);
      break;
    case PSOP_IDIV:
      i2 = static_cast<int>(Pop());
      i1 = static_cast<int>(Pop());
      if (i2) {
        result = i1;
        result /= i2;
        Push(result.ValueOrDefault(0));
      } else {
        Push(0);
      }
      break;
    case PSOP_MOD:
      i2 = static_cast<int>(Pop());
      i1 = static_cast<int>(Pop());
      if (i2) {
        result = i1;
        result %= i2;
        Push(result.ValueOrDefault(0));
      } else {
        Push(0);
      }
      break;
    case PSOP_NEG:
      d1 = Pop();
      Push(-d1);
      break;
    case PSOP_ABS:
      d1 = Pop();
      Push((float)fabs(d1));
      break;
    case PSOP_CEILING:
      d1 = Pop();
      Push((float)ceil(d1));
      break;
    case PSOP_FLOOR:
      d1 = Pop();
      Push((float)floor(d1));
      break;
    case PSOP_ROUND:
      d1 = Pop();
      Push(FXSYS_round(d1));
      break;
    case PSOP_TRUNCATE:
      i1 = (int)Pop();
      Push(i1);
      break;
    case PSOP_SQRT:
      d1 = Pop();
      Push((float)sqrt(d1));
      break;
    case PSOP_SIN:
      d1 = Pop();
      Push((float)sin(d1 * FX_PI / 180.0f));
      break;
    case PSOP_COS:
      d1 = Pop();
      Push((float)cos(d1 * FX_PI / 180.0f));
      break;
    case PSOP_ATAN:
      d2 = Pop();
      d1 = Pop();
      d1 = (float)(atan2(d1, d2) * 180.0 / FX_PI);
      if (d1 < 0) {
        d1 += 360;
      }
      Push(d1);
      break;
    case PSOP_EXP:
      d2 = Pop();
      d1 = Pop();
      Push((float)FXSYS_pow(d1, d2));
      break;
    case PSOP_LN:
      d1 = Pop();
      Push((float)log(d1));
      break;
    case PSOP_LOG:
      d1 = Pop();
      Push((float)log10(d1));
      break;
    case PSOP_CVI:
      i1 = (int)Pop();
      Push(i1);
      break;
    case PSOP_CVR:
      break;
    case PSOP_EQ:
      d2 = Pop();
      d1 = Pop();
      Push((int)(d1 == d2));
      break;
    case PSOP_NE:
      d2 = Pop();
      d1 = Pop();
      Push((int)(d1 != d2));
      break;
    case PSOP_GT:
      d2 = Pop();
      d1 = Pop();
      Push((int)(d1 > d2));
      break;
    case PSOP_GE:
      d2 = Pop();
      d1 = Pop();
      Push((int)(d1 >= d2));
      break;
    case PSOP_LT:
      d2 = Pop();
      d1 = Pop();
      Push((int)(d1 < d2));
      break;
    case PSOP_LE:
      d2 = Pop();
      d1 = Pop();
      Push((int)(d1 <= d2));
      break;
    case PSOP_AND:
      i1 = (int)Pop();
      i2 = (int)Pop();
      Push(i1 & i2);
      break;
    case PSOP_OR:
      i1 = (int)Pop();
      i2 = (int)Pop();
      Push(i1 | i2);
      break;
    case PSOP_XOR:
      i1 = (int)Pop();
      i2 = (int)Pop();
      Push(i1 ^ i2);
      break;
    case PSOP_NOT:
      i1 = (int)Pop();
      Push((int)!i1);
      break;
    case PSOP_BITSHIFT: {
      int shift = (int)Pop();
      result = (int)Pop();
      if (shift > 0) {
        result <<= shift;
      } else {
        // Avoids unsafe negation of INT_MIN.
        FX_SAFE_INT32 safe_shift = shift;
        result >>= (-safe_shift).ValueOrDefault(0);
      }
      Push(result.ValueOrDefault(0));
      break;
    }
    case PSOP_TRUE:
      Push(1);
      break;
    case PSOP_FALSE:
      Push(0);
      break;
    case PSOP_POP:
      Pop();
      break;
    case PSOP_EXCH:
      d2 = Pop();
      d1 = Pop();
      Push(d2);
      Push(d1);
      break;
    case PSOP_DUP:
      d1 = Pop();
      Push(d1);
      Push(d1);
      break;
    case PSOP_COPY: {
      int n = static_cast<int>(Pop());
      if (n < 0 || m_StackCount + n > PSENGINE_STACKSIZE ||
          n > static_cast<int>(m_StackCount))
        break;
      for (int i = 0; i < n; i++)
        m_Stack[m_StackCount + i] = m_Stack[m_StackCount + i - n];
      m_StackCount += n;
      break;
    }
    case PSOP_INDEX: {
      int n = static_cast<int>(Pop());
      if (n < 0 || n >= static_cast<int>(m_StackCount))
        break;
      Push(m_Stack[m_StackCount - n - 1]);
      break;
    }
    case PSOP_ROLL: {
      int j = static_cast<int>(Pop());
      int n = static_cast<int>(Pop());
      if (j == 0 || n == 0 || m_StackCount == 0)
        break;
      if (n < 0 || n > static_cast<int>(m_StackCount))
        break;

      j %= n;
      if (j > 0)
        j -= n;
      auto* begin_it = std::begin(m_Stack) + m_StackCount - n;
      auto* middle_it = begin_it - j;
      auto* end_it = std::begin(m_Stack) + m_StackCount;
      std::rotate(begin_it, middle_it, end_it);
      break;
    }
    default:
      break;
  }
  return true;
}