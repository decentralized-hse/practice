#!/bin/bash
set -euo pipefail

# Инициализируем GossipSync как автора (создаст ~/.gossipsync/public_feed и прочее)
/opt/gossipsync/gossipsync init &>/dev/null || true

# Если с хоста смонтирован публичный ключ подписчика, используем его как root authorized_keys
if [ -f /mnt/guest_key.pub ]; then
  mkdir -p /root/.ssh
  chmod 700 /root/.ssh
  cat /mnt/guest_key.pub > /root/.ssh/authorized_keys
  chown root:root /root/.ssh/authorized_keys
  chmod 600 /root/.ssh/authorized_keys
fi

# Заменить shell-процесс на sshd, чтобы он был главным процессом контейнера
exec /usr/sbin/sshd -D -e 

