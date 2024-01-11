## Preventing fake announces

A simple strategy to prevent routing table flooding (fake announces), based on announce proof of work

### Логика работы:  

> Всё построено на понятии "счастливый хэш" - хэш, первые n бит которого нули  

Route, совершающий анонс, должен посчитать и передать такой `nonce`, что sha256 хэши от
* `announce + nonce`
* `sha256(announce) + nonce`
* ...
* `sha256(sha256(...sha256(announce)...) + nonce`

являются "счастливыми"

> Длина такой последовательности счастливых хэшей должна соответствовать диаметру сети

Каждый route при получении `announce` (и `nonce`) должен передавать его дальше только в случае, если `sha256(announce + nonce)` - счастливый хэш

`nonce` в неизменном виде передается от route к route в качестве payload

###
### Proof of work

Чтобы подобрать такой `nonce`, отправителю нужно перебрать порядка `n^diam` значений, где `diam` - диаметр сети