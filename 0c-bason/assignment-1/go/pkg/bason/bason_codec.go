package bason

import (
	"encoding/binary"
	"errors"
	"fmt"
)

// BasonType represents the BASON value type
type BasonType int

const (
	TypeBoolean BasonType = iota
	TypeArray
	TypeString
	TypeObject
	TypeNumber
)

// Record is a single BASON record (TLV with key and value or children)
type Record struct {
	Type     BasonType
	Key      string
	Value    string // For leaf types
	Children []Record
}

// BASON tag bytes
const (
	tagBooleanShort = 'b'
	tagArrayShort   = 'a'
	tagStringShort  = 's'
	tagObjectShort  = 'o'
	tagNumberShort  = 'n'

	tagBooleanLong = 'B'
	tagArrayLong   = 'A'
	tagStringLong  = 'S'
	tagObjectLong  = 'O'
	tagNumberLong  = 'N'

	maxShortLength   = 15
	maxLongKeyLength = 255
)

var (
	ErrInsufficientData = errors.New("insufficient data for BASON record")
	ErrInvalidTag       = errors.New("invalid BASON tag")
	ErrKeyTooLong       = errors.New("key length exceeds maximum (255 bytes)")
)

func getTagForType(t BasonType, shortForm bool) byte {
	if shortForm {
		switch t {
		case TypeBoolean:
			return tagBooleanShort
		case TypeArray:
			return tagArrayShort
		case TypeString:
			return tagStringShort
		case TypeObject:
			return tagObjectShort
		case TypeNumber:
			return tagNumberShort
		}
	} else {
		switch t {
		case TypeBoolean:
			return tagBooleanLong
		case TypeArray:
			return tagArrayLong
		case TypeString:
			return tagStringLong
		case TypeObject:
			return tagObjectLong
		case TypeNumber:
			return tagNumberLong
		}
	}
	panic("unknown BASON type")
}

func getTypeFromTag(tag byte) (BasonType, error) {
	switch tag {
	case tagBooleanShort, tagBooleanLong:
		return TypeBoolean, nil
	case tagArrayShort, tagArrayLong:
		return TypeArray, nil
	case tagStringShort, tagStringLong:
		return TypeString, nil
	case tagObjectShort, tagObjectLong:
		return TypeObject, nil
	case tagNumberShort, tagNumberLong:
		return TypeNumber, nil
	default:
		return 0, fmt.Errorf("%w: 0x%02x", ErrInvalidTag, tag)
	}
}

func isShortForm(tag byte) bool {
	return tag >= 'a' && tag <= 'z'
}

func isContainerType(t BasonType) bool {
	return t == TypeArray || t == TypeObject
}

func readLittleEndian32(data []byte) uint32 {
	return binary.LittleEndian.Uint32(data)
}

func encodeChildren(children []Record) ([]byte, error) {
	var result []byte
	for _, child := range children {
		enc, err := EncodeBason(child)
		if err != nil {
			return nil, err
		}
		result = append(result, enc...)
	}
	return result, nil
}

// EncodeBason encodes a record to bytes.
// The encoder uses short form when both key and value fit in 15 bytes
func EncodeBason(record Record) ([]byte, error) {
	var valueData []byte
	var err error
	if isContainerType(record.Type) {
		valueData, err = encodeChildren(record.Children)
		if err != nil {
			return nil, err
		}
	} else {
		valueData = []byte(record.Value)
	}

	keyLen := len(record.Key)
	valueLen := len(valueData)

	useShortForm := keyLen <= maxShortLength && valueLen <= maxShortLength

	var result []byte
	tag := getTagForType(record.Type, useShortForm)
	result = append(result, tag)

	if useShortForm {
		lengths := byte(keyLen<<4) | byte(valueLen)
		result = append(result, lengths)
	} else {
		if keyLen > maxLongKeyLength {
			return nil, ErrKeyTooLong
		}
		valLenBuf := make([]byte, 4)
		binary.LittleEndian.PutUint32(valLenBuf, uint32(valueLen))
		result = append(result, valLenBuf...)
		result = append(result, byte(keyLen))
	}

	result = append(result, []byte(record.Key)...)
	result = append(result, valueData...)
	return result, nil
}

// DecodeBason decodes a single record from data.
// Returns the record and the number of bytes consumed
func DecodeBason(data []byte) (Record, int, error) {
	if len(data) < 2 {
		return Record{}, 0, fmt.Errorf("%w (need at least 2 bytes)", ErrInsufficientData)
	}

	offset := 0
	tag := data[offset]
	offset++

	recordType, err := getTypeFromTag(tag)
	if err != nil {
		return Record{}, 0, err
	}
	shortForm := isShortForm(tag)

	var keyLen, valueLen int
	if shortForm {
		lengths := data[offset]
		offset++
		keyLen = int(lengths>>4) & 0x0F
		valueLen = int(lengths & 0x0F)
	} else {
		if len(data) < 6 {
			return Record{}, 0, fmt.Errorf("%w for long form", ErrInsufficientData)
		}
		valueLen = int(readLittleEndian32(data[offset:]))
		offset += 4
		keyLen = int(data[offset])
		offset++
	}

	if offset+keyLen > len(data) {
		return Record{}, 0, fmt.Errorf("%w for key", ErrInsufficientData)
	}
	record := Record{
		Type: recordType,
		Key:  string(data[offset : offset+keyLen]),
	}
	offset += keyLen

	if offset+valueLen > len(data) {
		return Record{}, 0, fmt.Errorf("%w for value", ErrInsufficientData)
	}

	if isContainerType(record.Type) {
		childOffset := 0
		for childOffset < valueLen {
			child, childSize, err := DecodeBason(data[offset+childOffset : offset+valueLen])
			if err != nil {
				return Record{}, 0, err
			}
			record.Children = append(record.Children, child)
			childOffset += childSize
		}
	} else {
		record.Value = string(data[offset : offset+valueLen])
	}
	offset += valueLen

	return record, offset, nil
}

// DecodeBasonAll decodes all records from a byte buffer (concatenated records)
func DecodeBasonAll(data []byte) ([]Record, error) {
	var records []Record
	offset := 0
	for offset < len(data) {
		record, size, err := DecodeBason(data[offset:])
		if err != nil {
			return nil, err
		}
		records = append(records, record)
		offset += size
	}
	return records, nil
}
