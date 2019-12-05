rm -rf bin/Release/TCON_DP_UPDATE_TOOL
rm -rf obj/Release/main.o
g++ -O2 -c main.cpp -o obj/Release/main.o
g++ -o bin/Release/TCON_DP_UPDATE_TOOL obj/Release/main.o -s
