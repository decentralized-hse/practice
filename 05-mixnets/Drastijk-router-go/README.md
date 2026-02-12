### MTU и Congestion threshold

В рамках задачи реализовно:
1. Механизм MTU
2. Интегрирован алгоритм контроля загрузки сети.
Алгоритм реализован по мотивам этого объяснения: https://www.eventhelix.com/congestion-control/algorithms/
 
Значения для работы алгоритма и MTU выставлены в `константах в peer.go`