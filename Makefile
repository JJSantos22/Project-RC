project: Gs Player

Player:
		g++ -o Player.o Player.cpp
		
Gs:
		g++ -o Gs.o Gs.cpp
clean:
		rm *.o