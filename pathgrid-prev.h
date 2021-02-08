#ifndef __PATHGRID_H__
#define __PATHGRID_H__

#include <string.h>
#include <cstdint>
#include <vector>

class PathGrid {
    int width, height, wcount;
    uint64_t *fixed;
    uint64_t *loose;
    bool ready;

public:
    class PathFinder {
        PathGrid *grid;
        int wcount, xTarget, yTarget;
        bool toward;
        uint64_t *dirs;

    public:
        PathFinder(PathGrid* grid);
        ~PathFinder();

    private:
        static uint32_t altDirTable[63];
        void setDir(int x,int y,uint32_t v);
        uint32_t getDir(int x,int y);

    public:
        void destination(int x,int y,bool toward=true,int spread=-1);
        void getDestination(int& xDest,int& yDest);

        bool nextStep(int x,int y,int& xStep,int& yStep);
        bool direction(int x,int y,int& xOff,int& yOff);
    };

    PathGrid(int w,int h);
    ~PathGrid();

private:
    bool clipRect(int& x,int& y,int& w,int& h);
    void setBit(uint64_t *grid,bool on,int x,int y);
    void setBits(uint64_t *grid,bool on,int x,int y,int w,int h);
    bool isOn(uint64_t *grid,int x,int y);
    bool rectHasOn(uint64_t *grid,int x,int y,int w,int h);

public:

    inline int getWidth() { return width; }
    inline int getHeight() { return height; }

    bool getStatic(int x,int y);
    bool getDynamic(int x,int y);

    void addStatic(int x,int y,int w=1,int h=1);
    void removeStatic(int x,int y,int w=1,int h=1);

    void addDynamic(int x,int y,int w=1,int h=1);
    void removeDynamic(int x,int y,int w=1,int h=1);

    PathFinder* generatePath(int xTo,int yTo);
    PathFinder* generateFlee(int xFrom,int yFrom);
};

#endif // __PATHGRID_H__
