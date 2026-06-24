# Emarald Language Specification

## Overview
Emarald is a high-performance, dynamically-typed language that combines:
- **Python's simplicity** in syntax and ease of use
- **C-like memory control** for performance optimization
- **Embedded SQL** for database operations
- **R-inspired data visualization** capabilities
- **Type hints** for optional compile-time checking and IDE support

## Design Philosophy
1. **Performance First**: Native compilation + JIT for hot paths
2. **Explicit Memory Control**: Optional manual management while maintaining GC
3. **Data-Centric**: Built-in data structures, visualization, and database integration
4. **Progressive Typing**: Runtime flexibility with optional type annotations

---

## Core Features

### 1. Basic Syntax
Emarald uses indentation-based syntax and concise block headers, similar to Python:

```emarald
# Single-line comment
"""Multi-line comment"""

# Variables (type hints optional)
name: str = "Alice"
age = 25  # Type inferred as int
scores: [f32] = [95.5, 87.2, 91.0]

# Constants
const PI = 3.14159

# Functions with optional type hints
def add(a: i32, b: i32) -> i32:
    return a + b

# Arrow functions
square = def(x): x * x
```

### 2. Memory Management
Automatic GC by default, but manual control when needed:

```emarald
// Automatic memory management (default)
data = [1, 2, 3, 4, 5]

// Manual memory control (opt-in)
@manual
memory_buffer = alloc(1024)  // Allocate 1KB
free(memory_buffer)           // Explicit deallocation

// Memory hints
@pinned  // Keep in memory
cache = expensive_computation()

@volatile  // Can be GC'd aggressively
temp_data = [1, 2, 3]
```

### 3. Type System
Optional type hints with gradual typing:

```emarald
// Primitive types
x: i8, i16, i32, i64     // Signed integers
y: u8, u16, u32, u64     // Unsigned integers
z: f32, f64              // Floats
b: bool                  // Boolean
c: char                  // Character
s: str                   // String

// Collections
numbers: [i32]           // Array
tuple_val: (str, i32)    // Tuple
dict_val: {str: i32}     // Dictionary

// Union types
result: (str | error)    // Or operator
optional: i32?           // Nullable

// Custom types
struct Point {
    x: f64
    y: f64
}

enum Color {
    Red, Green, Blue, Custom(r: u8, g: u8, b: u8)
}
```

### 4. Embedded SQL
Native database support:

```emarald
// SQL queries are first-class
db = connect("postgres://localhost/mydb")

// Query with automatic result mapping
users: [{id: i32, name: str, age: i32}] = sql {
    SELECT id, name, age FROM users WHERE age > 18
    USING db
}

// Parameterized queries (SQL injection safe)
user = sql {
    SELECT * FROM users WHERE id = ? AND status = ?
    USING db, (user_id, "active")
}

// Insert/Update/Delete
rows_affected = sql {
    INSERT INTO logs (user_id, action) VALUES (?, ?)
    USING db, (user_id, "login")
}

// Transactions
sql {
    START TRANSACTION
    UPDATE accounts SET balance = balance - ? WHERE id = ?
    UPDATE accounts SET balance = balance + ? WHERE id = ?
    COMMIT
    USING db, (amount, from_id, amount, to_id)
}
```

### 5. Data Visualization (R-inspired)
Built-in plotting and visualization:

```emarald
use visual

// Simple plot
data = [1, 2, 3, 4, 5]
plot(data)  // Line plot

// Scatter plot
x = [1, 2, 3, 4, 5]
y = [2, 4, 6, 8, 10]
scatter(x, y)

// Histogram
histogram(data, bins: 10)

// Customization
plot(data, 
    title: "Sales Over Time",
    xlabel: "Month",
    ylabel: "Revenue",
    color: "blue",
    style: "line+markers"
)

// Multi-plot
subplot(2, 2)  // 2x2 grid
plot(data1, title: "Plot 1")
plot(data2, title: "Plot 2")
plot(data3, title: "Plot 3")
plot(data4, title: "Plot 4")

// Export
save_plot("chart.png", dpi: 300)
```

### 6. Control Flow
```emarald
# If/Else
if x > 0:
    print("positive")
elif x < 0:
    print("negative")
else:
    print("zero")

# Pattern matching (better than if/else)
match x:
    0 -> print("zero")
    1 -> print("one")
    2..10 -> print("small")
    _ -> print("other")

# Loops
for i in range(10):
    print(i)

for item in collection:
    process(item)

for key, value in dictionary:
    print(f"{key}: {value}")

while condition:
    do_work()

# Loop control
break       # Exit loop
continue    # Skip iteration
```

### 7. Functions & Closures
```emarald
# Regular function
def greet(name: str) -> str:
    return f"Hello, {name}!"

# Default parameters
def power(base, exponent = 2):
    return base ** exponent

# Variable arguments
def sum(*numbers):
    total = 0
    for n in numbers:
        total = total + n
    return total

# Closures
multiply = def(factor):
    return def(x): x * factor
double = multiply(2)

# First-class functions
def map_fn(f, arr):
    result = []
    for item in arr:
        result.push(f(item))
    return result
```


### 8. Error Handling
```emarald
# Try/Catch
try:
    result = risky_operation()
except error:
    print(f"Error: {error}")

# Result type (like Rust)
def divide(a: i32, b: i32) -> (i32 | error):
    if b == 0:
        return error("Division by zero")
    return a / b

# Using result
result = divide(10, 0)
match result:
    value -> print(f"Result: {value}")
    error -> print(f"Error: {error}")
```

### 9. Modules & Imports

Emarald modules can be built-in or user-created libraries. Library files use the `.ed` extension and are imported by name.

- `.ed` is the primary extension for programs that need full manual memory control, explicit I/O, and advanced performance features.
- `.rad` is a simpler companion extension for beginner-friendly scripts. `.rad` files use automatic memory semantics, avoid manual memory annotations, and keep I/O simple so the code reads more naturally.

Example `.rad` script:

```rad
print("Hello!")
name = input("What is your name?")
print("Nice to meet you, " + name + "!")
```

```emarald
// Import module
use math
use visual
use db

// Import specific items
use math: {sqrt, pow, pi}

// Aliasing
use long_module_name as lmn

# Create module
# File: mathlib.ed
export def sqrt(x: f64) -> f64:
    return x ** 0.5

export const PI = 3.14159
```

#### Library conventions

- Module file names should match the import name, e.g. `mylib.ed` for `use mylib`
- Use `.ed` for full memory control and rich runtime behavior.
- Use `.rad` for simpler scripts that favor automatic memory and I/O handling.
- Library files live beside the importing program or in a configured library path
- `export` marks public declarations
- Non-exported declarations are private to the module

#### Using a user library

```emarald
use mathlib

print(mathlib.sqrt(9))
print(mathlib.PI)
```

#### Importing specific public symbols

```emarald
use mathlib: {sqrt, PI}

print(sqrt(9))
print(PI)
```

#### Aliasing

```emarald
use mathlib as ml

print(ml.sqrt(9))
```

### 10. Classes & OOP
```emarald
class Person:
    name: str
    age: i32
    
    def new(name: str, age: i32) -> Person:
        return Person { name, age }
    
    def greet():
        print(f"Hi, I'm {this.name}")
    
    def birthday():
        this.age = this.age + 1

person = Person.new("Alice", 30)
person.greet()
person.birthday()
```

### 11. Iterators & Comprehensions
```emarald
// List comprehension (Python-style)
squares = [x * x for x in range(10)]
even = [x for x in numbers if x % 2 == 0]

// Dictionary comprehension
word_lengths = {word: len(word) for word in words}

// Generator expression (lazy evaluation)
large_squares = (x * x for x in range(1000) if x > 500)

// Iterator protocol
def fibonacci(limit):
    a, b = 0, 1
    while a < limit:
        yield a
        a, b = b, a + b

for num in fibonacci(1000):
    print(num)
```

---

## Performance Features

### Compiler Optimization Hints
```emarald
// Tell compiler this is hot code
@hot
def process_pixel(rgb):
    return (rgb.r + rgb.g + rgb.b) / 3

# Inline this function
@inline
def get_x(point):
    return point.x

# Vectorize loop if possible
@vectorize
for i in range(10000):
    results[i] = compute(i)

// GPU computation
@gpu
def matrix_multiply(a: [[f32]], b: [[f32]]) -> [[f32]]:
    # Automatically executed on GPU if available

```

### Benchmarking
```emarald
use bench

bench.start("operation")
for i in range(1000000):
    expensive_operation()
bench.stop("operation")
# Output: operation: 234ms
```

---

## Standard Library Highlights

### String Operations
```emarald
s = "Hello, World!"
print(s.upper())        // "HELLO, WORLD!"
print(s.lower())        // "hello, world!"
print(s.contains("World"))  // true
print(s.split(","))     // ["Hello", " World!"]
print(s[0..5])          // "Hello"
```

### Array Operations
```emarald
arr = [1, 2, 3, 4, 5]
arr.push(6)             // Add element
arr.pop()               // Remove last
arr.reverse()           // In-place reverse
arr.sort()              // Sort
print(arr.len())        // 5
print(arr[1:4])         // Slice [2, 3, 4]
```

### Math Operations
```emarald
use math
print(math.sqrt(16))    // 4.0
print(math.pow(2, 8))   // 256
print(math.sin(math.pi / 2))  // 1.0
```

### File I/O
```emarald
// Read file
content = read("file.txt")

// Write file
write("output.txt", "Hello, Emarald!")

// JSON support
data = {name: "Alice", age: 30}
json_str = to_json(data)
parsed = from_json(json_str)
```

---

## Example: Complete Program

**File:** `analytics.ed`

```emarald
use visual
use db

// Connect to database
db = connect("postgres://localhost/analytics")

// Fetch data
sales: [{month: str, amount: f64}] = sql {
    SELECT month, amount FROM monthly_sales 
    WHERE year = ? 
    ORDER BY month
    USING db, (2024)
}

// Process data
months = [s.month for s in sales]
amounts = [s.amount for s in sales]

// Visualize
plot(amounts,
    title: "2024 Monthly Sales",
    xlabel: "Month",
    ylabel: "Amount ($)",
    xticks: months,
    color: "green",
    marker: "o"
)

save_plot("sales_report.png", dpi: 300)
print("Report generated successfully!")
```

---

## Roadmap

### Phase 1: Specification & Design ✓
- [x] Language syntax
- [x] Type system
- [x] Core features

### Phase 2: Lexer & Parser
- [ ] Tokenization
- [ ] AST generation
- [ ] Error reporting

### Phase 3: Interpreter
- [ ] Basic execution
- [ ] Standard library
- [ ] Database integration

### Phase 4: Compiler
- [ ] Bytecode generation
- [ ] JIT compilation
- [ ] Optimization passes

### Phase 5: Optimization
- [ ] Performance tuning
- [ ] GPU support
- [ ] Parallel execution

---

## Comparison with Python

| Feature | Python | Emarald |
|---------|--------|---------|
| Syntax | Simple | Simple + Clear |
| Performance | Interpreted (slow) | JIT Compiled (fast) |
| Memory Control | Automatic only | Automatic + Manual |
| Type Safety | Runtime | Optional compile-time hints |
| Database | Requires libraries (SQLAlchemy, etc.) | Native SQL |
| Visualization | Requires matplotlib/seaborn | Built-in `visual` module |
| Memory Usage | High overhead | Lower overhead |
| Concurrency | GIL limitation | Native threads + async |

---

## Future Extensions

1. **Async/Await**: Lightweight concurrency
2. **Macros**: Compile-time code generation
3. **Package Manager**: `emerpm` (Emarald Package Manager)
4. **IDE Support**: VSCode, Vim extensions
5. **WASM Target**: Compile to WebAssembly
6. **Mobile Bindings**: iOS/Android support
