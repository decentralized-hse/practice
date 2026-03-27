# Сценарий: два терминала + браузер (реальный Beagle)

## Заводим тестовый worktree

```bash
mkdir -p ~/beagle-worktrees/demo
cd ~/beagle-worktrees/demo

mkdir -p src docs config tests web scripts pkg data

cat > README.md << 'EOF'
# Demo worktree
EOF

cat > src/main.c << 'EOF'
#include <stdio.h>
#include "util.h"
int main(void) {
    printf("version %s\n", DEMO_VERSION);
    return 0;
}
EOF

cat > src/util.h << 'EOF'
#ifndef UTIL_H
#define UTIL_H
#define DEMO_VERSION "1.0.0"
#endif
EOF

cat > tests/smoke.sh << 'EOF'
#!/bin/sh
echo "smoke ok"
EOF
chmod +x tests/smoke.sh

cat > scripts/helper.sh << 'EOF'
#!/bin/sh
echo "helper"
EOF
chmod +x scripts/helper.sh
```

## Подготовка

### Сварить `be` и `be-srv`

Оба бинарника собираются из репозитория **[librdx](https://github.com/gritzko/librdx)**.

```bash
cd ~
git clone https://github.com/gritzko/librdx.git
cd librdx
```

Дальше — по [официальному README librdx](https://github.com/gritzko/librdx/blob/master/README.md):

```bash
# пример для macOS с Homebrew (подробности — в README librdx)
brew install cmake ninja rocksdb libsodium curl lz4
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig"   # на Intel: /usr/local/lib/pkgconfig

cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang \
  -DWITH_INET=ON \
  -DCMAKE_C_FLAGS="-I/opt/homebrew/include" \
  -DCMAKE_EXE_LINKER_FLAGS="-L/opt/homebrew/lib"

ninja -C build be/be
```

Итоговые пути: **`~/librdx/build/be/be`** (CLI) и **`~/librdx/build/be/be-srv`** (HTTP-сервер).

### Пути в `PATH` и первый `post`

```bash
mkdir -p ~/bin
ln -sf ~/librdx/build/be/be ~/bin/be
ln -sf ~/librdx/build/be/be-srv ~/bin/be-srv
export PATH="$HOME/bin:$PATH"

cd ~/beagle-worktrees/demo
be post //demo.local/@myapp
```

### Проверка

```bash
test -f .be && cat .be
ls -la ~/.be/demo.local/
```

Если **`~/.be/demo.local/` нет** — `be post` не отработал; читай сообщение об ошибке и чини.

---

## Запуск

### Процесс 1 — HTTP Beagle (`be-srv`)

```bash
cd ~/beagle-worktrees/demo
~/bin/be-srv 8080
```

Ожидаемо: строка про **listening on port 8080**.

Если получаешь ROCKFAIL - возможно это из‑за лишнего `be-srv`. Проверь

```bash
pgrep -fl be-srv || true
```

Если видишь лишний `be-srv` - кильни его (`kill <pid>`)


### Процесс 2 — tarballer / releaser

Репозиторий приложения: **[seshWCS/tarballer_releaser](https://github.com/seshWCS/tarballer_releaser)**.

Нужен установленный **Go**.

```bash
cd ~
git clone https://github.com/seshWCS/tarballer_releaser.git
cd ~/tarballer_releaser

go build -o tarballer-releaser .
./tarballer-releaser --beagle http://127.0.0.1:8080 --listen :8888
```

В логе должно быть **be-srv reachable** и **listening on :8888**.

---

## Проверка

| Действие | URL |
|----------|-----|
| Главная | http://127.0.0.1:8888/ |
| Обзор файлов (корень) | http://127.0.0.1:8888/browse/ |
| Файл (пример) | http://127.0.0.1:8888/browse/README.md |
| Список релизов | http://127.0.0.1:8888/releases |
| Новый релиз (форма) | http://127.0.0.1:8888/releases/new |

Создание релиза в UI:

1. **Releases → New** (или `/releases/new`).
2. Заполни **Name** (например `v0.1.0`), **Description**, при необходимости **Formula**, **Prefix** (например `/` или `/src/`).
3. Отправь форму — откроется страница релиза.
4. Скачай архив: ссылка на tarball или  
   `http://127.0.0.1:8888/releases/v0.1.0/tarball`.

Просмотр с формулой релиза:

- `http://127.0.0.1:8888/releases/v0.1.0/browse/`
