package bason

import (
	"fmt"
	"math/rand"
)

type randomJSON struct {
	rng *rand.Rand
}

func newRandomJSON(seed int64) *randomJSON {
	return &randomJSON{rng: rand.New(rand.NewSource(seed))}
}

func (r *randomJSON) randomChar() byte {
	const chars = "abcdefghijklmnopqrstuvwxyz0123456789"
	return chars[r.rng.Intn(len(chars))]
}

func (r *randomJSON) randomString(maxLen int) string {
	if maxLen <= 0 {
		maxLen = 20
	}
	len := 1 + r.rng.Intn(maxLen)
	b := make([]byte, 0, len+2)
	b = append(b, '"')
	for i := 0; i < len; i++ {
		c := r.randomChar()
		if c == '"' || c == '\\' {
			b = append(b, '\\')
		}
		b = append(b, c)
	}
	b = append(b, '"')
	return string(b)
}

func (r *randomJSON) randomNumber() string {
	switch r.rng.Intn(5) {
	case 0:
		return "0"
	case 1:
		return "1"
	case 2:
		return fmt.Sprintf("%d", r.rng.Intn(1000))
	case 3:
		return fmt.Sprintf("%d", r.rng.Int63()%10000)
	default:
		return fmt.Sprintf("%.6f", float64(r.rng.Int63())/1e6)
	}
}

func (r *randomJSON) generateValue(depth, maxChildren int) string {
	if depth <= 0 {
		switch r.rng.Intn(5) {
		case 0:
			return "null"
		case 1:
			if r.rng.Intn(2) == 0 {
				return "true"
			}
			return "false"
		case 2:
			return r.randomNumber()
		case 3:
			return r.randomString(8)
		default:
			return "[]"
		}
	}
	switch r.rng.Intn(6) {
	case 0:
		return "null"
	case 1:
		if r.rng.Intn(2) == 0 {
			return "true"
		}
		return "false"
	case 2:
		return r.randomNumber()
	case 3:
		return r.randomString(15)
	case 4:
		return r.generateArray(depth, maxChildren)
	default:
		return r.generateObject(depth, maxChildren)
	}
}

func (r *randomJSON) generateArray(depth, maxChildren int) string {
	n := 1 + r.rng.Intn(maxChildren)
	s := "["
	for i := 0; i < n; i++ {
		if i > 0 {
			s += ","
		}
		s += r.generateValue(depth-1, maxChildren)
	}
	s += "]"
	return s
}

func (r *randomJSON) generateObject(depth, maxChildren int) string {
	n := 1 + r.rng.Intn(maxChildren)
	s := "{"
	for i := 0; i < n; i++ {
		if i > 0 {
			s += ","
		}
		s += r.randomString(10)
		s += ":"
		s += r.generateValue(depth-1, maxChildren)
	}
	s += "}"
	return s
}

func (r *randomJSON) generate(maxDepth, maxChildren int) string {
	return r.generateValue(maxDepth, maxChildren)
}
