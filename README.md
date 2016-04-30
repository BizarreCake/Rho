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
import std:streams;

var fibs = ls:cons(0, ls:cons(1, ls:map2(fun (a, b) { a + b },
  ls:relay(fun () { fibs }), ls:cdr(ls:relay(fun () { fibs })))));

print(ls:index(fibs, 100));
```

The snippet above uses the partially-implemented standard library `std:streams`
which implements lazy streams. Using lazy streams, a stream containing the
fibonacci sequence is recursively constructed, and is then used to output the
100th number in the sequence.

NOTE: To run the above snippet, be sure to copy the std directory in rholib/
to the directory containing the Rho executable!

What's to Come
--------------
In the future, a lot of symbolic programming stuff will come; along with lots
of visualization tools, so stay tuned!

