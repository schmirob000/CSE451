package sys

// in gccgo, string is defined was:
//
// struct go_string {
// 	const unsigned char *data;
// 	int length;
// };
//
// we can replace two arguments (data, lenth) with a string.

//extern sys_cputs
func sys_cputs(s string)

func sprintArg(arg interface{}) string {
	switch f := arg.(type) {
	case string:
		return f
	case int:
		return Itoa(f)
	default:
		return "XXX"
	}
}


func Sprint(a ...interface{}) string {
	s := ""
	for _, arg := range a {
		s += sprintArg(arg)
	}
	return s
}

func Print(a ...interface{}) {
	s := Sprint(a...)
	sys_cputs(s)
}

func Println(a ...interface{}) {
	Print(append(a, "\n")...)
}

func Itoa(i int) string {
	var a [32]byte

	neg := i < 0
	if neg {
		i = -i
	}

	k := len(a)
	for i >= 10 {
		k--
		q := i / 10
		a[k] = byte(i - q * 10 + '0')
		i = q
	}

	k--
	a[k] = byte(i + '0')

	if (neg) {
		k--
		a[k] = '-'
	}
	return string(a[k:])
}
