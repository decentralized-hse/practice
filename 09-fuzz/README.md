#   Задачи лекции "фаззинг"

Задание: взять любое решение прошлого года из практики [Форматы
сериализации][p] и фаззить на round-trip guarantee. А именно:

 1. решение должно отклонять любой ввод, не соответствующий формату
    (выход с кодом -1 и сообщением Malformed input в stderr)
 2. решение не должно крашиться
 3. round-trip преобразование должно создать побитно идентичный файл

Если решение прошлого года называлось `04-formats/xml-ivanov.c`, вы
создаёте копию `09-fuzz/xml-ivanov-petrov.c` (если ваша фамилия petrov),
фаззите, правите баги, и отправляете PR.
Лектор проверяет, выдержит ли ваш код k минут фаззинга на его ноутбуке.
Рекомендуется оставлять в коде комментарии об исправленных багах.
Формальные требования к формату ввода см в `bin-gritzko-check.c`.

Для golang рекомендую использовать встроенный фаззер [go-fuzz][g]. Для С и С++
рекомендую [libfuzzer][c]. С остальными сам не работал, ничего не рекомендую.
Для [Java][j], [Rust][r] и других популярных языков популярные фаззеры тоже
есть.


[p]: https://github.com/decentralized-hse/practice/tree/main/04-formats
[g]: https://go.dev/doc/tutorial/fuzz
[c]: https://github.com/google/fuzzing/blob/master/tutorial/libFuzzerTutorial.md
[j]: https://www.code-intelligence.com/blog/java-fuzzing-with-jazzer
[r]: https://rust-fuzz.github.io/book/introduction.html
