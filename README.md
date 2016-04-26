# Rho
A bytecode compiler/interpreter and REPL for a symbolic math programming language (to be).

Disclaimer
----------
This project is still in its infancy.

Introduction
------------
Rho is supposed to be a language whose main purpose is to serve as some sort of a
sandbox in which people could experiment with things that have something to do
with math.

Although that is its main purpose, in the long run it should also be able to
function as a general purpose language.

What's Implemented
------------------
Currently only a very small subset of features is implemented.

* Module support
* A functional REPL.
* Tail recursion
* Big integers
* ... among other things.

Take a look at a snippet of Rho, which illustrates what Rho looks like at the moment:
```
; var fib = fun (n) {
    (fun (a, b, i) {
      if i >= n
        then a
        else $(b, a + b, i + 1);
    })(0, 1, 0);
  };

; fib(100);
 => 280571172992510140037611932413038677189525
```

Here `$` refers to the function being executed.

What's to Come
--------------
In the future, a lot of symbolic programming stuff will come; along with lots
of visualization tools, so stay tuned!

