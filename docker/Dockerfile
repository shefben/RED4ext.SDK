FROM ubuntu:20.04
RUN apt-get update && apt-get install -y build-essential cmake
COPY . /src
WORKDIR /src
RUN cmake -Bbuild -S cp2077-coop && cmake --build build -j$(nproc)
