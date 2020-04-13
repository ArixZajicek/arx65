CC=g++
CFLAGS=-O2 `pkg-config --cflags --libs gtk+-3.0` -lglut -lGL -I../include

#Please excuse the crudity of this file :) 

arx65: ../src/*.cpp 
	$(CC) ../src/*.cpp ../src/*/*.cpp $(CFLAGS) -o arx65
