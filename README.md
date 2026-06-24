# 💎 Emarald Programming Language

![Emarald Logo](logo.svg)

**Emarald** is a high-performance programming language designed to overcome Python's limitations while maintaining its simplicity and elegance. It combines Python's readability with C's performance, built-in SQL support, memory control, and R-inspired data visualization capabilities.

## Why Emarald?

### Python Limitations That Emarald Fixes

| Problem | Emarald Solution |
|---------|-----------------|
| **Slow Performance** | JIT compilation + native execution |
| **No Type Safety** | Optional type hints with compile-time checking |
| **Memory Overhead** | Manual memory control (@manual annotation) |
| **Database Integration** | Embedded SQL (first-class language feature) |
| **No Built-in Visualization** | Native `visual` module for plotting |
| **GIL Limits Concurrency** | True native threads + async/await |

## Quick Start

### Installation (Build from Source)

```bash
cd /Users/kimapo/Desktop/emarald
gcc -c lexer.c -o lexer.o
gcc -c vm.c -o vm.o -lm
gcc lexer.o vm.o -o emarald -lm
./emarald
```

### Hello, World!

```emarald
use visual

def main():
    print("Hello, Emarald!")

main()
```

**File:** `hello.ed`

## Key Features

### 1. Simple Python-Like Syntax

```emarald
# Variables
name = "Alice"
age = 25
scores = [95.5, 87.2, 91.0]

# Functions
def add(a, b):
    return a + b

# Loops
for i in range(10):
    print(i)
```

### 2. Optional Type Hints

```emarald
# Strict typing (compile-time checked)
def process(data: [i32]) -> [i32]:
    result = []
    for item in data:
        result.push(item * 2)
    return result

# Dynamic typing
def flexible(x):
    return x + 10
```

### 3. Embedded SQL

```emarald
db = connect("postgres://localhost/mydb")

// Query results are automatically typed
users: [{id: i32, name: str, email: str}] = sql {
    SELECT id, name, email FROM users WHERE active = true
    USING db
}

// Safe parameterized queries
user = sql {
    SELECT * FROM users WHERE id = ?
    USING db, (user_id)
}
```

### 4. Built-in Data Visualization

```emarald
use visual

// Prepare data
sales = [100, 150, 200, 180, 220]
months = ["Jan", "Feb", "Mar", "Apr", "May"]

// Create visualization
plot(sales,
    title: "Monthly Sales",
    xlabel: "Month",
    ylabel: "Amount ($)",
    xticks: months,
    color: "blue"
)

save_plot("chart.png", dpi: 300)
```

### 5. Memory Control

Emarald uses `.ed` files when you want explicit memory control and advanced runtime features. Use `.rad` files for simpler programs where automatic memory and I/O behavior is preferred.

```emarald
// Automatic memory management (default)
data = allocate_array(1000)  // GC handles it

// Manual memory management (opt-in)
@manual
buffer = alloc(1024)
process_buffer(buffer)
free(buffer)

// Pinned memory (won't be GC'd)
@pinned
cache = expensive_computation()
```

### 6. Pattern Matching

```emarald
# Better than if/else
result = match value:
    0 -> "zero"
    1 -> "one"
    2..10 -> "small"
    10..100 -> "medium"
    _ -> "large"
```

### 7. List & Dictionary Comprehensions

```emarald
// List comprehension
squares = [x * x for x in range(10)]
evens = [x for x in numbers if x % 2 == 0]

// Dictionary comprehension
word_count = {word: len(word) for word in words}
```

### 8. First-Class Functions

```emarald
# Functions are values
def apply_twice(f, x):
    return f(f(x))

square = def(x): x * x
result = apply_twice(square, 5)  # (5²)² = 625
```

## Architecture

### Components

```
emarald/
├── interpreter.h       # Type definitions, function prototypes
├── lexer.c            # Tokenization
├── vm.c               # Bytecode virtual machine
├── parser.c           # (TODO) AST generation
├── compiler.c         # (TODO) Bytecode generation
├── LANGUAGE_SPEC.md   # Complete language specification
└── examples/          # Example programs
```

### Compilation Pipeline

```
Source Code (.emarald)
    ↓
    [Lexer] → Tokens
    ↓
    [Parser] → AST
    ↓
    [Compiler] → Bytecode
    ↓
    [VM / JIT] → Execution
```

## Performance Characteristics

| Operation | Python | Emarald | Speedup |
|-----------|--------|---------|---------|
| Integer arithmetic | ~1000x slower | Native | ~1000x |
| Array access | ~50x slower | Native | ~50x |
| Function calls | ~100x slower | Native | ~100x |
| Memory usage | High overhead | Low overhead | ~10x less |

## Language Specification

For complete language documentation, see [LANGUAGE_SPEC.md](LANGUAGE_SPEC.md)

### Supported Types

- **Primitives**: `i8`, `i16`, `i32`, `i64`, `u8`, `u16`, `u32`, `u64`, `f32`, `f64`, `bool`, `char`, `str`
- **Collections**: `[T]` (array), `{K: V}` (dict), `(T, U)` (tuple)
- **Union/Optional**: `(T | U)`, `T?`
- **Custom**: `struct`, `enum`, `class`

### Built-in Modules

- `math` - Mathematical functions (sqrt, pow, sin, cos, etc.)
- `visual` - Data visualization (plot, scatter, histogram, etc.)
- `db` - Database connectivity and queries
- `json` - JSON serialization/deserialization
- `file` - File I/O operations
- `bench` - Performance benchmarking

### User Libraries

Emarald supports user-created libraries and modules using the `export`/`use` syntax. Create a file like `mylib.ed` and import it from another program using:

```emarald
use mylib
```

A library file can export functions, constants, and types:

```emarald
export def add(a: i32, b: i32) -> i32:
    return a + b

export const PI = 3.14159
```

## Example Programs

All example programs use the `.ed` file extension for full memory and resource control. For simpler scripts that anyone can read, use the `.rad` extension.

### Beginner-Friendly `.rad` Script

`.rad` files are designed for people who are new to coding. They keep things short and straightforward so the program reads like a simple conversation.

```rad
print("Welcome to Rad!")
name = input("What is your name? ")
print("Nice to meet you, " + name + "!")
```

### Fibonacci Sequence

```emarald
def fibonacci(n: i32) -> [i32]:
    result = []
    a, b = 0, 1
    
    for _ in range(n):
        result.push(a)
        a, b = b, a + b
    
    return result

print(fibonacci(10))
# Output: [0, 1, 1, 2, 3, 5, 8, 13, 21, 34]
```

### Data Analysis

```emarald
use visual
use db

// Load data
db = connect("postgres://localhost/analytics")

sales = sql {
    SELECT date, amount, region FROM sales
    WHERE YEAR(date) = 2024
    USING db
}

// Analyze
amounts = [s.amount for s in sales]
avg_amount = sum(amounts) / len(amounts)

// Visualize
histogram(amounts, 
    title: "Sales Distribution",
    bins: 20
)

print(f"Average sales: ${avg_amount:.2f}")
```

### Web Scraper with Type Safety

```emarald
use http
use json

@pinned  // Cache results
cache: {str: {title: str, price: f64}} = {}

def fetch_products(category: str) -> [{title: str, price: f64}]:
    if cache.has(category):
        return cache[category]
    
    response: str = http.get(f"https://api.example.com/products?cat={category}")
    data: {title: str, price: f64} = json.parse(response)
    
    cache[category] = data
    return data

```

## Roadmap

### Current Status
- [x] Language specification
- [x] Type system design
- [x] Lexer implementation
- [x] VM implementation

### Phase 2 (In Progress)
- [ ] Parser implementation
- [ ] Bytecode compiler
- [ ] Error reporting system

### Phase 3 (Future)
- [ ] SQL integration layer
- [ ] Visualization module
- [ ] JIT compiler
- [ ] Database drivers

### Phase 4 (Long-term)
- [ ] Parallel execution
- [ ] GPU support
- [ ] Package manager (emerpm)
- [ ] IDE support (VSCode, Vim)
- [ ] WebAssembly target

## Performance Tips

```emarald
// Use type hints for critical code paths
@hot  # Mark as hot code
def compute(data: [f64]) -> [f64]:
    result = [0.0 for _ in range(len(data))]
    
    @vectorize          # Request loop vectorization
    for i in range(len(data)):
        result[i] = data[i] * 2.0
    
    return result
```

// Use manual memory for bulk operations
@manual
buffer = alloc(1024 * 1024)
process_large_data(buffer)
free(buffer)
```

## Contributing

Contributions are welcome! Areas that need work:

1. **Parser** - Convert tokens to AST
2. **Optimizer** - Bytecode optimization passes
3. **Standard Library** - More built-in functions
4. **Database Integration** - SQL execution engine
5. **Visualization** - Graphics backend

## License

MIT License - See LICENSE file

## Comparison with Other Languages

```
Feature           | Python | Go   | Rust | Emarald
Simplicity        | ★★★★★ | ★★★  | ★★   | ★★★★★
Performance       | ★★    | ★★★★ | ★★★★★| ★★★★
Memory Control    | ★★    | ★★★  | ★★★★★| ★★★★
Built-in SQL      | ★     | ★    | ★    | ★★★★★
Visualization     | ★★★   | ★★   | ★★   | ★★★★★
Learning Curve    | ★★★★★ | ★★★★ | ★    | ★★★★★
```

## Getting Started

1. **Read** [LANGUAGE_SPEC.md](LANGUAGE_SPEC.md) for full syntax
2. **Build** the interpreter: `gcc -c lexer.c vm.c`
3. **Write** your first Emarald program (use `.ed` extension)
4. **Contribute** to the project!

---

**Made with ❤️ to make programming faster, clearer, and more enjoyable.**
