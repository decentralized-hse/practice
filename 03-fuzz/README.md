#   Задачи лекции 3 "фаззинг"

Задание: взять любое решение прошлого семестра из практики [Форматы сериализации][p]
и нафаззить до 3х багов с нарушением round-trip guarantee. То есть, найти валидные
С структуры, которые код преобразует в другой формат и обратно так, что результат
не соответствует оригиналу. Если получится краш - ещё лучше.
Для багов сделать фикс и PR.

Для golang рекомендую использовать встроенный фаззер [go-fuzz][g].
Для С и С++ рекомендую [libfuzzer][c].
С остальными сам не работал, ничего не рекомендую. Для [Java][j], [Rust][r] 
и других популярных языков популярные фаззеры тоже есть.


[p]: https://github.com/decentralized-hse/practice/tree/main/04-formats
[g]: https://go.dev/doc/tutorial/fuzz
[c]: https://github.com/google/fuzzing/blob/master/tutorial/libFuzzerTutorial.md
[j]: https://www.code-intelligence.com/blog/java-fuzzing-with-jazzer
[r]: https://rust-fuzz.github.io/book/introduction.html
