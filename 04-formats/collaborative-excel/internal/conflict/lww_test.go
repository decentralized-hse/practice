package conflict

import "testing"

func TestShouldReplace(t *testing.T) {
	cases := []struct {
		name          string
		currentClock  uint64
		currentActor  string
		incomingClock uint64
		incomingActor string
		want          bool
	}{
		{
			name:          "incoming greater clock",
			currentClock:  1,
			currentActor:  "node-a",
			incomingClock: 2,
			incomingActor: "node-b",
			want:          true,
		},
		{
			name:          "incoming smaller clock",
			currentClock:  3,
			currentActor:  "node-a",
			incomingClock: 2,
			incomingActor: "node-z",
			want:          false,
		},
		{
			name:          "equal clock incoming actor greater",
			currentClock:  3,
			currentActor:  "node-b",
			incomingClock: 3,
			incomingActor: "node-c",
			want:          true,
		},
		{
			name:          "equal clock incoming actor smaller",
			currentClock:  3,
			currentActor:  "node-c",
			incomingClock: 3,
			incomingActor: "node-b",
			want:          false,
		},
	}

	for _, tt := range cases {
		t.Run(tt.name, func(t *testing.T) {
			if got := ShouldReplace(tt.currentClock, tt.currentActor, tt.incomingClock, tt.incomingActor); got != tt.want {
				t.Fatalf("expect %v, got %v", tt.want, got)
			}
		})
	}
}
