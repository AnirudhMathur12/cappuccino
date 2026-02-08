#ifndef CAPPUCCINO_SYMBOLTABLE_H
#define CAPPUCCINO_SYMBOLTABLE_H

#include "Type.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct Symbol {
    std::string name;
    Type type; // Variable Type of Function Return Type
    int offset;
    bool is_param;

    bool is_function = false;
    std::vector<Type> param_types;
};

class SymbolTable {
  public:
    SymbolTable();

    void enter_scope();
    void exit_scope();

    void dump() const;

    bool declare(const std::string &name, const Type &type);

    bool declareFunction(const std::string &name, const Type &return_type, const std::vector<Type> &param_types);

    std::optional<Symbol> lookup(const std::string &name) const;

    int getMaxStackSize() const;

    void reset();
    void reset_local_offset();

  private:
    struct Scope {
        std::unordered_map<std::string, Symbol> symbols;
    };

    std::vector<Scope> scopes;

    int current_stack_offset = 0;
};

#endif
