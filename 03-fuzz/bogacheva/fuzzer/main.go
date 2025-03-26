package main

import (
	"encoding/binary"
	"fmt"
	"log"
	"math/rand"
	"os"
	"os/exec"
)

type Project struct {
	Repo [59]byte
	Mark uint8
}

type Student struct {
	Name     [32]byte
	Login    [16]byte
	Group    [8]byte
	Practice [8]uint8
	Project
	Mark float32
}

func generateRandomBytes(n int) []byte {
	b := make([]byte, n)
	rand.Read(b)
	return b
}

func fuzzTest() {
	log.Printf("Start fuzzing")
	fileName := "test.bin"
	file, err := os.Create(fileName)
	if err != nil {
		panic(err)
	}
	defer file.Close()
	defer os.Remove(fileName)

	var students [1000]Student
	for i := range students {
		copy(students[i].Name[:], generateRandomBytes(32))
		copy(students[i].Login[:], generateRandomBytes(16))
		copy(students[i].Group[:], generateRandomBytes(8))
		for j := range students[i].Practice {
			students[i].Practice[j] = uint8(rand.Intn(256))
		}
		copy(students[i].Repo[:], generateRandomBytes(59))
		students[i].Mark = float32(rand.Float64())

		err = binary.Write(file, binary.LittleEndian, &students[i])
		if err != nil {
			panic(err)
		}
	}

	cmd := exec.Command("go", "run", "../sstable-panesh-bogacheva.go.go", fileName)
	cmd.Stderr = os.Stderr
	if err = cmd.Start(); err != nil {
		log.Fatal("Failed to start fuzzer: ", err)
	}

	err = cmd.Wait()
	if exitError, ok := err.(*exec.ExitError); ok {
		code := exitError.ExitCode()
		if code != 1 {
			fmt.Println("Fuzzing test failed: Binary expected to reject malformed input, but it did not.")
			fmt.Println(code)
		} else {
			fmt.Println("Fuzzing test passed: Binary correctly rejected malformed input.")
		}
		return
	}

	if err != nil {
		log.Fatal("Fuzzer crashed: ", err)
	}

	fmt.Println("Fuzzing test passed: Binary correctly handled malformed input.")
}

func main() {
	for i := 0; i < 10000000000; i++ {
		fuzzTest()
	}
}

/*Удаление паники: В изначальном коде при возникновении ошибки вызывалась паника (panic(err)), что приводило к нежелательному завершению работы программы. В моем коде я заменила эти вызовы паники на возврат ошибок функциями чтения и записи, с последующим контролем этих ошибок в функции main.

Устранение требования к формату файла: В изначальном коде не было проверки на соответствие формата файла, что могло привести к ошибкам при попытке прочитать файл, не удовлетворяющий ожидаемому формату. В моем коде я добавила проверки на ошибки после функций чтения, чтобы обеспечить правильное преждевременное завершение, если формат файла не поддерживается.

Обратимость преобразования данных: Исходный код не гарантировал, что преобразование данных в обе стороны (из бинарного в SSTable и обратно) будет порождать идентичные данные. В моем коде, методы записи и чтения обеспечивают обратимость этого преобразования, за исключением того, что порядок студентов в исходном и конечном массиве может отличаться.*/
