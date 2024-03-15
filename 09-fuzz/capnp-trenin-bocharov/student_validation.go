package main

import (
	"errors"
	"fmt"
	"math"
	"unicode/utf8"
)

func isValidStr(str []byte, size int) (int, error) {
	var i int = 0
	for i < size && str[i] != 0 {
		i++
	}
	len := i
	for i < len && str[i] == 0 {
		i++
	}
	if i != len {
		return len, errors.New("invalid string")
	}
	return len, nil
}

func validateStudent(s Student) error {
	if len, err := isValidStr(s.Name[:], len(s.Name)); err != nil || !utf8.Valid(s.Name[:len]) {
		return errors.New("invalid UTF-8 in name")
	}

	if !isValidASCIIWord(s.Login[:]) {
		return errors.New("login is not ASCII")
	}

	if !isValidASCIIWord(s.Group[:]) {
		for _, b := range s.Group {
			fmt.Printf("%c ", rune(b))
		}
		return errors.New("login is not ASCII")
	}

	if !isASCII(s.Project.Repo[:]) {
		return errors.New("url is not ASCII")
	}

	if s.Project.Mark > 10 {
		return errors.New("project/mark invalid")
	}

	for _, p := range s.Practice {
		if p != 0 && p != 1 {
			return errors.New("invalid value in practice")
		}
	}

	if math.IsNaN(float64(s.Mark)) || s.Mark < 0 || s.Mark > 10 {
		return errors.New("mark invalid")
	}

	return nil
}

func isASCII(b []byte) bool {
	len, err := isValidStr(b, len(b))
	if err != nil {
		return false
	}
	for _, c := range b[:len] {
		if c > 0x7F {
			return false
		}
	}
	return true
}

func isValidASCIIWord(s []byte) bool {
	if len(s) == 0 {
		return false
	}
	len, err := isValidStr(s, len(s))
	if err != nil {
		return false
	}

	for _, b := range s[:len] {
		if !isASCIIWordChar(b) {
			return false
		}
	}
	return true
}

func isASCIIWordChar(b byte) bool {
	return (b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z') || (b >= '0' && b <= '9') || b == '_'
}
