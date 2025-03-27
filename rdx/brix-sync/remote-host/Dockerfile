FROM ubuntu:20.04

RUN apt-get update && \
    apt-get install -y openssh-server sudo && \
    mkdir /var/run/sshd

RUN useradd -ms /bin/bash testuser && \
    echo "testuser:testpass" | chpasswd && \
    echo "testuser ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

RUN sed -i 's/#PasswordAuthentication yes/PasswordAuthentication yes/' /etc/ssh/sshd_config && \
    sed -i 's/UsePAM yes/UsePAM no/' /etc/ssh/sshd_config

EXPOSE 22
CMD ["/usr/sbin/sshd", "-D"]
