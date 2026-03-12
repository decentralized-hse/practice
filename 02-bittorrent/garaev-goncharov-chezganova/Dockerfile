FROM debian:bookworm-slim

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        openssh-server \
        rsync && \
    rm -rf /var/lib/apt/lists/*

# Собираем gossipsync внутри контейнера
WORKDIR /opt/gossipsync
COPY src src
COPY include include
COPY Makefile .
RUN make

# SSHD runtime directory
RUN mkdir -p /var/run/sshd

# Базовая настройка sshd
RUN sed -ri 's/^#?PermitRootLogin .*/PermitRootLogin without-password/' /etc/ssh/sshd_config && \
    sed -ri 's/^#?PasswordAuthentication .*/PasswordAuthentication no/' /etc/ssh/sshd_config && \
    sed -ri 's/^#?ChallengeResponseAuthentication .*/ChallengeResponseAuthentication no/' /etc/ssh/sshd_config && \
    sed -ri 's/^#?UsePAM .*/UsePAM no/' /etc/ssh/sshd_config

# Скрипт entrypoint, который подтягивает публичный ключ подписчика с хоста
# и кладёт его в /root/.ssh/authorized_keys
COPY docker-entrypoint.sh /usr/local/bin/docker-entrypoint.sh
RUN chmod +x /usr/local/bin/docker-entrypoint.sh

EXPOSE 22

ENTRYPOINT ["/usr/local/bin/docker-entrypoint.sh"]

