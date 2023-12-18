package router

import (
	"encoding/binary"
	"errors"
)

func TLVShortLit(lit byte) bool {
	return lit >= 'a' && lit <= 'z'
}

func TLVLongLit(lit byte) bool {
	return lit >= 'A' && lit <= 'Z'
}

func TLVlit(lit byte) bool {
	return TLVShortLit(lit) || TLVLongLit(lit)
}

var NoDataYet = errors.New("incomplete data")
var BadRecord = errors.New("bad record format")

func TLVTake(data []byte) (lit byte, body, rest []byte, err error) {
	rest = data
	if len(data) < 2 {
		err = NoDataYet
		return
	}
	lit = data[0]
	if TLVShortLit(lit) {
		lit = lit - ('a' - 'A')
		reclen := int(data[1])
		if len(data) < 2+reclen {
			err = NoDataYet
		} else {
			body = data[2 : 2+reclen]
			rest = data[2+reclen:]
		}
	} else if TLVLongLit(lit) {
		if len(data) < 5 {
			err = NoDataYet
			return
		}
		reclen := binary.LittleEndian.Uint32(data[1:5])
		if reclen > 1<<30 {
			err = BadRecord
		} else if len(data) < 5+int(reclen) {
			err = NoDataYet
		} else {
			body = data[5 : 5+reclen]
			rest = data[5+reclen:]
		}
	} else {
		err = BadRecord
	}
	return
}

func TLVAppend(data []byte, lit byte, body []byte) (newdata []byte, err error) {
	if !TLVLongLit(lit) {
		return nil, BadRecord
	}
	blen := len(body)
	i := [4]byte{}
	binary.LittleEndian.PutUint32(i[:], uint32(blen))
	if blen < 255 {
		newdata = append(data, lit+('a'-'A'), i[0])
		newdata = append(newdata, body...)
	} else {
		newdata = append(data, lit)
		newdata = append(newdata, i[:]...)
		newdata = append(newdata, body...)
	}
	return
}

func TLVAppend2(data []byte, lit byte, body1, body2 []byte) (newdata []byte, err error) {
	if !TLVLongLit(lit) {
		return nil, BadRecord
	}
	blen := len(body1) + len(body2)
	i := [4]byte{}
	binary.LittleEndian.PutUint32(i[:], uint32(blen))
	if blen < 255 {
		newdata = append(data, lit+('a'-'A'), i[0])
		newdata = append(newdata, body1...)
		newdata = append(newdata, body2...)
	} else {
		newdata = append(data, lit)
		newdata = append(newdata, i[:]...)
		newdata = append(newdata, body1...)
		newdata = append(newdata, body2...)
	}
	return
}
