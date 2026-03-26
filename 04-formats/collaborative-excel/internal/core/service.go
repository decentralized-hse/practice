package core

import (
	"collaborative-excel/internal/conflict"
	"sync"
	"time"
)

type Service struct {
	mu       sync.Mutex
	actorID  string
	clock    uint64
	sheet    *Sheet
	watchers []chan AppliedUpdate
}

type AppliedUpdate struct {
	Cell  Cell
	Dirty bool
}

func NewService(actorID string) *Service {
	return &Service{
		actorID: actorID,
		sheet:   NewSheet(),
	}
}

func (s *Service) ActorID() string {
	return s.actorID
}

func (s *Service) Clock() uint64 {
	s.mu.Lock()
	defer s.mu.Unlock()

	return s.clock
}

func (s *Service) LocalUpdate(cellID CellID, value CellValue) (SetCellOp, AppliedUpdate) {
	s.mu.Lock()
	s.clock++

	op := SetCellOp{
		CellID:    cellID,
		Value:     value,
		Clock:     s.clock,
		ActorID:   s.actorID,
		UpdatedAt: time.Now().UTC(),
	}
	s.mu.Unlock()

	cell, dirty := s.sheet.Apply(op)
	update := AppliedUpdate{Cell: cell, Dirty: dirty}

	if dirty {
		s.broadcast(update)
	}

	return op, update
}

func (s *Service) RemoteUpdate(op SetCellOp) AppliedUpdate {
	s.mu.Lock()
	s.clock = conflict.MaxClock(s.clock, op.Clock)
	s.mu.Unlock()

	cell, dirty := s.sheet.Apply(op)
	update := AppliedUpdate{Cell: cell, Dirty: dirty}

	if dirty {
		s.broadcast(update)
	}

	return update
}

func (s *Service) GetCell(id CellID) (Cell, bool) {
	return s.sheet.GetCell(id)
}

func (s *Service) Snapshot() map[CellID]Cell {
	return s.sheet.Snapshot()
}

func (s *Service) Subscribe() <-chan AppliedUpdate {
	ch := make(chan AppliedUpdate, 64)

	s.mu.Lock()
	defer s.mu.Unlock()

	s.watchers = append(s.watchers, ch)
	return ch
}

func (s *Service) broadcast(update AppliedUpdate) {
	s.mu.Lock()
	watchers := append([]chan AppliedUpdate{}, s.watchers...)
	s.mu.Unlock()

	for _, watcher := range watchers {
		select {
		case watcher <- update:
		default:
		}
	}
}
