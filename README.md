# CForge

A small C compiler front end written in C, with a **static complexity analyzer** and an **algorithm-improvement recommender**.

Most student compilers stop at parsing. CForge goes one step further: it reads the syntax tree of a program, estimates how its running time grows, and suggests alternatives when it recognizes a known inefficient pattern.

> **Status:** work in progress. The lexer, parser, analyzer, and recommender work. Code generation is not implemented yet.

## What it does

- **Lexer and parser:** recursive-descent parser that builds an abstract syntax tree (AST) for a subset of C.
- **Complexity analyzer:** walks the AST and estimates time complexity from loop structure (`for` loops: O(1), O(n), O(log n), and nested combinations such as O(n^2) and O(n log n)).
- **Honest uncertainty:** loops it cannot analyze, such as loops that never terminate or `while` loops (not supported yet), are reported as *Unknown* instead of guessed.
- **Recommender:** recognizes the duplicate-search pattern (`arr[i] == arr[j]` inside nested loops) and suggests alternatives, listing time, memory, trade-offs, and when the suggestion does not apply.

## Example

Input (`tests/cases/array_dupes.c`):

```c
int main(void) {
    int n = 5;
    int arr[5];
    int dupes = 0;

    for (int i = 0; i < n; i++) {
        arr[i] = i * 2;
    }

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j += 1) {
            if (arr[i] == arr[j]) {
                dupes++;
            }
        }
    }

    return dupes;
}
```

Part of the output:

```
--- CForge analysis ---
Loops detected: 3
Maximum loop nesting: 2
  Loop 1 (i): O(n) - steps by a constant towards an input-dependent bound
  Loop 2 (i): O(n) - steps by a constant towards an input-dependent bound
  Loop 3 (j): O(n) - steps by a constant towards an input-dependent bound
Estimated time complexity: approximately O(n^2)

--- CForge recommendations ---
Pattern: duplicate search on array 'arr'
  Option 1: sort a copy of the array, then compare neighbours.
    Time: O(n log n)   Extra memory: O(n)
  Option 2: keep a hash set of values seen so far.
    Time: O(n) on average   Extra memory: O(n)
  Assumptions and limits:
    - CForge has NOT verified that a replacement keeps the same
      behaviour as your loop body.
```

## Build and run

Requires GCC (for example MinGW-w64 on Windows).

```
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror src/main.c src/lexer.c src/parser.c src/analyzer.c src/recommender.c -o cforge.exe
.\cforge.exe tests/cases/array_dupes.c
```

## Tests

Regression tests run CForge on sample programs and check the output:

```
powershell -ExecutionPolicy Bypass -File tests\run_tests.ps1
```

## Project layout

```
src/
  lexer.c / lexer.h            tokenizer
  parser.c / parser.h          recursive-descent parser, AST utilities
  ast.h                        AST node types
  analyzer.c / analyzer.h      loop and complexity analysis
  recommender.c / .h           pattern detection and suggestions
  main.c                       command-line driver
tests/
  cases/                       sample programs used by the tests
  run_tests.ps1                test runner
examples/                      example input programs
```

## Supported language subset

`int` variables and arrays, arithmetic, comparisons, `&&` / `||`, `if` / `else`, `for`, `while`, `return`, function calls with string literals, and `++`, `--`, `+=`, `-=`, `*=`, `/=`. Programs must consist of a single `int main(void)` function.

## Limitations

- Only `for` loops are analyzed. `while` loops are reported as Unknown.
- Any variable in a loop bound is treated as the input size `n`.
- Only one pattern is recognized so far (duplicate search).
- Recommendations are suggestions; CForge does not verify that a rewritten program behaves identically.
- No code generation yet.

## Roadmap

- Analyze `while` loops and estimate space complexity
- Semantic analysis (symbol table, undeclared variables) and multiple functions
- More recommendation patterns
- Intermediate representation, constant folding, and code generation
- GitHub Actions build and test workflow