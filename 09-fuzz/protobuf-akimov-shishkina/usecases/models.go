package usecases

import "errors"

type Project struct {
	Repo [59]byte
	Mark uint8
}

type Student struct {
	Name     [32]byte
	Login    [16]byte
	Group    [8]byte
	Practice [8]uint8
	Project  Project
	Mark     float32
}

var ErrIncorrectFormat = errors.New("incorrect input")
