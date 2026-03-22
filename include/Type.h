#ifndef CAPPUCCINO_TYPE_H
#define CAPPUCCINO_TYPE_H

#include <string>
#include <unordered_map>
#include <vector>

enum class TypeKind { PRIMITIVE, VOID, POINTER, ARRAY, CLASS, SLICE };

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

struct FieldInfo {
    Type type;
    size_t offset;
};

struct MethodInfo {
    Type return_type;
    std::vector<Type> param_types;
};

struct ClassTypeInfo {
    std::string name;
    size_t total_size_bytes;
    std::unordered_map<std::string, FieldInfo> fields;
};

extern std::unordered_map<std::string, ClassTypeInfo> class_registry;

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
