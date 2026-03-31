package core

import "time"

type CellID string
type CellValue string

type Cell struct {
	ID        CellID    `json:"id"`
	Value     CellValue `json:"value"`
	Clock     uint64    `json:"clock"`
	ActorID   string    `json:"actor_id"`
	UpdatedAt time.Time `json:"updated_at"`
}

type SetCellOp struct {
	CellID    CellID    `json:"cell_id"`
	Value     CellValue `json:"value"`
	Clock     uint64    `json:"clock"`
	ActorID   string    `json:"actor_id"`
	UpdatedAt time.Time `json:"updated_at"`
}
