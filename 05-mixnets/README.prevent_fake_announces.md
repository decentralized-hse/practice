## Preventing fake announces

A simple strategy to prevent routing table flooding (fake announces), based on announce proof of work

### Логика работы:  

> Всё построено на понятии "счастливый хэш" - хэш, первые n бит которого нули  

Route, совершающий анонс, должен посчитать и передать такой хэш, что 
* `sha256(happy_hash)`
* `sha256(sha256(happy_hash))`
* ...
* `sha256(sha256(...sha256(announce)...)`

являются "счастливыми"

> Длина такой последовательности счастливых хэшей должна соответствовать диаметру сети

Каждый route при получении `hash` должен передавать его дальше только в случае, если `sha256(hash)` - счастливый хэш

###
### Proof of work

Чтобы подобрать такой `happy_hash`, отправителю нужно перебрать порядка `n^diam` значений, где `diam` - диаметр сети