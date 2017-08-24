#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>


using namespace std;


namespace paiv {

typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint8_t u8;

typedef double r64;

constexpr s32 s32_min = numeric_limits<s32>::min();
constexpr r64 r64_min = numeric_limits<r64>::min();

typedef vector<u64> u64vec;
typedef unordered_set<u8> u8set;

template<typename T, size_t N>
constexpr
size_t array_len(T(&)[N]) {
    return N;
}


static random_device _rngeesus;
static u64vec hash_table; // [height][width][states]

static void
_fill_random(u64vec& table) {
    mt19937 gen(_rngeesus());
    uniform_int_distribution<u64> distr;

    for (auto& x : table) {
        x = distr(gen);
    }
}

template<typename Iter>
Iter
random_choice(Iter begin, Iter end) {
    mt19937 gen(_rngeesus());
    uniform_int_distribution<> distr(0, distance(begin, end) - 1);
    advance(begin, distr(gen));
    return begin;
}


enum class cell_symbol : u8 {
    empty = ' ',
    earth = '.',
    wall = '#',
    rock = '*',
    lambda = '\\',
    lift = 'L',
    openlift = 'O',
    robot = 'R',
};

enum class cell : u8 {
    empty = 0,
    earth = 1,
    wall = 2,
    rock = 3,
    lambda = 4,
    lift = 5,
    openlift = 6,
    robot = 7,
};

static const cell all_cells[] = {
    cell::empty,
    cell::earth,
    cell::wall,
    cell::rock,
    cell::lambda,
    cell::lift,
    cell::openlift,
    cell::robot,
};


cell static inline
cell_from_symbol(cell_symbol sym) {
    switch (sym) {
        case cell_symbol::empty: return cell::empty;
        case cell_symbol::earth: return cell::earth;
        case cell_symbol::wall: return cell::wall;
        case cell_symbol::rock: return cell::rock;
        case cell_symbol::lambda: return cell::lambda;
        case cell_symbol::lift: return cell::lift;
        case cell_symbol::openlift: return cell::openlift;
        case cell_symbol::robot: return cell::robot;
    }
}

cell_symbol static inline
symb_from_cell(cell val) {
    switch (val) {
        case cell::empty: return cell_symbol::empty;
        case cell::earth: return cell_symbol::earth;
        case cell::wall: return cell_symbol::wall;
        case cell::rock: return cell_symbol::rock;
        case cell::lambda: return cell_symbol::lambda;
        case cell::lift: return cell_symbol::lift;
        case cell::openlift: return cell_symbol::openlift;
        case cell::robot: return cell_symbol::robot;
    }
}

u64 static inline
cell_hash(size_t offset, cell value) {
    return hash_table[offset * (u8) value];
}


typedef enum action : u8 {
    left = 'L',
    right = 'R',
    up = 'U',
    down = 'D',
    wait = 'W',
    abort = 'A',
} action;


typedef vector<cell> board_t;
typedef vector<action> program_t;

typedef s16 coord;
typedef u32 coosq;

typedef struct pos {
    coord x;
    coord y;
} pos;


typedef struct map_info {
    coord width;
    coord height;
    pos lift_pos;
    u32 lambdas_total;
} map_info;


typedef struct sim_state {
    board_t board;
    u64 board_hash;
    pos robot_pos;
    s32 score;
    u32 lambdas_collected;
    u8 is_ended;
} sim_state;


typedef tuple<map_info, sim_state> game_state;

static inline const map_info&
getmap(const game_state& state) {
    return get<0>(state);
}

static inline const sim_state&
getsim(const game_state& state) {
    return get<1>(state);
}


bool operator == (const pos& a, const pos& b) {
    return a.x == b.x && a.y == b.y;
}


ostream& operator << (ostream& so, const board_t& board) {
    const auto stride = so.width();
    so.width(0);

    for (coosq offset = 0; offset + stride <= board.size(); offset += stride) {
        for (coord col = 0; col < stride; col++) {
            so << (char) symb_from_cell(board[offset + col]);
        }
        so << '\n';
    }
    return so;
}

ostream& operator << (ostream& so, const game_state& state) {
    return so << setw(getmap(state).width) << getsim(state).board;
}

ostream& operator << (ostream& so, const program_t& prog) {
    for (char x : prog) {
        so << x;
    }
    return so;
}

}


namespace std {

template<>
struct hash<paiv::program_t> {
    inline size_t operator()(const paiv::program_t& v) const {
        hash<uint8_t> h;
        size_t seed = 0;
        for (auto x : v) {
            seed ^= h(x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

}


namespace paiv {

game_state static
read_map(istream& si) {
    coord max_width = 0;
    coord row = 0;
    vector<vector<cell_symbol>> board({ {} });
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
                board[row].push_back(cell_symbol(c));
                break;

            case '\\': // lambda
                lambdas++;
                board[row].push_back(cell_symbol(c));
                break;

            case 'L': // lift
            case 'O': // openlift
                lift = { .x = (coord) board[row].size(), .y = (coord) row };
                board[row].push_back(cell_symbol(c));
                break;

            case ' ': // empty
            case '.': // earth
            case '#': // wall
            case '*': // rock
                board[row].push_back(cell_symbol(c));
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

    board_t flat_board;
    flat_board.reserve(max_width * row);

    for (auto& row : board) {
        row.resize(max_width, cell_symbol::empty);
        for (auto& sym : row) {
            flat_board.push_back(cell_from_symbol(sym));
        }
    }

    auto width = max_width;
    auto height = row;

    hash_table.resize(width * height * array_len(all_cells));
    _fill_random(hash_table);

    u64 zobrist = 0;
    size_t offset = 0;
    for (auto x : flat_board) {
        zobrist ^= cell_hash(offset, x);
        offset++;
    }


    return make_tuple<map_info, sim_state>(
        {
            width,      // .width
            height,     // .height
            lift,       // .lift_pos
            lambdas,    // .lambdas_total
        },
        {
            flat_board, // .board
            zobrist,    // .board_hash
            robot,      // .robot_pos
            0,          // .score
            0,          // .lambdas_collected
            0,          // .is_ended
        }
    );
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


u64 static inline
_update_hash_on_move(u64 value, coosq source, cell s, coosq target, cell t, cell empty) {
    value ^= cell_hash(target, t);
    value ^= cell_hash(target, s);
    value ^= cell_hash(source, s);
    value ^= cell_hash(source, cell::empty);
    return value;
}

u64 static inline
_update_hash_on_change(u64 value, coosq source, cell s, cell t) {
    value ^= cell_hash(source, s);
    value ^= cell_hash(source, t);
    return value;
}


void static inline
move_entity(board_t& board, u64& board_hash, coord stride, coord from_x, coord from_y, coord to_x, coord to_y) {
    coosq source = from_y * stride + from_x;
    coosq target = to_y * stride + to_x;

    auto t = board[target];
    auto s = board[source];
    board[target] = s;
    board[source] = cell::empty;

    board_hash = _update_hash_on_move(board_hash, source, s, target, t, cell::empty);
}

void static inline
move_entity(sim_state& state, coord stride, const pos& from, const pos& to) {
    move_entity(state.board, state.board_hash, stride, from.x, from.y, to.x, to.y);
}


void static inline
move_robot(const map_info& map, sim_state& state, const pos& from, const pos& to) {
    move_entity(state, map.width, from, to);
    state.robot_pos = to;
}


void static inline
move_rock(board_t& board, u64& board_hash, coord stride,
    coord from_x, coord from_y, coord to_x, coord to_y,
    const pos& robot_pos, u8* robot_destroyed) {

    move_entity(board, board_hash, stride, from_x, from_y, to_x, to_y);

    if (to_x == robot_pos.x && to_y + 1 == robot_pos.y) {
        *robot_destroyed = 1;
    }
}


void static inline
open_lift(sim_state& state, const map_info& map) {
    auto& board = state.board;
    const auto& lift = map.lift_pos;
    coosq source = lift.y * map.width + lift.x;

    auto s = board[source];

    if (s == cell::lift) {
        board[source] = cell::openlift;

        state.board_hash = _update_hash_on_change(state.board_hash, source, s, cell::openlift);
    }
}


typedef void (*simcb_t) (const map_info&, const sim_state&);


sim_state static
simulator_step(const map_info& map, const sim_state& currentState, action mv, simcb_t callback = nullptr) {
    auto state = currentState;
    auto current_pos = currentState.robot_pos;
    auto next_pos = advance_pos(current_pos, mv);
    coord stride = map.width;

    switch (mv) {

        case action::left:
        case action::right:
        case action::up:
        case action::down:

            if (next_pos.x >= 0 && next_pos.x < map.width && next_pos.y >= 0 && next_pos.y < map.height) {
                auto target = state.board[next_pos.y * stride + next_pos.x];

                switch (target) {

                    case cell::lambda:
                        state.lambdas_collected++;
                        state.score += 50;
                        move_robot(map, state, current_pos, next_pos);
                        break;

                    case cell::openlift:
                        state.is_ended = 1;
                        state.score += 25 * state.lambdas_collected;
                        move_robot(map, state, current_pos, next_pos);
                        break;

                    case cell::empty:
                    case cell::earth:
                        move_robot(map, state, current_pos, next_pos);
                        break;

                    case cell::rock:
                        switch (mv) {
                            case action::left:
                            case action::right: {
                                    auto rock_pos = advance_pos(next_pos, mv);
                                    if (rock_pos.x >= 0 && rock_pos.x < map.width) {
                                        if (state.board[rock_pos.y * stride + rock_pos.x] == cell::empty) {
                                            move_entity(state, stride, next_pos, rock_pos);
                                            move_robot(map, state, current_pos, next_pos);
                                        }
                                    }
                                }
                                break;
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
            break;
    }

    if (callback != nullptr) {
        callback(map, state);
    }

    if (state.lambdas_collected >= map.lambdas_total) {
        open_lift(state, map);
    }

    u8 robot_destroyed = 0;

    for (s32 row = map.height - 2; row >= 0; row--) {
        for (s32 col = 0; col < map.width; col++) {
            if (state.board[row * stride + col] == cell::rock) {
                auto bottom = state.board[(row+1) * stride + col];
                switch (bottom) {

                    case cell::empty:
                        move_rock(state.board, state.board_hash, stride, col, row, col, row + 1, state.robot_pos, &robot_destroyed);
                        break;

                    case cell::rock:
                        if (col + 1 < map.width && state.board[row * stride + (col+1)] == cell::empty
                            && state.board[(row+1) * stride + (col+1)] == cell::empty) {
                                move_rock(state.board, state.board_hash, stride, col, row, col + 1, row + 1, state.robot_pos, &robot_destroyed);
                        }
                        else if (col - 1 >= 0 && state.board[row * stride + (col-1)] == cell::empty
                            && state.board[(row+1) * stride + (col-1)] == cell::empty) {
                                move_rock(state.board, state.board_hash, stride, col, row, col - 1, row + 1, state.robot_pos, &robot_destroyed);
                        }
                        break;

                    case cell::lambda:
                        if (col + 1 < map.width && state.board[row * stride + (col+1)] == cell::empty
                            && state.board[(row+1) * stride + (col+1)] == cell::empty) {
                                move_rock(state.board, state.board_hash, stride, col, row, col + 1, row + 1, state.robot_pos, &robot_destroyed);
                        }
                        break;

                    default:
                        break;
                }
            }
        }
    }

    #if 0
    #include <cassert>
    u64 zobrist = 0;
    size_t offset = 0;
    for (auto x : state.board) {
        zobrist ^= cell_hash(offset, x);
        offset++;
    }
    assert(state.board_hash == zobrist);
    #endif

    if (!state.is_ended && robot_destroyed) {
        state.is_ended = 1;
        state.score -= 25 * state.lambdas_collected;
    }

    if (callback != nullptr) {
        callback(map, state);
    }

    return state;
}


enum class runsim_opts {
    no_abort = 0,
    force_abort = 1,
};


sim_state static
runsim(const map_info& map, const sim_state& currentState, const program_t& prog,
    runsim_opts opts = runsim_opts::force_abort, simcb_t callback = nullptr) {

    auto state = currentState;
    coosq turns = 0;
    coosq max_turns = map.width * map.height;

    if (callback != nullptr) {
        callback(map, state);
    }

    for (auto mv : prog) {
        if (state.is_ended || turns >= max_turns) {
            break;
        }

        state = simulator_step(map, state, mv, callback);

        turns++;
    }

    if (!state.is_ended && opts == runsim_opts::force_abort) {
        state = simulator_step(map, state, action::abort, callback);
    }

    return state;
}


sim_state static inline
runsim(const map_info& map, const sim_state& currentState, const program_t& prog, simcb_t callback) {
    return runsim(map, currentState, prog, runsim_opts::force_abort, callback);
}


}
