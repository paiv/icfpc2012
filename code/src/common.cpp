#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>


using namespace std;


namespace paiv {

typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;
typedef uint32_t u32;
typedef uint8_t u8;


typedef enum cell : u8 {
    empty = ' ',
    earth = '.',
    wall = '#',
    rock = '*',
    lambda = '\\',
    lift = 'L',
    openlift = 'O',
    robot = 'R',
} cell;


typedef enum action : u8 {
    left = 'L',
    right = 'R',
    up = 'U',
    down = 'D',
    wait = 'W',
    abort = 'A',
} action;


typedef vector<vector<cell>> board_t;
typedef vector<action> program_t;

typedef s16 coord;
typedef u32 coosq;

typedef struct pos {
    coord x;
    coord y;
} pos;


typedef struct game_state {
    coord width;
    coord height;
    board_t board;
    pos lift_pos;
    u32 lambdas_total;

    pos robot_pos;
    s32 score;
    u32 lambdas_collected;

    u8 is_ended;
} game_state;


bool operator == (const pos& a, const pos& b) {
    return a.x == b.x && a.y == b.y;
}


ostream& operator << (ostream& so, const board_t& board) {
    for (auto& row : board) {
        for (char x : row) {
            so << x;
        }
        so << '\n';
    }
    return so;
}

ostream& operator << (ostream& so, const game_state& state) {
    return so << state.board;
}

ostream& operator << (ostream& so, const program_t& prog) {
    for (char x : prog) {
        so << x;
    }
    return so;
}


game_state static
read_map(istream& si) {
    coord max_width = 0;
    coord row = 0;
    board_t board = board_t({ {} });
    pos robot = {};
    pos lift = {};
    u32 lambdas = 0;
    char c = 0;

    for (u8 done = 0; !done && si.get(c); ) {
        switch (c) {
            case '\n':
                if (board[row].size() == 0) {
                    done = 1;
                }
                else {
                    max_width = max(max_width, (coord) board[row].size());
                    row++;
                    board.push_back({});
                    board[row].reserve(max_width);
                }
                break;

            case 'R': // robot
                robot = { .x = (coord) board[row].size(), .y = (coord) row };
                board[row].push_back(cell(c));
                break;

            case '\\': // lambda
                lambdas++;
                board[row].push_back(cell(c));
                break;

            case 'L': // lift
            case 'O': // openlift
                lift = { .x = (coord) board[row].size(), .y = (coord) row };
                board[row].push_back(cell(c));
                break;

            case ' ': // empty
            case '.': // earth
            case '#': // wall
            case '*': // rock
                board[row].push_back(cell(c));
                break;

            case '\r':
                break;

            default:
                done = 1;
                break;
        }
    }

    max_width = max(max_width, (coord) board[row].size());

    if (board[row].size() == 0) {
        board.pop_back();
    }
    else {
        row++;
    }

    for (auto& row : board) {
        row.resize(max_width, cell::empty);
    }

    return {
        max_width,  // .width
        row,        // .height
        board,      // .board
        lift,       // .lift_pos
        lambdas,    // .lambdas_total
        robot,      // .robot_pos
        0,          // .score
        0,          // .lambdas_collected
        0,          // .is_ended
    };
}


program_t static
read_program(istream& si) {
    program_t prog;
    char c;

    for (u8 done = 0; !done && si.get(c); ) {
        switch (c) {
            case '\n':
                done = 1;
                break;

            case 'L': // left
            case 'R': // right
            case 'U': // up
            case 'D': // down
            case 'W': // wait
            case 'A': // abort
                prog.push_back(action(c));
                break;
        }
    }

    return prog;
}


program_t static
read_program(const string& s) {
    stringstream so(s);
    return read_program(so);
}


pos static inline
advance_pos(pos value, action mv) {
    switch (mv) {
        case action::left:
            value.x--;
            break;

        case action::right:
            value.x++;
            break;

        case action::up:
            value.y--;
            break;

        case action::down:
            value.y++;
            break;

        default:
            break;
    }

    return value;
}


void static inline
move_robot(game_state& state, const pos& from, const pos& to) {
    state.board[from.y][from.x] = cell::empty;
    state.board[to.y][to.x] = cell::robot;
    state.robot_pos = to;
}


void static inline
move_rock(board_t& board, const pos& from, const pos& to) {
    board[from.y][from.x] = cell::empty;
    board[to.y][to.x] = cell::rock;
}


void static inline
move_rock(board_t& board, u32 from_x, u32 from_y, u32 to_x, u32 to_y, const pos& robot_pos, u8* robot_destroyed) {
    board[from_y][from_x] = cell::empty;
    board[to_y][to_x] = cell::rock;
    if (to_x == robot_pos.x && to_y + 1 == robot_pos.y) {
        *robot_destroyed = 1;
    }
}


void static inline
open_lift(game_state& state) {
    const auto& lift = state.lift_pos;
    auto& x = state.board[lift.y][lift.x];
    if (x == cell::lift) {
        x = cell::openlift;
    }
}


game_state static
run_step(const game_state& currentState, action mv) {
    auto state = currentState;
    auto current_pos = currentState.robot_pos;
    auto next_pos = advance_pos(current_pos, mv);

    switch (mv) {

        case action::left:
        case action::right:
        case action::up:
        case action::down:

            if (next_pos.x >= 0 && next_pos.x < state.width && next_pos.y >= 0 && next_pos.y < state.height) {
                auto target = state.board[next_pos.y][next_pos.x];

                switch (target) {

                    case cell::lambda:
                        state.lambdas_collected++;
                        state.score += 25;
                        move_robot(state, current_pos, next_pos);
                        break;

                    case cell::openlift:
                        state.is_ended = 1;
                        state.score += 50 * state.lambdas_collected;
                        move_robot(state, current_pos, next_pos);
                        break;

                    case cell::empty:
                    case cell::earth:
                        move_robot(state, current_pos, next_pos);
                        break;

                    case cell::rock:
                        switch (mv) {
                            case action::left:
                            case action::right: {
                                auto rock_pos = advance_pos(next_pos, mv);
                                if (rock_pos.x >= 0 && rock_pos.x < state.width) {
                                    if (state.board[rock_pos.y][rock_pos.x] == cell::empty) {
                                        move_rock(state.board, next_pos, rock_pos);
                                        move_robot(state, current_pos, next_pos);
                                    }
                                }
                            }
                            default: break;
                        }
                        break;

                    case cell::wall:
                    case cell::lift:
                    case cell::robot:
                        break;
                }
            }

            state.score--;
            break;

        case action::wait:
            state.score--;
            break;

        case action::abort:
            state.is_ended = 1;
            state.score += 25 * state.lambdas_collected;
            break;
    }

    if (state.lambdas_collected >= state.lambdas_total) {
        open_lift(state);
    }

    auto next_board = state.board;
    auto robot_pos = state.robot_pos;
    u8 robot_destroyed = 0;

    for (s32 row = state.height - 2; row >= 0; row--) {
        for (s32 col = 0; col < state.width; col++) {
            if (state.board[row][col] == cell::rock) {
                auto bottom = state.board[row+1][col];
                switch (bottom) {

                    case cell::empty:
                        move_rock(next_board, col, row, col, row + 1, robot_pos, &robot_destroyed);
                        break;

                    case cell::rock:
                        if (col + 1 < state.width && state.board[row][col+1] == cell::empty
                            && state.board[row+1][col+1] == cell::empty) {
                                move_rock(next_board, col, row, col + 1, row + 1, robot_pos, &robot_destroyed);
                        }
                        else if (col - 1 >= 0 && state.board[row][col-1] == cell::empty
                            && state.board[row+1][col-1] == cell::empty) {
                                move_rock(next_board, col, row, col - 1, row + 1, robot_pos, &robot_destroyed);
                        }
                        break;

                    case cell::lambda:
                        if (col + 1 < state.width && state.board[row][col+1] == cell::empty
                            && state.board[row+1][col+1] == cell::empty) {
                                move_rock(next_board, col, row, col + 1, row + 1, robot_pos, &robot_destroyed);
                        }
                        break;

                    default:
                        break;
                }
            }
        }
    }

    state.board = next_board;

    if (!state.is_ended && robot_destroyed) {
        state.is_ended = 1;
    }

    return state;
}


game_state static
runsim(const game_state& currentState, const program_t& prog) {
    auto state = currentState;
    coosq turns = 0;
    coosq max_turns = state.width * state.height;

    for (auto mv : prog) {
        if (state.is_ended || turns >= max_turns) {
            break;
        }

        state = run_step(state, mv);

        turns++;
    }

    if (!state.is_ended) {
        state = run_step(state, action::abort);
    }

    return state;
}


}
