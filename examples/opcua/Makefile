# SPDX-License-Identifier: CC0-1.0
# Construit le client OPC UA en lecture seule et le faux serveur de l'exemple.
# Dépend de libopen62541-1.4-dev (Debian).
CFLAGS ?= -O2 -g -Wall -Wextra -Wformat=2
OPEN62541 = $(shell pkg-config --cflags --libs open62541)

all: opcua-lire faux-serveur-opcua

opcua-lire: opcua-lire.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $< $(OPEN62541)

faux-serveur-opcua: faux-serveur-opcua.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $< $(OPEN62541) -lm

clean:
	rm -f opcua-lire faux-serveur-opcua

.PHONY: all clean
