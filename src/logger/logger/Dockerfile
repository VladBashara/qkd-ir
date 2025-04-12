FROM ubuntu

RUN apt update
RUN apt install -y build-essential
RUN apt-get install -y g++
RUN apt-get install -y git

RUN apt install -y cmake

RUN mkdir /opt/logger
WORKDIR /opt/logger

COPY CMakeLists.txt .
COPY logger.h .
COPY logger-main.cpp .
COPY logger.cpp .

RUN mkdir build
WORKDIR /opt/logger/build

RUN cmake ..
RUN cmake --build .

# ENTRYPOINT /bin/bash
