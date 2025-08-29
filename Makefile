game: game.cpp
	g++ -o game game.cpp -I "*\SDL\x86_64-w64-mingw32\include" -I "*\SDL3_image\x86_64-w64-mingw32\include" -L "*\SDL\x86_64-w64-mingw32\lib" -lSDL3 -L "*\SDL3_image\x86_64-w64-mingw32\lib" -lSDL3_image -std=c++20
clean:
	rm game.exe


# Replace * in the quoted sections with wherever you placed your SDL files
