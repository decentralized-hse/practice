package main

import (
	"crypto/sha256"
	"encoding/hex"
	"fmt"
	"os"
	"strings"
)

type Args struct {
	Path []string
	Hash string
}
type ArgsError struct {
	Info string
}

func (s ArgsError) Error() string {
	return "Поданы неправильные аргументы на вход. " + s.Info
}

type IntegrityError struct {
	Info string
}

func (s IntegrityError) Error() string {
	return "Целостность была нарушена. " + s.Info
}

func Validate(rowArgs []string) (*Args, error) {
	if len(rowArgs) != 3 {
		return nil, ArgsError{
			Info: "Должно быть 3 аргумента",
		}
	}
	if len(rowArgs[2]) != 64 {
		return nil, ArgsError{
			Info: "Третим аргументом должен передоваться sha256 хэш",
		}
	}
	args := Args{
		Hash: rowArgs[2],
		Path: strings.Split(rowArgs[1], "/"),
	}
	return &args, nil
}

func FindAndPrintFile(args *Args) error {
	hash := args.Hash
	for i := 0; i < len(args.Path); i++ {
		file, err := os.ReadFile(hash)
		if err != nil {
			return err
		}
		fileHash := sha256.Sum256(file)
		if hash != hex.EncodeToString(fileHash[:]) {
			return IntegrityError{
				Info: hash + " Целостность Данного файла была нарушена",
			}
		}
		if i == len(args.Path)-1 {
			fileHashName, err := TryFineFile(file, args.Path[i])
			if err != nil {
				return err
			}
			err = PrintFile(fileHashName)
			if err != nil {
				return err
			}
		} else {
			dirHashName, err := TryFineDir(file, args.Path[i])
			if err != nil {
				return err
			}
			hash = dirHashName
		}
	}
	return nil
}

func PrintFile(fileName string) error {
	file, err := os.ReadFile(fileName)
	if err != nil {
		return err
	}
	lines := strings.Split(string(file), "\n")
	for _, line := range lines {
		fmt.Println(line)
	}
	return nil
}

func TryFineFile(file []byte, fileName string) (string, error) {
	lines := strings.Split(string(file), "\n")
	for _, line := range lines {
		nameAndHash := strings.Split(line, ":\t")
		if len(nameAndHash) != 2 {
			continue
		}
		name := strings.TrimSpace(nameAndHash[0])
		if name == fileName {
			return strings.TrimSpace(nameAndHash[1]), nil
		}
	}
	return "", ArgsError{
		Info: "Файл " + fileName + " не найден",
	}
}

func TryFineDir(file []byte, dirName string) (string, error) {
	lines := strings.Split(string(file), "\n")
	for _, line := range lines {
		nameAndHash := strings.Split(line, "/\t")
		if len(nameAndHash) != 2 {
			continue
		}
		name := strings.TrimSpace(nameAndHash[0])
		if name == dirName {
			return strings.TrimSpace(nameAndHash[1]), nil
		}
	}
	return "", ArgsError{
		Info: "Директория " + dirName + " не найдена",
	}
}

func main() {
	args := os.Args
	prettyArgs, err := Validate(args)
	if err != nil {
		os.Stderr.WriteString(err.Error())
		os.Exit(1)
	}
	err = FindAndPrintFile(prettyArgs)
	if err != nil {
		os.Stderr.WriteString(err.Error())
		os.Exit(1)
	}
}
