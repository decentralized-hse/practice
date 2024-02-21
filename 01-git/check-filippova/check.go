package main

import (
	"crypto/sha256"
	"encoding/hex"
	"fmt"
	"os"
	"io/ioutil"
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

func TryFindDir(file []byte, dirName string) (string, error) {
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

func ValidateFile(fileHashName string) (bool, error) {
	file, err := ioutil.ReadFile(fileHashName)
	if err != nil {
		fmt.Println("Incorrect tree, file with hash", fileHashName, "can't be read")
		return false, nil
	}
	fileHash := sha256.Sum256(file)
	if fileHashName != hex.EncodeToString(fileHash[:]) {
		fmt.Println("Incorrect tree, file hash", fileHashName, "is incorrect")
		return false, nil
	}
	return true, nil
}

func ValidateDir(dirHashName string) (bool, error) {
	file, err := ioutil.ReadFile(dirHashName)
	if err != nil {
		fmt.Println("Incorrect tree, file with hash", dirHashName, "can't be read")
		return false, nil
	}
	fileHash := sha256.Sum256(file)
	if dirHashName != hex.EncodeToString(fileHash[:]) {
		fmt.Println("Incorrect tree, file hash", dirHashName, "is incorrect")
		return false, nil
	}

	lines := strings.Split(string(file), "\n")
	for _, line := range lines {
		if line == "" {
			break
		}
		nameAndHash := strings.Split(line, "/\t")
		if len(nameAndHash) == 2 {
			// is dir
			fl, err := ValidateDir(strings.TrimSpace(nameAndHash[1]))
			if err != nil {
				return false, err
			}
			if !fl {
				return false, nil
			}
			continue
		} 
		nameAndHash = strings.Split(line, ":\t")
		if len(nameAndHash) == 2 {
			// is file
			fl, err := ValidateFile(strings.TrimSpace(nameAndHash[1]))
			if err != nil {
				return false, err
			}
			if !fl {
				return false, nil
			}
			continue
		} 
		fmt.Println("Incorrect tree, dir with hash", dirHashName, "has incorrect format")
		return false, nil
		
	}

	return true, nil

}

func ValidateTree(args *Args) error {
	hash := args.Hash
	for i := 0; i < len(args.Path); i++ {
		file, err := ioutil.ReadFile(hash)
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
			dirHashName, err := TryFindDir(file, args.Path[i])
			if err != nil {
				return err
			}
			fl, valErr := ValidateDir(dirHashName)
			if valErr != nil {
				return valErr
			}
			if fl {
				fmt.Println("Tree is correct")
			}

		} else {
			dirHashName, err := TryFindDir(file, args.Path[i])
			if err != nil {
				return err
			}
			hash = dirHashName
		}
	}
	return nil
}

func main() {
	args := os.Args
	prettyArgs, err := Validate(args)
	if err != nil {
		os.Stderr.WriteString(err.Error())
		os.Exit(1)
	}
	err = ValidateTree(prettyArgs)
	if err != nil {
		os.Stderr.WriteString(err.Error())
		os.Exit(1)
	}
}
