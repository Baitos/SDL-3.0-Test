game: game.cpp
	g++ -o game game.cpp -I "C:\Users\ryani\Development Things\SDL\x86_64-w64-mingw32\include" -I "C:\Users\ryani\Development Things\SDL3_image\x86_64-w64-mingw32\include" -L "C:\Users\ryani\Development Things\SDL\x86_64-w64-mingw32\lib" -lSDL3 -L "C:\Users\ryani\Development Things\SDL3_image\x86_64-w64-mingw32\lib" -lSDL3_image -std=c++20
clean:
	rm game.exe


