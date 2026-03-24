package main

import (
	"fmt"
	"os"
	"strconv"

	"github.com/LevAlimpiev/hse-dec-sys-hw3/go/pkg/bason"
)

func printUsage(prog string) {
	fmt.Fprintf(os.Stderr, "Usage: %s [options] <file>\n\n", prog)
	fmt.Fprintf(os.Stderr, "Options:\n")
	fmt.Fprintf(os.Stderr, "  --hex           Show hex dump with annotations\n")
	fmt.Fprintf(os.Stderr, "  --json          Convert to JSON\n")
	fmt.Fprintf(os.Stderr, "  --validate=N    Validate with strictness mask N (hex)\n")
	fmt.Fprintf(os.Stderr, "  --flatten       Convert nested to flat\n")
	fmt.Fprintf(os.Stderr, "  --unflatten     Convert flat to nested\n")
	fmt.Fprintf(os.Stderr, "  --help          Show this help\n")
}

func typeName(t bason.BasonType) string {
	switch t {
	case bason.TypeBoolean:
		return "Boolean"
	case bason.TypeArray:
		return "Array"
	case bason.TypeString:
		return "String"
	case bason.TypeObject:
		return "Object"
	case bason.TypeNumber:
		return "Number"
	default:
		return "?"
	}
}

func printHexDump(data []byte) {
	fmt.Println("=== Hex Dump ===")
	offset := 0
	for offset < len(data) {
		record, size, err := bason.DecodeBason(data[offset:])
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error at offset %d: %v\n", offset, err)
			break
		}
		fmt.Printf("%08x: ", offset)
		end := offset + size
		if end > offset+32 {
			end = offset + 32
		}
		for i := offset; i < end; i++ {
			fmt.Printf("%02x ", data[i])
		}
		if size > 32 {
			fmt.Print("...")
		}
		fmt.Println()
		fmt.Printf("  Type: %s, Key: %q, Value: %q, Size: %d bytes\n\n", typeName(record.Type), record.Key, record.Value, size)
		offset += size
	}
}

func main() {
	var filename string
	var showHex, convertJSON, doFlatten, doUnflatten, doValidate bool
	var validateMask uint64

	for i := 1; i < len(os.Args); i++ {
		arg := os.Args[i]
		switch arg {
		case "--help":
			printUsage(os.Args[0])
			os.Exit(0)
		case "--hex":
			showHex = true
		case "--json":
			convertJSON = true
		case "--flatten":
			doFlatten = true
		case "--unflatten":
			doUnflatten = true
		default:
			if len(arg) > 11 && arg[:11] == "--validate=" {
				doValidate = true
				var err error
				validateMask, err = strconv.ParseUint(arg[11:], 16, 16)
				if err != nil {
					fmt.Fprintf(os.Stderr, "Invalid validate mask: %v\n", err)
					os.Exit(1)
				}
			} else if arg[0] != '-' {
				filename = arg
			} else {
				fmt.Fprintf(os.Stderr, "Unknown option: %s\n", arg)
				printUsage(os.Args[0])
				os.Exit(1)
			}
		}
	}

	if filename == "" {
		fmt.Fprintf(os.Stderr, "Error: No input file specified\n")
		printUsage(os.Args[0])
		os.Exit(1)
	}

	data, err := os.ReadFile(filename)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error: Cannot open file: %v\n", err)
		os.Exit(1)
	}
	if len(data) == 0 {
		fmt.Fprintf(os.Stderr, "Error: File is empty\n")
		os.Exit(1)
	}

	switch {
	case showHex:
		printHexDump(data)
	case convertJSON:
		jsonStr, err := bason.BasonToJson(data)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error: %v\n", err)
			os.Exit(1)
		}
		fmt.Println(jsonStr)
	case doFlatten:
		records, err := bason.DecodeBasonAll(data)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error: %v\n", err)
			os.Exit(1)
		}
		for _, record := range records {
			flat := bason.FlattenBason(record)
			for _, fr := range flat {
				enc, err := bason.EncodeBason(fr)
				if err != nil {
					fmt.Fprintf(os.Stderr, "Error: %v\n", err)
					os.Exit(1)
				}
				os.Stdout.Write(enc)
			}
		}
	case doUnflatten:
		records, err := bason.DecodeBasonAll(data)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error: %v\n", err)
			os.Exit(1)
		}
		nested := bason.UnflattenBason(records)
		enc, err := bason.EncodeBason(nested)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error: %v\n", err)
			os.Exit(1)
		}
		os.Stdout.Write(enc)
	case doValidate:
		records, err := bason.DecodeBasonAll(data)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error: %v\n", err)
			os.Exit(1)
		}
		allValid := true
		for _, record := range records {
			if !bason.ValidateBason(record, uint16(validateMask)) {
				allValid = false
				fmt.Fprintf(os.Stderr, "Validation failed for record with key: %q\n", record.Key)
			}
		}
		if allValid {
			fmt.Println("All records valid")
			os.Exit(0)
		}
		os.Exit(1)
	default:
		records, err := bason.DecodeBasonAll(data)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error: %v\n", err)
			os.Exit(1)
		}
		fmt.Printf("Found %d records\n", len(records))
		for i, record := range records {
			fmt.Printf("Record %d: key=%q\n", i, record.Key)
		}
	}
}
