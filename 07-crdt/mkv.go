package rdx

import (
	"bytes"
	"container/heap"
)

type MKeyValue struct {
	klit byte
	kt   Time
	kval []byte
	vlit byte
	vt   Time
	vval []byte
}

func MKVParse(tlv []byte) (kv MKeyValue, rest []byte, err error) {
	// Key
	kv.klit, kv.kt, kv.kval, rest, err = MelParse(tlv)
	if err != nil || kv.kt.rev < 0 {
		return
	}
	// Value
	kv.vlit, kv.vt, kv.vval, rest, err = MelParse(rest)
	return
}

func MKVsParse(tlv []byte) (kvs []MKeyValue, err error) {
	var kv MKeyValue
	var rest []byte
	for len(tlv) > 0 {
		kv, rest, err = MKVParse(tlv)
		if err != nil {
			return
		}
		if len(kvs) > 0 {
			last := kvs[len(kvs)-1]
			if MelCompare(last.klit, last.kval, kv.klit, kv.kval) != -1 {
				err = ErrBadM
				return
			}
		}
		tlv = rest
		kvs = append(kvs, kv)
	}
	return
}

func MKVsTlv(kvs []MKeyValue) (tlv []byte) {
	for _, kv := range kvs {
		tlv = MelAppend(tlv, kv.klit, kv.kt, kv.kval)
		if kv.vlit != 0 {
			tlv = MelAppend(tlv, kv.vlit, kv.vt, kv.vval)
		}
	}
	return tlv
}

type mkvMergeElem struct {
	kv   MKeyValue
	gidx int
}

type mkvMergeHeap []mkvMergeElem

func (h mkvMergeHeap) Len() int { return len(h) }
func (h mkvMergeHeap) Less(i, j int) bool {
	return MelCompare(
		h[i].kv.klit, h[i].kv.kval,
		h[j].kv.klit, h[j].kv.kval,
	) == -1
}
func (h mkvMergeHeap) Swap(i, j int) { h[i], h[j] = h[j], h[i] }

func (h *mkvMergeHeap) Push(e any) {
	*h = append(*h, e.(mkvMergeElem))
}
func (h *mkvMergeHeap) Pop() any {
	old := *h
	n := len(old)
	x := old[n-1]
	*h = old[0 : n-1]
	return x
}

func MKVsMerge(groups [][]MKeyValue) (kvs []MKeyValue) {
	mHeap := make(mkvMergeHeap, 0, len(groups))
	for i, group := range groups {
		if len(group) == 0 {
			continue
		}
		kv := group[0]
		groups[i] = group[1:]
		mHeap = append(mHeap, mkvMergeElem{kv: kv, gidx: i})
	}
	heap.Init(&mHeap)

	for len(mHeap) > 0 {
		mElem := heap.Pop(&mHeap).(mkvMergeElem)
		group := groups[mElem.gidx]
		if len(group) > 0 {
			kv := group[0]
			groups[mElem.gidx] = group[1:]
			mHeap = append(mHeap, mkvMergeElem{kv: kv, gidx: mElem.gidx})
		}

		if len(kvs) == 0 {
			kvs = append(kvs, mElem.kv)
			continue
		}

		last := kvs[len(kvs)-1]
		curr := mElem.kv
		if MelCompare(last.klit, last.kval, curr.klit, curr.kval) == -1 {
			kvs = append(kvs, mElem.kv)
			continue
		}

		absRev := func(rev int64) int64 {
			if rev < 0 {
				return -rev
			}
			return rev
		}

		// LWW key
		if last.kt != curr.kt {
			lrev := absRev(last.kt.rev)
			lsrc := last.kt.src
			crev := absRev(curr.kt.rev)
			csrc := curr.kt.src
			if crev > lrev || (crev == lrev && csrc > lsrc) {
				kvs[len(kvs)-1] = curr
			}
			continue
		}

		// LWW value
		if absRev(last.vt.rev) != absRev(curr.vt.rev) {
			if absRev(last.vt.rev) < absRev(curr.vt.rev) {
				kvs[len(kvs)-1] = curr
			}
			continue
		}
		bcmp := bytes.Compare(last.vval, curr.vval)
		if bcmp != 0 {
			if bcmp == -1 {
				kvs[len(kvs)-1] = curr
			}
			continue
		}
		if last.vt.src < curr.vt.src {
			kvs[len(kvs)-1] = curr
		}
	}

	return
}
