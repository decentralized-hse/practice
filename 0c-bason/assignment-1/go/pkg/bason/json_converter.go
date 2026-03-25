package bason

import (
	"bytes"
	"errors"
	"unicode"
)

var (
	ErrUnexpectedEnd = errors.New("unexpected end of JSON")
	ErrExpectedChar  = errors.New("expected character")
	ErrInvalidEscape = errors.New("invalid escape sequence")
	ErrUnicodeEscape = errors.New("unicode escapes not fully supported")
	ErrInvalidNumber = errors.New("invalid number")
	ErrInvalidLiteral = errors.New("invalid literal")
	ErrUnexpectedChar = errors.New("unexpected character in JSON")
)

type jsonParser struct {
	json []byte
	pos  int
}

func newJSONParser(json string) *jsonParser {
	return &jsonParser{json: []byte(json)}
}

func (p *jsonParser) skipWhitespace() {
	for p.pos < len(p.json) && unicode.IsSpace(rune(p.json[p.pos])) {
		p.pos++
	}
}

func (p *jsonParser) peek() (byte, error) {
	p.skipWhitespace()
	if p.pos >= len(p.json) {
		return 0, ErrUnexpectedEnd
	}
	return p.json[p.pos], nil
}

func (p *jsonParser) next() (byte, error) {
	p.skipWhitespace()
	if p.pos >= len(p.json) {
		return 0, ErrUnexpectedEnd
	}
	b := p.json[p.pos]
	p.pos++
	return b, nil
}

func (p *jsonParser) expect(ch byte) error {
	b, err := p.next()
	if err != nil {
		return err
	}
	if b != ch {
		return ErrExpectedChar
	}
	return nil
}

func (p *jsonParser) parseString() (string, error) {
	if err := p.expect('"'); err != nil {
		return "", err
	}
	var result []byte
	for p.pos < len(p.json) && p.json[p.pos] != '"' {
		if p.json[p.pos] == '\\' {
			p.pos++
			if p.pos >= len(p.json) {
				return "", ErrInvalidEscape
			}
			switch p.json[p.pos] {
			case '"':
				result = append(result, '"')
			case '\\':
				result = append(result, '\\')
			case '/':
				result = append(result, '/')
			case 'b':
				result = append(result, '\b')
			case 'f':
				result = append(result, '\f')
			case 'n':
				result = append(result, '\n')
			case 'r':
				result = append(result, '\r')
			case 't':
				result = append(result, '\t')
			case 'u':
				return "", ErrUnicodeEscape
			default:
				return "", ErrInvalidEscape
			}
			p.pos++
		} else {
			result = append(result, p.json[p.pos])
			p.pos++
		}
	}
	if err := p.expect('"'); err != nil {
		return "", err
	}
	return string(result), nil
}

func (p *jsonParser) parseNumber() (string, error) {
	start := p.pos
	if p.json[p.pos] == '-' {
		p.pos++
	}
	if p.pos >= len(p.json) || !isDigit(p.json[p.pos]) {
		return "", ErrInvalidNumber
	}
	for p.pos < len(p.json) {
		b := p.json[p.pos]
		if isDigit(b) || b == '.' || b == 'e' || b == 'E' || b == '+' || b == '-' {
			p.pos++
		} else {
			break
		}
	}
	return string(p.json[start:p.pos]), nil
}

func (p *jsonParser) parseValue(key string) (Record, error) {
	ch, err := p.peek()
	if err != nil {
		return Record{}, err
	}
	switch ch {
	case '"':
		s, err := p.parseString()
		if err != nil {
			return Record{}, err
		}
		return Record{Type: TypeString, Key: key, Value: s}, nil
	case '{':
		return p.parseObject(key)
	case '[':
		return p.parseArray(key)
	case 't', 'f', 'n':
		return p.parseLiteral(key)
	case '-':
		fallthrough
	default:
		if isDigit(ch) {
			n, err := p.parseNumber()
			if err != nil {
				return Record{}, err
			}
			return Record{Type: TypeNumber, Key: key, Value: n}, nil
		}
		return Record{}, ErrUnexpectedChar
	}
}

func (p *jsonParser) parseLiteral(key string) (Record, error) {
	if p.pos+4 <= len(p.json) && bytes.Equal(p.json[p.pos:p.pos+4], []byte("true")) {
		p.pos += 4
		return Record{Type: TypeBoolean, Key: key, Value: "true"}, nil
	}
	if p.pos+5 <= len(p.json) && bytes.Equal(p.json[p.pos:p.pos+5], []byte("false")) {
		p.pos += 5
		return Record{Type: TypeBoolean, Key: key, Value: "false"}, nil
	}
	if p.pos+4 <= len(p.json) && bytes.Equal(p.json[p.pos:p.pos+4], []byte("null")) {
		p.pos += 4
		return Record{Type: TypeBoolean, Key: key, Value: ""}, nil
	}
	return Record{}, ErrInvalidLiteral
}

func (p *jsonParser) parseObject(key string) (Record, error) {
	record := Record{Type: TypeObject, Key: key}
	if err := p.expect('{'); err != nil {
		return Record{}, err
	}
	p.skipWhitespace()
	ch, _ := p.peek()
	if ch == '}' {
		p.next()
		return record, nil
	}
	for {
		fieldName, err := p.parseString()
		if err != nil {
			return Record{}, err
		}
		if err := p.expect(':'); err != nil {
			return Record{}, err
		}
		child, err := p.parseValue(fieldName)
		if err != nil {
			return Record{}, err
		}
		record.Children = append(record.Children, child)
		p.skipWhitespace()
		ch, _ = p.peek()
		if ch == '}' {
			p.next()
			return record, nil
		}
		if err := p.expect(','); err != nil {
			return Record{}, err
		}
	}
}

func (p *jsonParser) parseArray(key string) (Record, error) {
	record := Record{Type: TypeArray, Key: key}
	if err := p.expect('['); err != nil {
		return Record{}, err
	}
	p.skipWhitespace()
	ch, _ := p.peek()
	if ch == ']' {
		p.next()
		return record, nil
	}
	index := uint64(0)
	for {
		indexKey := EncodeRon64(index)
		index++
		child, err := p.parseValue(indexKey)
		if err != nil {
			return Record{}, err
		}
		record.Children = append(record.Children, child)
		p.skipWhitespace()
		ch, _ = p.peek()
		if ch == ']' {
			p.next()
			return record, nil
		}
		if err := p.expect(','); err != nil {
			return Record{}, err
		}
	}
}

func (p *jsonParser) parse() (Record, error) {
	p.skipWhitespace()
	return p.parseValue("")
}

func recordToJSON(record Record, out *bytes.Buffer) {
	switch record.Type {
	case TypeString:
		out.WriteByte('"')
		for i := 0; i < len(record.Value); i++ {
			ch := record.Value[i]
			switch ch {
			case '"':
				out.WriteString(`\"`)
			case '\\':
				out.WriteString(`\\`)
			case '\b':
				out.WriteString(`\b`)
			case '\f':
				out.WriteString(`\f`)
			case '\n':
				out.WriteString(`\n`)
			case '\r':
				out.WriteString(`\r`)
			case '\t':
				out.WriteString(`\t`)
			default:
				if ch < 32 {
					out.WriteString(`\u00`)
					out.WriteByte("0123456789abcdef"[ch>>4])
					out.WriteByte("0123456789abcdef"[ch&0x0f])
				} else {
					out.WriteByte(ch)
				}
			}
		}
		out.WriteByte('"')
	case TypeNumber:
		out.WriteString(record.Value)
	case TypeBoolean:
		if record.Value == "" {
			out.WriteString("null")
		} else {
			out.WriteString(record.Value)
		}
	case TypeObject:
		out.WriteByte('{')
		for i, child := range record.Children {
			if i > 0 {
				out.WriteByte(',')
			}
			out.WriteByte('"')
			out.WriteString(child.Key)
			out.WriteString(`":`)
			recordToJSON(child, out)
		}
		out.WriteByte('}')
	case TypeArray:
		out.WriteByte('[')
		for i, child := range record.Children {
			if i > 0 {
				out.WriteByte(',')
			}
			recordToJSON(child, out)
		}
		out.WriteByte(']')
	}
}

// JsonToBason converts JSON text to a BASON byte stream (nested mode).
func JsonToBason(json string) ([]byte, error) {
	parser := newJSONParser(json)
	record, err := parser.parse()
	if err != nil {
		return nil, err
	}
	return EncodeBason(record)
}

// BasonToJson converts a BASON byte stream to JSON text.
func BasonToJson(data []byte) (string, error) {
	record, _, err := DecodeBason(data)
	if err != nil {
		return "", err
	}
	var out bytes.Buffer
	recordToJSON(record, &out)
	return out.String(), nil
}
