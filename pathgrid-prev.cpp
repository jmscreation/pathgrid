// PathGrid

#include "pathgrid.h"

PathGrid::PathFinder::PathFinder(PathGrid *grid): grid(grid), wcount(grid->width/16 + !!(grid->width % 16)) {
    dirs = new uint64_t[wcount * grid->height];

    // initiate dirs with DIR_UNSET
    memset(dirs, 0, wcount * grid->height * sizeof(uint64_t));
}

PathGrid::PathFinder::~PathFinder() {
    delete[] dirs;
}

#define DIR_UNSET 0
#define DIR_SOLID 1
#define DIR_DESTIN 2

#define MAKE_DIR(x,y) uint32_t(((x)+2) & 3) | (uint32_t(((y)+2) & 3) << 2)
#define DIR_X(dir) ((dir & 3) - 2)
#define DIR_Y(dir) (((dir >> 2) & 3) - 2)

#define PUSH_DIR(x,y,dir) \
    do { target->push_back((x) + (y)*w); setDir((x), (y), (dir)); } while(0)

uint32_t PathGrid::PathFinder::altDirTable[63] = {
    // up-left: -1,-1
    MAKE_DIR(-1,0), MAKE_DIR(0,-1), MAKE_DIR(-1,1), MAKE_DIR(1,-1), MAKE_DIR(0,1), MAKE_DIR(1,0), MAKE_DIR(1,1),
    // up: 0,-1
    MAKE_DIR(-1,-1), MAKE_DIR(1,-1), MAKE_DIR(-1,0), MAKE_DIR(1,0), MAKE_DIR(-1,1), MAKE_DIR(1,1), MAKE_DIR(0,1),
    // up-right: 1,-1
    MAKE_DIR(0,-1), MAKE_DIR(1,0), MAKE_DIR(-1,-1), MAKE_DIR(1,1), MAKE_DIR(-1,0), MAKE_DIR(0,1), MAKE_DIR(-1,1),
    // left: -1,0
    MAKE_DIR(-1,1), MAKE_DIR(-1,-1), MAKE_DIR(0,1), MAKE_DIR(0,-1), MAKE_DIR(1,1), MAKE_DIR(1,-1), MAKE_DIR(1,0),
    // center: 0,0 - irrelevant
    0, 0, 0, 0, 0, 0, 0,
    // right: 1,0
    MAKE_DIR(1,-1), MAKE_DIR(1,1), MAKE_DIR(0,-1), MAKE_DIR(0,1), MAKE_DIR(-1,-1), MAKE_DIR(-1,1), MAKE_DIR(-1,0),
    // down-left: -1,1
    MAKE_DIR(0,1), MAKE_DIR(-1,0), MAKE_DIR(1,1), MAKE_DIR(-1,-1), MAKE_DIR(1,0), MAKE_DIR(0,-1), MAKE_DIR(1,-1),
    // down: 0,1
    MAKE_DIR(1,1), MAKE_DIR(-1,1), MAKE_DIR(1,0), MAKE_DIR(-1,0), MAKE_DIR(1,-1), MAKE_DIR(-1,-1), MAKE_DIR(0,-1),
    // down-right: 1,1
    MAKE_DIR(1,0), MAKE_DIR(0,1), MAKE_DIR(1,-1), MAKE_DIR(-1,1), MAKE_DIR(0,-1), MAKE_DIR(-1,0), MAKE_DIR(-1,-1)
};

void PathGrid::PathFinder::setDir(int x,int y,uint32_t v) {
    if(x >= 0 && x < grid->width && y >= 0 && y < grid->height) {
        int ind = wcount*y + x/16, xoff = x % 16 * 4;
        dirs[ind] = (dirs[ind] & ~(15ull << xoff)) | (uint64_t(v & 15) << xoff);
    }
}
uint32_t PathGrid::PathFinder::getDir(int x,int y) {
    return uint32_t(dirs[wcount*y + x/16] >> x % 16 * 4) & 15;
}

void PathGrid::PathFinder::destination(int x,int y,bool toward,int spread) {
    std::vector<int> a, b, *current = &a, *target = &b;

    // clear dirs for a reset
    memset(dirs, 0, wcount * grid->height * sizeof(uint64_t));

    int w = grid->width, h = grid->height;
    if(x < 0) x = 0; else if(x >= w) x = w-1;
    if(y < 0) y = 0; else if(y >= h) y = h-1;

    xTarget = x;
    yTarget = y;
    this->toward = toward;

    current->push_back(x + y*w);
    setDir(x,y,DIR_DESTIN);

    // if target cell is occupied, find nearest unoccupied cell(s)
    if(grid->isOn(grid->fixed,x,y)) {
        setDir(x,y,DIR_SOLID);

        while(current->size()) {
            // check current set for open cells
            for(int c : *current) {
                x = c%w; y = c/w; // extract x,y

                if(!grid->isOn(grid->fixed,x,y)) // found open cell
                    PUSH_DIR(x, y, DIR_DESTIN);
            }
            if(target->size()) { // we found at least 1 open cell
                current = target;
                target = current == &a ? &b : &a;
                target->clear();
                break;
            }

            // we didn't find any; spread out

            // iterate openset
            for(int c : *current) {
                x = c%w; y = c/w; // extract x,y

                // push all adjacent cells
                if(x > 0 && !getDir(x-1,y)) PUSH_DIR(x-1, y, DIR_SOLID);
                if(x < w-1 && !getDir(x+1,y)) PUSH_DIR(x+1, y, DIR_SOLID);
                if(y > 0 && !getDir(x,y-1)) PUSH_DIR(x, y-1, DIR_SOLID);
                if(y < h-1 && !getDir(x,y+1)) PUSH_DIR(x, y+1, DIR_SOLID);
                if(x > 0) {
                    if(y > 0 && !getDir(x-1,y-1)) PUSH_DIR(x-1, y-1, DIR_SOLID);
                    if(y < h-1 && !getDir(x-1,y+1)) PUSH_DIR(x-1, y+1, DIR_SOLID);
                }
                if(x < w-1) {
                    if(y > 0 && !getDir(x+1,y-1)) PUSH_DIR(x+1, y-1, DIR_SOLID);
                    if(y < h-1 && !getDir(x+1,y+1)) PUSH_DIR(x+1, y+1, DIR_SOLID);
                }
            }

            // openset is filled; now switch to openset and clear previous current as target for next openset
            current = target;
            target = current == &a ? &b : &a;
            target->clear();
        }
    }

    // current is populated with destination(s)
    while(current->size() && spread != 0) {
        // iterate current openset
        for(int c : *current) {
            x = c%w; y = c/w; // extract x,y

            if(x > 0 && !grid->isOn(grid->fixed,x-1,y) && !getDir(x-1,y))
                PUSH_DIR(x-1, y, MAKE_DIR(1,0));
            if(x < w-1 && !grid->isOn(grid->fixed,x+1,y) && !getDir(x+1,y))
                PUSH_DIR(x+1, y, MAKE_DIR(-1,0));
            if(y > 0 && !grid->isOn(grid->fixed,x,y-1) && !getDir(x,y-1))
                PUSH_DIR(x, y-1, MAKE_DIR(0,1));
            if(y < h-1 && !grid->isOn(grid->fixed,x,y+1) && !getDir(x,y+1))
                PUSH_DIR(x, y+1, MAKE_DIR(0,-1));
             if(x > 0) {
                if(y > 0 && !grid->isOn(grid->fixed,x-1,y-1) && !getDir(x-1,y-1))
                    PUSH_DIR(x-1, y-1, MAKE_DIR(1,1));
                if(y < h-1 && !grid->isOn(grid->fixed,x-1,y+1) && !getDir(x-1,y+1))
                    PUSH_DIR(x-1, y+1, MAKE_DIR(1,-1));
            }
            if(x < w-1) {
                if(y > 0 && !grid->isOn(grid->fixed,x+1,y-1) && !getDir(x+1,y-1))
                    PUSH_DIR(x+1, y-1, MAKE_DIR(-1,1));
                if(y < h-1 && !grid->isOn(grid->fixed,x+1,y+1) && !getDir(x+1,y+1))
                    PUSH_DIR(x+1, y+1, MAKE_DIR(-1,-1));
            }
        }
        current = target;
        target = current == &a ? &b : &a;
        target->clear();
        --spread;
    }
}
void PathGrid::PathFinder::getDestination(int& xDest,int& yDest) {
    xDest = xTarget;
    yDest = yTarget;
}
bool PathGrid::PathFinder::nextStep(int x,int y,int& xStep,int& yStep) {
    if(x >= 0 && x < grid->width && y >= 0 && y < grid->height) {
        uint32_t dir = getDir(x,y);
        if(dir == DIR_DESTIN && toward) return false;

        if(dir == DIR_UNSET || dir == DIR_SOLID || dir == DIR_DESTIN) {
            if(x == xTarget && y == yTarget) {
                if(toward)
                    return false;
                else {
                    dir = MAKE_DIR(0,-1);
                }
            } else {
                dir = MAKE_DIR(
                    xTarget < x ? -1 : xTarget > x ? 1 : 0,
                    yTarget < y ? -1 : yTarget > y ? 1 : 0
                );
            }
        }

        uint32_t altDir = dir;
        int flip = toward ? 1 : -1,
            ind = DIR_X(dir) + DIR_Y(dir)*3 + 4, i = 0;

        while(1) {
            xStep = x + DIR_X(altDir)*flip;
            yStep = y + DIR_Y(altDir)*flip;

            if(xStep >= 0 && xStep < grid->width && yStep >= 0 && yStep < grid->height
               && !grid->isOn(grid->loose,xStep,yStep) && !grid->isOn(grid->fixed,xStep,yStep)) {
                int newDir = getDir(xStep,yStep),
                    newX = xStep + DIR_X(newDir)*flip,
                    newY = yStep + DIR_Y(newDir)*flip;
                if(newX != x || newY != y) // don't move to cell that returns right to this cell again
                    return true;
            }

            if(i > 6) break;
            altDir = altDirTable[ind*7 + (i++)]; // if direction cell was loosely occupied, try another direction
        }
    }
    xStep = x;
    yStep = y;
    return false;
}
bool PathGrid::PathFinder::direction(int x,int y,int& xOff,int& yOff) {
    if(x >= 0 && x < grid->width && y >= 0 && y < grid->height) {
        uint32_t dir = getDir(x,y);
        if(dir == DIR_UNSET || dir == DIR_SOLID || dir == DIR_DESTIN) return false;
        xOff = DIR_X(dir);
        yOff = DIR_Y(dir);
        return true;
    }
    return false;
}

PathGrid::PathGrid(int w,int h): width(w), height(h), wcount(w/64 + !!(w % 64)) {
    // wcount: the number of 64-bit values per row
    fixed = new uint64_t[wcount * height];
    loose = new uint64_t[wcount * height];

    // 0-initialize grids
    memset(fixed, 0, wcount * height * sizeof(uint64_t));
    memset(loose, 0, wcount * height * sizeof(uint64_t));

    ready = false;
}
PathGrid::~PathGrid() {
    delete[] fixed;
    delete[] loose;
}

// clip rect at x,y with w,h size to grid's width/height
// returns true if the rect still has area, or false if the rect's w or h == 0
bool PathGrid::clipRect(int& x,int& y,int& w,int& h) {
    // if rect is out of bounds, nothing is affected:
    if(x >= width || y >= height) {
        w = h = 0;
        return false;
    }

    if(x < 0) w += x, x = 0; // left-clip
    if(y < 0) h += y, y = 0; // top-clip

    if(x + w > width) w = width - x; // right-clip
    if(y + h > height) h = height - y; // bottom-clip

    return w && h;
}

// 0 or 1-bits all cells on the grid within the given rect
void PathGrid::setBit(uint64_t *grid,bool on,int x,int y) {
    if(x >= 0 && x < width && y >= 0 && y < height) {
        if(on)
            grid[wcount*y + x/64] |= (1ull << x % 64);
        else
            grid[wcount*y + x/64] &= ~(1ull << x % 64);
    }
}
void PathGrid::setBits(uint64_t *grid,bool on,int x,int y,int w,int h) {
    if(!clipRect(x,y,w,h)) return;

    int _x = (x + w)/64,
        _y = y + h - 1,
        xstart = x % 64,
        xend = (x + w) % 64;
    x /= 64;

    uint64_t mask[2];

    mask[0] = -1ull << xstart; // truncate start
    if(xend) {
        if(x == _x)
            mask[0] &= (-1ull >> (64-xend)); // truncate end
        else
            mask[1] = on ? (-1ull >> (64-xend)) : (-1ull << xend); // truncate end
    } else {
        _x--;
        if(x < _x)
            mask[1] = on ? -1ull : 0;
    }

    uint64_t *c;
    if(on) {
        for(int yy=y; yy<=_y; yy++) {
            c = grid + wcount*yy;
            for(int xx=x; xx<=_x; xx++) {
                if(xx == x)
                    c[xx] |= mask[0];
                else if(xx == _x)
                    c[xx] |= mask[1];
                else
                    c[xx] = -1ull;
            }
        }
    } else {
        mask[0] = ~mask[0];
        for(int yy=y; yy<=_y; yy++) {
            c = grid + wcount*yy;
            for(int xx=x; xx<=_x; xx++) {
                if(xx == x)
                    c[xx] &= mask[0];
                else if(xx == _x)
                    c[xx] &= mask[1];
                else
                    c[xx] = 0ull;
            }
        }
    }
}

// returns if bit at x,y is on
bool PathGrid::isOn(uint64_t *grid,int x,int y) {
    return grid[wcount*y + x/64] & (1ull << x % 64);
}

// returns if any bit in the rect x,y,w,h is on
bool PathGrid::rectHasOn(uint64_t *grid,int x,int y,int w,int h) {
    if(!clipRect(x,y,w,h)) return false;

    int _x = (x + w)/64,
        _y = y + h - 1,
        xstart = x % 64,
        xend = (x + w) % 64;
    x /= 64;

    uint64_t mask[2];

    mask[0] = -1ull << xstart; // truncate start
    if(xend) {
        if(x == _x)
            mask[0] &= (-1ull >> (64-xend)); // truncate end
        else
            mask[1] = (-1ull >> (64-xend)); // truncate end
    } else {
        _x--;
        if(x < _x)
            mask[1] = -1ull;
    }

    uint64_t *c;
    for(int yy=y; yy<=_y; yy++) {
        c = grid + wcount*yy;
        for(int xx=x; xx<=_x; xx++) {
            if(xx == x) {
                if(c[xx] & mask[0])
                    return true;
            } else if(xx == _x) {
                if(c[xx] & mask[1])
                    return true;
            } else if(c[xx])
                return true;
        }
    }
    return false;
}

// get if static / dynamic cells are set
bool PathGrid::getStatic(int x,int y) {
    return isOn(fixed,x,y);
}
bool PathGrid::getDynamic(int x,int y) {
    return isOn(loose,x,y);
}

// adds / removes static and loose regions to the grid
void PathGrid::addStatic(int x,int y,int w,int h) {
    ready = false;

    if(w == 1 && h == 1)
        setBit(fixed,true,x,y);
    else
        setBits(fixed,true,x,y,w,h);
}
void PathGrid::removeStatic(int x,int y,int w,int h) {
    ready = false;

    if(w == 1 && h == 1)
        setBit(fixed,false,x,y);
    else
        setBits(fixed,false,x,y,w,h);
}
void PathGrid::addDynamic(int x,int y,int w,int h) {
    if(w == 1 && h == 1)
        setBit(loose,true,x,y);
    else
        setBits(loose,true,x,y,w,h);
}
void PathGrid::removeDynamic(int x,int y,int w,int h) {
    if(w == 1 && h == 1)
        setBit(loose,false,x,y);
    else
        setBits(loose,false,x,y,w,h);
}

PathGrid::PathFinder* PathGrid::generatePath(int xTo,int yTo) {
    PathFinder *field = new PathFinder(this);
    field->destination(xTo,yTo,true);
    return field;
}
PathGrid::PathFinder* PathGrid::generateFlee(int xFrom,int yFrom) {
    PathFinder *field = new PathFinder(this);
    field->destination(xFrom,yFrom,false);
    return field;
}
