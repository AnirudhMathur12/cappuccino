# Cappuccino Language Syntax

Cappuccino is a statically-typed, C-like programming language. This document outlines the supported syntax, data types, and control structures available in the language.

## 1. Program Structure
A `main` function is required as an entry point. Statements are executed sequentially. Much like C, a lot of the structure comes from the form it takes when it compiles down to assembly, like, requiring a main function because `_main` is required as an entry point.

```capp
// This is valid code.
int main() {
  int x = 10;
  return 0;
}
```
Statements must be terminated with a semicolon(`;`) because this is a real language.

## 2. Comments
Single-line comments are supported using `//`

```capp
// This is a comment
int x = 5; // This, too, is a comment.
```

## 3. Data Types
The language supports the following primitive data types:
|Type|Description|Example|
|:---|:----------|:------|
|`int`|64-bit signed integer|`42`,`-10`|
|`float`|64-bit floating point number|`3.14`,`-0.01`|
|`string`|String literals (enclosed in double quotes)|`"Hello World"`|
|`void`|Used for functions that do not return a value|`void func() {}`|

>**Note**
>There's no explicit boolean type. Integers are used for logic (0 is false, non-zero is true).

## 4. Variables
Variables must be declared with a type. They can be initialized at declaration or assigned later.

### Declaration & Initialization
```capp
int count = 10;
float pi = 3.14159;
string message = "Welcome to Cappuccino";
```
### Assignment
```capp
count = 20;
count = count + 5;
```
>**Note**
>Variable shadowing is supported in nested blocks.

## 5. Operators
Cappuccino supports standard arithmetic and relational operators.

### Arithmetic
|Operator|Description|
|:-------|:----------|
|`+`|Addition|
|`-`|Subtraction|
|`*`|Multiplication|
|`/`|Division|

### Arithmetic
These operators return `1`(true) or `0`(false).
|Operator|Description|
|:-------|:----------|
|`<`|Less than|
|`<=`|Less than or equal|
|`>`|Greater than|
|`>=`|Greater than or equal|
|`==`|Equality|
|`!=`|Inequality|

### Logical/Unary
|Operator|Description|
|:-------|:----------|
|`!`|Logical NOT (negates truthiness)|
|`-`|Unary negation(e.g., `-5`)|

## 6. Control Flow
Standard C-style control flow structures are available.

### If-Else
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
The `for` loop follows the standard `(initializer; condition; increment)` pattern.
```capp
for (int i = 0; i < 10; i = i + 1) {
    print(i);
}
```

>**Note**
>Since there is not `++` operator, increments must be written as assignments (e.g., `i = i + 1`).

## 7. Functions
Functions are defined with a return type, a name, parameters, and a body block.

### Definition
```capp
int add(int a, int b) {
    return a + b;
}

void greet() {
    print_s("Hello!");
}
```
### Calling Functions
```capp
int result = add(5, 10);
greet();
```

### Return Statements
- `return value;` (for typed functions)
- `return;` (for `void` functions)

## 8. Built-in Standard Library
The compiler includes a set of intrinsic functions.

### Input/Output
|Function|Signature|Description|
|:-------|:--------|:----------|
|`print`|`void print(int)`|Prints an integer followed by a newline.|
|`print_f`|`void print_f(float)`|Prints an float followed by a newline.|
|`print_s`|`void print_s(string)`|Prints an string followed by a newline.|
|`input_i`|`int input_i()`|Reads an integer from stdin.|
|`input_f`|`float input_f()`|Reads a float from stdin.|

### Math
|Function|Signature|Description|
|:-------|:--------|:----------|
|`sqrt_f`|`float sqrt_f(float)`|Square root.|
|`sin_f`|`float sin_f(float)`|Sin.|
|`cos_f`|`float cos_f(float)`|Cosine.|
|`tan_f`|`float tan_f(float)`|Tangent.|
|`abs_f`|`float abs_f(float)`|Absolute value.|

## 9. Example Code
```capp
// Calculate area of a circle
float calculate_area(float r) {
  return 3.14 * r * r;
}

int main() {
  float radius;
  print_s("Enter radius")
  radius = input_f();

  print_s("The area is:");
  print_f(calculate_area(radius));
  return 0;
}
```
