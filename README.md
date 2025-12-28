[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
![Language](https://img.shields.io/badge/language-C%2B%2B20-blue)
![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20ARM64-lightgrey)
![Build System](https://img.shields.io/badge/build-CMake-green)

# Cappuccino
A small hobby programming language compiler written in C++. Built to learn the ins and outs of a compiler because they're straight up magic.

## Overview
This project is the compiler for a small language called Cappuccino. It goes from source code ->
tokens -> AST -> actually ARM64 assembly. 
No LLVM, no IR, no giant dependencies. LLVM is great, but using it for, arguably, the hardest part of this project felt like cheating.


> **Note**
> As this compiler directly compiles to ARM64 assembly it only works on M-series MacBooks. Whether it works on other Aarch64 devices is untested as of now.

## Building

Requirements:
- C++20 or newer
- CMake
- XCode Command Line Tools (specifically `as` and `ld`)


``` bash
git clone https://github.com/AnirudhMathur12/cappuccino
cd cappuccino
mkdir build && cd build
cmake ..
make
```

You'll get a binary called `cappuccino`.

## Usage
```bash
./cappuccino <file.capp> [-o <output-binary>] [--tokens] [--ast]
```
## Syntax
The syntax on how to write code in Cappuccino is outlined [here](syntax.md).

I have attached some examples of cappuccino code in the [examples directory](examples/). 

## How it works
- The lexer turns characters into tokens
- The parser turns these tokens in an AST using a basic recursive-descent parser.
- CodeGen travels through the AST and spits out ARM64 assembly.
- The compiler invokes the system assembler (`as`) to generate an object file.
- Finally, the system linker (`ld`) is called to link against the macOS system libraries and produce the final executable.

## Benchmarks
 One of the few benchmarks I can perform in this language is calculating prime numbers upto 10 million

<img width="1914" height="956" alt="chart-4" src="https://github.com/user-attachments/assets/311775d0-e7f2-4133-98ac-f55508bbad44" />


From the perspective of a compiled language, it's not that performant. The stack machine overhead is pretty apparent, but hey, it's at least faster than Python, not that that's a big achievement.

## FAQs
### Why is called Cappuccino?
Because I like how "Java" sounds. And I like coffee.
### Why is the syntax almost exactly like C?
I just really liked C's syntax.
### Really? Because it feels like you're just lazy.
No comments.
### Why did you make this project?
I just really wanted to write a compiler ever since I saw one of those videos that explained how the LLVM compiler works. Seeing all the different parts of a compiler really made it seem like I could make one too.
