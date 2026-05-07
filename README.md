# Kaleidoscope Front-End (C Implementation)

This project is a "from-scratch" implementation of the **Kaleidoscope** language front-end, written entirely in **C**. While the original LLVM tutorial uses C++, this version focuses on manual memory management, custom data structures, and a pure C approach to compiler design.

---

##  Features

* **Handwritten Lexer:** Custom tokenizer handling doubles, identifiers, comments, and operators.
* **Recursive Descent Parser:** Implements expression parsing with a built-in operator-precedence mechanism.
* **Reference Counting:** A manual `refcount` system to manage memory for objects and AST nodes.
* **Lightweight & Portable:** No external dependencies, just standard C libraries.

---

##  Project Structure

* **`frontend.c`**: The core engine containing the Lexer, Parser, and AST logic.
* **`Makefile`**: Build automation script.
* **`program.ks`**: A sample source file for testing Kaleidoscope syntax.

---

## Build & Execution

The project uses a **Makefile** to streamline the compilation process.

### 1. Compile the project
To build the executable, run the following command in your terminal:
```bash
make all
```

### 2. Run the compiler
Pass a source file as an argument to the generated binary:

```bash
./frontend.o program.ks
```

For a source file containing x + y, the tool outputs:

Program Content: The raw source code read from the file.

Token List: A serialized view of the tokenized input (e.g., [x+y]).

AST Visualization: A structured representation of the parsed expression (e.g., (x + y)).

### Lexer (Tokenizer)
The lexer strips whitespaces and ignores comments (starting with #). It categorizes input into:

KSOBJ_TYPE_NUMBER: Floating-point constants.

KSOBJ_TYPE_IDENTIFIER: Variable or function names.

KSOBJ_TYPE_OPERATOR: Mathematical symbols (+, -, *, /).

Note: This project is developed for educational purposes to explore compiler internals and systems programming in C.
