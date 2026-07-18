#include "raylib.h"
#include <climits>

const int CELL_SIZE   = 25;
const int COLS        = 40;
const int ROWS         = 22;
const int TOP_BAR      = 90;
const int BOTTOM_BAR   = 40;
const int SCREEN_W     = COLS * CELL_SIZE;
const int SCREEN_H     = ROWS * CELL_SIZE + TOP_BAR + BOTTOM_BAR;
const int MAX_CELLS    = ROWS * COLS;

const int WEIGHT_COST  = 5;
const int NORMAL_COST  = 1;

const Color BG_COLOR       = { 18, 18, 28, 255 };
const Color EMPTY_COLOR    = { 42, 42, 58, 255 };
const Color GRID_LINE_COLOR= { 70, 70, 95, 255 };
const Color WALL_COLOR     = { 235, 235, 240, 255 };
const Color START_COLOR    = { 57, 255, 140, 255 };
const Color END_COLOR      = { 255, 64, 129, 255 };
const Color VISITED_COLOR  = { 0, 200, 220, 255 };
const Color PATH_COLOR     = { 255, 205, 0, 255 };
const Color WEIGHTED_COLOR = { 178, 89, 255, 255 };
const Color TEXT_COLOR     = { 230, 230, 235, 255 };
const Color HEADER_COLOR   = { 0, 200, 220, 255 };

enum CellState { EMPTY, WALL, START, END, VISITED, PATH, WEIGHTED };

class Position {
public:
    int row, col;

    Position(int r = -1, int c = -1) {
        row = r;
        col = c;
    }

    bool equals(const Position& other) const {
        return row == other.row && col == other.col;
    }
};

class PositionList {
public:
    Position items[MAX_CELLS];
    int count;

    PositionList() {
        count = 0;
    }

    void clear() {
        count = 0;
    }

    void add(const Position& p) {
        items[count] = p;
        count++;
    }

    int size() const {
        return count;
    }

    Position get(int index) const {
        return items[index];
    }

    void reverse() {
        int i = 0, j = count - 1;
        while (i < j) {
            Position temp = items[i];
            items[i] = items[j];
            items[j] = temp;
            i++;
            j--;
        }
    }
};

class OpenEntry {
public:
    int dist, row, col;

    OpenEntry(int dist_ = 0, int row_ = 0, int col_ = 0) {
        dist = dist_;
        row = row_;
        col = col_;
    }
};

class OpenNode {
public:
    int dist, row, col;
    OpenNode* next;

    OpenNode(int dist_, int row_, int col_) {
        dist = dist_;
        row = row_;
        col = col_;
        next = nullptr;
    }
};

class OpenList {
private:
    OpenNode* head;

public:
    OpenList() {
        head = nullptr;
    }

    bool isEmpty() const {
        return head == nullptr;
    }

    void insert(int dist, int row, int col) {
        OpenNode* node = new OpenNode(dist, row, col);
        node->next = head;
        head = node;
    }

    OpenEntry popMin() {
        OpenNode* minNode = head;
        OpenNode* minPrev = nullptr;

        OpenNode* prev = nullptr;
        OpenNode* cur = head;

        while (cur != nullptr) {
            if (cur->dist < minNode->dist) {
                minNode = cur;
                minPrev = prev;
            }
            prev = cur;
            cur = cur->next;
        }

        if (minPrev == nullptr) {
            head = minNode->next;
        } else {
            minPrev->next = minNode->next;
        }

        OpenEntry result(minNode->dist, minNode->row, minNode->col);
        delete minNode;
        return result;
    }
};

CellState grid[ROWS][COLS];
Position parent[ROWS][COLS];
int dist[ROWS][COLS];
bool hasStart = false, hasEnd = false;
Position startPos, endPos;

enum AppState { IDLE, ANIMATING_VISITED, ANIMATING_PATH, DONE, NO_PATH };
AppState appState = IDLE;

PositionList visitedOrder;
PositionList pathCells;
int revealIndex = 0;
float revealTimer = 0.0f;
const float VISIT_DELAY = 0.01f;
const float PATH_DELAY  = 0.03f;

void resetGridKeepWalls() {
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            if (grid[r][c] == VISITED || grid[r][c] == PATH)
                grid[r][c] = EMPTY;
    appState = IDLE;
    visitedOrder.clear();
    pathCells.clear();
    revealIndex = 0;
    revealTimer = 0.0f;
}

void clearEverything() {
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            grid[r][c] = EMPTY;
    hasStart = hasEnd = false;
    startPos = Position();
    endPos = Position();
    resetGridKeepWalls();
}

bool inBounds(int r, int c) {
    return r >= 0 && r < ROWS && c >= 0 && c < COLS;
}

void runDijkstra() {
    visitedOrder.clear();
    pathCells.clear();

    bool closed[ROWS][COLS];
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++) {
            closed[r][c] = false;
            parent[r][c] = Position();
            dist[r][c] = INT_MAX;
        }

    OpenList open;
    dist[startPos.row][startPos.col] = 0;
    open.insert(0, startPos.row, startPos.col);

    int dr[] = { -1, 1, 0, 0 };
    int dc[] = { 0, 0, -1, 1 };

    bool found = false;

    while (!open.isEmpty()) {
        OpenEntry node = open.popMin();
        int r = node.row, c = node.col;

        if (closed[r][c]) continue;
        closed[r][c] = true;

        Position cur(r, c);
        visitedOrder.add(cur);

        if (r == endPos.row && c == endPos.col) {
            found = true;
            break;
        }

        for (int d = 0; d < 4; d++) {
            int nr = r + dr[d];
            int nc = c + dc[d];

            if (!inBounds(nr, nc)) continue;
            if (grid[nr][nc] == WALL) continue;
            if (closed[nr][nc]) continue;

            int stepCost = (grid[nr][nc] == WEIGHTED) ? WEIGHT_COST : NORMAL_COST;
            int tentative = dist[r][c] + stepCost;

            if (tentative < dist[nr][nc]) {
                dist[nr][nc] = tentative;
                parent[nr][nc] = cur;
                open.insert(tentative, nr, nc);
            }
        }
    }

    if (found) {
        Position cur = endPos;
        while (!(cur.row == startPos.row && cur.col == startPos.col)) {
            pathCells.add(cur);
            cur = parent[cur.row][cur.col];
        }
        pathCells.add(startPos);
        pathCells.reverse();
    }

    appState = ANIMATING_VISITED;
    revealIndex = 0;
    revealTimer = 0.0f;
}

Color colorFor(CellState s) {
    switch (s) {
        case EMPTY:    return EMPTY_COLOR;
        case WALL:     return WALL_COLOR;
        case START:    return START_COLOR;
        case END:      return END_COLOR;
        case VISITED:  return VISITED_COLOR;
        case PATH:     return PATH_COLOR;
        case WEIGHTED: return WEIGHTED_COLOR;
    }
    return EMPTY_COLOR;
}

void drawGrid() {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            int x = c * CELL_SIZE;
            int y = r * CELL_SIZE + TOP_BAR;
            DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, colorFor(grid[r][c]));
            DrawRectangleLines(x, y, CELL_SIZE, CELL_SIZE, GRID_LINE_COLOR);
        }
    }
}

void drawLegend() {
    int y = SCREEN_H - BOTTOM_BAR + 12;
    int x = 12;
    int swatch = 16;

    struct LegendItem { Color color; const char* label; };
    LegendItem items[] = {
        { WALL_COLOR,     "Wall" },
        { START_COLOR,    "Start" },
        { END_COLOR,      "End" },
        { WEIGHTED_COLOR, "Weighted (5)" },
        { VISITED_COLOR,  "Visited" },
        { PATH_COLOR,     "Path" }
    };

    for (int i = 0; i < 6; i++) {
        DrawRectangle(x, y, swatch, swatch, items[i].color);
        DrawRectangleLines(x, y, swatch, swatch, GRID_LINE_COLOR);
        DrawText(items[i].label, x + swatch + 6, y + 1, 14, TEXT_COLOR);
        x += swatch + 6 + MeasureText(items[i].label, 14) + 22;
    }
}

int main() {
    InitWindow(SCREEN_W, SCREEN_H, "Dijkstra's Algorithm ");
    SetTargetFPS(60);

    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            grid[r][c] = EMPTY;

    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();
        int col = (int)(mouse.x / CELL_SIZE);
        int row = (int)((mouse.y - TOP_BAR) / CELL_SIZE);
        bool overGrid = inBounds(row, col) && mouse.y > TOP_BAR;

        if (appState == IDLE && overGrid) {
            if (IsKeyDown(KEY_S) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (hasStart) grid[startPos.row][startPos.col] = EMPTY;
                grid[row][col] = START;
                startPos = Position(row, col);
                hasStart = true;
            }
            else if (IsKeyDown(KEY_E) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (hasEnd) grid[endPos.row][endPos.col] = EMPTY;
                grid[row][col] = END;
                endPos = Position(row, col);
                hasEnd = true;
            }
            else if (IsKeyDown(KEY_W) && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                if (grid[row][col] == EMPTY) grid[row][col] = WEIGHTED;
            }
            else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                if (grid[row][col] == EMPTY) grid[row][col] = WALL;
            }
            else if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                if (grid[row][col] == WALL || grid[row][col] == WEIGHTED) grid[row][col] = EMPTY;
            }
        }

        if (IsKeyPressed(KEY_SPACE) && hasStart && hasEnd && appState == IDLE) {
            runDijkstra();
        }
        if (IsKeyPressed(KEY_R)) {
            resetGridKeepWalls();
        }
        if (IsKeyPressed(KEY_C)) {
            clearEverything();
        }

        float dt = GetFrameTime();

        if (appState == ANIMATING_VISITED) {
            revealTimer += dt;
            while (revealTimer >= VISIT_DELAY && revealIndex < visitedOrder.size()) {
                Position p = visitedOrder.get(revealIndex);
                if (grid[p.row][p.col] == EMPTY || grid[p.row][p.col] == WEIGHTED)
                    grid[p.row][p.col] = VISITED;
                revealIndex++;
                revealTimer -= VISIT_DELAY;
            }
            if (revealIndex >= visitedOrder.size()) {
                if (pathCells.size() > 0) {
                    appState = ANIMATING_PATH;
                    revealIndex = 0;
                    revealTimer = 0.0f;
                } else {
                    appState = NO_PATH;
                }
            }
        }
        else if (appState == ANIMATING_PATH) {
            revealTimer += dt;
            while (revealTimer >= PATH_DELAY && revealIndex < pathCells.size()) {
                Position p = pathCells.get(revealIndex);
                if (grid[p.row][p.col] != START && grid[p.row][p.col] != END)
                    grid[p.row][p.col] = PATH;
                revealIndex++;
                revealTimer -= PATH_DELAY;
            }
            if (revealIndex >= pathCells.size()) {
                appState = DONE;
            }
        }

        BeginDrawing();
        ClearBackground(BG_COLOR);

        DrawText("DIJKSTRA'S ALGORITHM", 10, 6, 24, HEADER_COLOR);
        // DrawText("LMB: wall | RMB: erase | S+LMB: start | E+LMB: end", 10, 36, 15, TEXT_COLOR);
        // DrawText("W+LMB: weighted terrain (costs 5 to cross, shown purple)", 10, 56, 15, TEXT_COLOR);
        // DrawText("SPACE: run | R: reset run | C: clear all", 10, 76, 15, TEXT_COLOR);

        if (appState == NO_PATH)
            DrawText("NO PATH FOUND", SCREEN_W - 170, 10, 20, END_COLOR);
        else if (appState == DONE)
            DrawText("PATH FOUND!", SCREEN_W - 150, 10, 20, START_COLOR);

        drawGrid();
        drawLegend();

        EndDrawing();
    }

    CloseWindow();
    return 0;
}