# GossipSync

Децентрализованный веб-хостинг на `rsync` + `SSH`.
Контент распространяется транзитивно через граф дружбы.

## Как это работает

Каждый узел хранит директорию `feeds/` со всеми известными лентами.
Когда друг синхронизируется с тобой, он забирает всю `feeds/` —
и твою ленту, и всё, что к тебе пришло от других.
```
Алиса --- Боб --- Чарли --- Дейв

Алиса публикует пост.
Раунд 1: Боб забирает у Алисы.
Раунд 2: Чарли забирает у Боба → получает и Алису.
Раунд 3: Дейв забирает у Чарли → получает Алису, Боба, Чарли.
```

Никакой специальной маршрутизации нет. Транзитивность —
следствие того, что каждый хранит и отдаёт всё.

## Сборка
```bash
make
```

## Быстрый старт
```bash
# Создать узел
./gossipsync init charlie guest_sync@charlie.net:/~/.gossipsync/feeds/

# Добавить друзей
./gossipsync add-friend alice guest_sync@alice.net:/~/.gossipsync/feeds/
./gossipsync add-friend bob guest_sync@bob.net:/~/.gossipsync/feeds/

# Подписаться (фильтр для чтения, sync качает всё от друзей)
./gossipsync subscribe alice

# Синхронизация
./gossipsync sync

# Опубликовать пост (тело из stdin)
echo "Привет мир" | ./gossipsync post "Первый пост"

# Посмотреть все ленты
./gossipsync feeds
```

## Тестирование в Docker

Docker-compose поднимает три узла (alice, bob, charlie),
каждый со своим SSH-сервером. Они видят друг друга по имени.
```bash
# Сгенерировать ключ (один раз)
mkdir -p ~/.gossipsync
ssh-keygen -t ed25519 -f ~/.gossipsync/guest_key -N '' -q

# Поднять узлы
docker compose up -d --build

# Зайти в Боба, подружиться с Алисой, синхронизироваться
docker exec -it gossipsync-bob bash
/opt/gossipsync/gossipsync add-friend alice root@alice:/root/.gossipsync/feeds/
/opt/gossipsync/gossipsync sync
/opt/gossipsync/gossipsync feeds
```

## Структура проекта
```
├── src/main.cpp              точка входа, разбор команд
├── include/
│   ├── common.hpp            типы (Paths, Friend, Subscription) и утилиты
│   ├── publisher.hpp         init, post, list-feed
│   ├── friends.hpp           add-friend, remove-friend, friends
│   └── subscriber.hpp        subscribe, sync, feeds
├── Makefile
├── Dockerfile
├── docker-compose.yml
├── docker-entrypoint.sh
└── rsync_ssh_decentralized_web.md   теоретический документ
```

## Данные на диске
```
~/.gossipsync/
├── node.conf              имя узла и SSH-адрес
├── friends.txt            список друзей
├── subscriptions.txt      на какие ленты подписан
├── guest_key              SSH-ключ
└── feeds/                 все ленты
    ├── charlie/           моя лента
    ├── alice/             от друга
    ├── dave/              транзит (пришла через друга)
    └── ...
```

## Алгоритм sync

1. Для каждого друга — `rsync --list-only`, чтобы узнать какие ленты у него есть.
2. Для каждой ленты — отдельный `rsync -avz` этой подпапки.
   Не всей `feeds/` целиком, чтобы не затереть ленты от других друзей.
3. Свою ленту пропускаем.
4. `--jitter` — случайная задержка 0–5 мин для cron.

## Ограничения

**NAT** — друзья должны быть доступны по SSH.
Обходные пути: IPv6, Tor, Tailscale.

**Подмена** — промежуточный узел может изменить файлы.
Нужна GPG-подпись, в текущей версии не реализована.

**Скорость** — контент доходит за O(d) раундов sync,
где d — расстояние в графе дружбы.

**Thundering herd** — если все cron-задачи срабатывают одновременно.
Решается флагом `--jitter`.
