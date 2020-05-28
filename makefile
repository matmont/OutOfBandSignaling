# ---------------------------------------------------------------------------
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2 as 
#  published by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
#  As a special exception, you may use this file as part of a free software
#  library without restriction.  Specifically, if other files instantiate
#  templates or use macros or inline functions from this file, or you compile
#  this file and link it with other files to produce an executable, this
#  file does not by itself cause the resulting executable to be covered by
#  the GNU General Public License.  This exception does not however
#  invalidate any other reasons why the executable file might be covered by
#  the GNU General Public License.
#
# ---------------------------------------------------------------------------

# # # # # # # # # # # # # # # # 
#    Matteo Montalbetti       #
#    MAT. 559881              #
#    Corso B                  #
#    "OUT-OF-BAND SIGNALING"  #
# # # # # # # # # # # # # # # #

# Dichiarazione di alcune variabili che verranno poi
# riutilizzate nel makefile

# Comando di compilazione
CC = gcc
# Opzioni di compilazione
CFLAGS += -std=c99 -Wall -pedantic
# Altre opzioni di compilazione
INCLUDES = -I.
LDFLAGS = -L.
# Flag per il debugging
OPTFLAGS = -g   
# Flag per indicare la presenza di thread
LIBS = -lpthread
# Lista degli oggetti corrispondenti ai file ausiliari da me creati
AUX = aux-functions.o err-managements.o pipes.o
# Lista dei target di questo makefile
TARGETS	= supervisor       \
		  server		   \
		  client		   \

# Lista dei vari comandi presenti
.PHONY: all clean test

# Compilazione e linking automatica di tutti i file
%: %.c
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< $(LDFLAGS) 

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -c -o $@ $<

all: $(TARGETS)

# Cio che ho modificato rispetto al modello precedentemente fornito.
# Ho inserito, per ogni eseguibile, le dipendenze rispetto al corrispondente
# file oggetto e ai file oggetto delle funzioni ausiliare utilizzate.
supervisor: supervisor.o aux-functions.o err-managements.o pipes.o
	$(CC) $(CCFLAGS) $(INCLUDES) $(OPTFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

server: server.o err-managements.o
	$(CC) $(CCFLAGS) $(INCLUDES) $(OPTFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

client: client.o aux-functions.o err-managements.o
	$(CC) $(CCFLAGS) $(INCLUDES) $(OPTFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

# Lancio del test tramite lo script apposito
test:
	bash test.sh

# Comandi di pulizia della directory
clean:
	rm -rf OOB-server-* $(TARGETS) *.txt *.o  
