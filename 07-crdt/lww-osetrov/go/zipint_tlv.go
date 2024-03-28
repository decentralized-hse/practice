package main

// typedef const char cchar_t;
import "C"
import (
	"bytes"
	"encoding/binary"
	"errors"
	"unicode/utf8"
	"unsafe"
)

func toCBytes(buf []byte) (*C.cchar_t, int) {
	res := C.CBytes(buf)
	return (*C.cchar_t)(res), len(buf)
}

func charToBytes(src *C.cchar_t, sz int) []byte {
	return C.GoBytes(unsafe.Pointer(src), C.int(sz))
}

// ZipUint64 packs uint64 into a shortest possible byte string
//
//export ZipUint64
func ZipUint64(v uint64) (*C.cchar_t, int) {
	buf := [8]byte{}
	i := 0
	for v > 0 {
		buf[i] = uint8(v)
		v >>= 8
		i++
	}
	return toCBytes(buf[0:i])
}

//export UnzipUint64
func UnzipUint64(data *C.cchar_t, sz int) (v uint64) {
	zip := charToBytes(data, sz)
	for i := len(zip) - 1; i >= 0; i-- {
		v <<= 8
		v |= uint64(zip[i])
	}
	return
}

func zigZagInt64(i int64) uint64 {
	return uint64(i*2) ^ uint64(i>>63)
}

func zagZigUint64(u uint64) int64 {
	half := u >> 1
	mask := -(u & 1)
	return int64(half ^ mask)
}

//export zipZagInt64
func zipZagInt64(i int64) (*C.cchar_t, int) {
	return ZipUint64(zigZagInt64(i))
}

//export ZipInt64
func ZipInt64(v int64) (*C.cchar_t, int) {
	return ZipUint64(zigZagInt64(v))
}

//export UnzipInt64
func UnzipInt64(data *C.cchar_t, sz int) int64 {
	return zagZigUint64(UnzipUint64(data, sz))
}

// ID is an 64-bit locator/identifier that contains:
//   - op seq number (30 bits),
//   - offset (10 bits), and
//   - the replica id (24 bits).
//
// This is NOT a Lamport timestamp (need more bits for that).
type ID uint64

const seqBits = 32
const offBits = 12
const srcBits = 20
const seqOffBits = seqBits + offBits
const seqOffMask = uint64(uint64(1)<<seqOffBits) - 1
const offMask = uint64(1<<offBits) - 1

const BadId = ID(uint64(0xfff) << seqOffBits)

const bytes4 = 0xffffffff
const bytes2 = 0xffff
const bytes1 = 0xff

//export SrcSeqOff
func SrcSeqOff(id ID) (src uint32, seq uint32, off uint16) {
	n := uint64(id)
	off = uint16(n & offMask)
	n >>= offBits
	seq = uint32(n)
	n >>= seqBits
	src = uint32(n)
	return
}

func byteLen(n uint64) int {
	if n <= bytes1 {
		if n == 0 {
			return 0
		}
		return 1
	}
	if n <= bytes2 {
		return 2
	}
	if n <= bytes4 {
		return 4
	}
	return 8
}

// zipUint64Pair packs a pair of uint64 into a byte string.
// The smaller the ints, the shorter the string TODO 4+3 etc
func zipUint64Pair(big, lil uint64) []byte {
	var ret = [16]byte{}
	pat := (byteLen(big) << 4) | byteLen(lil)
	switch pat {
	case 0x00:
		return ret[0:0]
	case 0x10:
		ret[0] = byte(big)
		return ret[0:1]
	case 0x01, 0x11: // 2
		ret[0] = byte(big)
		ret[1] = byte(lil)
		return ret[0:2]
	case 0x20, 0x21:
		binary.LittleEndian.PutUint16(ret[0:2], uint16(big))
		ret[2] = byte(lil)
		return ret[0:3]
	case 0x02, 0x12, 0x22:
		binary.LittleEndian.PutUint16(ret[0:2], uint16(big))
		binary.LittleEndian.PutUint16(ret[2:4], uint16(lil))
		return ret[0:4]
	case 0x40, 0x41:
		binary.LittleEndian.PutUint32(ret[0:4], uint32(big))
		ret[4] = byte(lil)
		return ret[0:5]
	case 0x42:
		binary.LittleEndian.PutUint32(ret[0:4], uint32(big))
		binary.LittleEndian.PutUint16(ret[4:6], uint16(lil))
		return ret[0:6]
	case 0x04, 0x14, 0x24, 0x44:
		binary.LittleEndian.PutUint32(ret[0:4], uint32(big))
		binary.LittleEndian.PutUint32(ret[4:8], uint32(lil))
		return ret[0:8]
	case 0x80, 0x81:
		binary.LittleEndian.PutUint64(ret[0:8], big)
		ret[8] = byte(lil)
		return ret[0:9]
	case 0x82:
		binary.LittleEndian.PutUint64(ret[0:8], big)
		binary.LittleEndian.PutUint16(ret[8:10], uint16(lil))
		return ret[0:10]
	case 0x84:
		binary.LittleEndian.PutUint64(ret[0:8], big)
		binary.LittleEndian.PutUint32(ret[8:12], uint32(lil))
		return ret[0:12]
	case 0x08, 0x18, 0x28, 0x48, 0x88:
		binary.LittleEndian.PutUint64(ret[0:8], big)
		binary.LittleEndian.PutUint64(ret[8:16], lil)
		return ret[0:16]
	}
	return ret[:]
}

func unzipUint64Pair(buf []byte) (big, lil uint64) {
	switch len(buf) {
	case 0:
		return 0, 0
	case 1:
		return uint64(buf[0]), 0
	case 2:
		return uint64(buf[0]), uint64(buf[1])
	case 3:
		big = uint64(binary.LittleEndian.Uint16(buf[0:2]))
		lil = uint64(buf[2])
	case 4:
		big = uint64(binary.LittleEndian.Uint16(buf[0:2]))
		lil = uint64(binary.LittleEndian.Uint16(buf[2:4]))
	case 5:
		big = uint64(binary.LittleEndian.Uint32(buf[0:4]))
		lil = uint64(buf[4])
	case 6:
		big = uint64(binary.LittleEndian.Uint32(buf[0:4]))
		lil = uint64(binary.LittleEndian.Uint16(buf[4:6]))
	case 8:
		big = uint64(binary.LittleEndian.Uint32(buf[0:4]))
		lil = uint64(binary.LittleEndian.Uint32(buf[4:8]))
	case 9:
		big = uint64(binary.LittleEndian.Uint64(buf[0:8]))
		lil = uint64(buf[8])
	case 10:
		big = binary.LittleEndian.Uint64(buf[0:8])
		lil = uint64(binary.LittleEndian.Uint16(buf[8:10]))
	case 12:
		big = binary.LittleEndian.Uint64(buf[0:8])
		lil = uint64(binary.LittleEndian.Uint32(buf[8:12]))
	case 16:
		big = binary.LittleEndian.Uint64(buf[0:8])
		lil = binary.LittleEndian.Uint64(buf[8:16])
	default:
		// error!
	}
	return
}

//export ZipID
func ZipID(id ID) (*C.char, int) {
	i := uint64(id)
	zipped := zipUint64Pair(i&seqOffMask, i>>seqOffBits)
	return toCBytes(zipped)
}

//export UnzipID
func UnzipID(cZip *C.cchar_t, szZip int) ID {
	zipped := charToBytes(cZip, szZip)
	big, lil := unzipUint64Pair(zipped)
	return ID(big | (lil << seqOffBits))
}

//export MakeID
func MakeID(src uint32, seq uint32, off uint16) ID {
	ret := uint64(src)
	ret <<= seqBits
	ret |= uint64(seq)
	ret <<= offBits
	ret |= uint64(off)
	return ID(ret)
}

//export ParseID
func ParseID(cId *C.cchar_t, szId int) ID {
	id := charToBytes(cId, szId)
	if len(id) > 16+2 {
		return BadId
	}
	var parts [3]uint64
	i, p := 0, 0
	for i < len(id) && p < 3 {
		c := id[i]
		if c >= '0' && c <= '9' {
			parts[p] = (parts[p] << 4) | uint64(c-'0')
		} else if c >= 'A' && c <= 'F' {
			parts[p] = (parts[p] << 4) | uint64(10+c-'A')
		} else if c >= 'a' && c <= 'f' {
			parts[p] = (parts[p] << 4) | uint64(10+c-'a')
		} else if c == '-' {
			p++
		} else {
			return BadId
		}
		i++
	}
	switch p {
	case 0: // off
		parts[2] = parts[0]
		parts[0] = 0
	case 1: // src-seq
	case 2: // src-seq-off
	case 3:
		return BadId
	}
	if parts[1] > 0xffffffff || parts[2] > 0xfff || parts[0] > 0xfffff {
		return BadId
	}
	return MakeID(uint32(parts[0]), uint32(parts[1]), uint16(parts[2]))
}

const caseBit uint8 = 'a' - 'A'

// ProbeHeader probes a TLV record header. Return values:
//   - 0  0 0 	incomplete header
//   - '-' 0 0 	bad format
//   - 'A' 2 123 success
//
//export ProbeHeader
func ProbeHeader(cData *C.cchar_t, sz int) (lit byte, hdrlen, bodylen int) {
	data := charToBytes(cData, sz)
	if len(data) == 0 {
		return 0, 0, 0
	}
	dlit := data[0]
	if dlit >= '0' && dlit <= '9' { // tiny
		lit = '0'
		bodylen = int(dlit - '0')
		hdrlen = 1
	} else if dlit >= 'a' && dlit <= 'z' { // short
		if len(data) < 2 {
			return
		}
		lit = dlit - caseBit
		hdrlen = 2
		bodylen = int(data[1])
	} else if dlit >= 'A' && dlit <= 'Z' { // long
		if len(data) < 5 {
			return
		}
		bl := binary.LittleEndian.Uint32(data[1:5])
		if bl > 0x7fffffff {
			lit = '-'
			return
		}
		lit = dlit
		bodylen = int(bl)
		hdrlen = 5
	} else {
		lit = '-'
	}
	return
}

// Feeds the header into the buffer.
// Subtle: lower-case lit allows for defaulting, uppercase must be explicit.
func appendHeader(into []byte, lit byte, bodylen int) (ret []byte) {
	biglit := lit &^ caseBit
	if biglit < 'A' || biglit > 'Z' {
		panic("ToyTLV record type is A..Z")
	}
	if bodylen < 10 && (lit&caseBit) != 0 {
		ret = append(into, byte('0'+bodylen))
	} else if bodylen > 0xff {
		if bodylen > 0x7fffffff {
			panic("oversized TLV record")
		}
		ret = append(into, biglit)
		ret = binary.LittleEndian.AppendUint32(ret, uint32(bodylen))
	} else {
		ret = append(into, lit|caseBit, byte(bodylen))
	}
	return ret
}

// Record composes a record of a given type
//
//export Record
func Record(lit byte, cTime *C.cchar_t, szTime int, cTlv *C.cchar_t, szTlv int) (*C.char, int) {
	time := charToBytes(cTime, szTime)
	tlv := charToBytes(cTlv, szTlv)
	total := len(time) + len(tlv)
	ret := make([]byte, 0, total+5)
	ret = appendHeader(ret, lit, total)
	ret = append(ret, time...)
	ret = append(ret, tlv...)
	return toCBytes(ret)
}

// JSON Unicode stuff: see https://tools.ietf.org/html/rfc7159#section-7

const supplementalPlanesOffset = 0x10000
const highSurrogateOffset = 0xD800
const lowSurrogateOffset = 0xDC00

const basicMultilingualPlaneReservedOffset = 0xDFFF
const basicMultilingualPlaneOffset = 0xFFFF

func combineUTF16Surrogates(high, low rune) rune {
	return supplementalPlanesOffset + (high-highSurrogateOffset)<<10 + (low - lowSurrogateOffset)
}

const badHex = -1

func h2I(c byte) int {
	switch {
	case c >= '0' && c <= '9':
		return int(c - '0')
	case c >= 'A' && c <= 'F':
		return int(c - 'A' + 10)
	case c >= 'a' && c <= 'f':
		return int(c - 'a' + 10)
	}
	return badHex
}

// decodeSingleUnicodeEscape decodes a single \uXXXX escape sequence. The prefix \u is assumed to be present and
// is not checked.
// In JSON, these escapes can either come alone or as part of "UTF16 surrogate pairs" that must be handled together.
// This function only handles one; decodeUnicodeEscape handles this more complex case.
func decodeSingleUnicodeEscape(in []byte) (rune, bool) {
	// We need at least 6 characters total
	if len(in) < 6 {
		return utf8.RuneError, false
	}

	// Convert hex to decimal
	h1, h2, h3, h4 := h2I(in[2]), h2I(in[3]), h2I(in[4]), h2I(in[5])
	if h1 == badHex || h2 == badHex || h3 == badHex || h4 == badHex {
		return utf8.RuneError, false
	}

	// Compose the hex digits
	return rune(h1<<12 + h2<<8 + h3<<4 + h4), true
}

// isUTF16EncodedRune checks if a rune is in the range for non-BMP characters,
// which is used to describe UTF16 chars.
// Source: https://en.wikipedia.org/wiki/Plane_(Unicode)#Basic_Multilingual_Plane
func isUTF16EncodedRune(r rune) bool {
	return highSurrogateOffset <= r && r <= basicMultilingualPlaneReservedOffset
}

func decodeUnicodeEscape(in []byte) (rune, int) {
	if r, ok := decodeSingleUnicodeEscape(in); !ok {
		// Invalid Unicode escape
		return utf8.RuneError, -1
	} else if r <= basicMultilingualPlaneOffset && !isUTF16EncodedRune(r) {
		// Valid Unicode escape in Basic Multilingual Plane
		return r, 6
	} else if r2, ok := decodeSingleUnicodeEscape(in[6:]); !ok { // Note: previous decodeSingleUnicodeEscape success guarantees at least 6 bytes remain
		// UTF16 "high surrogate" without manditory valid following Unicode escape for the "low surrogate"
		return utf8.RuneError, -1
	} else if r2 < lowSurrogateOffset {
		// Invalid UTF16 "low surrogate"
		return utf8.RuneError, -1
	} else {
		// Valid UTF16 surrogate pair
		return combineUTF16Surrogates(r, r2), 12
	}
}

// backslashCharEscapeTable: when '\X' is found for some byte X, it is to be replaced with backslashCharEscapeTable[X]
var backslashCharEscapeTable = [...]byte{
	'"':  '"',
	'\\': '\\',
	'/':  '/',
	'b':  '\b',
	'f':  '\f',
	'n':  '\n',
	'r':  '\r',
	't':  '\t',
}

// unescapeToUTF8 unescapes the single escape sequence starting at 'in' into 'out' and returns
// how many characters were consumed from 'in' and emitted into 'out'.
// If a valid escape sequence does not appear as a prefix of 'in', (-1, -1) to signal the error.
func unescapeToUTF8(in, out []byte) (inLen int, outLen int) {
	if len(in) < 2 || in[0] != '\\' {
		// Invalid escape due to insufficient characters for any escape or no initial backslash
		return -1, -1
	}

	// https://tools.ietf.org/html/rfc7159#section-7
	switch e := in[1]; e {
	case '"', '\\', '/', 'b', 'f', 'n', 'r', 't':
		// Valid basic 2-character escapes (use lookup table)
		out[0] = backslashCharEscapeTable[e]
		return 2, 1
	case 'u':
		// Unicode escape
		if r, inLen := decodeUnicodeEscape(in); inLen == -1 {
			// Invalid Unicode escape
			return -1, -1
		} else {
			// Valid Unicode escape; re-encode as UTF8
			outLen := utf8.EncodeRune(out, r)
			return inLen, outLen
		}
	}

	return -1, -1
}

var MalformedStringEscapeError = errors.New("malformed string escape")

// Unescape unescapes the string contained in 'in' and returns it as a slice.
// If 'in' contains no escaped characters:
//
//	Returns 'in'.
//
// Else, if 'out' is of sufficient capacity (guaranteed if cap(out) >= len(in)):
//
//	'out' is used to build the unescaped string and is returned with no extra allocation
//
// Else:
//
//	A new slice is allocated and returned.
//
//export Unescape
func Unescape(cIn *C.cchar_t, szIn int) (*C.char, int) {
	in := charToBytes(cIn, szIn)

	firstBackslash := bytes.IndexByte(in, '\\')
	if firstBackslash == -1 {
		return toCBytes(in)
	}

	out := make([]byte, len(in))

	// Copy the first sequence of unescaped bytes to the output and obtain a buffer pointer (subslice)
	copy(out, in[:firstBackslash])
	in = in[firstBackslash:]
	buf := out[firstBackslash:]

	for len(in) > 0 {
		// Unescape the next escaped character
		inLen, bufLen := unescapeToUTF8(in, buf)
		if inLen == -1 {
			panic(MalformedStringEscapeError)
		}

		in = in[inLen:]
		buf = buf[bufLen:]

		// Copy everything up until the next backslash
		nextBackslash := bytes.IndexByte(in, '\\')
		if nextBackslash == -1 {
			copy(buf, in)
			buf = buf[len(in):]
			break
		} else {
			copy(buf, in[:nextBackslash])
			buf = buf[nextBackslash:]
			in = in[nextBackslash:]
		}
	}

	// Trim the out buffer to the amount that was actually emitted
	return toCBytes(out[:len(out)-len(buf)])
}

func main() {}
