package main

import (
	"fmt"
	"os"
	"protobuf-akimov-shishkina-fuzzer/usecases"
	"strings"
)

func run(path string) error {
	extension := strings.Split(path, ".")[1]
	switch extension {
	case "bin":
		students, err := usecases.LoadFromBinary(path)
		if err != nil {
			return err
		}
		err = usecases.SaveToProto(students, path)
		if err != nil {
			return err
		}
	case "protobuf":
		students, err := usecases.LoadFromProto(path)
		if err != nil {
			return err
		}
		err = usecases.SaveToBinary(students, path)
		if err != nil {
			return err
		}
	default:
		fmt.Fprintln(os.Stderr, "Malformed input")
		os.Exit(-1)
	}
	return nil
}

func main() {
	if len(os.Args) != 2 {
		fmt.Fprintln(os.Stderr, "Malformed input")
		os.Exit(1)
	}

	if err := run(os.Args[1]); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}
