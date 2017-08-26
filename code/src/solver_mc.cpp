#include <list>
#include <unordered_map>
#include <unordered_set>
#include <utility>


namespace paiv {
namespace mc {


typedef struct search_state {
    sim_state sim;
    u8 is_win;
    program_t prog;
} search_state;

typedef struct node {
    u64 board_hash;
    action mv;
    u8 explored;
    u32 depth;
    u32 visits;
    r64 acc_score;
    r64 acc_score_sq;
    node* parent;
    node* children[sizeof(all_actions)];
} node;


struct memo_t {
    typedef program_t Key;
    typedef search_state Value;
    typedef list<pair<Key,Value>> Container;
    typedef Container::const_iterator Iter;
    typedef unordered_map<Key, Iter> Map;

    static constexpr size_t max_size = 200 * 1024 * 1024 / sizeof(Value);

    Container _container;
    Map _map;

    Iter find(const Key& key) const {
        auto it = _map.find(key);
        if (it == ::end(_map)) {
            return ::end(_container);
        }
        return it->second;
    }

    Iter end() const {
        return ::end(_container);
    }

    void add(Key key, const Value& value) {
        if (_container.size() >= max_size) {
            sweep();
        }
        _container.push_front(make_pair(key, value));
        _map[key] = begin(_container);
    }

    void sweep() {
        // clog << "memo sweep\n";
        auto it = begin(_container);
        advance(it, max_size / 4);
        for (; it != ::end(_container); it++) {
            _map.erase(it->first);
            _container.erase(it);
        }
    }
};


search_state static inline
advance_search(const map_info& map, const search_state& currentState, action mv) {
    auto state = currentState;
    state.sim = simulator_step(map, state.sim, mv);
    state.is_win = state.sim.is_ended && state.sim.robot_pos == map.lift_pos;
    state.prog.push_back(mv);
    return state;
}


search_state static inline
mc_dive(const map_info& map, const search_state& initialState, const u8& cancelled) {
    auto state = initialState;

    unordered_set<u64> visited;
    visited.insert(state.sim.board_hash);

    while (!state.sim.is_ended && !cancelled) {
        u8set exclude;

        auto mv = random_move(map, state.sim, state.prog);

        auto nextState = advance_search(map, state, mv);

        while (!nextState.sim.is_ended && visited.find(nextState.sim.board_hash) != end(visited) && !cancelled) {
            exclude.insert(mv);
            mv = random_move(map, state.sim, state.prog, exclude);
            nextState = advance_search(map, state, mv);
        }

        state = nextState;
        visited.insert(state.sim.board_hash);
    }

    return state;
}


r64 static inline
select_heuristic(const node& n) {
    #if 0
    return n.acc_score / n.visits + sqrt(2.0 * log(n.parent->visits) / n.visits);
    #elif 1
    return n.acc_score / n.visits / 10000.0 + sqrt(2.0 * log(n.parent->visits) / n.visits);
    #else
    constexpr r64 C = 0.5;
    constexpr r64 D = 10000;
    auto x = n.acc_score / n.visits;
    return x + C * sqrt(log(n.parent->visits) / n.visits) + sqrt((n.acc_score_sq - n.visits * x * x + D) / n.visits);
    #endif
}


search_state static inline
player(const map_info& map, const search_state& initialState, const r64 timelimit, const u8& cancelled) {
    auto started = appclock::now();

    s32 best_score = initialState.sim.score;
    auto best_state = initialState;

    static constexpr size_t tree_max_size = 500 * 1024 * 1024 / sizeof(node);

    vector<node> search_tree;
    unordered_map<u64,node*> visited;

    search_tree.reserve(tree_max_size);

    node root = {
        initialState.sim.board_hash,   // .board_hash
        action::abort,          // .mv
        0,                      // .explored
        0,                      // .depth
        0,                      // .visits
        0,                      // .acc_score
        0,                      // .acc_score_sq
        nullptr,                // .parent
        {nullptr},              // .children
    };

    if (search_tree.size() < tree_max_size) {
        search_tree.push_back(root);
        visited[initialState.sim.board_hash] = &search_tree.back();
    }

    memo_t memo;
    memo.add({}, initialState);

    program_t path;

    while (!cancelled) {
        logger << "loop score: " << best_score << ", tree size: " << search_tree.size() << " (max " << tree_max_size << ")"<< endl;

        auto now = appclock::now();
        chrono::duration<r64> elapsed = now - started;
        if (elapsed.count() >= timelimit) {
            break;
        }

        // select
        // logger << "select\n";
        node* selected = &search_tree[0];
        path.clear();

        if (selected->explored) {
            break;
        }

        while (1) {
            if (selected->visits == 0) {
                break;
            }

            vector<node*> candidates;

            for (auto p : selected->children) {
                if (p != nullptr && p->visits == 0) {
                    candidates.push_back(p);
                }
            }

            if (candidates.size() > 0) {
                selected = *random_choice(begin(candidates), end(candidates));
                path.push_back(selected->mv);
                break;
            }

            r64 child_score = r64_min;
            node* best_child = nullptr;

            for (auto p : selected->children) {
                if (p != nullptr && !p->explored) {
                    auto score = select_heuristic(*p);
                    if (score > child_score) {
                        child_score = score;
                        best_child = p;
                    }
                }
            }

            if (best_child != nullptr) {
                selected = best_child;
                path.push_back(selected->mv);
            }
            else {
                selected->explored = 1;
                break;
            }
        }

        r64 score = s32_min;

        if (selected->explored) {
            score = selected->acc_score / selected->visits;
        }
        else {
            // expand
            // logger << "expand for " << path << "\n";
            search_state state = {};

            auto it = memo.find(path);
            if (it != memo.end()) {
                state = it->second;
            }
            else {
                state.sim = runsim(map, initialState.sim, path, runsim_opts::no_abort);
                state.is_win = state.sim.is_ended && state.sim.robot_pos == map.lift_pos;
                state.prog = path;
                memo.add(path, state);
            }

            const auto& selected_state = state;
            size_t child_index = 0;

            auto moves = legal_moves(map, selected_state.sim, selected_state.prog);
            selected->explored = moves.size() == 0;

            for (auto mv : moves) {
                auto child = advance_search(map, selected_state, mv);
                auto child_hash = child.sim.board_hash;
                auto child_depth = selected->depth + 1;

                auto it = visited.find(child_hash);
                if (it != end(visited) && it->second->depth <= child_depth) {
                    continue;
                }

                memo.add(child.prog, child);

                if (search_tree.size() < tree_max_size) {
                    node child_node = {
                        child_hash,         // .board_hash
                        mv,                 // .mv
                        0,                  // .explored
                        child_depth,        // .depth
                        0,                  // .visits
                        0,                  // .acc_score
                        0,                  // .acc_score_sq
                        selected,           // .parent
                        {nullptr},          // .children
                    };

                    search_tree.push_back(child_node);
                    auto& n = search_tree.back();
                    selected->children[child_index++] = &n;

                    visited[child_hash] = &n;
                }
            }

            // simulate
            // logger << "simulate\n";
            auto deep_state = mc_dive(map, selected_state, cancelled);
            score = deep_state.sim.score;

            if (score > best_score) {
                best_score = score;
                best_state = deep_state;
                // logger << "best update: " << best_score << " " << best_state.prog << endl;
            }
        }

        // backprop
        // logger << "backprop\n";
        for (auto n = selected; n != nullptr; n = n->parent) {
            n->acc_score += score;
            n->acc_score_sq += score * score;
            n->visits++;
        }
    }

    #if 0
    for (const auto& n : search_tree) {
        logger << "  " << n.board_hash;
        if (n.parent != nullptr) logger << " -> " << n.parent->board_hash;
        logger << " | " << (char)n.mv << " visits:" << n.visits;
        logger << ", acc:" << n.acc_score << "\n";
    }
    for (const auto& n : search_tree) {
        logger << n.board_hash << endl;
        auto p = visited[n.board_hash];
        program_t path;
        for (; p != nullptr && p->parent != nullptr; p = p->parent) {
            path.push_back(p->mv);
        }
        logger << path << endl;
        // logger << setw(map.width) << state.sim.board << endl;
    }
    #endif

    return best_state;
}


program_t static inline
player(const game_state& game, const r64 timelimit, const u8& cancelled, const u32 retries = 1) {
    s32 best_score = s32_min;
    program_t best_prog;

    for (u32 i = 0; i < retries; i++) {
        auto& map = getmap(game);
        auto& sim = getsim(game);

        search_state state = {
            sim,
            sim.robot_pos == map.lift_pos,
            program_t(),
        };

        state = player(map, state, timelimit / retries, cancelled);

        if (state.sim.score > best_score) {
            best_score = state.sim.score;
            best_prog = state.prog;
        }
    }

    return best_prog;
}


} // namespace mc
}
