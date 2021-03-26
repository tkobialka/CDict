# CDict

A simple implementation of a Python-esque dictionary in C with REPL.

No nesting. Keys and values are stored as strings and integers respectively.

## Installation

```bash
gcc -o dict dict.c
```

## Run

```bash
$ ./dict
> x
{}
> x[a] = 1
> x
{ a:1 }
> x[a]   
1
> x[b] = 2
> x
{ a:1, b:2 }
> x[a] = 3
> x
{ a:3, b:2 }
> y
{}
> y[a] = 5
> y[b] = 6
> y[3] = 7
> y
{ a:5, b:6, 3:7 }
> y[a] = a              
Statement ignored.
>
```

Ctrl-C/Ctrl-Z to exit
