# Ubuntu tabanlı bir imaj kullanıyoruz (Pop!_OS ile uyumlu)
FROM ubuntu:22.04

# Etkileşimsiz kurulum için değişken
ARG DEBIAN_FRONTEND=noninteractive

# Gerekli araçların kurulumu
# build-essential: gcc, g++, make vb.
# gdb: Debug işlemi için şart
# cmake ve git: Proje yönetimi için
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    gdb \
    git \
    curl \
    wget \
    && apt-get clean -y && rm -rf /var/lib/apt/lists/*

# (Opsiyonel) Eğer vcpkg vb. paket yöneticileri kullanacaksan buraya ekleyebilirsin.

# Çalışma dizini (Otomatik olarak map edilecek ama biz yine de belirtelim)
WORKDIR /workspace