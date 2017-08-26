#include <chrono>
#include <fstream>


#define VERBOSE 1

using appclock = chrono::high_resolution_clock;


namespace paiv {

class nullstream : public ofstream {};

#if VERBOSE
#define logger clog
#else
static nullstream logger;
#endif


static constexpr action all_actions[] = {
    action::left,
    action::right,
    action::up,
    action::down,
    action::wait,
    // action::abort,
};


vector<action> static inline
legal_moves(const map_info& map, const sim_state& sim, const program_t& prog, const u8set& exclude = {}) {

    if (sim.is_ended) {
        return {};
    }

    if (prog.size() >= map.width * map.height) {
        return {};
    }

    vector<action> res;
    auto stride = map.width;

    for (auto mv : all_actions) {
        if (exclude.find(mv) != end(exclude)) continue;

        u8 is_valid = 1;

        switch (mv) {

            case action::left:
            case action::right:
            case action::up:
            case action::down: {
                    auto& current_pos = sim.robot_pos;
                    auto next_pos = advance_pos(current_pos, mv);

                    u8 inside_bounds = next_pos.x >= 0 && next_pos.x < map.width && next_pos.y >= 0 && next_pos.y < map.height;
                    if (!inside_bounds) {
                        is_valid = 0;
                    }
                    else {
                        auto target = sim.board[next_pos.y * stride + next_pos.x];
                        switch (target) {
                            case cell::none:
                            case cell::empty:
                            case cell::earth:
                            case cell::lambda:
                            case cell::openlift:
                                break;

                            case cell::rock:
                                switch (mv) {
                                    case action::left:
                                    case action::right: {
                                            auto rock_pos = advance_pos(next_pos, mv);
                                            is_valid = rock_pos.x >= 0 && rock_pos.x < map.width;
                                            if (is_valid)
                                                is_valid = sim.board[rock_pos.y * stride + rock_pos.x] == cell::empty;
                                        }
                                        break;
                                    default:
                                        is_valid = 0;
                                        break;
                                }
                                break;

                            case cell::wall:
                            case cell::lift:
                            case cell::robot:
                                is_valid = 0;
                                break;
                        }
                    }
                }
                break;

            case action::wait:
            case action::abort:
                break;
        }

        if (is_valid) {
            res.push_back(mv);
        }
    }

    return res;
}


action static inline
random_move(const map_info& map, const sim_state& sim, const program_t& prog, const u8set& exclude = {}) {
    auto moves = legal_moves(map, sim, prog, exclude);
    if (moves.size() > 0) {
        return *random_choice(begin(moves), end(moves));
    }
    return action::abort;
}

}


// #include "solver_bfs.cpp"
// #include "solver_mc.cpp"
#include "solver_pl.cpp"

namespace paiv {

s32 static
solve(istream& si, ostream& so, const r64 timelimit, const u8& cancelled) {
    auto game = read_map(si);

    // program_t prog = bfs::player(game, timelimit, cancelled);
    // program_t prog = mc::player(game, timelimit, cancelled);
    program_t prog = pl::player(game, timelimit, cancelled);

    so << prog;

    return 0;
}


}
