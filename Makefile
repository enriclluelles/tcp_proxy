TARGET = tcp_proxy
LIBS = -lpthread
CC = gcc
CFLAGS = -g -Wall -O0 -Wno-strict-aliasing -Isrc
IMAGE_NAME = tcp_proxy_docker

define _DOCKERFILE
FROM ubuntu:14.04

RUN apt-get update
RUN apt-get install -y make gcc

VOLUME /app
WORKDIR /app
endef

export _DOCKERFILE

.PHONY: default all clean crosscompile

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(SOURCES))
HEADERS = $(wildcard src/*.h)

TEST_SRC = $(wildcard tests/*.c)
TEST_OBJ = $(patsubst %.c, %.o, $(TEST_SRC))

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS) $(TARGET).o
	$(CC) -flto $(TARGET).o $(OBJECTS) $(LIBS) -o $@

clean:
	@rm -f $(OBJECTS) $(TARGET).o
	@rm -f $(TEST_OBJ)
	@rm -f $(TARGET)

#crosscompile to linux using a container
cross:
	@echo "$$_DOCKERFILE" > Dockerfile
	@echo "*" > .dockerignore
	@docker build -t $(IMAGE_NAME) .
	@rm Dockerfile .dockerignore
	docker run --rm -v $(PWD):/app $(IMAGE_NAME) /usr/bin/make clean default
