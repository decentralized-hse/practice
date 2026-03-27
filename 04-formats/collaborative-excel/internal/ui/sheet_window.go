package ui

import (
	"collaborative-excel/internal/core"
	"fmt"
	"sync"
	"time"

	"fyne.io/fyne/v2"
	"fyne.io/fyne/v2/app"
	"fyne.io/fyne/v2/container"
	"fyne.io/fyne/v2/widget"
)

func Run(service *core.Service, rows, cols int, publishLocal func(core.SetCellOp), remote <-chan core.AppliedUpdate) {
	application := app.New()
	window := application.NewWindow("Collaborative Excel")
	window.SetContent(NewSheetWindow(application, service, rows, cols, publishLocal, remote))
	window.Resize(fyne.NewSize(1000, 700))
	window.ShowAndRun()
}

func NewSheetWindow(application fyne.App, service *core.Service, rows, cols int, publishLocal func(core.SetCellOp), remote <-chan core.AppliedUpdate) fyne.CanvasObject {
	// keep current values to avoid querying service for every paint.
	values := valuesSnapshot(service.Snapshot())
	entries := make(map[core.CellID]*widget.Entry)
	entriesMu := sync.RWMutex{}
	publishDelay := 300 * time.Millisecond
	publishTimers := make(map[core.CellID]*time.Timer)
	publishVersions := make(map[core.CellID]int)

	scheduleLocalPublish := func(cellID core.CellID, value string) {
		entriesMu.Lock()
		defer entriesMu.Unlock()

		if existing := values[cellID]; existing == value {
			return
		}

		if existingTimer, ok := publishTimers[cellID]; ok {
			existingTimer.Stop()
		}

		nextVersion := publishVersions[cellID] + 1
		publishVersions[cellID] = nextVersion

		timer := time.AfterFunc(publishDelay, func(cellID core.CellID, value string, expectedVersion int) func() {
			return func() {
				entriesMu.Lock()
				if publishVersions[cellID] != expectedVersion {
					entriesMu.Unlock()
					return
				}
				publishVersions[cellID] = expectedVersion
				delete(publishTimers, cellID)
				entriesMu.Unlock()

				op, applied := service.LocalUpdate(cellID, core.CellValue(value))
				if !applied.Dirty {
					return
				}

				entriesMu.Lock()
				values[cellID] = string(op.Value)
				entriesMu.Unlock()

				if publishLocal != nil {
					publishLocal(op)
				}
			}
		}(cellID, value, nextVersion))

		publishTimers[cellID] = timer
	}

	content := container.NewVBox()
	content.Add(makeHeaderRow(cols))

	for row := 1; row <= rows; row++ {
		rowContainer := container.NewGridWithColumns(cols + 1)

		rowContainer.Add(widget.NewLabel(fmt.Sprintf("%d", row)))

		for col := 1; col <= cols; col++ {
			cellID := makeCellID(row, col)
			entry := widget.NewEntry()
			entry.SetText(values[cellID])
			entriesMu.Lock()
			entries[cellID] = entry
			entriesMu.Unlock()

			currentCellID := cellID
			entry.OnChanged = func(value string) {
				scheduleLocalPublish(currentCellID, value)
			}
			entry.OnSubmitted = func(value string) {
				scheduleLocalPublish(currentCellID, value)
			}

			rowContainer.Add(entry)
		}

		content.Add(rowContainer)
	}

	if remote != nil {
		go func() {
			for update := range remote {
				if !update.Dirty {
					continue
				}
				cellID := update.Cell.ID
				value := string(update.Cell.Value)
				entriesMu.Lock()
				values[cellID] = value
				entry, ok := entries[cellID]
				entriesMu.Unlock()
				if !ok {
					continue
				}

				fyne.Do(func() {
					entry.SetText(value)
				})
			}
		}()
	}

	return container.NewScroll(content)
}

func makeHeaderRow(cols int) *fyne.Container {
	header := container.NewGridWithColumns(cols + 1)
	header.Add(widget.NewLabel(""))

	for col := 1; col <= cols; col++ {
		header.Add(widget.NewLabel(columnLabel(col - 1)))
	}

	return header
}

func makeCellID(row, col int) core.CellID {
	return core.CellID(fmt.Sprintf("%s%d", columnLabel(col-1), row))
}

func valuesSnapshot(snapshot map[core.CellID]core.Cell) map[core.CellID]string {
	values := make(map[core.CellID]string, len(snapshot))
	for id, cell := range snapshot {
		values[id] = string(cell.Value)
	}
	return values
}

func columnLabel(index int) string {
	if index < 0 {
		return ""
	}

	result := make([]byte, 0)
	for {
		remainder := index % 26
		result = append([]byte{byte('A' + remainder)}, result...)
		index = index / 26

		if index == 0 {
			return string(result)
		}

		index--
	}
}
