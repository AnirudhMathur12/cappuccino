#include "Type.h"
#include "Errors.h"
#include <string>

std::string kind_to_string(TypeKind tk) {
    switch (tk) {
    case TypeKind::PRIMITIVE: return "PRIMITIVE"; break;
    case TypeKind::ARRAY: return "ARRAY"; break;
    case TypeKind::POINTER: return "POINTER"; break;
    case TypeKind::STRUCT: return "STRUCT"; break;
    case TypeKind::VOID: return "VOID"; break;
    case TypeKind::SLICE: return "SLICE"; break;
    default: return "NO TYPE"; break;
    }
}

// Signed Integers

const Type TypeSystem::Int64 = {.name = "int64", .kind = TypeKind::PRIMITIVE, .size_bytes = 8, .is_signed = true, .is_float = false};

const Type TypeSystem::Int32 = {.name = "int32", .kind = TypeKind::PRIMITIVE, .size_bytes = 4, .is_signed = true, .is_float = false};

const Type TypeSystem::Int16 = {.name = "int16", .kind = TypeKind::PRIMITIVE, .size_bytes = 2, .is_signed = true, .is_float = false};

const Type TypeSystem::Int8 = {.name = "int8", .kind = TypeKind::PRIMITIVE, .size_bytes = 1, .is_signed = true, .is_float = false};

// Unsigned Integers

const Type TypeSystem::UInt64 = {.name = "uint64", .kind = TypeKind::PRIMITIVE, .size_bytes = 8, .is_signed = false, .is_float = false};

const Type TypeSystem::UInt32 = {.name = "uint32", .kind = TypeKind::PRIMITIVE, .size_bytes = 4, .is_signed = false, .is_float = false};

const Type TypeSystem::UInt16 = {.name = "uint16", .kind = TypeKind::PRIMITIVE, .size_bytes = 2, .is_signed = false, .is_float = false};

const Type TypeSystem::UInt8 = {.name = "uint8", .kind = TypeKind::PRIMITIVE, .size_bytes = 1, .is_signed = false, .is_float = false};

// Floating Point

const Type TypeSystem::Float64 = {.name = "float64", .kind = TypeKind::PRIMITIVE, .size_bytes = 8, .is_signed = true, .is_float = true};

const Type TypeSystem::Float32 = {.name = "float32", .kind = TypeKind::PRIMITIVE, .size_bytes = 4, .is_signed = true, .is_float = true};

// Special Types

const Type TypeSystem::Void = {.name = "void", .kind = TypeKind::VOID, .size_bytes = 0, .is_signed = false, .is_float = false};

// StringLiteral is treated as a pointer (8 bytes) to a uint8 (char)
const Type TypeSystem::StringLiteral = {.name = "string",
                                        .kind = TypeKind::SLICE,
                                        .size_bytes = 16,
                                        .is_signed = false,
                                        .is_float = false,
                                        .baseType = std::make_shared<Type>(TypeSystem::UInt8)};

Type TypeSystem::from_string(const std::string &typeName) {
    // Signed Ints
    if (typeName == "int64") return Int64;
    if (typeName == "int32") return Int32;
    if (typeName == "int16") return Int16;
    if (typeName == "int8") return Int8;

    // Unsigned Ints
    if (typeName == "uint64") return UInt64;
    if (typeName == "uint32") return UInt32;
    if (typeName == "uint16") return UInt16;
    if (typeName == "uint8") return UInt8;

    // Floats
    if (typeName == "float64") return Float64;
    if (typeName == "float32") return Float32;

    // Others
    if (typeName == "string") return StringLiteral;
    if (typeName == "void") return Void;

    throw TypeError("Unknown type '" + typeName + "'");
    // return Int64;
}

Type TypeSystem::createArray(const Type &base, int length) {
    return Type{.name = base.name + "[" + std::to_string(length) + "]",
                .kind = TypeKind::ARRAY,
                .size_bytes = base.size_bytes * length,
                .is_signed = false,
                .is_float = false,
                .baseType = std::make_shared<Type>(base),
                .array_length = length};
}

bool Type::operator==(const Type &other) const {
    if (kind != other.kind) return false;
    if (kind == TypeKind::PRIMITIVE) return name == other.name;
    if (kind == TypeKind::ARRAY) return (array_length == other.array_length) && (*baseType == *other.baseType);
    return *baseType == *other.baseType;
}

bool Type::operator!=(const Type &other) const { return !(*this == other); }

Type TypeSystem::createPointer(const Type &base) {
    return Type{.name = base.name + "*",
                .kind = TypeKind::POINTER,
                .size_bytes = 8, // Assuming 64-bit architecture
                .is_signed = false,
                .is_float = false,
                .baseType = std::make_shared<Type>(base),
                .array_length = 0};
}
