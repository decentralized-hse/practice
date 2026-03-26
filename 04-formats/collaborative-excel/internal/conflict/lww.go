package conflict

func ShouldReplace(currentClock uint64, currentActor string, incomingClock uint64, incomingActor string) bool {
	if incomingClock > currentClock {
		return true
	}

	if incomingClock < currentClock {
		return false
	}

	return incomingActor > currentActor
}

func MaxClock(currentClock uint64, incomingClock uint64) uint64 {
	if incomingClock > currentClock {
		return incomingClock
	}
	return currentClock
}
