#include <queue>
#include <unordered_set>


namespace paiv {
namespace bfs {


typedef struct search_state {
    sim_state sim;
    u8 is_win;
    program_t prog;
} search_state;


vector<search_state> static inline
children(const map_info& map, const search_state& currentState) {
    vector<search_state> res;

    for (auto mv : legal_moves(map, currentState.sim, currentState.prog)) {
        auto state = currentState;
        state.prog.push_back(mv);
        res.push_back(state);
    }

    return res;
}


search_state static
player(const map_info& map, const search_state& initialState, const u8& cancelled) {

    auto best = initialState;
    // size_t depth = 0;

    queue<search_state> fringe;
    unordered_set<u64> visited;

    fringe.push(initialState);

    while (fringe.size() > 0 && !cancelled) {
        auto current = fringe.front();
        fringe.pop();


        if (current.prog.size() > 0) {
            current.sim = simulator_step(map, current.sim, current.prog.back());
            current.is_win = current.sim.is_ended && current.sim.robot_pos == map.lift_pos;
        }


        if (visited.find(current.sim.board_hash) != end(visited)) {
            continue;
        }

        visited.insert(current.sim.board_hash);

        // depth = max(depth, current.prog.size());


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
    logger << "max depth: " << depth << endl;
    logger << "visited: " << visited.size() << " (" << visited.size() * sizeof(*begin(visited)) << " bytes)" << endl;
    logger << "fringe: " << fringe.size() << " (" << fringe.size() * sizeof(search_state) << " bytes)" << endl;
    #endif

    return best;
}


program_t static inline
player(const game_state& game, const r64 timelimit, const u8& cancelled) {

    auto& map = getmap(game);
    auto& sim = getsim(game);

    search_state state = {
        sim,
        sim.robot_pos == map.lift_pos,
        program_t(),
    };

    state = player(map, state, cancelled);

    return state.prog;
}


} // namespace bfs
}
