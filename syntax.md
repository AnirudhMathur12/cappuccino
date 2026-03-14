# Cappuccino Language Syntax

Cappuccino is a statically-typed, C-like programming language that compiles directly to ARM64 assembly. This document is the full language reference.

> This syntax is not set in stone and as the language develops and improves more features will be added. A lot of the missing features are due to the language being in its early stages.

---

## 1. Program Structure

A `main` function is required as the entry point. All statements must be terminated with a semicolon.

```capp
uint8 main() {
    print_s("Hello, world!");
    return 0;
}
```

---

## 2. Comments

Single-line comments only, using `//`.

```capp
// This is a comment
uint32 x = 5; // This is also a comment
```

---

## 3. Data Types

Cappuccino uses explicit-width types. There is no generic `int` or `float` — you pick the size.

### Integer Types

| Type     | Width   | Signed | Range |
|:---------|:--------|:-------|:------|
| `int8`   | 8-bit   | Yes    | -128 to 127 |
| `int16`  | 16-bit  | Yes    | -32,768 to 32,767 |
| `int32`  | 32-bit  | Yes    | -2,147,483,648 to 2,147,483,647 |
| `int64`  | 64-bit  | Yes    | -9.2×10¹⁸ to 9.2×10¹⁸ |
| `uint8`  | 8-bit   | No     | 0 to 255 |
| `uint16` | 16-bit  | No     | 0 to 65,535 |
| `uint32` | 32-bit  | No     | 0 to 4,294,967,295 |
| `uint64` | 64-bit  | No     | 0 to 1.8×10¹⁹ |

### Floating Point Types

| Type      | Width  | Notes |
|:----------|:-------|:------|
| `float32` | 32-bit | Single precision |
| `float64` | 64-bit | Double precision |

### Other Types

| Type     | Description |
|:---------|:------------|
| `void`   | Used as a function return type when no value is returned |
| `string` | String literals (read-only, passed to `print_s`) |

> **Note:**
> - There is no boolean type. Integers serve as booleans — `0` is false, any non-zero value is true.
> - An actually functional string type is being worked on and will be added soon.

---

## 4. Variables

Variables must be declared with a type before use.

```capp
int32 count = 10;
float64 pi = 3.14159;
uint8 flag = 1;
```

Variables can be declared without an initializer:

```capp
float32 result;
```

Reassignment uses `=`:

```capp
count = count + 1;
```

> **Note:** Variable shadowing is supported in nested blocks.

---

## 5. Arrays

Fixed-length arrays are declared with `[length]` after the type. The length must be a literal integer.

```capp
int32[5] scores = {10, 20, 30, 40, 50};
float64[3] coords;
```

Array elements are accessed and assigned with `[]`. Index bounds are checked at runtime — an out-of-bounds access will trap with an error message.

```capp
scores[0] = 99;
int32 first = scores[0];
```

---

## 6. Pointers

Pointer types are written with `*` after the base type. Take the address of a variable with `&`, dereference with `*`.

```capp
int64 x = 42;
int64* ptr = &x;
*ptr = 100; // x is now 100
```

---

## 7. Operators

### Arithmetic

| Operator | Description    |
|:---------|:---------------|
| `+`      | Addition       |
| `-`      | Subtraction    |
| `*`      | Multiplication |
| `/`      | Division (integer division for integer types, `sdiv`/`udiv` depending on signedness) |

### Comparison

These return `1` (true) or `0` (false).

| Operator | Description           |
|:---------|:----------------------|
| `==`     | Equal                 |
| `!=`     | Not equal             |
| `<`      | Less than             |
| `<=`     | Less than or equal    |
| `>`      | Greater than          |
| `>=`     | Greater than or equal |

Signed and unsigned comparisons use the correct underlying instructions — mixing signed and unsigned types falls back to signed comparison.

### Unary

| Operator | Description                                      |
|:---------|:-------------------------------------------------|
| `-`      | Arithmetic negation                              |
| `!`      | Logical NOT — returns `1` if operand is `0`, else `0` |
| `&`      | Address-of (requires a variable identifier)      |
| `*`      | Pointer dereference                              |

### Type Coercion

Implicit coercions happen at assignment and in arithmetic:
- Integer → Float: automatic via `scvtf`
- Float → Integer: truncation via `fcvtzs`
- `float32` ↔ `float64`: widened or narrowed as needed

---

## 8. Control Flow

### If / Else

```capp
if (x > 0) {
    print(x);
} else {
    print(0);
}
```

### While Loop

```capp
while (x < 10) {
    x = x + 1;
}
```

### For Loop

Follows the standard `(initializer; condition; increment)` form.

```capp
for (uint32 i = 0; i < 10; i = i + 1) {
    print(i);
}
```

> **Note:** There is no `++` or `+=` operator. Increments must be written as full assignments.

---

## 9. Functions

### Definition

```capp
int64 add(int64 a, int64 b) {
    return a + b;
}

void greet() {
    print_s("Hello!");
}
```

### Calling

```capp
int64 result = add(5, 10);
greet();
```

### Return

- `return <expr>;` for functions with a return type
- `return;` for `void` functions

> **Note:** Functions support up to 8 parameters (matching the ARM64 register calling convention). Parameters beyond 8 are ignored.

### Recursion

Recursion is fully supported:

```capp
uint64 fib(uint64 n) {
    if (n < 2) {
        return n;
    }
    return fib(n-1) + fib(n-2);
}
```

---

## 10. Built-in Standard Library

These functions are available in every program without any import.

### I/O

| Function    | Signature                  | Description                          |
|:------------|:---------------------------|:-------------------------------------|
| `print`     | `void print(int64)`        | Prints an integer followed by a newline |
| `print_f`   | `void print_f(float64)`    | Prints a float followed by a newline |
| `print_s`   | `void print_s(string)`     | Prints a string followed by a newline |
| `input_i`   | `int64 input_i()`          | Reads a 64-bit integer from stdin    |
| `input_f`   | `float64 input_f()`        | Reads a float64 from stdin           |

### Math

All math functions take and return `float64`.

| Function  | Description     |
|:----------|:----------------|
| `sqrt_f`  | Square root     |
| `sin_f`   | Sine            |
| `cos_f`   | Cosine          |
| `tan_f`   | Tangent         |
| `abs_f`   | Absolute value  |

---

## 11. Full Example

```capp
// Solve a quadratic equation: ax^2 + bx + c = 0
void solve(float32 a, float32 b, float32 c) {
    if (a == 0.0) {
        print_s("Not a quadratic equation");
        return;
    }

    float64 D = (b * b) - (4.0 * a * c);

    if (D < 0.0) {
        print_s("No real roots");
    } else {
        float32 root1 = (-b + sqrt_f(D)) / (2.0 * a);
        float32 root2 = (-b - sqrt_f(D)) / (2.0 * a);
        print_f(root1);
        print_f(root2);
    }
}

uint8 main() {
    print_s("Solves: ax^2 + bx + c = 0");

    print_s("Enter a:");
    float32 a = input_f();
    print_s("Enter b:");
    float32 b = input_f();
    print_s("Enter c:");
    float32 c = input_f();

    solve(a, b, c);
    return 0;
}
```
