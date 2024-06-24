#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <time.h>

#define WIDTH 10
#define HEIGHT 24
#define PRINTABLE_HEIGHT 20

#define BLOCK_COUNT 4
#define DIMENSIONS 2
#define POINT_COUNT 5

#define QUEUE_SIZE 5 

typedef enum mino_type {
    oblock,
    iblock,
    tblock,
    lblock,
    jblock,
    sblock,
    zblock,
    mino_size
} mino_type;


const char* tetrimino_name[] = { "O-Block", "I-Block", "T-Block", "L-Block", "J-Block", "S-Block", "Z-Block", "You Fucked Up" };

typedef enum face {
    North,
    East,
    South,
    West,
    face_size
} face;

const char *face_name[] = {"North", "East", "South", "West", "Fucked Up"};

typedef enum direction {
    Left,
    Right,
    direction_size,
    Down,
	MoveLeft,
	MoveRight
} direction;

// offsets[0] == rotations_points[0]
typedef struct Tetrimino {
    int8_t offsets[4][2];
    face facing;
    mino_type type;
} Tetrimino;

typedef struct GameState {
    int8_t board[HEIGHT][WIDTH];
    uint64_t timer;
    uint8_t bag;
    Tetrimino active_piece;
	mino_type hold_piece;
    mino_type queue[QUEUE_SIZE];
	int head;
} GameState;


// Rotation Point Map
//
// The offsets for the super rotation system
// rot_map[tetrimino][face][point][rotation]
// wall_rot_map[tetrimino][face][rotation][wall]
// wall_rot_map[tetrimino][face][rotation][wall]

/*
0 1 2 3
4 P 6 7
8 9 A B
C D E F

0 1 2 3
4 P 6 7
8 9 A B
C D E F

offsets[4]
*/

static int8_t rot_map[mino_size][face_size][direction_size][BLOCK_COUNT][DIMENSIONS];

static int8_t tetrimino_map[mino_size][face_size][BLOCK_COUNT][DIMENSIONS] = {
	// square
	{
		// north
		{{0, 0}, {0, 1}, {1, 1}, {1, 0}},
		// east
		{{0, 0}, {0, 1}, {1, 1}, {1, 0}},
		// south
		{{0, 0}, {0, 1}, {1, 1}, {1, 0}},
		// west
		{{0, 0}, {0, 1}, {1, 1}, {1, 0}},
	},
	// line
	{
		// north
		{{1, 0}, {1, 1}, {1, 2}, {1, 3}},
		// east
		{{0, 2}, {1, 2}, {2, 2}, {3, 2}},
		// south
		{{2, 3}, {2, 2}, {2, 1}, {2, 0}},
		// west
		{{3, 1}, {2, 1}, {1, 1}, {0, 1}},
	},
	// t piece
	{
		// north
		{{1, 1}, {1, 0}, {0, 1}, {1, 2}},
		// east
		{{1, 1}, {0, 1}, {2, 1}, {1, 2}},
		// south
		{{1, 1}, {2, 1}, {1, 0}, {1, 2}},
		// west
		{{1, 1}, {0, 1}, {1, 0}, {2, 1}},
	},
	// L block
	{
		// north
		{{1, 1}, {1, 0}, {0, 2}, {1, 2}},
		// east
		{{1, 1}, {0, 1}, {2, 1}, {2, 2}},
		// south
		{{1, 1}, {2, 0}, {1, 0}, {1, 2}},
		// west
		{{1, 1}, {0, 0}, {0, 1}, {2, 1}},
	},
	// Reverse L Block
	{
		// north
		{{1, 1}, {1, 0}, {0, 0}, {1, 2}},
		// east
		{{1, 1}, {0, 1}, {0, 2}, {2, 1}},
		// south
		{{1, 1}, {1, 0}, {1, 2}, {2, 2}},
		// west
		{{1, 1}, {0, 1}, {2, 0}, {2, 1}},
	},
	// Squiggly Block
	{
		// north
		{{1, 1}, {1, 0}, {0, 1}, {0, 2}},
		// east
		{{1, 1}, {0, 1}, {1, 2}, {2, 2}},
		// south
		{{1, 1}, {2, 0}, {2, 1}, {1, 2}},
		// west
		{{1, 1}, {0, 0}, {1, 0}, {2, 1}},
	},
	// Reverse Squiggly Block
	{
		// north
		{{1, 1}, {0, 0}, {0, 1}, {1, 2}},
		// east
		{{1, 1}, {0, 2}, {1, 2}, {2, 1}},
		// south
		{{1, 1}, {1, 0}, {2, 1}, {2, 2}},
		// west
		{{1, 1}, {0, 1}, {1, 0}, {2, 0}},
	},
};

static int8_t offset_map[mino_size][face_size][direction_size][POINT_COUNT][DIMENSIONS] = {
	// Square
	{
		// North 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
		},
		// East 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
		},
		// South 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
		},
		// West 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
		}
	},
	// I-Block
	{
		// North 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, -1}, {0, 2}, {-2, -1}, {1, 2}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, -2}, {0, 1}, {1, -2}, {-2, 1}},
		},
		// East 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, 2}, {0, -1}, {-1, 2}, {2, -1}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, -1}, {0, 2}, {-2, -1}, {1, 2}},
		},
		// South 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, 1}, {0, -2}, {2, 1}, {-1, -2}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, 2}, {0, -1}, {-1, 2}, {2, -1}},
		},
		// West 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, -2}, {0, 1}, {1, -2}, {-2, 1}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, 1}, {0, -2}, {2, 1}, {-1, -2}},
		}
	},
	// T-Block
	{
		// North 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, 1}, {-1, 1}, {-1, 1}, {2, 1}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, -1}, {-1, -1}, {-1, -1}, {2, -1}},
		},
		// East 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, 1}, {1, 1}, {-2, 0}, {-2, 1}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, 1}, {1, 1}, {-2, 0}, {-2, 1}},
		},
		// South 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, -1}, {0, -1}, {2, 0}, {2, -1}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, 1}, {0, 1}, {2, 0}, {2, 1}},
		},
		// West 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, -1}, {1, -1}, {-2, 0}, {-2, -1}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, -1}, {1, -1}, {-2, 0}, {-2, -1}},
		}
	},
	// L-Block
	{
		// North 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, 1}, {-1, 1}, {2, 0}, {2, 1}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, -1}, {-1, -1}, {2, 0}, {2, -1}},
		},
		// East 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, 1}, {1, 1}, {-2, 0}, {-2, 1}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, 1}, {1, 1}, {-2, 0}, {-2, 1}},
		},
		// South 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, -1}, {-1, -1}, {2, 0}, {2, -1}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, 1}, {-1, 1}, {2, 0}, {2, 1}},
		},
		// West 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, -1}, {1, -1}, {-2, 0}, {-2, -1}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, -1}, {1, -1}, {-2, 0}, {-2, -1}},
		}
	},
	// J-Block
	{
		// North 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, 1}, {-1, 1}, {2, 0}, {2, 1}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, -1}, {-1, -1}, {2, 0}, {2, -1}},
		},
		// East 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, 1}, {1, 1}, {-2, 0}, {-2, 1}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, 1}, {1, 1}, {-2, 0}, {-2, 1}},
		},
		// South 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, -1}, {-1, -1}, {2, 0}, {2, -1}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, 1}, {-1, 1}, {2, 0}, {2, 1}},
		},
		// West 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, -1}, {1, -1}, {-2, 0}, {-2, -1}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, -1}, {1, -2}, {-2, 0}, {-2, -1}},
		}
	},
		// Square
	{
		// North 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, 1}, {-1, 1}, {-2, 0}, {-2, 1}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, -1}, {-1, -1}, {-2, 0}, {-2, -1}},
		},
		// East 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, 1}, {1, 1}, {-2, 0}, {-2, 1}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, 1}, {1, 1}, {-2, 0}, {-2, 1}},
		},
		// South 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, -1}, {-1, -1}, {2, 0}, {2, -1}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, 1}, {-1, 1}, {2, 0}, {2, 1}},
		},
		// West 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, -1}, {1, -1}, {-2, 0}, {-2, -1}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, -1}, {1, -1}, {-2, 0}, {-2, -1}},
		}
	},
	// Reverse Squiggly
	{
		// North 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, 1}, {-1, 1}, {2, 0}, {2, 1}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, -1}, {-1, -1}, {2, 0}, {2, -1}},
		},
		// East 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, 1}, {1, 1}, {-2, 0}, {-2, 1}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, 1}, {1, 1}, {-2, 0}, {-2, 1}},
		},
		// South 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, -1}, {-1, -1}, {2, 0}, {2, -1}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, 1}, {-1, 1}, {2, 0}, {2, 1}},
		},
		// West 
		{
			// Left: 1, 2, 3, 4, 5
			{{0, 0}, {0, -1}, {1, -1}, {-2, 0}, {-2, -1}},
			// Right: 1, 2, 3, 4, 5
			{{0, 0}, {0, -1}, {1, -1}, {-2, 0}, {-2, -1}},
		}
	},
};


// This is a 7bag random number generator
// imagine a bag with 7 pieces in it. You draw a piece (0-6).
// that number won't be returned again.
// Keep going until you run out of pieces
// Refill the bag with 7 new pieces
// I like complicated (but it's basically a set but using a bit array) 
mino_type next_piece(GameState *gs) {
    // If the bag is empty, refill it
    if (gs->bag == 0) {
		gs->bag = 0x7F;
    }

    // Count bits in bag
    int n = gs->bag;
    int count = 0;
    while (n) {
		n &= (n - 1);
		count++;
    }
    
    // Choose the n'th 1 bit from the bag and remove it  
    int shift = rand() % count; 
    for (int i = 0; i < 7; i++) {
		bool isone = (gs->bag & (1 << i)) > 0;  
		if (shift > 0 && isone) {
			shift--;
		}
		if (isone && shift == 0) {
			gs->bag &= ~(1 << i);
			return i;
		}
    }

    return rand() % 7;
}

mino_type dequeue_piece(GameState *gs) {
	mino_type r = gs->queue[gs->head];
	printf("Good\n");
	gs->queue[gs->head] = next_piece(gs);

	gs->head = (gs->head + 1) % QUEUE_SIZE;

	return r;
}

// Attempts to spawn a piece on the board
bool force_piece(GameState *gs, mino_type type) {
	// Generate the Tetrimino variables
	face facing = North;

	gs->active_piece.type = type;
	gs->active_piece.facing = facing;
	for (int i = 0; i < BLOCK_COUNT; i++) {
		gs->active_piece.offsets[i][0] = 4 + tetrimino_map[type][facing][i][0];
		gs->active_piece.offsets[i][1] = 3 + tetrimino_map[type][facing][i][1];
	}
	bool raise = false;
	do {
		raise = false;
		for (int i = 0; i < BLOCK_COUNT; i++) {
			if (gs->board[gs->active_piece.offsets[i][0]][gs->active_piece.offsets[i][1]] != mino_size) {
				raise = true;
			}
		}

		if (raise) {
			for (int i = 0; i < BLOCK_COUNT; i++) {
				gs->active_piece.offsets[i][0]--;
			}
		}
	} while(raise);

	return true;
}

bool spawn_piece(GameState *gs) {
	mino_type type = dequeue_piece(gs);
	printf("Hi\n");
	return force_piece(gs, type);
}

// Attempts to move a piece in a direction, returns false if unable to do so
bool move_piece(GameState *gs, direction d) {
    Tetrimino *active = &(gs->active_piece);
	
	if (d >= direction_size) {
		// Move the temp tetrimino down
		int arr[4][2] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
		for (int j = 0; j < 4; j++) {
			arr[j][0] = active->offsets[j][0] + (1 & (d == Down));
			arr[j][1] = active->offsets[j][1] + (1 & (d == MoveRight)) + (-1 * (d == MoveLeft));
		}

		for (int j = 0; j < 4; j++) {
			// If it doesn't fit
			if ((arr[j][0] < 0 || arr[j][0] >= HEIGHT) || 
				(arr[j][1] < 0 || arr[j][1] >= WIDTH) || 
				(gs->board[arr[j][0]][arr[j][1]] != mino_size	)) {
				return false;
			}
		}

		for (int j = 0; j < 4; j++) {
			active->offsets[j][0] = arr[j][0];
			active->offsets[j][1] = arr[j][1];
		}
		return true;
	}
	else {
		// Then check each point to see if it's valid
		for (int i = 0; i < POINT_COUNT; i++) {
			// Get a temporary vector which is the rotated, offseted tetrimino 

			int arr[4][2] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};

			for (int j = 0; j < 4; j++) {
				arr[j][0] = active->offsets[j][0] + rot_map[active->type][active->facing][d][j][0] + offset_map[active->type][active->facing][d][i][0];
				arr[j][1] = active->offsets[j][1] + rot_map[active->type][active->facing][d][j][1] + offset_map[active->type][active->facing][d][i][1];
			}

			bool is_valid = true;
			for (int j = 0; j < 4; j++) {
				// If it doesn't fit
				if ((arr[j][0] < 0 || arr[j][0] >= HEIGHT) || 
					(arr[j][1] < 0 || arr[j][1] >= WIDTH) || 
					(gs->board[arr[j][0]][arr[j][1]] != mino_size)) {
					is_valid = false;
					break;
				}
			}

			// If it's valid, then set the tetrimino and break out of the loop and return true
			if (is_valid) {
				for (int j = 0; j < 4; j++) {
					active->offsets[j][0] = arr[j][0];
					active->offsets[j][1] = arr[j][1];
				}
				
				active->facing += ((-1 * (d == Left)) + (1 * (d == Right)));
				if (active->facing == -1) {
					active->facing = 3;
				}
				else if (active->facing == 4) {
					active->facing = 0;
				}
				return true;
			}
		}
		

	}

	return false;
}

void generate_rotations() {
	for (mino_type t = oblock; t < mino_size; t++) {
		for (face f = North; f < face_size; f++) {
			for (int i = 0; i < BLOCK_COUNT; i++) {
				// left y
				rot_map[t][f][Left][i][0] = - tetrimino_map[t][f][i][0] + tetrimino_map[t][(f - 1)%4][i][0];
				// left x
				rot_map[t][f][Left][i][1] = - tetrimino_map[t][f][i][1] + tetrimino_map[t][(f - 1)%4][i][1];

				// right y
				rot_map[t][f][Right][i][0] = tetrimino_map[t][(f + 1)%4][i][0] - tetrimino_map[t][f][i][0] ;

				// right x
				rot_map[t][f][Right][i][1] = tetrimino_map[t][(f + 1)%4][i][1] - tetrimino_map[t][f][i][1] ;
			}
		}
	}
}

void print_tetrimino_map() {
	int map[4][4];

	for (int i = 0; i < 16; i++) {
		map[i / 4][i % 4] = 0;
	}

	for (mino_type t = oblock; t < mino_size; t++) {
		for (face f = North; f < face_size; f++) {
			for (int i = 0; i < BLOCK_COUNT; i++) {
				map[tetrimino_map[t][f][i][0]][tetrimino_map[t][f][i][1]] = i + 1;
			}
			printf("%s %s\n", tetrimino_name[t], face_name[f]);
			for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 4; j++) {
					printf("%d", map[i][j]);
				}
				printf("\n");
			}
			printf("\n");

			for (int i = 0; i < 16; i++) {
				map[i / 4][i % 4] = 0;
			}
		}
	}
}

void print_rot_map() {
	for (mino_type t = oblock; t < mino_size; t++) {
		for (face f = North; f < face_size; f++) {
			printf("%s %s Left\n", tetrimino_name[t], face_name[f]);
			for (int i = 0; i < 4; i++) {
				printf("(%d %d)", rot_map[t][f][Left][i][0], rot_map[t][f][Left][i][1]);
			}
			printf("\n");

			printf("%s %s Right\n", tetrimino_name[t], face_name[f]);
			for (int i = 0; i < 4; i++) {
				printf("(%d %d)", rot_map[t][f][Right][i][0], rot_map[t][f][Right][i][1]);
			}
			printf("\n");
		}
	}
}

void fill_queue(GameState *gs) {
	for (int i = 0; i < QUEUE_SIZE; i++) {
		gs->queue[i] = next_piece(gs);
	}
}

// Resets game to initial configuration
void init_game(GameState *gs) {
    for (int i = 0; i < HEIGHT; i++) {
		for (int j = 0; j < WIDTH; j++) {
			gs->board[i][j] = mino_size;
		}
    } 
	
	gs->hold_piece = mino_size;
    gs->bag = 0x7F;
	gs->head = 0;

	generate_rotations();

	fill_queue(gs);

}

void place_piece(GameState *gs) {
	for (int i = 0; i < BLOCK_COUNT; i++) {
		gs->board[gs->active_piece.offsets[i][0]][gs->active_piece.offsets[i][1]] = gs->active_piece.type;
	}

	spawn_piece(gs);
}

void print_board(GameState *gs) {
	for (int i = HEIGHT - PRINTABLE_HEIGHT; i < HEIGHT; i++) {
		for (int j = 0; j < WIDTH; j++) {
			bool active_piece = false;
			for (int k = 0; k < 4; k++) {
				if (gs->active_piece.offsets[k][0] == i && gs->active_piece.offsets[k][1] == j) {
					printf("%d ", k + 1);
					active_piece = true;
					break;
				}
			}
			if (!active_piece && gs->board[i][j] == mino_size) {
				printf("  ");
			}
			else if (!active_piece) {
				printf("%d ", gs->board[i][j]);
			}
		}
		printf("\n");
	}
}

void hard_drop(GameState *gs) {
	while (move_piece(gs, Down));
	place_piece(gs);
	spawn_piece(gs);
}

bool check_loss(GameState *gs) {
	for (int i = 0; i < HEIGHT - PRINTABLE_HEIGHT; i++) {
		for (int j = 0; j < WIDTH; j++) {
			if (gs->board[i][j] != mino_size) {
				return true;
			}
		}
	}

	return false;
}

int clear_line(GameState *gs) {
	for (int i = 0; i < HEIGHT; i++) {
		bool line = true;
		for (int j = 0; j < WIDTH; j++) {
			if (gs->board[i][j] == mino_size) {
				line = false;
			};
		}
		if (line) {
			for (int j = i; j > HEIGHT - PRINTABLE_HEIGHT; j--) {
				for (int k = 0; k < WIDTH; k++) {
					gs->board[j][k] = gs->board[j - 1][k];
				}
			}
		}

	}
}

int hold_piece(GameState *gs) {
	// If there was no held piece before
	if (gs->hold_piece == mino_size) {
		gs->hold_piece = gs->active_piece.type;

		spawn_piece(gs);
	}
	else {
		mino_type temp = gs->active_piece.type;

		force_piece(gs, gs->hold_piece);

		gs->hold_piece = temp;
	}
}

int main(void) {
    GameState game;
    GameState *gs = &game;

    srand(time(NULL));
    init_game(gs);
	printf("Hello\n");	
	spawn_piece(gs);
	printf("Hello\n");	

	char c;
	print_board(gs);
	while ((c = getchar()) != 'o') {
		bool move_succeeded = true;

		switch (c) {
			case 'a': move_succeeded = move_piece(gs, Left); break;
			case 's': move_succeeded = move_piece(gs, Down); break;
			case 'd': move_succeeded = move_piece(gs, Right); break;
			case 'q': move_succeeded = move_piece(gs, MoveLeft); break;
			case 'e': move_succeeded = move_piece(gs, MoveRight); break;
			case 'x': hard_drop(gs); break;
			case 'h': hold_piece(gs); break;
			case 'n': spawn_piece(gs); break;
			default: break;
		}

		if (!move_succeeded && c == 's') {
			place_piece(gs);
		}

		if (check_loss(gs)) {
			break;
		}
		clear_line(gs);
		print_board(gs);
		printf("----------\n");
	}
	printf("You lose!\n");
	return 0;
}
