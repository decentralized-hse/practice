package bason

import (
	"strings"
	"testing"
)

const (
	benchKeySize   = 8
	benchValueSize = 64
)

func makeLeafRecord() Record {
	return Record{
		Type:  TypeString,
		Key:   strings.Repeat("k", benchKeySize),
		Value: strings.Repeat("v", benchValueSize),
	}
}

// BenchmarkEncodeDecodeThroughput measures encode+decode throughput (target >= 500 MB/s guideline).
func BenchmarkEncodeDecodeThroughput(b *testing.B) {
	record := makeLeafRecord()
	b.ResetTimer()
	var totalBytes int64
	for i := 0; i < b.N; i++ {
		enc, _ := EncodeBason(record)
		dec, _, _ := DecodeBason(enc)
		_ = dec
		totalBytes += int64(len(enc)) * 2 // encode output + decode input
	}
	if b.Elapsed().Seconds() > 0 {
		b.ReportMetric(float64(totalBytes)/(1024*1024)/b.Elapsed().Seconds(), "MB/s")
	}
}

// BenchmarkEncodeThroughput measures encode-only throughput.
func BenchmarkEncodeThroughput(b *testing.B) {
	record := makeLeafRecord()
	b.ResetTimer()
	var totalBytes int64
	for i := 0; i < b.N; i++ {
		enc, _ := EncodeBason(record)
		totalBytes += int64(len(enc))
	}
	if b.Elapsed().Seconds() > 0 {
		b.ReportMetric(float64(totalBytes)/(1024*1024)/b.Elapsed().Seconds(), "MB/s")
	}
}

// BenchmarkDecodeThroughput measures decode-only throughput.
func BenchmarkDecodeThroughput(b *testing.B) {
	record := makeLeafRecord()
	encoded, _ := EncodeBason(record)
	data := encoded
	b.ResetTimer()
	var totalBytes int64
	for i := 0; i < b.N; i++ {
		dec, consumed, _ := DecodeBason(data)
		_ = dec
		totalBytes += int64(consumed)
	}
	if b.Elapsed().Seconds() > 0 {
		b.ReportMetric(float64(totalBytes)/(1024*1024)/b.Elapsed().Seconds(), "MB/s")
	}
}

// BenchmarkJsonToBason measures JSON -> BASON conversion.
func BenchmarkJsonToBason(b *testing.B) {
	json := `{"name":"Alice","age":25,"scores":[95,87,92],"active":true}`
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _ = JsonToBason(json)
	}
}

// BenchmarkBasonToJson measures BASON -> JSON conversion.
func BenchmarkBasonToJson(b *testing.B) {
	json := `{"name":"Alice","age":25,"scores":[95,87,92],"active":true}`
	data, _ := JsonToBason(json)
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, _ = BasonToJson(data)
	}
}

// BenchmarkFlatten measures flatten of a nested record.
func BenchmarkFlatten(b *testing.B) {
	record := Record{
		Type: TypeObject, Key: "",
		Children: []Record{
			{Type: TypeString, Key: "name", Value: "Alice"},
			{Type: TypeNumber, Key: "age", Value: "25"},
			{
				Type: TypeObject, Key: "addr",
				Children: []Record{
					{Type: TypeString, Key: "city", Value: "NYC"},
					{Type: TypeString, Key: "zip", Value: "10001"},
				},
			},
		},
	}
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_ = FlattenBason(record)
	}
}

// BenchmarkUnflatten measures unflatten of flat records.
func BenchmarkUnflatten(b *testing.B) {
	flat := []Record{
		{Type: TypeString, Key: "name", Value: "Alice"},
		{Type: TypeNumber, Key: "age", Value: "25"},
		{Type: TypeString, Key: "addr/city", Value: "NYC"},
		{Type: TypeString, Key: "addr/zip", Value: "10001"},
	}
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_ = UnflattenBason(flat)
	}
}
