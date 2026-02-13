FROM debian:latest

RUN apt-get update && \
    apt-get install -y build-essential wget && \
    wget https://download.libsodium.org/libsodium/releases/libsodium-1.0.20.tar.gz && \
    tar -xzf libsodium-1.0.20.tar.gz && \
    cd libsodium-1.0.20 && \
    ./configure && \
    make && \
    make install && \
    ldconfig && \
    apt-get purge -y build-essential wget && \
    apt-get autoremove -y && \
    rm -rf /var/lib/apt/lists/* libsodium-1.0.20*

RUN apt-get update && \
    apt-get install -y openssh-server sudo && \
    mkdir /var/run/sshd && \
    rm -rf /var/lib/apt/lists/*

RUN useradd -ms /bin/bash testuser && \
    echo "testuser:testpass" | chpasswd && \
    echo "testuser ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

RUN sed -i 's/#PasswordAuthentication yes/PasswordAuthentication yes/' /etc/ssh/sshd_config && \
    sed -i 's/UsePAM yes/UsePAM no/' /etc/ssh/sshd_config

EXPOSE 22
CMD ["/usr/sbin/sshd", "-D"]
