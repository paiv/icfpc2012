#include <functional>
#include <queue>
#include <unordered_set>


namespace paiv {


typedef struct search_state {
    sim_state sim;

    u8 is_win;
    program_t prog;

    #if 0
    bool operator == (const search_state& b) const {
        return sim.board == b.sim.board && next_move == b.next_move;
    }
    #endif
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


vector<search_state> static inline
children(const map_info& map, const search_state& currentState) {
    vector<search_state> res;
    auto stride = map.width;

    #if 0
    if (currentState.sim.is_ended) {
        return res;
    }
    #endif

    if (currentState.prog.size() >= map.width * map.height) {
        return res;
    }

    for (auto mv : all_actions) {
        auto state = currentState;
        u8 is_valid = 1;

        switch (mv) {

            case action::left:
            case action::right:
            case action::up:
            case action::down: {
                    auto& current_pos = state.sim.robot_pos;
                    auto next_pos = advance_pos(current_pos, mv);

                    u8 inside_bounds = next_pos.x >= 0 && next_pos.x < map.width && next_pos.y >= 0 && next_pos.y < map.height;
                    if (!inside_bounds) {
                        is_valid = 0;
                    }
                    else {
                        auto target = state.sim.board[next_pos.y * stride + next_pos.x];
                        switch (target) {
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
                                                is_valid = state.sim.board[rock_pos.y * stride + rock_pos.x] == cell::empty;
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
            state.prog.push_back(mv);
            res.push_back(state);
        }
    }

    return res;
}



search_state static
bfs_player(const map_info& map, const search_state& initialState, const u8& cancelled) {

    auto best = initialState;
    size_t depth = 0;

    queue<search_state> fringe;
    unordered_set<u64> visited;

    fringe.push(initialState);

    while (fringe.size() > 0 && !cancelled) {
        auto current = fringe.front();
        fringe.pop();


        if (current.prog.size() > 0) {
            current.sim = simulator_step(map, current.sim, current.prog.back());
            current.is_win = current.sim.robot_pos == map.lift_pos;
        }


        if (visited.find(current.sim.board_hash) != end(visited)) {
            continue;
        }

        visited.insert(current.sim.board_hash);

        depth = max(depth, current.prog.size());


        if (current.is_win) {
            return current;
        }

        if (current.sim.score > best.sim.score) {
            best = current;
        }

        for (auto& child : children(map, current)) {
            fringe.push(child);
        }
    }

    #if 0
    clog << "max depth: " << depth << endl;
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
