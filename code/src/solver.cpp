#include <functional>
#include <queue>
#include <unordered_set>


namespace paiv {


typedef struct search_state {
    board_t board;

    pos robot_pos;
    s32 score;
    u32 lambdas_collected;

    u8 is_terminal;
    u8 is_win;

    program_t prog;

    bool operator == (const search_state& b) const {
        return board == b.board;
    }
} search_state;


}


namespace std {

using namespace paiv;

template<>
struct hash<paiv::board_t> {
    inline size_t operator()(const board_t& v) const {
        hash<uint8_t> h;
        size_t seed = 0;
        for (uint8_t x : v) {
            seed ^= h(x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

template<>
struct hash<search_state> {
    inline size_t operator()(const search_state& v) const {
        hash<board_t> b;
        return b(v.board);
    }
};


}


namespace paiv {


vector<search_state> static
children(const search_state& currentState, const game_state& game) {
    vector<search_state> res;

    if (currentState.is_terminal) {
        return res;
    }

    for (auto mv : all_actions) {
        game_state sim_state = {
            game.width,
            game.height,
            game.lift_pos,
            game.lambdas_total,

            currentState.board,
            currentState.robot_pos,
            currentState.score,
            currentState.lambdas_collected,

            currentState.is_terminal,
        };

        sim_state = simulator_step(sim_state, mv);

        auto prog = currentState.prog;
        prog.push_back(mv);

        search_state nextState = {
            sim_state.board,
            sim_state.robot_pos,
            sim_state.score,
            sim_state.lambdas_collected,
            sim_state.is_ended,
            sim_state.robot_pos == game.lift_pos,
            prog,
        };

        res.push_back(nextState);
    }

    return res;
}



search_state static
bfs_player(const game_state& game, const search_state& initialState, const u8& cancelled) {

    auto best = initialState;

    queue<search_state> fringe;
    unordered_set<search_state> visited;

    fringe.push(initialState);

    while (fringe.size() > 0 && !cancelled) {
        auto current = fringe.front();
        fringe.pop();

        if (visited.find(current) != end(visited)) {
            continue;
        }

        visited.insert(current);

        if (current.is_win) {
            return current;
        }
        else if (current.is_terminal) {
            if (current.score > best.score) {
                best = current;
            }
        }

        for (auto& child : children(current, game)) {
            fringe.push(child);
        }
    }

    return best;
}


program_t static inline
bfs_player(const game_state& game, const u8& cancelled) {

    search_state state = {
        game.board,
        game.robot_pos,
        game.score,
        game.lambdas_collected,
        game.is_ended,
        0,
        program_t(),
    };

    state = bfs_player(game, state, cancelled);

    return state.prog;
}


s32 static
solve(istream& si, ostream& so, const u8& cancelled) {
    auto game = read_map(si);

    program_t prog = bfs_player(game, cancelled);

    so << prog;

    return 0;
}


}
