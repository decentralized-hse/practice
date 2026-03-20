#!/bin/bash
set -euo pipefail

NODE_NAME=$(hostname)

# init теперь принимает имя и SSH-адрес
/opt/gossipsync/gossipsync init "$NODE_NAME" "root@${NODE_NAME}:/root/.gossipsync/feeds/" &>/dev/null || true

# Публичный ключ подписчика -> authorized_keys
if [ -f /mnt/guest_key.pub ]; then
    mkdir -p /root/.ssh
    chmod 700 /root/.ssh
    cat /mnt/guest_key.pub > /root/.ssh/authorized_keys
    chown root:root /root/.ssh/authorized_keys
    chmod 600 /root/.ssh/authorized_keys
fi

exec /usr/sbin/sshd -D -e
