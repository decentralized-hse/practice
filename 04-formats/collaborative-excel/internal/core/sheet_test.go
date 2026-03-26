package core

import (
	"testing"
	"time"
)

func TestSheetApply(t *testing.T) {
	sheet := NewSheet()

	now := time.Now().UTC()
	op1 := SetCellOp{
		CellID:    "A1",
		Value:     "10",
		Clock:     1,
		ActorID:   "node-a",
		UpdatedAt: now,
	}

	cell, applied := sheet.Apply(op1)
	if !applied {
		t.Fatal("first operation should apply")
	}
	if cell.Value != "10" {
		t.Fatalf("unexpected value: %s", cell.Value)
	}

	op2 := SetCellOp{
		CellID:    "A1",
		Value:     "20",
		Clock:     2,
		ActorID:   "node-a",
		UpdatedAt: now.Add(time.Second),
	}
	cell, applied = sheet.Apply(op2)
	if !applied {
		t.Fatal("newer clock operation should apply")
	}
	if cell.Value != "20" {
		t.Fatalf("expected '20', got %q", cell.Value)
	}

	op3 := SetCellOp{
		CellID:    "A1",
		Value:     "30",
		Clock:     2,
		ActorID:   "node-b",
		UpdatedAt: now.Add(2 * time.Second),
	}
	cell, applied = sheet.Apply(op3)
	if !applied {
		t.Fatal("higher actor should replace on equal clock")
	}
	if cell.Value != "30" {
		t.Fatalf("expected '30', got %q", cell.Value)
	}

	op4 := SetCellOp{
		CellID:    "A1",
		Value:     "40",
		Clock:     2,
		ActorID:   "node-a",
		UpdatedAt: now.Add(3 * time.Second),
	}
	cell, applied = sheet.Apply(op4)
	if applied {
		t.Fatal("lower actor should be ignored on equal clock")
	}
	if cell.Value != "30" {
		t.Fatalf("expected kept value '30', got %q", cell.Value)
	}

	snapshot := sheet.Snapshot()
	if len(snapshot) != 1 {
		t.Fatalf("expected one cell, got %d", len(snapshot))
	}
}
