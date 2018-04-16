package main

import "sys"

// fib returns a function that returns
// successive Fibonacci numbers.
func fib() func() int {
	a, b := 0, 1
	return func() int {
		a, b = b, a+b
		return a
	}
}

func main() {
	f := fib()
	for i := 0; i < 10; i++ {
		sys.Println(f())
	}
}
