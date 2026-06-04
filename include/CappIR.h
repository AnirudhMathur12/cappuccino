#include "Type.h"
#include "Visitor.h"

#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

enum class OPCode {
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

enum class IRType { INT, FLOAT, POINTER };

struct VirtualReg {
    uint32_t id;
    IRType irt;
    VirtualReg(uint32_t p_id) : id(p_id) {}
    bool operator==(const VirtualReg&) const = default;
};

// Variant between: VR, Int imm, Float imm, label/literals/names
using OperandValue = std::variant<VirtualReg, uint64_t, double, std::string>;

struct Operand {
    OperandValue val;

    Operand(VirtualReg reg) : val(reg) {}
    Operand(uint64_t i) : val(i) {}
    Operand(double d) : val(d) {}
    Operand(std::string s) : val(std::move(s)) {}
};

class Instruction {
    OPCode opc;
    std::optional<VirtualReg> destination;
    std::vector<Operand> sources;

  public:
    Instruction(OPCode op, std::optional<VirtualReg> dest, std::vector<Operand> srcs)
        : opc(op), destination(std::move(dest)), sources(std::move(srcs)) {}
};

class Block {
  public:
    std::string identifier;
    std::vector<Instruction> instruction_list;

    // CFG Edges
    std::vector<Block*> successors;
    std::vector<Block*> predecessors;
};

class IRFunction {
  public:
    std::string func_name;
    Type ret_type;

    std::vector<VirtualReg> parameters;

    std::vector<std::unique_ptr<Block>> blocks;

    unsigned int next_vreg_id;

    VirtualReg allocate_vreg();
};

class IRGenerator : public Visitor {
    std::vector<std::unique_ptr<IRFunction>> program;
    IRFunction* current_function;
    Block* current_block;
    std::optional<VirtualReg> current_result;

    int block_count = 0;

    // Helper
    void emit(OPCode opc, std::optional<VirtualReg> destination, std::vector<Operand> sources);
    Block* createBlock(const std::string& prefix);

  public:
    // Expressions
    void visitLiteralExpr(const LiteralExpr* expr) override;       //
    void visitIdentifierExpr(const IdentifierExpr* expr) override; //
    void visitUnaryExpr(const UnaryExpr* expr) override;
    void visitBinaryExpr(const BinaryExpr* expr) override;
    void visitGroupingExpr(const GroupingExpr* expr) override;
    void visitFunctionCallExpr(const FunctionCallExpr* expr) override;
    void visitArrayAccessExpr(const ArrayAccessExpr* expr) override;
    void visitArrayLiteralExpr(const ArrayLiteralExpr* expr) override;
    void visitPropertyAccessExpr(const PropertyAccessExpr* expr) override;

    // Statements
    void visitExprStmt(const ExprStmt* stmt) override;
    void visitVariableDeclStmt(const VariableDeclStmt* stmt) override;
    void visitBlockStmt(const BlockStmt* stmt) override;
    void visitIfStmt(const IfStmt* stmt) override;
    void visitWhileStmt(const WhileStmt* stmt) override;
    void visitForStmt(const ForStmt* stmt) override;
    void visitReturnStmt(const ReturnStmt* stmt) override;
    void visitFunctionParameterStmt(const FunctionParameterStmt* stmt) override;
    void visitFunctionDeclStmt(const FunctionDeclStmt* stmt) override;
    void visitClassDeclStmt(const ClassDeclStmt* stmt) override;
};
