# Variáveis
CC = gcc
CFLAGS = -Wall
LDFLAGS = -lssl -lcrypto   # Flags necessárias para linkar com OpenSSL
SERVER = servidor
CLIENT = cliente
SERVER_SRC = tcp-server.c
CLIENT_SRC = tcp-client.c

# Regras
all: $(SERVER) $(CLIENT)

# Compilar o servidor
$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(SERVER) $(LDFLAGS)

# Compilar o cliente
$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT) $(LDFLAGS)

# Executar o servidor
run-server: $(SERVER)
	./$(SERVER)

# Executar o cliente
run-client: $(CLIENT)
	./$(CLIENT)

# Limpeza dos arquivos gerados
clean:
	rm -f $(SERVER) $(CLIENT)
