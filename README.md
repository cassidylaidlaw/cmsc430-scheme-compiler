# CMSC430 Scheme compiler

This is a final project for a compilers class I took at the University of Maryland. It consists of a simple Scheme compiler which supports basic features of the language and compiles to LLVM. It is written in Racket and C/C++.

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

## Features

Here is an overview of the basic types and primitive operations associated with those types that the compiler supports.

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

## How it works

## Acknowledgements

Some of the compiler code is taken from [Thomas Gilray's fall 2017 CMSC430 class](http://cs.umd.edu/class/fall2017/cmsc430/).

### Academic integrity statement

I, Cassidy Laidlaw, pledge on my honor that I have not given or received any unauthorized assistance on this assignment.
