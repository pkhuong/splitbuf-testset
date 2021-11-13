#include "opcode.h"

std::string OpcodeName(Opcode op) {
  switch (op) {
    case Opcode::SkipN:
      return "SkipN";
    case Opcode::OneField:
      return "OneField";
    case Opcode::TwoFields:
      return "TwoFields";
    case Opcode::OpenField:
      return "OpenField";
    case Opcode::FieldClose:
      return "FieldClose";
    case Opcode::FieldSeparate:
      return "FieldSeparate";
    case Opcode::FieldN:
      return "FieldN";
  }

  return "Unknown";
}
