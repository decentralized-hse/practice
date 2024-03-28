package rdx

import (
	"bytes"
	"errors"
	"fmt"
	"strings"

	"github.com/learn-decentralized-systems/toyqueue"
	"github.com/learn-decentralized-systems/toytlv"
)

var ErrBadM = errors.New("bad M record")

// Parses an enveloped ISFR record
func MelParse(data []byte) (lit byte, t Time, value, rest []byte, err error) {
	var hlen, blen int
	lit, hlen, blen = toytlv.ProbeHeader(data)
	if lit == 0 || hlen+blen > len(data) {
		err = toytlv.ErrIncomplete
		return
	}
	if lit != 'I' && lit != 'S' && lit != 'F' && lit != 'R' {
		err = ErrBadISFR
		return
	}
	rec := data[hlen : hlen+blen]
	rest = data[hlen+blen:]
	tlit, thlen, tblen := toytlv.ProbeHeader(rec)
	tlen := thlen + tblen
	if (tlit != 'T' && tlit != '0') || (tlen > len(rec)) {
		err = ErrBadISFR
		return
	}
	tsb := rec[thlen:tlen]
	t.rev, t.src = UnzipIntUint64Pair(tsb)
	value = rec[tlen:]
	return
}

func MelAppend(to []byte, lit byte, t Time, body []byte) []byte {
	tb := toytlv.TinyRecord('T', t.ZipBytes())
	return toytlv.Append(to, lit, tb, body)
}

func MelReSource(isfr []byte, src uint64) (ret []byte, err error) {
	var lit byte
	var time Time
	var body []byte
	rest := isfr
	for len(rest) > 0 {
		at := len(isfr) - len(rest)
		lit, time, body, rest, err = MelParse(rest)
		if err != nil {
			return
		}
		if time.src != src {
			ret = make([]byte, at, len(isfr)*2)
			copy(ret, isfr[:at])
			break
		}
	}
	if ret == nil && err == nil {
		return isfr, nil
	}
	for err == nil {
		time.src = src
		ret = MelAppend(ret, lit, time, body)
		if len(rest) == 0 {
			break
		}
		lit, time, body, rest, err = MelParse(rest)
	}
	return
}

func MelCompare(llit byte, lval []byte, rlit byte, rval []byte) int {
	// (F, I, R, S, T)
	if llit < rlit {
		return -1
	}
	if llit == rlit {
		return bytes.Compare(lval, rval)
	}
	return 1
}

func MelString(lit byte, val []byte) string {
	tlv := ISFRtlvt(val, Time{})
	switch lit {
	case 'F':
		return Fstring(tlv)
	case 'I':
		return Istring(tlv)
	case 'R':
		return Rstring(tlv)
	case 'S':
		return Sstring(tlv)
	case 'T':
		return "null"
	default:
		return "bad value"
	}
}

func Mmerge(records toyqueue.Records) (tlv []byte) {
	var groups [][]MKeyValue

	for _, record := range records {
		group, err := MKVsParse(record)
		if err != nil {
			return
		}
		groups = append(groups, group)
	}

	return MKVsTlv(MKVsMerge(groups))
}

func Mstring(tlv []byte) string {
	kvs, err := MKVsParse(tlv)
	if err != nil {
		return ""
	}

	var sb strings.Builder
	sb.WriteRune('{')
	recs := 0
	for _, kv := range kvs {
		if kv.vlit == 0 {
			continue
		}
		if recs != 0 {
			sb.WriteRune(',')
		}
		ks := MelString(kv.klit, kv.kval)
		vs := MelString(kv.vlit, kv.vval)
		fmt.Fprintf(&sb, "%s:%s", ks, vs)
		recs++
	}
	sb.WriteRune('}')

	return sb.String()
}

func Emerge(records toyqueue.Records) (tlv []byte) {
	return nil
}

func Estring(tlv []byte) string {
	return "{1,2,3,4}"
}

func Lmerge(records toyqueue.Records) (tlv []byte) {
	return nil
}

func Lstring(tlv []byte) string {
	return "[1,2,3,4,5]"
}
