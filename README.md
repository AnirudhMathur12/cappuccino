# Cappuccino
A small hobby programming language compiler written in C++. Built to learn the ins and outs of a compiler becuase they're straight up magic.

## Overview
This project is the compiler for a small language called Cappuccino. It goes from source code ->
tokens -> AST -> actualy ARM64 assembly. 
No LLVM, no IR, no giant dependencies. LLVM is great, but using it for, arguably, the hardest part of this project felt like cheating.


> **Note**
> As this compiler directly compiles to ARM64 assembly it only works on M-series MacBooks. Whether it works on other Aarch64 devices is untested as of now.

## Building

Requirements:
- C++20 or newer
- CMake
- Clang (yes it's not dependant on LLVM IR but it still uses the assembler)

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

## Writing code in Cappuccino
Cappuccino's syntax is simple and familiar if you know C or C-like langauges. Every statement needs to be terminated with a semicolo, like a real langauge. 
### Variables
```capp
int x = 10;
float pi = 3.14;
string name = "hi!"
```

### Expressions & Operators
```capp
int result = x * (y + 5);
```

### Control Flow
```capp
if(x > 0) {
  print(x);
} else {
  print(0);
}
```

### Functions
```capp
float abs(float x) {
  if(x < 0.0) {
    return -x;
  }
  return x;
}
```

### Loops
```capp
while(i < 10) {
  i = i - 2;
}
```

I have attached some examples in the examples directory. 

## How it works
- The lexer turns characters into tokens
- The parser turns these tokens in an AST using a basic recursive-descent parser.
- CodeGen travels through the AST and spits out ARM64 assembly.
- The clang assembler + linked take over and give an executable.

## FAQs
### Why is called Cappuccino?
Because I like how "Java" sounds. And I like coffee.
### Why is the syntax almost exactly like C?
I just really liked C's syntax.
### Why did you make this project?
I just really wanted to write a compiler ever since I saw one of those videos that explained how the LLVM compiler works. Seeing all the different parts of a compiler really made it seem like I could make one too.
### Are these questions really frequently asked?
No, I just wanted to dicuss these points.
