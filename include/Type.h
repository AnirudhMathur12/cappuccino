#ifndef CAPPUCCINO_TYPE_H
#define CAPPUCCINO_TYPE_H

#include <stdexcept>
#include <string>

class UnknownTypeError : public std::runtime_error {
  public:
    explicit UnknownTypeError(const std::string &message) : std::runtime_error(message) {}
};

enum class TypeKind {
    PRIMITIVE,
    VOID,
    POINTER,
    ARRAY,
    STRUCT // TODO: Implement a struct eventually...
};

std::string kind_to_string(TypeKind tk);

struct Type {
    std::string name;
    TypeKind kind;
    int size_bytes;

    bool is_signed = true;
    bool is_float = false;
    bool is_array = false;

    std::shared_ptr<Type> baseType;
    int array_length = 0;

    bool operator==(const Type &other) const;
    bool operator!=(const Type &other) const;
};

class TypeSystem {
  public:
    // Signed Integers
    static const Type Int64;
    static const Type Int32;
    static const Type Int16;
    static const Type Int8;
    // Unsigned Integers
    static const Type UInt64;
    static const Type UInt32;
    static const Type UInt16;
    static const Type UInt8;
    // Floats
    static const Type Float64;
    static const Type Float32;
    // Void
    static const Type Void;
    // Strint Literals
    static const Type StringLiteral;

    static Type from_string(const std::string &typeName);
    static Type createArray(const Type &base, int length);
    static Type createPointer(const Type &base);
};

#endif
