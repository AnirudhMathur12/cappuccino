#include "Type.h"

#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

enum class OPCodes {
    // Memory and Transfer
    LOAD_CONST,
    LOAD_LOCAL,
    STORE_LOCAL,
    GET_LOCAL_ADDR,
    LOAD_PTR,
    STORE_PTR,

    // Integer Arithmetic & Logic
    ADD,
    SUB,
    MUL,
    SDIV,
    UDIV,
    NEG,
    NOT, // Logical NOT

    // Floating Arithmetic
    FADD,
    FSUB,
    FMUL,
    FDIV,
    FNEG,

    // Comparisions
    CMP_EQ,
    CMP_NE,

    // Signed Integer
    CMP_LT,
    CMP_LE,
    CMP_GT,
    CMP_GE,

    // Unsigned Integer
    CMP_ULT,
    CMP_ULE,
    CMP_UGT,
    CMP_UGE,

    // Floating Point
    FCMP_EQ,
    FCMP_NE,
    FCMP_LT,
    FCMP_LE,
    FCMP_GT,
    FCMP_GE,

    // Control Flow
    JMP,
    JMP_IF_FALSE,
    CALL,
    RET,

    // Casting
    I2F,
    F2I,
    FEXT,   // Float32 to Float64 upcast
    FTRUNC, // Float64 to Float32 downcast
    SEXT,   // Sign extend, not sexting
    ZEXT,   // Zero extend

    // Safety
    BOUNDS_CHECK
};

struct VirtualReg {
    uint32_t id;
    VirtualReg(uint32_t p_id) : id(p_id) {}
    bool operator==(const VirtualReg&) const = default;
};

// Variant between: VR, Int imm, Float imm, label/literals/names
using OperandValue = std::variant<VirtualReg, uint64_t, double, std::string>;

struct Operand {
    OperandValue val;
};

class Instruction {
    OPCodes opc;
    std::optional<VirtualReg> destination;
    std::vector<OperandValue> sources;
};

class Block {
    std::string identifier;
    std::vector<Instruction> instruction_list;

    // CFG Edges
    std::vector<Block*> successors;
    std::vector<Block*> predecessors;
};

class IRFunction {
    std::string func_name;
    Type ret_type;

    std::vector<VirtualReg> parameters;

    std::vector<std::unique_ptr<Block>> blocks;

    unsigned int next_vreg_id;

    VirtualReg allocate_vreg();
};
