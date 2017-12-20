# CMSC430 Scheme compiler

This is a final project for a compilers class, [CMSC430](http://cs.umd.edu/class/fall2017/cmsc430/), that I took at the University of Maryland. It consists of a simple Scheme compiler which supports basic features of the language and compiles to LLVM. It is written in Racket and C/C++.

This documentation is divided into three sections. The first, "building and testing," describes how to build the compiler and run the tests. The second, "features," describes the built-in types, primitive operations, and runtime exceptions that compiler supports. The third, "notes on the final project," has a short write-up about each section of the project.

## Building and testing

To build the compiler and run the provided tests, do the following:

 1. Make sure you have [Racket](http://racket-lang.org/) and [Clang](https://clang.llvm.org/) installed.
 2. Build the runtime by running `make all` in the project root directory.
 3. Test the compiler by running `racket tests.rkt all`. This should compile all files in the `tests/` directory and run them, checking against the expected result:
    ```
    $ racket tests.rkt all
    Test add-0 passed.
    Test amb passed.
    Test and-0 passed.
    Test and-or passed.
    ...
    ```
    You can also see all the available tests by running `racket tests.rkt` or run a specific test by running `racket tests.rkt <test-name>`. The compiler has been tested on Mac OS X and Linux and all tests (70 in total) passed on both platforms. It should take about 10 minutes to run them all.

## Features

Here is an overview of the basic types, primitive operations associated with those types, and runtime exceptions that the compiler supports.

### Basic types

The compiler supports a few basic types:

| Type | Description |
| --- | --- |
| void | No value. This is returned from some procedures. |
| null | Null, or the empty list. |
| boolean | Either #t or #f. |
| int | A 32-bit signed integer. |
| char | A Unicode character. |
| string | A string of Unicode characters. |
| symbol | Like a string, but any two symbols with the same name always refer to the same value. |
| cons cell | A pair of two values. Cons cells can be chained to form a linked list. |
| vector | An array of values. |
| procedure | A closure that can be called on zero or more arguments. |

### Primitive operations

These are the built-in operations that can be used to interact with the basic types.

#### Types

```
(void? v) -> boolean
(null? v) -> boolean
(cons? v) -> boolean
(number? v) -> boolean
(integer? v) -> boolean
(char? v) -> boolean
(string? v) -> boolean
(symbol? v) -> boolean
(vector? v) -> boolean
(procedure? v) -> boolean
```

These check if the given value is of the specified type. `number?` and `integer?` are equivalent.

#### Booleans

```
(eq? a b) -> boolean
(eqv? a b) -> boolean
```

These two operations are equivalent and check to see if a and b refer to the same object in memory.

```
(not v) -> boolean
```

This operation inverts the value of the boolean or other value passed. `(not #f)` returns `#t` and `(not v)` for any other `v` returns `#t`.

#### Integers

```
(= a b) -> boolean
(< a b) -> boolean
(> a b) -> boolean
(<= a b) -> boolean
(>= a b) -> boolean
```

These operations compare two integers to see if a equals b, if a is less than b, etc.

```
(+ a ...) -> integer
(* a ...) -> integer
```

Adds or multiplies the given integers and returns the result. Note that `(+)` returns 0 and `(*)` returns 1.

```
(- a b ...) -> integer
```

Takes two or more integers and subtracts from the first integer all that follow, returning the result. For instance, `(- 10 3)` returns 7 and `(- 10 4 3 2 1)` returns 0.

```
(/ a b ...) -> integer
(quotient a b ...) -> integer
```

These are equivalent. Each takes a list of integers and divides the first integer by all subsequent integers, returning the result rounded down.

#### Strings

```
(string c ...) -> string
```

Takes any number of characters and constructs a string from them in sequence.

```
(string->list s) -> list
```

Takes a string and returns a list of all the characters in the string.

```
(string-length s) -> integer
```

Takes a string and returns the number of characters in the string.

```
(string-ref s n) -> char
```

Takes a string and a non-negative integer n and returns the nth character in the string.

```
(substring s n [m]) -> string
```

Takes a string and one or two integers. A substring is taken starting at the nth character and ending before the mth character (or ending at the end of the string if m is omitted). For instance, `(substring "abcde" 1 3)` returns `"bcd"` while `(substring "abcde" 2)` returns `"cde"`.

```
(string-append s ...) -> string
```

Takes any number of strings and appends them together, returning the concatenated result.

```
(equal? s1 s2) -> boolean
```

Takes two strings and determines if they contain the same character sequence.

#### Lists and cons cells

```
(cons a d) -> cons cell
```

Returns a cons cell constructed from the values a and d.

```
(car c) -> any
(cdr c) -> any
```

These return the first and second values, respectively, stored in a cons cell.

```
(list v ...) -> list
```

This returns a linked list (made of cons cells) from the given values. Thus, `(list)` returns null, `(list v)` returns a cons cell made from `v` and null, and so on.

```
(first l) -> any
(second l) -> any
(third l) -> any
(fourth l) -> any
```

These each take a single list and return the nth value. For instance, `(fourth '(1 2 3 4))` returns 4.

```
(length l) -> integer
```

Returns the length of a list.

```
(drop l n) -> list
(take l n) -> list
```

These take a list and an integer n and split the list after the nth element. Then `drop` returns the remainder of the list while `take` returns the first n elements. For instance, `(drop '(1 2 3 4) 2)` returns `'(3 4)` and `(take '(1 2 3 4) 2)` returns `'(1 2)`.

```
(memv v l) -> list
```

This takes a value and a list and then checks if any member of the list matches the value. If so, it returns the tail of the list starting with that value. If not, it returns `#f`.

```
(map f l ...) -> list
```

Takes a procedure and one or more lists. It applies f to each element in the lists and returns a list containing the result. The first element of the return value is the result of applying f to all first elements of the given lists, and so on.

```
(append l1 l2) -> list
```

Takes two lists and appends the second to the end of the first, returning the resulting list.

```
(foldl f i l ...) -> any
(foldr f i l ...) -> any
```

These take a procedure, an initial value, and one or more lists. It applies the procedure to subsequent elements of the lists along with the accumulator value, which starts as the initial value and then is replaced by the return value of the procedure. `foldl` starts from the left side of the list, and `foldr` starts from the right side. For instance, `(foldl f i (list a b c) (list x y z))` is equivalent to `(f c z (f b y (f a x i)))`.

#### Vectors

```
(vector v ...) -> vector
```

Returns a vector created from the given values.

```
(make-vector n v) -> vector
```

Takes a non-negative integer n and a value v. Returns a vector of length n with all the elements set to v.

```
(vector-ref v n) -> vector
```

Takes a vector v and a non-negative integer n. Returns the nth element of v.

```
(vector-set! v n x) -> void
```

Takes a vector v, a non-negative integer n, and a value x. Sets the nth element of v to x.

```
(vector-length v) -> integer
```

Takes a vector and returns its length.

#### Utilities

```
(print v) -> void
```

Prints the given value to stdout using S-expression syntax.

```
(halt v)
```

Prints the given value and halts the program.

### Runtime errors

The compiler allows programs to handle a variety of runtime errors, which are summarized below. All errors are raised as a list of the format `'(error ...)`, which can be caught using a `guard` statement.

#### Incorrect arity

Applying a procedure to an invalid number of arguments will raise one of two exceptions, either:

1. `'(error too-few-arguments)` if there are not enough arguments for the procedure.
2. `'(error too-many-arguments)` if there are too many arguments for the procedure.

#### Nonprocedure application

Applying a value that is not a procedure will raise the exception `'(error nonprocedure-application)`.

#### Division by zero

Dividing by zero using `/` or `quotient` will raise the exception `'(error zero-division)`.

#### Evaluating an unitialized variable

Evaluating an unitialized binding from `letrec` or `letrec*` will raise the exception `'(error uninitialized-variable)`.

## Notes on the final project

This section contains notes specific to how the various parts of the final project for CMSC430 were implemented.

### Part 1: "piecing it together"

For this part, I created a file `compile.rkt` which contains a procedure `(compile e)` which takes an expression e and compiles it to LLVM. Arbitrary programs can by compiled an executed by running `(eval-llvm (compile ...))`.

I modified `tests.rkt` from one of the previous projects to look for tests in the `tests/` directory of the project folder. The included tests are from the given ones for the final project as well as previous projects and a number of my own tests. See the section "building and testing" for details about how to run the tests.

Finally, I wrote this documentation for the types and primitives that the compiler supports!

### Part 2: "run-time errors"

See the section "runtime errors" for the supported runtime errors. Here I have additional implementation details about how the errors are handled. Each of the classes of errors is handled by a pass of the compiler defined in `errors.rkt`.

I also wrote a number of tests for the runtime error handling:

```
too-few-arguments-0
too-few-arguments-1
too-many-arguments-0
too-many-arguments-1
nonprocedure-application-0
nonprocedure-application-1
zero-division-0
zero-division-1
uninitialized-0
uninitialized-1
```

#### Incorrect arity

These errors are handled by the pass `handle-wrong-arity`. The pass essentially wraps all lambdas in a few conditionals to check if the number of arguments is within the correct bounds; otherwise it raises an exception. This pass is run before `top-level`. The bounds are calculated as follows:

1. `(lambda x ...)` can take any number of arguments.
2. `(lambda (x y ...) ...)` must take exactly the number of arguments specified.
3. `(lambda (x y ... [z ...] ...) ...)` must take a number of arguments between the number of required arguments and the total number of required and optional arguments.
4. `(lambda (x y ... . z) ...)` must take at least the number of positional arguments.

#### Nonprocedure application

These errors are handled by the pass `handle-nonprocedure-application`, which is run immediately after `top-level`. This pass wraps all procedure applications in a conditional which uses the `procedure?` predicate to determine if the applied value is a procedure. If it isn't, it raises the exception.

#### Zero division

These arrors are handled by the pass `handle-zero-division`, which is run at the very beginning of compilation. This wraps all calls to the `/` and `quotient` prims in a conditional that makes sure that the denominator is nonzero before performing the division. If the denominator is zero, then it raises the exception.

#### Uninitialized `letrec` and `letrec*` variables

These errors are handles by the pass `handle-letrec-uninitialized`, which is run after `top-level` and before `desugar`. Handling these errors also involved establishing a special symbol in `desugar.rkt` referred to as `letrec-uninitialized-tag`. In `desugar`, `letrec`/`letrec*` variables are initialized to this symbol. Then, `handle-letrec-uninitialized` traverses the syntax tree and determines which variables could potentially be uninitialized. It wraps all references to these variables in a conditional which checks if they match `letrec-uninitialized-tag`; if they do, it raises the exception.

### Part 3: "add a feature"

I chose to implement additional string functionality (including character datums) for my added feature. My string and character types support Unicode by internally encoding using UTF-8. Strings are `char*` pointers tagged with `STR_TAG`; they point to null-terminated UTF-8 strings.

Character values are a pointer tagged with `OTHER_TAG`. The pointer refers to a struct `utf8_char` whose first member is a `u64` tag with the value `CHAR_OTHERTAG`. The second member is a `char[5]` which holds the character as a null-terminated UTF-8 string.

I implemented the following prims involving strings/chars: `string`, `string->list`, `string-length`, `string-ref`, `substring`, `string-append`, `equal?`, `string?`, and  `char?`. Each prim can be used directly or with `apply`.

I also wrote these tests to test the new functionality:

```
string-0
string-1
string-2
string-3
string-4
char-0
unicode-0
```

### Part 4: "Boehm GC"

I chose not to do this part, partially because I ran out of time (the other parts combined took about 10-12 hours), and partially because I am already doing well enough in the class that I don't think I need all the points for the final project. Hopefully, I can get credit for the first three parts as well as going above and beyond with part 3 (adding additional prims), the Unicode implementation, and GitHub upload, making my maximum potential score 20+20+30+10+8+3=91.

## Acknowledgements

Some of the compiler code is taken from [Thomas Gilray's fall 2017 CMSC430 class](http://cs.umd.edu/class/fall2017/cmsc430/).

Unicode support uses code from http://www.zedwood.com.

### Academic integrity statement

I, Cassidy Laidlaw, pledge on my honor that I have not given or received any unauthorized assistance on this assignment.
