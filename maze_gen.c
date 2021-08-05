// Jack Carter - R00193042

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WALL 'w'
#define POTION '#'
#define NEEDED_POTIONS 3
struct maze{
    char **a; // 2D array supporting maze
    unsigned int w; // width
    unsigned int h; // height
    unsigned int cell_size; // number of chars per cell; walls are 1 char
};

/**
 * Represents a cell in the 2D matrix.
 */
struct cell{
    unsigned int x;
    unsigned int y;
};

/**
 * Stack structure using a list of cells.
 * At element 0 in the list we have NULL.
 * Elements start from 1 onwards.
 * top_of_stack represents the index of the top of the stack
 * in the cell_list.
 */
struct stack{
    struct cell *cell_list;
    unsigned int top_of_stack;
    unsigned int capacity;
};

/**
 * Initialises the stack by allocating memory for the internal list
 */
void init_stack(struct stack *stack, unsigned int capacity){
    stack->cell_list = (struct cell*)malloc(sizeof(struct cell)*(capacity+1));
    stack->top_of_stack = 0;
    stack->capacity = capacity;
}

void free_stack(struct stack *stack){
    free(stack->cell_list);
}

/**
 * Returns the element at the top of the stack and removes it
 * from the stack.
 * If the stack is empty, returns NULL
 */
struct cell stack_pop(struct stack *stack){
    struct cell cell = stack->cell_list[stack->top_of_stack];
    if (stack->top_of_stack > 0) stack->top_of_stack --;
    return cell;
}

/**
 * Pushes an element to the top of the stack.
 * If the stack is already full (reached capacity), returns -1.
 * Otherwise returns 0.
 */
int stack_push(struct stack *stack, struct cell cell){
    if (stack->top_of_stack == stack->capacity) return -1;
    stack->top_of_stack ++;
    stack->cell_list[stack->top_of_stack] = cell;
    return 0;
}

int stack_isempty(struct stack *stack){
    return stack->top_of_stack == 0;
}

//-----------------------------------------------------------------------------

void mark_visited(struct maze *maze, struct cell cell){
    maze->a[cell.y][cell.x] = 'v';
}

/**
 * Convert a cell coordinate to a matrix index.
 * The matrix also contains the wall elements and a cell might span
 * multiple matrix cells.
 */
int cell_to_matrix_idx(struct maze *m, int cell){
    return (m->cell_size+1)*cell+(m->cell_size/2)+1;
}

/**
 * Convert maze dimension to matrix dimension.
 */
int maze_dimension_to_matrix(struct maze *m, int dimension){
    return (m->cell_size+1)*dimension+1;
}

/**
 * Returns the index of the previous cell (cell - 1)
 */
int matrix_idx_prev_cell(struct maze *m, int cell_num){
    return cell_num - (m->cell_size+1);
}

/**
 * Returns the index of the next cell (cell + 1)
 */
int matrix_idx_next_cell(struct maze *m, int cell_num){
    return cell_num + (m->cell_size+1);
}

/**
 * Returns into neighbours the unvisited neighbour cells of the given cell.
 * Returns the number of neighbours.
 * neighbours must be able to hold 4 cells.
 */
int get_available_neighbours(struct maze *maze, struct cell cell, struct cell *neighbours){
    int num_neighbrs = 0;

    // Check above
    if ((cell.y > cell_to_matrix_idx(maze,0)) && (maze->a[matrix_idx_prev_cell(maze, cell.y)][cell.x] != 'v')){
        neighbours[num_neighbrs].x = cell.x;
        neighbours[num_neighbrs].y = matrix_idx_prev_cell(maze, cell.y);
        num_neighbrs ++;
    }

    // Check left
    if ((cell.x > cell_to_matrix_idx(maze,0)) && (maze->a[cell.y][matrix_idx_prev_cell(maze, cell.x)] != 'v')){
        neighbours[num_neighbrs].x = matrix_idx_prev_cell(maze, cell.x);
        neighbours[num_neighbrs].y = cell.y;
        num_neighbrs ++;
    }

    // Check right
    if ((cell.x < cell_to_matrix_idx(maze,maze->w-1)) && (maze->a[cell.y][matrix_idx_next_cell(maze, cell.x)] != 'v')){
        neighbours[num_neighbrs].x = matrix_idx_next_cell(maze, cell.x);
        neighbours[num_neighbrs].y = cell.y;
        num_neighbrs ++;
    }

    // Check below
    if ((cell.y < cell_to_matrix_idx(maze,maze->h-1)) && (maze->a[matrix_idx_next_cell(maze, cell.y)][cell.x] != 'v')){
        neighbours[num_neighbrs].x = cell.x;
        neighbours[num_neighbrs].y = matrix_idx_next_cell(maze, cell.y);
        num_neighbrs ++;
    }

    return num_neighbrs;
}


/**
 * Removes a wall between two cells.
 */
void remove_wall(struct maze *maze, struct cell a, struct cell b){
    int i;
    if (a.y == b.y){
        for (i=0;i<maze->cell_size;i++)
            maze->a[a.y-maze->cell_size/2+i][a.x-(((int)a.x-(int)b.x))/2] = ' ';
    }else{
        for (i=0;i<maze->cell_size;i++)
            maze->a[a.y-(((int)a.y-(int)b.y))/2][a.x-maze->cell_size/2+i] = ' ';
    }
}

/**
 * Fill all matrix elements corresponding to the cell
 */
void fill_cell(struct maze *maze, struct cell c, char value){
    int i,j;
    for (i=0;i<maze->cell_size;i++)
        for (j=0;j<maze->cell_size;j++)
            maze->a[c.y-maze->cell_size/2+i][c.x-maze->cell_size/2+j] = value;
}

/**
 * This function generates a maze of width x height cells.
 * Each cell is a square of cell_size x cell_size characters.
 * The maze is randomly generated based on the supplied rand_seed.
 * Use the same rand_seed value to obtain the same maze.
 *
 * The function returns a struct maze variable containing:
 * - the maze represented as a 2D array (field a)
 * - the width (number of columns) of the array (field w)
 * - the height (number of rows) of the array (field h).
 * In the array, walls are represented with a 'w' character, while
 * pathways are represented with spaces (' ').
 * The edges of the array consist of walls, with the exception
 * of two openings, one on the left side (column 0) and one on
 * the right (column w-1) of the array. These should be used
 * as entry and exit.
 */
struct maze generate_maze(unsigned int width, unsigned int height, unsigned int cell_size, int rand_seed){
    int row, col, i;
    struct stack stack;
    struct cell cell;
    struct cell neighbours[4];  // to hold neighbours of a cell
    int num_neighbs;
    struct maze maze;
    maze.w = width;
    maze.h = height;
    maze.cell_size = cell_size;
    maze.a = (char**)malloc(sizeof(char*)*maze_dimension_to_matrix(&maze, height));

    // Initialise RNG
    srand(rand_seed);

    // Initialise stack
    init_stack(&stack, width*height);

    // Initialise the matrix with walls
    for (row=0;row<maze_dimension_to_matrix(&maze, height);row++){
        maze.a[row] = (char*)malloc(maze_dimension_to_matrix(&maze, width));
        memset(maze.a[row], WALL, maze_dimension_to_matrix(&maze, width));
    }

    // Select a random position on a border.
    // Border means x=0 or y=0 or x=2*width+1 or y=2*height+1
    cell.x = cell_to_matrix_idx(&maze,0);
    cell.y = cell_to_matrix_idx(&maze,rand()%height);
    mark_visited(&maze, cell);
    stack_push(&stack, cell);

    while (! stack_isempty(&stack)){
        // Take the top of stack
        cell = stack_pop(&stack);
        // Get the list of non-visited neighbours
        num_neighbs = get_available_neighbours(&maze, cell, neighbours);
        if (num_neighbs > 0){
            struct cell next;
            // Push current cell on the stack
            stack_push(&stack, cell);
            // Select one random neighbour
            next = neighbours[rand()%num_neighbs];
            // Mark it visited
            mark_visited(&maze, next);
            // Break down the wall between the cells
            remove_wall(&maze, cell, next);
            // Push new cell on the stack
            stack_push(&stack, next);
        }
    }

    // Finally, replace 'v' with spaces
    for (row=0;row<maze_dimension_to_matrix(&maze, height);row++)
        for (col=0;col<maze_dimension_to_matrix(&maze, width);col++)
            if (maze.a[row][col] == 'v'){
                cell.y = row;
                cell.x = col;
                fill_cell(&maze, cell, ' ');
            }

    // Select an entry point in the top right corner.
    // The first border cell that is available.
    for (row=0;row<maze_dimension_to_matrix(&maze, height);row++)
        if (maze.a[row][1] == ' ') { maze.a[row][0] = ' '; break; }

    // Select the exit point
    for (row=maze_dimension_to_matrix(&maze, height)-1;row>=0;row--)
        if (maze.a[row][cell_to_matrix_idx(&maze,width-1)] == ' ') {
            maze.a[row][maze_dimension_to_matrix(&maze, width)-1] = ' ';
            break;
        }

    maze.w = maze_dimension_to_matrix(&maze, maze.w);
    maze.h = maze_dimension_to_matrix(&maze, maze.h);

    // Add the potions inside the maze at three random locations
    for (i=0;i<NEEDED_POTIONS;i++){
        do{
            row = rand()%(maze.h-1);
            col = rand()%(maze.w-1);
        }while (maze.a[row][col] != ' ');
        maze.a[row][col] = POTION;
    }

    return maze;
}



// int charPos[2] = {playX, playY};


// Print the maze
void printMaze(struct maze maze01, int playX, int playY){
    for(int i = 0; i < maze01.h; i++){
            for(int j = 0; j < maze01.w; j++){

                if (playX != j){
                    printf("%c", maze01.a[i][j]);
                }
                else if (playY != i){
                    printf("%c", maze01.a[i][j]);
                }
                else{
                    printf("@");
                }
            }
        printf("\n");
        }
}


void printFogMaze(struct maze maze01, int playX, int playY, int radius){
    int boxUpperX = playX - radius; 
    int boxUpperY = playY - radius; 
    int boxLowerX = playX + radius; 
    int boxLowerY = playY + radius; 
    for(int i = 0; i < maze01.h; i++){
        for(int j = 0; j < maze01.w; j++){
            if(i >= boxUpperY && i <= boxLowerY){
                if(j >= boxUpperX && j <= boxLowerX){
                    if (playX != j){
                        printf("%c", maze01.a[i][j]);
                    }
                    else if (playY != i){
                        printf("%c", maze01.a[i][j]);
                    }
                    else{
                        printf("@");
                    }
                }
            }
            else{
                printf(" ");
            }
        }
        printf("\n");
    }
}

// Finding the last Column
int findColumn(struct maze maze01){
    int lastCol = 0;
    for(int i = 0; i < maze01.w; i++){
        lastCol += 1;
    }
    return lastCol;
}

// Finding the last Row
int findRow(struct maze maze01){
    int lastRow = 0;
    for(int i = 0; i < maze01.h; i++){
        lastRow += 1;
    }

    return lastRow;
}


// Main
void main(){
    // Variable Declaration
    int height, width, cell_size, seed, radius, won;
    char move;
    int playX = 0;
    int playY = 1;
    int playX_change= 0;
    int playY_change = 0;
    int potionCount = 0;
    int totalPotions = 3;

    // Maze Parameters
    printf("Height: ");
    scanf("%d", &height);
    getchar();
    printf("Width: ");
    scanf("%d", &width);
    getchar();
    printf("Cell Size (odd number): ");
    scanf("%d", &cell_size);
    getchar();
    printf("Seed: ");
    scanf("%d", &seed);
    getchar();
    printf("Radius (0 for no fog): ");
    scanf("%d", &radius);
    getchar();

    // Generate Maze
    struct maze maze01 = generate_maze(width, height, cell_size, seed);

    printf("%d\n", height);
    printf("%d\n", maze01.w);
    printf("%d\n", maze01.a);

    int lastCol = findColumn(maze01);
    int lastRow = findRow(maze01);


    // Game Loop Begins
    while(won != 1){
        
        // Resetting the player's requested move
        playY_change = 0;
        playX_change = 0;

        // Clearing Space
        printf("\n\n\n\n\n\n\n\n\n\n");

        // Displaying Information
        printf("Player's X: %d\n", playX);
        printf("Player's Y: %d\n", playY);
        printf("Potion Count: %d/%d\n", potionCount, totalPotions);

        // Printing Maze
        if(radius == 0){
            // Without fog
            printMaze(maze01, playX, playY);
        } 
        else{
            // With fog
            printFogMaze(maze01, playX, playY, radius);
        }
        

        // Getting user move
        move = getchar();
        getchar();

        // Player Movement
        switch(move){

            // Move Right
            case 'd':
                playX_change = playX_change + 1;

                // Checking for wall
                if(maze01.a[playY][playX + 1] == 'w'){
                    printf("Invalid Move\n");
                    playX_change = 0;
                }
                // Checking for potion
                else if(maze01.a[playY][playX + 1] == '#'){
                    potionCount = potionCount + 1;
                    maze01.a[playY][playX + 1] = ' ';
                }
                break;

                // Move Left
            case 'a':
                playX_change = playX_change - 1;

                // Checking for wall
                if(maze01.a[playY][ playX - 1] == 'w'){
                    printf("Invalid Move\n");
                    playX_change = 0;
                }
                // Checking for potion
                else if(maze01.a[playY][ playX - 1] == '#'){
                    potionCount = potionCount + 1;
                    maze01.a[playY][playX - 1] = ' ';
                }
                break;

                // Move Up
            case 'w':
                playY_change = playY_change - 1;
                
                // Checking for wall
                if(maze01.a[playY - 1][playX] == 'w'){
                    printf("Invalid Move\n");
                    playY_change = 0;
                }
                // Checking for potion
                else if(maze01.a[playY - 1][playX] == '#'){
                    potionCount = potionCount + 1;
                    maze01.a[playY - 1][playX] = ' ';
                }
                break;

                // Move Down
            case 's':
                playY_change = playY_change + 1;

                // Checking for wall
                if(maze01.a[playY + 1][playX] == 'w'){
                    printf("Invalid Move\n");
                    playY_change = 0;
                }
                // Checking for potion
                else if(maze01.a[playY + 1][playX] == '#'){
                    potionCount = potionCount + 1;
                    maze01.a[playY + 1][playX] = ' ';
                }
        }


        // printf("Player's X: %d\n", playX + playX_change);
        // printf("Player's Y: %d\n", playY + playY_change);

        // Checking if the player wants to exit the maze
        if(playX + playX_change == lastCol - 1 && playY + playY_change == lastRow - 2){
            // If the player has collected all the potions
            if(potionCount == totalPotions){
                won = 1;
            }
            // If the player hasn't collected all the potions
            else{
                printf("\nHah! You think you can just walk past me without paying? Not likely.\n");
                printf("Tell you what.. bring me 3 of those potions and I'll think about letting you pass\n");
                printf("Press Enter...\n");
                getchar();
            }
        }
        // Increasing the players X & Y coordinates
        else{
            playX = playX + playX_change;
            playY = playY + playY_change;
        }
    }

    // Winning Message
    printf("Congratulations! You've won!");

}
