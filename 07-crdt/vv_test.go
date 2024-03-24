package rdx

import (
	"github.com/learn-decentralized-systems/toytlv"
	"github.com/stretchr/testify/assert"
	"testing"
)

func TestVMerge(t *testing.T) {
	args := [][]byte{
		toytlv.Concat(
			toytlv.Record('V', ParseIDString("a-123").ZipBytes()),
			toytlv.Record('V', ParseIDString("b-345").ZipBytes()),
		),
		toytlv.Concat(
			toytlv.Record('V', ParseIDString("a-234").ZipBytes()),
			toytlv.Record('V', ParseIDString("b-344").ZipBytes()),
			toytlv.Record('V', ParseIDString("c-567").ZipBytes()),
		),
	}
	actual := Vmerge(args)
	correct := toytlv.Concat(
		toytlv.Record('V', ParseIDString("a-234").ZipBytes()),
		toytlv.Record('V', ParseIDString("b-345").ZipBytes()),
		toytlv.Record('V', ParseIDString("c-567").ZipBytes()),
	)
	//ac := Vplain(actual)
	//fmt.Println(ac.String())
	assert.Equal(t, correct, actual)

}
