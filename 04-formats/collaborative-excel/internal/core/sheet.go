package core

import (
	"collaborative-excel/internal/conflict"
	"sync"
)

type Sheet struct {
	mu    sync.RWMutex
	cells map[CellID]Cell
}

func NewSheet() *Sheet {
	return &Sheet{
		cells: make(map[CellID]Cell),
	}
}

func (s *Sheet) Apply(op SetCellOp) (Cell, bool) {
	s.mu.Lock()
	defer s.mu.Unlock()

	current, ok := s.cells[op.CellID]
	if ok {
		if !conflict.ShouldReplace(current.Clock, current.ActorID, op.Clock, op.ActorID) {
			return current, false
		}
	}

	cell := Cell{
		ID:        op.CellID,
		Value:     op.Value,
		Clock:     op.Clock,
		ActorID:   op.ActorID,
		UpdatedAt: op.UpdatedAt,
	}

	s.cells[op.CellID] = cell
	return cell, true
}

func (s *Sheet) GetCell(id CellID) (Cell, bool) {
	s.mu.RLock()
	defer s.mu.RUnlock()

	cell, ok := s.cells[id]
	return cell, ok
}

func (s *Sheet) Snapshot() map[CellID]Cell {
	s.mu.RLock()
	defer s.mu.RUnlock()

	snapshot := make(map[CellID]Cell, len(s.cells))
	for id, cell := range s.cells {
		snapshot[id] = cell
	}

	return snapshot
}
