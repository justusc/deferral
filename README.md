# Deferral Library for C++

Deferral is a modern C++ library that provides a scope-exit API, inspired by the `defer` statement found in languages like [Go](https://go.dev/blog/defer-panic-and-recover) and [Swift](https://docs.swift.org/swift-book/documentation/the-swift-programming-language/statements/#Defer-Statement), as well as [ScopeGuard](https://youtu.be/WjTrfoiB0MQ) and [`scope_exit` TR v3](https://cplusplus.github.io/fundamentals-ts/v3.html#scopeguard). It enables developers to ensure that specific blocks of code are executed when the current scope ends, either successfully or due to an error, enhancing the management of resource cleanup and exception safety.

While there are numerous implementations already available, Deferral aims to include many of the unique features from other scope guard packages. It also includes a few minor tweeks to the implementation to improve performance. Finally, it provides a `defer` macro to match the syntax of Go and Swift.

Scope guard / scope exit has been inplemented by many times. See the links below for various implementations.

 1. [`scope_exit` reference implementation](https://github.com/PeterSommerlad/SC22WG21_Papers/tree/master/workspace/P0052_scope_exit)
 2. [ScopeGuard in Folly](https://github.com/facebook/folly/blob/main/folly/ScopeGuard.h).
 3. [Boost.ScopeExit](https://www.boost.org/doc/libs/1_82_0/libs/scope_exit/doc/html/index.html) 
 4. [Other examples](https://github.com/topics/scope-guard)

# How to Use the Library

## Requirements

Deferral's only requirement is a C++11 compatible compiler.

## Incorperating Deferral in your project

To integrate Deferral into your C++ projects, may choose include the header files from the `/include` directory in your project. Ensure your compiler supports C++11 or later, as this library utilizes modern C++ features and deduction guides for template type inference.

1. Copy the deferral.hh file into your project.
2. Add the Deferral repo as a submodual or subtree to your project.
3. CMake `find_project(deferral)`, if deferral is installed on the system


Deferral offers three main components: `DeferExit`, `DeferFail`, and `DeferSuccess`. Each of these can be used to execute code at the end of a scope depending on the exit condition:
- **DeferExit**: Executes code unconditionally at the end of a scope.
- **DeferFail**: Executes code only if the scope exits due to an exception.
- **DeferSuccess**: Executes code only if the scope exits without throwing any exception.

You can also use the `defer`, `defer_fail`, and `defer_success` macros for ease of use, which mimic the `defer` keyword from Go and Swift.

## Usage Examples

Deferral provides four different interfaces:
 - the macros
 - factory functions
 - class constructor (C++17 and later)
The macros provide the most consise API and is recommended interface. The macros closely resemple
the syntax of `defer` in other langages and keeps the code clean.

### Basic Usage

The simplest usage of Deferral is to unconditionally execute a block of code on scope exit. Below are examples of using each of Deferral interfaces to automatically close an opened file. 

1. `defer` keyword-like macro
   ```cpp
   #include "deferral.hh"
   
   void exampleFunction() {
     FILE* file = fopen("example.txt", "w");
     if (!file) return;
     
     defer { fclose(file); }; // Ensure file is closed at the end of the scope
     
     // Your code here, e.g., writing to the file
   }
   ```

2. `DEFER` macro
   ```cpp
   #include "deferral.hh"
   
   void exampleFunction() {
     FILE* file = fopen("example.txt", "w");
     if (!file) return;
     
     DEFER { fclose(file); }; // Ensure file is closed at the end of the scope
     
     // Your code here, e.g., writing to the file
   }
   ```

3. `make_defer_exit` factory function
   ```cpp
   #include "deferral.hh"
   
   void exampleFunction() {
     FILE* file = fopen("example.txt", "w");
     if (!file) return;
     
     auto d = deferral::make_defer_exit{[&]() { fclose(file); }}; // Ensure file is closed at the end of the scope
     
     // Your code here, e.g., writing to the file
   }
   ```

4. `DeferExit` class with template deduction guide (C++17 only)
   ```cpp
   #include "deferral.hh"
   
   void exampleFunction() {
     FILE* file = fopen("example.txt", "w");
     if (!file) return;
     
     auto d = deferral::DeferExit{[&]() { fclose(file); }}; // Ensure file is closed at the end of the scope
     
     // Your code here, e.g., writing to the file
   }
   ```

### Conditional Execution

Deferral includes success and failure scope guards that conditionally execute the deferred function
if an exception is or is not thrown respectively. That is, `defer_success` operation are executed
iif the the scope exits without an exception being thrown. `defer_fail` is the opposite, in a sense,
to `defer_success`, where the operation is only executed when the scope exits due to an unhandled
exception.

```cpp
#include "deferral.hh"

void conditionalExample() {
    try {
        defer_success { std::cout << "No exceptions thrown.\n"; };
        defer_fail { std::cout << "An exception was thrown.\n"; };

        // Code that might throw ...
    } catch (...) {
        // fail deferral IS executed on throw ...
        // success deferral is NOT executed on throw ...

        throw;
    }

    // Success deferral is executed on normal return, 
    // and failure deferral is NOT executed
}
```

## Disable Deferral Function

Deferral provides the `release()` function to disable the deferred operation. To use this
feature, the deferral object must have a named variable that can be referrenced in the function.
This can be done using the following:
  - `defer_(x)`, `DEFER_(x)`, `deferral::make_defer_exit()`, `DeferExit{}`
  - `defer_success_(x)`, `DEFER_SUCCESS_(x)`, `deferral::make_defer_exit()`, `DeferExit{}`
  - `defer_fail_(x)`, `DEFER_FAIL_(x)`, `deferral::make_defer_fail()`, `DeferFail{}`

The `defer_(success_|fail_|)_n` and `DEFER_(SUCCESS_|FAIL_|)_N` marcros create a variable with the
name give agrument.

```cpp
#include "deferral.hh"

std::byte* exampleFunction(int n) {
    std::byte* buf = new std::byte[n];

    // Ensure buf is cleaned up on early exit
    defer_(d) { delete [] buf; }; // Create variable `d`.
    
    // Perform some initialization that might fail.

    d.release(); // buf is not deleted after this point
    return buf;
}
```

The factory functions can be used to achive the same result.

  - `deferral::make_defer_exit()`
  - `deferral::make_defer_fail()`
  - `deferral::make_defer_success()`

```cpp
#include "deferral.hh"

std::byte* exampleFunction(int n) {
    std::byte* buf = new std::byte[n];

    // Ensure buf is cleaned up on early exit
    auto d = deferral::make_defer_exit([&]() { delete [] buf; });
    
    // Perform some initialization that might fail.

    d.release(); // buf is not deleted after this point
    return buf;
}
```

If you are using the c++17 standard, or later, you can use the class classes directly with the

```cpp
#include "deferral.hh"

std::byte* exampleFunction(int n) {
    std::byte* buf = new std::byte[n];

    // Ensure buf is cleaned up on early exit
    auto d = deferral::DeferExit{[&]() { delete [] buf; }}; // C++17 or later
    
    // Perform some initialization that might fail.

    d.release(); // buf is not deleted after this point
    return buf;
}
```

## Disable Macros and Keywords

You can disable macro genration in the code using the following macro definitions either as command line arguments or in-file macro definitions.

### Disable Macros and Keywords

1. Compiler command 
   ```shell
   $ c++ -DDEFERRAL_NO_MACROS=1 ...
   ```
2. In file
   ```c++
   #define DEFERRAL_NO_MACROS 1
   #include <deferral.hh>
   ```

### Disable keywords
1. Compiler command
   ```shell
   $ c++ -DDEFERRAL_NO_KEYWORDS=1 ...
   ```
2. In file
   ```c++
   #define DEFERRAL_NO_KEYWORDS 1
   #include <deferral.hh>
   ```
   
# API Overview

```c++
namespace deferral {

template <typename EF>
class DeferExit {

  template <typename F>
  explicit DeferExit(F&& f) noexcept(...);
  DeferExit(DeferExit&& other) noexcept(...);
  DeferExit(const DeferExit&) = delete;

  ~DeferExit() noexcept(...);

  DeferExit& operator=(const DeferExit&) = delete;
  DeferExit& operator=(DeferExit&&) = delete;

  void release() noexcept;
}; // class DeferExit


template <typename EF>
class DeferFail {

  template <typename F>
  explicit DeferFail(F&& f) noexcept(...);
  DeferFail(DeferFail&& other) noexcept(...);
  DeferFail(const DeferFail&) = delete;

  ~DeferFail() noexcept;

  DeferFail& operator=(const DeferFail&) = delete;
  DeferFail& operator=(DeferFail&&) = delete;

  void release() noexcept;
}; // class DeferFail


template <typename EF>
class DeferSuccess {

  template <typename F>
  explicit DeferSuccess(F&& f) noexcept(...);
  DeferSuccess(DeferSuccess&& other) noexcept(...);
  DeferSuccess(const DeferSuccess&) = delete;

  ~DeferSuccess() noexcept;

  DeferSuccess& operator=(const DeferSuccess&) = delete;
  DeferSuccess& operator=(DeferSuccess&&) = delete;

  void release() noexcept;
}; // class DeferSuccess


// Deduction guide, C++17 only.
#if __cplusplus >= 201703L

template <typename funcT>
DeferExit(funcT) -> DeferExit<funcT>;

template <typename funcT>
DeferFail(funcT) -> DeferFail<funcT>;

template <typename funcT>
DeferSuccess(funcT) -> DeferSuccess<funcT>;

#endif

template <typename funcT>
DEFERRAL_VISIBILITY_HIDDEN inline DeferExit<typename std::decay<funcT>::type>
make_defer_exit(funcT&& f) noexcept(...);

template <typename funcT>
inline DeferFail<typename std::decay<funcT>::type>
make_defer_fail(funcT&& f) noexcept(...);

template <typename funcT>
inline DeferSuccess<typename std::decay<funcT>::type>
make_defer_success(funcT&& f) noexcept(...);

} // namespace deferral


#if !defined(DEFERRAL_NO_MACROS)

// Macros defining the primary API
#define DEFER                        ...
#define DEFER_(variable_name)        ...
#define DEFER_FAIL                   ...
#define DEFER_FAIL_(variable_name)   ...
#define DEFER_SUCCESS                ...
#define DEFER_SUCCESS(variable_name) ...


#if !defined(DEFERRAL_NO_KEYWORDS)

// Define macros that simulate key words
#define defer_(variable_name)          DEFER_(variable_name)
#define defer                          DEFER
#define defer_fail_(variable_name)     DEFER_FAIL_(xvariable_name)
#define defer_fail                     DEFER_FAIL
#define defer_success_(variable_name)  DEFER_SUCCESS_(variable_name)
#define defer_success                  DEFER_SUCCESS

#endif

#endif

```

# Contributing

We welcome contributions to the Deferral library! Whether it's bug reports, feature requests, or pull requests, your help is valuable to us. Please feel free to contribute to making this library even better.

# License

This library is distributed under the MIT License. Feel free to use it in your projects as you see fit.

> MIT License
>
> Copyright (c) [year] [fullname]
> 
> Permission is hereby granted, free of charge, to any person obtaining a copy
> of this software and associated documentation files (the "Software"), to deal
> in the Software without restriction, including without limitation the rights
> to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
> copies of the Software, and to permit persons to whom the Software is
> furnished to do so, subject to the following conditions:
> 
> The above copyright notice and this permission notice shall be included in all
> copies or substantial portions of the Software.
> 
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
> OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
> SOFTWARE.
