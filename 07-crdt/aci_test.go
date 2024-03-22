package rdx

import (
	"github.com/learn-decentralized-systems/toyqueue"
	"github.com/learn-decentralized-systems/toytlv"
	"github.com/stretchr/testify/assert"
	"math/rand"
	"os"
	"testing"
)

func DupAndShuffle(inputs toyqueue.Records) toyqueue.Records {
	ret := make(toyqueue.Records, len(inputs)+1)
	copy(ret[1:], inputs)
	ret[0] = inputs[rand.Int()%len(inputs)]
	rand.Shuffle(len(ret), func(i, j int) {
		ret[i], ret[j] = ret[j], ret[i]
	})
	return ret
}

func SaveEnveloped(filename string, records toyqueue.Records, lit byte) (err error) {
	f, err := os.Create(filename)
	for i := 0; i < len(records) && err == nil; i++ {
		_, err = f.Write(toytlv.Record(lit, records[i]))
	}
	_ = f.Close()
	return
}

func TestImerge(t *testing.T) {
	inputs := toyqueue.Records{
		Itlvt(1, Time{1, 1}),
		Itlvt(2, Time{2, 2}),
		Itlvt(3, Time{2, 1}),
	}
	//assert.Nil(t, SaveEnveloped("test_data/I0.tlv", inputs, 'I'))
	merged := Imerge(DupAndShuffle(inputs))
	assert.Equal(t, "3", Istring(merged))
}

func TestSmerge(t *testing.T) {
	inputs := toyqueue.Records{
		Stlvt("one", Time{1, 1}),
		Stlvt("two", Time{2, 2}),
		Stlvt("three", Time{2, 1}),
	}
	//assert.Nil(t, SaveEnveloped("test_data/S0.tlv", inputs, 'S'))
	merged := Smerge(DupAndShuffle(inputs))
	assert.Equal(t, "\"two\"", Sstring(merged))
}

func TestNMerge(t *testing.T) {
	inputs := toyqueue.Records{
		toytlv.Concat(
			toytlv.Record('U', ZipUint64Pair(2, 2)),
			toytlv.Record('U', ZipUint64Pair(3, 3)),
			toytlv.Record('U', ZipUint64Pair(2, 4)),
		),
		toytlv.Concat(
			toytlv.Record('U', ZipUint64Pair(3, 3)),
			toytlv.Record('U', ZipUint64Pair(4, 4)),
			toytlv.Record('U', ZipUint64Pair(5, 5)),
		),
	}
	//assert.Nil(t, SaveEnveloped("test_data/N0.tlv", inputs, 'N'))
	merged := Smerge(DupAndShuffle(inputs))
	assert.Equal(t, "14", Nstring(merged))
}

func TestZMerge(t *testing.T) {
	// winning versions: 1 by #1 = -1, 2 by #2 = -2, 3 by #3 = 3
	inputs := toyqueue.Records{
		toytlv.Concat(
			toytlv.Record('I', Itlvt(-1, Time{1, 1})),
			toytlv.Record('I', Itlvt(-2, Time{2, 2})),
			toytlv.Record('I', Itlvt(2, Time{2, 3})),
		),
		toytlv.Concat(
			toytlv.Record('I', Itlvt(-1, Time{1, 1})),
			toytlv.Record('I', Itlvt(-1, Time{1, 2})),
			toytlv.Record('I', Itlvt(3, Time{3, 3})),
		),
	}
	//assert.Nil(t, SaveEnveloped("test_data/Z0.tlv", inputs, 'Z'))
	merged := Zmerge(DupAndShuffle(inputs))
	assert.Equal(t, "0", Zstring(merged))
}

func TestMMerge(t *testing.T) { // maps
	inputs := toyqueue.Records{
		toytlv.Concat( // {1:1,3:4,5:6}
			toytlv.Record('I', Itlvt(1, Time{3, 1})),
			toytlv.Record('I', Itlvt(1, Time{3, 1})),

			toytlv.Record('I', Itlvt(3, Time{2, 1})),
			toytlv.Record('I', Itlvt(4, Time{2, 1})),

			toytlv.Record('I', Itlvt(5, Time{1, 1})),
			toytlv.Record('I', Itlvt(6, Time{1, 1})),
		),
		toytlv.Concat( // {1:2,3:4,5:6}
			toytlv.Record('I', Itlvt(1, Time{3, 1})),
			toytlv.Record('I', Itlvt(2, Time{4, 1})),

			toytlv.Record('I', Itlvt(3, Time{2, 1})),
			toytlv.Record('I', Itlvt(4, Time{2, 1})),

			toytlv.Record('I', Itlvt(5, Time{1, 1})),
			toytlv.Record('I', Itlvt(6, Time{1, 1})),
		),
		toytlv.Concat( // remove 5:6
			toytlv.Record('I', Itlvt(5, Time{-1, 1})),
		),
	}

	//assert.Nil(t, SaveEnveloped("test_data/M0.tlv", inputs, 'M'))
	merged := Mmerge(DupAndShuffle(inputs))
	assert.Equal(t, "{1:2,3:4}", Mstring(merged))
}

func TestEMerge(t *testing.T) { // sets
	inputs := toyqueue.Records{
		toytlv.Concat( // {1,3,4,5} by #1
			toytlv.Record('I', Itlvt(1, Time{1, 1})),
			toytlv.Record('I', Itlvt(3, Time{2, 1})),
			toytlv.Record('I', Itlvt(4, Time{3, 1})),
			toytlv.Record('I', Itlvt(5, Time{4, 1})),
		),
		toytlv.Concat( // {1,2}, 1 is added concurrently by #1 and #2
			toytlv.Record('I', Itlvt(1, Time{1, 2})),
			toytlv.Record('I', Itlvt(2, Time{2, 2})),
		),
		toytlv.Concat( // #2 removes 5 that #1 added (rev 4)
			toytlv.Record('I', Itlvt(5, Time{-4, 2})),
		),
	}

	//assert.Nil(t, SaveEnveloped("test_data/E0.tlv", inputs, 'E'))
	merged := Emerge(DupAndShuffle(inputs))
	assert.Equal(t, "{1,2,3,4}", Estring(merged))
}
