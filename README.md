[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
![Language](https://img.shields.io/badge/language-C%2B%2B20-blue)
![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20ARM64-lightgrey)
![Build System](https://img.shields.io/badge/build-CMake-green)

# Cappuccino

A small hobby programming language compiler written in C++. Built to learn the ins and outs of a compiler because they're straight up magic.

## Overview

Cappuccino is a compiler for a small statically-typed, C-like language of the same name. It goes from source code → tokens → AST → ARM64 assembly, with no LLVM, no IR, and no giant dependencies. LLVM is great, but using it for the hardest part of the project felt like cheating.

The language supports sized integer and float types, arrays, pointers, functions, and standard control flow. See [syntax.md](syntax.md) for the full language reference.

> **Note**
> This compiler targets ARM64 assembly directly and only works on Apple Silicon (M-series) Macs. Support for other AArch64 devices is untested.

## Building

**Requirements:**
- C++20 or newer
- CMake 3.12+
- Xcode Command Line Tools (provides `as` and `ld`)

```bash
git clone https://github.com/AnirudhMathur12/cappuccino
cd cappuccino
mkdir build && cd build
cmake ..
make
```

This produces a binary called `cappuccino`.

## Usage

```bash
./cappuccino <file.capp> [options]
```

| Flag | Description |
|:-----|:------------|
| `-o <output>` | Set the output binary name (default: `a.out`) |
| `--tokens` | Print the token stream and continue |
| `--till_tokens` | Print the token stream and stop |
| `--ast` | Print the AST and continue |
| `--till_ast` | Print the AST and stop |
| `--version`, `-v` | Print version information and exit |

## How It Works

1. **Lexer** — Turns source characters into a flat stream of typed tokens, handling UTF-8 input and reporting lex errors with line/column info.
2. **Parser** — Consumes tokens and builds an AST using a recursive-descent parser. Performs scope-aware symbol resolution and stack offset calculation during parsing.
3. **Code Generation** — Walks the AST via the Visitor pattern and emits ARM64 assembly. Handles type coercions, function calling conventions (up to 8 register arguments), arrays with bounds checking, and pointer dereferencing.
4. **Assembly** — The system assembler (`as`) is invoked on the generated `.s` file to produce an object file.
5. **Linking** — The system linker (`ld`) links against macOS system libraries to produce the final executable.

The standard library (I/O and math intrinsics) is embedded directly as inline assembly in `capp_stdlib.h` and appended to every compiled output.

## Examples

A few example programs live in the [`examples/`](examples/) directory:

- `fibonacci.capp` — Recursive Fibonacci
- `quadratic_formula.capp` — Solves ax² + bx + c = 0
- `arctan.capp` — Computes arctan via Taylor series

## Benchmarks

One benchmark currently available is computing all prime numbers up to 10 million.

<img width="1914" height="956" alt="chart-4" src="https://github.com/user-attachments/assets/311775d0-e7f2-4133-98ac-f55508bbad44" />

From the perspective of a compiled language, it's not that performant. The stack machine overhead is pretty apparent, but hey, it's at least faster than Python, not that that's a big achievement.

## FAQs
### Why is it called Cappuccino?
Because I like how "Java" sounds. And I like coffee.
 
### Why is the syntax almost exactly like C?
I just really liked C's syntax.
 
### Really? Because it feels like you're just lazy.
No comments.
