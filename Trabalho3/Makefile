CC = gcc
CFLAGS = -Wall -g
TARGETS = udp_cliente udp_servidor

all: $(TARGETS)

udp_cliente: udp_cliente.c
	$(CC) $(CFLAGS) -o cliente udp_cliente.c

udp_servidor: udp_servidor.c
	$(CC) $(CFLAGS) -o servidor udp_servidor.c

clean:
	rm -f $(TARGETS)
