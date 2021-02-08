#include "cc.cpp"
#include <iostream>
#include "../pathgrid.cpp"
#include <cmath>
#include <ctime>
#include <algorithm>

#define GWIDTH      40
#define GHEIGHT     40

using namespace std;

static PathGrid grid(GWIDTH,GHEIGHT);
static PathGrid::PathFinder *finder;

static const char *hexpair[256] = {
    "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0a", "0b", "0c", "0d", "0e", "0f",
    "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "1a", "1b", "1c", "1d", "1e", "1f",
    "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "2a", "2b", "2c", "2d", "2e", "2f",
    "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3a", "3b", "3c", "3d", "3e", "3f",
    "40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "4a", "4b", "4c", "4d", "4e", "4f",
    "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "5a", "5b", "5c", "5d", "5e", "5f",
    "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6a", "6b", "6c", "6d", "6e", "6f",
    "70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "7a", "7b", "7c", "7d", "7e", "7f",
    "80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "8a", "8b", "8c", "8d", "8e", "8f",
    "90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9a", "9b", "9c", "9d", "9e", "9f",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9", "aa", "ab", "ac", "ad", "ae", "af",
    "b0", "b1", "b2", "b3", "b4", "b5", "b6", "b7", "b8", "b9", "ba", "bb", "bc", "bd", "be", "bf",
    "c0", "c1", "c2", "c3", "c4", "c5", "c6", "c7", "c8", "c9", "ca", "cb", "cc", "cd", "ce", "cf",
    "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "da", "db", "dc", "dd", "de", "df",
    "e0", "e1", "e2", "e3", "e4", "e5", "e6", "e7", "e8", "e9", "ea", "eb", "ec", "ed", "ee", "ef",
    "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "fa", "fb", "fc", "fd", "fe", "ff"
};

class Obj;

void updateCell(int x,int y,Obj* unit=NULL);

class Obj {
public:
    static std::vector<Obj*> objs;
    static int gx, gy;

    Obj(int x,int y);
    ~Obj();

    int x, y, id;
    bool stuck;

    bool collision();
};

std::vector<Obj*> Obj::objs;
int Obj::gx = 0;
int Obj::gy = 0;

Obj::Obj(int x,int y): x(x), y(y), id(objs.size()), stuck(false) {
    objs.push_back(this);
    grid.removeStatic(x,y);
    updateCell(x,y,this);
}
Obj::~Obj() {
    objs.erase(std::find_if(objs.begin(), objs.end(), [this](Obj* o) { return this == o; }));
}
bool Obj::collision() {
    for(Obj* o : objs) {
        if(o == this) continue;
        if(o->x == x && o->y == y)
            return true;
    }
    return false;
}

void updateCell(int x,int y,Obj* unit) {
    if(unit != NULL && x == unit->x && y == unit->y)
        setChar(x*2,y, hexpair[unit->id & 255], unit->stuck ? (FOREGROUND_RED|FOREGROUND_INTENSITY) : 0x0F);
    else if(x == Obj::gx && y == Obj::gy)
        setChar(x*2,y, "GG");
    else if(grid.getStatic(x, y))
        setChar(x*2,y, "лл");
    else {
        int xo,yo;
        if(!finder->direction(x,y,xo,yo))
            setChar(x*2,y, "  ");
        else {
            if(!xo) setChar(x*2,y, yo>0 ? "vv" : "^^", FOREGROUND_INTENSITY);
            else if(!yo) setChar(x*2,y, xo>0 ? ">>" : "<<", FOREGROUND_INTENSITY);
            else if(xo == yo) setChar(x*2,y, xo>0 ? "\\." : "'\\", FOREGROUND_INTENSITY);
            else setChar(x*2,y, xo>0 ? "/'" : "./", FOREGROUND_INTENSITY);
        }
    }
}

void printGrid() {
    int w = grid.getWidth(),
        h = grid.getHeight();

    //cls();

    for(int y=0; y<h; y++)
        for(int x=0; x<w; x++)
            updateCell(x,y);
    setChar(GWIDTH/2, GHEIGHT + 1, (string("                     ")).data());
    setChar(GWIDTH/2, GHEIGHT + 2, (string("NO Warn ")).data());
}

void randomizeGrid() {
    int w = grid.getWidth(),
        h = grid.getHeight();

    for(int y=0; y<h; y++) {
        for(int x=0; x<w; x++)
            if(!(rand() % 7))
                grid.addStatic(x, y, rand()%4, rand()%4);
    }
    grid.addStatic(20,0,3,GHEIGHT);
}

int main() {
    srand(time(NULL));

    randomizeGrid();

    for(int i=0; i<200; ++i) {
        int x,y;
        while(1) {
            x = rand() % GWIDTH;
            y = rand() % GHEIGHT;
            if(grid.getDynamic(x,y))
                continue;

            Obj *o = new Obj(x,y);
            grid.addDynamic(o->x,o->y);
            break;
        }
    }

    //Sleep(5000);

    Obj::gx = rand() % GWIDTH;
    Obj::gy = rand() % GHEIGHT;
    grid.removeStatic(Obj::gx - 1,Obj::gy - 1, 3, 3);
    finder = grid.generatePath(Obj::gx, Obj::gy);

    printGrid();

    int counter = 0;

    while(1) {
        int gx, gy;

        for(Obj *o : Obj::objs) {
            if(finder->nextStep(o->x, o->y, gx, gy) == PathGrid::Move) {
                int px = o->x, py = o->y;
                o->x = gx;
                o->y = gy;
                if(o->collision()) {
                    setChar(GWIDTH/2, GHEIGHT + 2, (string("COLLISION WARNING: ") + to_string(o->id)).data());
                }
                if(gx < 0 || gx >= grid.getWidth() || gy < 0 || gy >= grid.getHeight()) {
                    setChar(GWIDTH/2, GHEIGHT + 2, (string("OUT OF BOUNDS WARNING: ") + to_string(o->x) + "," + to_string(o->y)).data());
                }
                o->stuck = false;
                grid.removeDynamic(px,py);
                grid.addDynamic(o->x,o->y);
                updateCell(px,py,o);
            } else o->stuck = true;

            updateCell(o->x,o->y,o);
        }
        //Sleep(300);
        if(counter++ > 100){
            counter = 0;
            Obj::gx = rand() % GWIDTH;
            Obj::gy = rand() % GHEIGHT;
            bool toward = rand() % 2;
            //grid.removeStatic(Obj::gx - 1,Obj::gy - 1, 3, 3);
            finder->destination(Obj::gx, Obj::gy, toward); //, rand()%80);
            printGrid();
            if(!toward){
                setChar(GWIDTH/2, GHEIGHT + 1, (string("RUN AWAY")).data());
            }
        }
    }
    return 0;
}
