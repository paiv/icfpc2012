#include <functional>
#include <queue>
#include <unordered_set>


namespace paiv {


typedef struct search_state {
    sim_state sim;

    u8 is_terminal;
    u8 is_win;
    program_t prog;

    bool operator == (const search_state& b) const {
        return sim.board == b.sim.board;
    }
} search_state;


}


namespace std {

using namespace paiv;

#if 0
template<>
struct hash<paiv::board_t> {
    inline size_t operator()(const board_t& v) const {
        hash<uint8_t> h;
        size_t seed = 0;
        for (cell x : v) {
            seed ^= h((uint8_t) x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

template<>
struct hash<search_state> {
    inline size_t operator()(const search_state& v) const {
        hash<board_t> b;
        return b(v.sim.board);
    }
};
#endif

}


namespace paiv {


vector<search_state> static
children(const map_info& map, const search_state& currentState) {
    vector<search_state> res;

    if (currentState.is_terminal) {
        return res;
    }

    for (auto mv : all_actions) {

        auto sim = simulator_step(map, currentState.sim, mv);

        auto prog = currentState.prog;
        prog.push_back(mv);

        search_state nextState = {
            sim,
            sim.is_ended,
            sim.robot_pos == map.lift_pos,
            prog,
        };

        res.push_back(nextState);
    }

    return res;
}



search_state static
bfs_player(const map_info& map, const search_state& initialState, const u8& cancelled) {

    auto best = initialState;

    queue<search_state> fringe;
    unordered_set<u64> visited;

    fringe.push(initialState);

    while (fringe.size() > 0 && !cancelled) {
        auto current = fringe.front();
        fringe.pop();

        if (visited.find(current.sim.board_hash) != end(visited)) {
            continue;
        }

        visited.insert(current.sim.board_hash);

        if (current.is_win) {
            return current;
        }
        else if (current.is_terminal) {
            if (current.sim.score > best.sim.score) {
                best = current;
            }
        }

        for (auto& child : children(map, current)) {
            fringe.push(child);
        }
    }

    #if 0
    clog << "visited: " << visited.size() << " (" << visited.size() * sizeof(*begin(visited)) << " bytes)" << endl;
    clog << "fringe: " << fringe.size() << " (" << fringe.size() * sizeof(search_state) << " bytes)" << endl;
    #endif

    return best;
}


program_t static inline
bfs_player(const game_state& game, const u8& cancelled) {

    auto& map = getmap(game);
    auto& sim = getsim(game);

    search_state state = {
        sim,
        sim.is_ended,
        sim.robot_pos == map.lift_pos,
        program_t(),
    };

    state = bfs_player(map, state, cancelled);

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
