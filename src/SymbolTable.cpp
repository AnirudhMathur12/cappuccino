#include "SymbolTable.h"
#include <iostream>
#include <optional>

SymbolTable::SymbolTable() { scopes.emplace_back(); }

void SymbolTable::enter_scope() { scopes.emplace_back(); }

void SymbolTable::exit_scope() {
    if (scopes.size() > 1) {
        scopes.pop_back();
    } else {
        std::cerr << "Compiler Error: Compiler is trying to exit global scope\n";
    }
}

void SymbolTable::reset_local_offset() { current_stack_offset = 0; }

void SymbolTable::dump() const {
    std::cout << "\n===== Symbol Table Dump =====" << std::endl;
    std::cout << "Total Active Scopes: " << scopes.size() << std::endl;
    std::cout << "Current Stack Height: " << current_stack_offset << std::endl;

    for (size_t i = 0; i < scopes.size(); ++i) {
        std::cout << "\n  [Scope " << i << "]" << (i == 0 ? " (Global)" : "") << std::endl;
        const auto &scope = scopes[i];

        if (scope.symbols.empty()) {
            std::cout << "    (empty)" << std::endl;
            continue;
        }

        for (const auto &entry : scope.symbols) {
            const Symbol &sym = entry.second;

            std::cout << "    - " << sym.name;

            // Print Type
            std::cout << " (" << sym.type.name << ")";

            if (sym.is_function) {
                std::cout << " [Function]";
                std::cout << " Params: (";
                for (size_t j = 0; j < sym.param_types.size(); ++j) {
                    std::cout << sym.param_types[j].name;
                    if (j < sym.param_types.size() - 1) std::cout << ", ";
                }
                std::cout << ")";
            } else {
                std::cout << " [" << (sym.is_param ? "Param" : "Var") << "]";
                std::cout << " Offset: " << sym.offset;
            }
            std::cout << std::endl;
        }
    }
    std::cout << "=============================\n" << std::endl;
}

bool SymbolTable::declare(const std::string &name, const Type &type) {
    if (scopes.empty()) return false;

    Scope &current_scope = scopes.back();

    // Redeclaration is not allowed
    if (current_scope.symbols.count(name)) return true;

    // Calculate offset
    int alignment = type.size_bytes;
    while (current_stack_offset % alignment != 0) {
        current_stack_offset++;
    }
    current_stack_offset += type.size_bytes;

    Symbol s = {.name = name, .type = type, .offset = current_stack_offset, .is_param = false, .is_function = false};
    current_scope.symbols[name] = s;

    return true;
}

bool SymbolTable::declareFunction(const std::string &name, const Type &return_type, const std::vector<Type> &param_types) {
    if (scopes.empty()) return false;

    Scope &global_scope = scopes[0];

    // Redeclaration is not allowed
    if (global_scope.symbols.count(name)) return true;

    Symbol s = {.name = name, .type = return_type, .offset = 0, .is_param = false, .is_function = true, .param_types = param_types};
    global_scope.symbols[name] = s;

    return true;
}

std::optional<Symbol> SymbolTable::lookup(const std::string &name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->symbols.find(name);
        if (found != it->symbols.end()) {
            return found->second;
        }
    }

    return std::nullopt;
}

int SymbolTable::getMaxStackSize() const { return current_stack_offset; }

void SymbolTable::reset() {
    scopes.clear();
    scopes.emplace_back();
    current_stack_offset = 0;
}
