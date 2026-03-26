package core

import (
	"testing"
	"time"
)

func TestServiceClockAndApply(t *testing.T) {
	s := NewService("node-a")

	op, applied := s.LocalUpdate("B2", "42")
	if !applied.Dirty {
		t.Fatal("local update must apply")
	}
	if op.Clock != 1 {
		t.Fatalf("expected clock 1, got %d", op.Clock)
	}

	remote := SetCellOp{
		CellID:    "B2",
		Value:     "84",
		Clock:     3,
		ActorID:   "node-b",
		UpdatedAt: time.Now().UTC(),
	}
	applied = s.RemoteUpdate(remote)
	if !applied.Dirty {
		t.Fatal("remote update with larger clock must apply")
	}

	if s.Clock() != 3 {
		t.Fatalf("clock should sync to remote, got %d", s.Clock())
	}

	localOp, localApplied := s.LocalUpdate("B2", "21")
	if !localApplied.Dirty {
		t.Fatal("local follow-up update must apply")
	}
	if localOp.Clock != 4 {
		t.Fatalf("expected next clock 4, got %d", localOp.Clock)
	}
}
