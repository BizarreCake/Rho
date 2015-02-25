# Rho
A bytecode compiler/interpter and REPL for a symbolic math programming language.

Disclaimer
----------
Very little of things mentioned below are implemented!
This project is still in its infancy.

Introduction
------------
Rho is a symbolic math programming language, in which a lot of mathematical objects
will be first-class objects in the language (Mathematical sets, matrices, etc...).
The Rho REPL is intended to be a sandbox-like environment to experiment with things.

Rho is a symbolic language, meaning that expressions can contain unknown variables
in them, and there will be constructs built into the language to interact with them
(simplification, differentiation, integration, solving equations, etc...)

What's Implemented
------------------
Since this is a new project, very little of the things mentioned above are currently implemented.
Here is a taste of what IS currently possible:

To compute the value of PI to a precision of 1000 digits:
```
; let pi = fun (r) { 1/(:
  2*2^(1/2)/9801 *
  sum k<-0..r {
    ((4*k)!*(1103 + 26390*k))
    /((k!)^4 * 396^(4*k))
  }
};

; N:1000 {: pi(125);
=> 3.1415926535897932384626433832795028841971693993751...
```

`(:` and `{:` are syntax for lazy people that just wrap an expression in parentheses/brackets (`{: <something>` is equivalent to `{ <something> }`).

`N:<prec> { <expr> }` computes the value of `<expr>` to a precision of `<prec>` decimal digits.

What's to Come
--------------
Rho being a symbolic math programming language, means that symbolic expressions will play a big part.
By prepending an aprostrophe `'` to an identifier, an unknown is created.

In the future, it will be possible to solve equations:
```
solve 'x { `x^2 - 16 }
=> ${ -4, 4 }
```
Differentiate:
```
D 'x { sin('x^2) }
=> 2*x*cos(x^2)
```
... and more things of that nature...
