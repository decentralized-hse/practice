package rdx

import (
	"bytes"
	"errors"
	"fmt"
	"github.com/learn-decentralized-systems/toytlv"
)

// Common LWW functions

// Logical time: revision number + source id.
// For tombstones, revision numbers are negative.
type Time struct {
	rev int64
	src uint64
}

var T0 = Time{0, 0}

func (t Time) ZipBytes() []byte {
	return ZipIntUint64Pair(t.rev, t.src)
}

func TimeFromZipBytes(zip []byte) (t Time) {
	// todo bad data
	t.rev, t.src = UnzipIntUint64Pair(zip)
	return
}

var ErrBadISFR = errors.New("bad ISFR record")

// for bad format, value==nil (an empty value is an empty slice)
func ISFRparse(bulk []byte) (rev int64, src uint64, value []byte) {
	lit, hlen, blen := toytlv.ProbeHeader(bulk)
	if lit != 'T' && lit != '0' || hlen+blen > len(bulk) {
		return
	}
	tsb := bulk[hlen : hlen+blen]
	rev, src = UnzipIntUint64Pair(tsb)
	value = bulk[hlen+blen:]
	return
}

func ISFRtlvt(value []byte, t Time) (bulk []byte) {
	time := t.ZipBytes()
	bulk = make([]byte, 0, len(time)+len(value)+2)
	bulk = toytlv.AppendTiny(bulk, 'T', time)
	bulk = append(bulk, value...)
	return
}

func ISFRmerge(tlvs [][]byte) (tlv []byte) {
	var winrec []byte
	var winrev int64
	var winsrc uint64
	var winhlbl int
	for _, rec := range tlvs {
		l, hlen, blen := toytlv.ProbeHeader(rec)
		hlbl := hlen + blen
		tsb := rec[hlen:hlbl]
		rev, src := UnzipIntUint64Pair(tsb)
		if rev < 0 {
			rev = -rev
		}
		if l != 'T' && l != '0' {
			continue
		}
		if rev < winrev {
			continue
		}
		if rev == winrev {
			valtie := bytes.Compare(rec[hlbl:], winrec[winhlbl:])
			if valtie < 0 {
				continue
			}
			if valtie == 0 && src < winsrc {
				continue
			}
		}
		winrev = rev
		winrec = rec
		winsrc = src
		winhlbl = hlbl
	}
	return winrec
}

func ISFRdiff(tlv []byte, vvdiff VV) []byte {
	_, src, _ := ISFRparse(tlv)
	_, ok := vvdiff[src]
	if ok {
		return tlv
	} else {
		return nil
	}
}

var ErrBadPacket = errors.New("bad packet")

func IsfrReSource(bare []byte, src uint64) (res []byte, err error) {
	time, s, value := ISFRparse(bare)
	if value == nil {
		return nil, ErrBadPacket
	}
	if s != src { // ensure correct attribution
		res = ISFRtlvt(value, Time{time, src})
	} else {
		res = bare
	}
	return
}

// I is a last-write-wins int64

// produce a text form (for REPL mostly)
func Istring(tlv []byte) (txt string) {
	return fmt.Sprintf("%d", Inative(tlv))
}

// parse a text form into a TLV value
func Iparse(txt string) (tlv []byte) {
	var i int64
	_, _ = fmt.Sscanf(txt, "%d", &i)
	return Itlv(i)
}

// convert native golang value into a TLV form
func Itlv(i int64) (tlv []byte) {
	return ISFRtlvt(ZipInt64(i), Time{})
}

func Itlvt(i int64, t Time) (tlv []byte) {
	return ISFRtlvt(ZipInt64(i), t)
}

// convert a TLV value to a native golang value
func Inative(tlv []byte) int64 {
	_, _, val := ISFRparse(tlv)
	return UnzipInt64(val)
}

// merge TLV values
func Imerge(tlvs [][]byte) (tlv []byte) {
	return ISFRmerge(tlvs)
}

// produce an op that turns the old value into the new one
func Idelta(tlv []byte, new_val int64) (tlv_delta []byte) {
	rev, _, val := ISFRparse(tlv)
	if rev < 0 {
		rev = -rev
	}
	nv := ZipInt64(new_val)
	if bytes.Compare(val, nv) != 0 {
		tlv_delta = ISFRtlvt(nv, Time{rev + 1, 0})
	}
	return
}

// checks a TLV value for validity (format violations)
func Ivalid(tlv []byte) bool {
	_, src, val := ISFRparse(tlv)
	return val != nil && len(val) <= 8 && src <= MaxSrc
}

func Idiff(tlv []byte, vvdiff VV) []byte {
	return ISFRdiff(tlv, vvdiff)
}

// S is a last-write-wins UTF-8 string

const hex = "0123456789abcdef"

// produce a text form (for REPL mostly)
func Sstring(tlv []byte) (txt string) {
	dst := make([]byte, 0, len(tlv)*2)
	_, _, val := ISFRparse(tlv)
	dst = append(dst, '"')
	for _, b := range val {
		switch b {
		case '\\', '"':
			dst = append(dst, '\\', b)
		case '\n':
			dst = append(dst, '\\', 'n')
		case '\r':
			dst = append(dst, '\\', 'r')
		case '\t':
			dst = append(dst, '\\', 't')
		case 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0xb, 0xc, 0xe, 0xf,
			0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a,
			0x1b, 0x1c, 0x1d, 0x1e, 0x1f:
			dst = append(dst, '\\', 'u', '0', '0', hex[b>>4], hex[b&0xF])
		default:
			dst = append(dst, b)
		}
	}
	dst = append(dst, '"')
	return string(dst)
}

// parse a text form into a TLV value; nil on error
func Sparse(txt string) (tlv []byte) {
	if len(txt) < 2 || txt[0] != '"' || txt[len(txt)-1] != '"' {
		return nil
	}
	unq := txt[1 : len(txt)-1]
	unesc, _ := Unescape([]byte(unq), nil)
	return ISFRtlvt(unesc, Time{})
}

// convert native golang value into a TLV form
func Stlv(s string) (tlv []byte) {
	return ISFRtlvt([]byte(s), Time{})
}

func Stlvt(s string, t Time) (tlv []byte) {
	return ISFRtlvt([]byte(s), t)
}

// convert a TLV value to a native golang value
func Snative(tlv []byte) string {
	_, _, val := ISFRparse(tlv)
	return string(val)
}

// merge TLV values
func Smerge(tlvs [][]byte) (tlv []byte) {
	return ISFRmerge(tlvs)
}

// produce an op that turns the old value into the new one
func Sdelta(tlv []byte, new_val string) (tlv_delta []byte) {
	rev, _, val := ISFRparse(tlv)
	if rev < 0 {
		rev = -rev
	}
	nv := []byte(new_val)
	if bytes.Compare(val, nv) != 0 {
		tlv_delta = ISFRtlvt([]byte(new_val), Time{rev + 1, 0})
	}
	return
}

// checks a TLV value for validity (format violations)
func Svalid(tlv []byte) bool {
	_, src, val := ISFRparse(tlv)
	return val != nil && src <= MaxSrc
}

func Sdiff(tlv []byte, vvdiff VV) []byte {
	return ISFRdiff(tlv, vvdiff)
}

// R is a last-write-wins ID

// produce a text form (for REPL mostly)
func Rstring(tlv []byte) (txt string) {
	return Rnative(tlv).String()
}

// parse a text form into a TLV value
func Rparse(txt string) (tlv []byte) {
	id := IDFromString([]byte(txt))
	return Rtlv(id)
}

// convert native golang value into a TLV form
func Rtlv(id ID) (tlv []byte) {
	return ISFRtlvt(id.ZipBytes(), Time{})
}

// convert a TLV value to a native golang value
func Rnative(tlv []byte) ID {
	_, _, val := ISFRparse(tlv)
	return IDFromZipBytes(val)
}

// merge TLV values
func Rmerge(tlvs [][]byte) (tlv []byte) {
	return ISFRmerge(tlvs)
}

// produce an op that turns the old value into the new one
func Rdelta(tlv []byte, new_val ID) (tlv_delta []byte) {
	rev, _, val := ISFRparse(tlv)
	if rev < 0 {
		rev = -rev
	}
	nv := new_val.ZipBytes()
	if bytes.Compare(val, nv) != 0 {
		tlv_delta = ISFRtlvt(nv, Time{rev + 1, 0})
	}
	return
}

// checks a TLV value for validity (format violations)
func Rvalid(tlv []byte) bool {
	_, src, val := ISFRparse(tlv)
	return val != nil && src <= MaxSrc
	// todo correct sizes
}

func Rdiff(tlv []byte, vvdiff VV) []byte {
	return ISFRdiff(tlv, vvdiff)
}

// F is a last-write-wins float64

// produce a text form (for REPL mostly)
func Fstring(tlv []byte) (txt string) {
	return fmt.Sprintf("%f", Fnative(tlv))
}

// parse a text form into a TLV value
func Fparse(txt string) (tlv []byte) {
	var i float64
	_, _ = fmt.Sscanf(txt, "%f", &i)
	return Ftlv(i)
}

// convert native golang value into a TLV form
func Ftlv(i float64) (tlv []byte) {
	return ISFRtlvt(ZipFloat64(i), T0)
}

// convert a TLV value to a native golang value
func Fnative(tlv []byte) float64 {
	_, _, val := ISFRparse(tlv)
	return UnzipFloat64(val)
}

// merge TLV values
func Fmerge(tlvs [][]byte) (tlv []byte) {
	return ISFRmerge(tlvs)
}

// produce an op that turns the old value into the new one
func Fdelta(tlv []byte, new_val float64) (tlv_delta []byte) {
	rev, _, val := ISFRparse(tlv)
	if rev < 0 {
		rev = -rev
	}
	nv := ZipFloat64(new_val)
	if bytes.Compare(val, nv) != 0 {
		tlv_delta = ISFRtlvt(nv, Time{rev + 1, 0})
	}
	return
}

// checks a TLV value for validity (format violations)
func Fvalid(tlv []byte) bool {
	_, src, val := ISFRparse(tlv)
	return val != nil && src <= MaxSrc && len(val) <= 8
}

func Fdiff(tlv []byte, vvdiff VV) []byte {
	return ISFRdiff(tlv, vvdiff)
}
