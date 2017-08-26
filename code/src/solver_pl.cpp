#include <algorithm>
#include <forward_list>
#include <list>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include "astar.hpp"


namespace paiv {
namespace pl {


typedef struct search_state {
    sim_state sim;
    u8 is_win;
    program_t prog;
} search_state;


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


typedef struct path_search_graph {
    typedef search_state Node;
    typedef program_t Location;
    typedef program_t Path;
    typedef s32 Distance;

    static constexpr Distance MaxDistance = s32_max;

    const map_info& map;
    const Location root;
    unordered_map<Location, search_state> tree;
    unordered_set<u64> visited;

    path_search_graph(const map_info& map, const sim_state& initial, const Location& root) : map(map), root(root) {
        u8 is_win = initial.robot_pos == map.lift_pos;
        tree[root] = {initial, is_win, root};
    }

    Location stub_node(const pos& at) {
        search_state state = {};
        state.sim.robot_pos = at;
        state.prog = { action::abort };
        tree[state.prog] = state;
        return state.prog;
    }

    u8 check_goal(const Location& at, const Location& goal) const {
        auto fromit = tree.find(at);
        auto goalit = tree.find(goal);
        if (fromit != end(tree) && goalit != end(tree)) {
            return fromit->second.sim.robot_pos == goalit->second.sim.robot_pos;
        }
        return 0;
    }

    vector<Location> children(const Location& from) {
        auto fromit = tree.find(from);
        if (fromit == end(tree)) {
            return {};
        }

        auto parent = fromit->second;

        visited.insert(parent.sim.board_hash);

        vector<Location> res;

        auto moves = legal_moves(map, parent.sim, parent.prog);

        for (auto mv : moves) {
            #if 0
            switch (mv) {
                case action::left: {
                        auto& pp = parent.sim.robot_pos;
                        auto& board = parent.sim.board;
                        cell left = board[pp.y * map.width + pp.x - 1];
                        if (left == cell::rock) {
                            continue;
                        }
                    }
                    break;
                case action::right: {
                        auto& pp = parent.sim.robot_pos;
                        auto& board = parent.sim.board;
                        cell left = board[pp.y * map.width + pp.x + 1];
                        if (left == cell::rock) {
                            continue;
                        }
                    }
                    break;
                default:
                    break;
            }
            #endif

            auto sim = simulator_step(map, parent.sim, mv);
            auto path = parent.prog;
            path.push_back(mv);

            if (visited.find(sim.board_hash) != end(visited)) {
                continue;
            }

            u8 is_win = sim.robot_pos == map.lift_pos;
            tree[path] = {sim, is_win, path};
            res.push_back(path);
        }

        return res;
    }

    Path path(const Node& from, const Node& to) const {
        auto it = begin(to.prog);
        for (auto x = begin(from.prog);
            x != end(from.prog) && it != end(to.prog) && *x == *it;
            x++, it++) {
        }

        return Path(it, end(to.prog));
    }

    Distance distance(const Location& from, const Location& to) const {
        auto fromit = tree.find(from);
        auto toit = tree.find(to);
        if (fromit != end(tree) && toit != end(tree)) {
            return path(fromit->second, toit->second).size();
        }
        return MaxDistance;
    }

    Distance path_estimate(const Location& from, const Location& goal) const {
        auto fromit = tree.find(from);
        auto goalit = tree.find(goal);
        if (fromit != end(tree) && goalit != end(tree)) {
            return manhattan_distance(fromit->second.sim.robot_pos, goalit->second.sim.robot_pos);
        }
        return MaxDistance;
    }

} path_search_graph;


search_state static
find_path(const map_info& map, const search_state& initialState, const pos& goal,
    const r64 timelimit, const u8& cancelled) {

    auto started = appclock::now();

    path_search_graph gen(map, initialState.sim, initialState.prog);
    auto goal_state = gen.stub_node(goal);

    astar::search<path_search_graph> find_path(gen);
    auto path = find_path(gen.root, goal_state, timelimit);

    if (path.size() > 0) {
        // logger << "found " << path.back() << endl;
        return gen.tree[path.back()];
    }

    search_state invalid;
    invalid.sim.robot_pos = {-1, -1};

    return invalid;
}


unordered_set<pos> static inline
plan_goals(const map_info& map, const search_state& state) {
    const auto& board = state.sim.board;
    auto stride = map.width;

    unordered_set<pos> goals;
    u8 waiting_ok = 0;

    for (coord row = 0; row < map.height; row++) {
        for (coord col = 0; col < map.width; col++) {

            cell value = board[row * stride + col];

            switch (value) {

                case cell::lambda:
                case cell::openlift:
                    goals.insert({col, row});
                    break;

                case cell::earth: {
                        coosq offset = row * stride + col;
                        cell up = row > 0 ? board[offset - stride] : cell::none;
                        cell left = col > 0 ? board[offset - 1] : cell::none;
                        cell upleft = (row > 0 && col > 0) ? board[offset - stride - 1] : cell::none;
                        cell right = col + 1 < map.width ? board[offset + 1] : cell::none;
                        cell left2 = col > 1 ? board[offset - 2] : cell::none;
                        cell left3 = col > 2 ? board[offset - 3] : cell::none;
                        cell right2 = col + 2 < map.width ? board[offset + 2] : cell::none;
                        cell right3 = col + 3 < map.width ? board[offset + 3] : cell::none;

                        if (up == cell::rock) {
                            goals.insert({col, row});
                        }
                        else if (left == cell::rock) {
                            switch (left2) {
                                case cell::empty:
                                case cell::earth:
                                case cell::lambda:
                                    goals.insert({col, row});
                                    break;
                                default:
                                    break;
                            }
                        }
                        else if (right == cell::rock) {
                            switch (right2) {
                                case cell::empty:
                                case cell::earth:
                                case cell::lambda:
                                    goals.insert({col, row});
                                    break;
                                default:
                                    break;
                            }
                        }
                        else if (left == cell::lambda && upleft == cell::rock) {
                            goals.insert({col, row});
                        }
                        else if (left == cell::empty && left2 == cell::rock) {
                            goals.insert({col, row});
                        }
                        else if (left == cell::empty && left2 == cell::empty && left3 == cell::rock) {
                            goals.insert({col, row});
                        }
                        else if (right == cell::empty && right2 == cell::rock) {
                            goals.insert({col, row});
                        }
                        else if (right == cell::empty && right2 == cell::empty && right3 == cell::rock) {
                            goals.insert({col, row});
                        }
                    }
                    break;

                case cell::rock: {
                        coosq offset = row * stride + col;
                        cell left = col > 0 ? board[offset - 1] : cell::none;
                        cell right = col + 1 < map.width ? board[offset + 1] : cell::none;

                        if (left == cell::robot && right == cell::empty) {
                            goals.insert({col, row});
                        }
                        else if (right == cell::robot && left == cell::empty) {
                            goals.insert({col, row});
                        }

                        if (!waiting_ok) {
                            cell down = row + 1 < map.height ? board[offset + stride] : cell::none;

                            if (down == cell::empty) {
                                waiting_ok = 1;
                            }
                            else if (down == cell::rock) {
                                cell downright = col + 1 < map.width && row + 1 < map.height ? board[offset + stride + 1] : cell::none;
                                cell downleft = col > 0 && row + 1 < map.height ? board[offset + stride - 1] : cell::none;

                                waiting_ok = (right == cell::empty && downright == cell::empty) ||
                                    (left == cell::empty && downleft == cell::empty);
                            }
                            else if (down == cell::lambda) {
                                cell downright = col + 1 < map.width && row + 1 < map.height ? board[offset + stride + 1] : cell::none;

                                waiting_ok = right == cell::empty && downright == cell::empty;
                            }
                        }
                    }
                    break;

                case cell::none:
                case cell::empty:
                case cell::wall:
                case cell::lift:
                case cell::robot:
                    break;
            }
        }
    }

    if (waiting_ok) {
        goals.insert(state.sim.robot_pos);
    }

    return goals;
}


search_state static inline
advance_search(const map_info& map, const search_state& currentState, action mv) {
    auto state = currentState;
    state.sim = simulator_step(map, state.sim, mv);
    state.is_win = state.sim.is_ended && state.sim.robot_pos == map.lift_pos;
    state.prog.push_back(mv);
    return state;
}


vector<search_state> static inline
plan_children(const map_info& map, const search_state& initialState, const r64 timelimit, const u8& cancelled) {
    vector<search_state> res;

    auto goals = plan_goals(map, initialState);

    auto tm = timelimit / (goals.size() + 2);

    for (auto& goal : goals) {
        if (goal == initialState.sim.robot_pos) {
            auto state = advance_search(map, initialState, action::wait);

            if (state.sim.robot_pos == goal) {
                res.push_back(state);
            }
        }
        else {
            auto state = find_path(map, initialState, goal, tm, cancelled);
            // logger << "path from " << initialState.sim.robot_pos << " to " << goal << ": " << state.prog << endl;

            if (state.sim.robot_pos == goal) {
                res.push_back(state);
            }
        }
    }

    return res;
}


template<typename T>
unordered_set<T>
_difference(const unordered_set<T>& a, const unordered_set<T>& b) {
    unordered_set<T> res;

    copy_if(begin(a), end(a), inserter(res, begin(res)),
        [&b] (const T& x) { return b.find(x) == b.end(); }
    );

    return res;
}


search_state static inline
plan_random(const map_info& map, const search_state& initialState, const u8& cancelled, const unordered_set<pos>& exclude = {}) {
    auto goals = plan_goals(map, initialState);
    goals = _difference(goals, exclude);

    if (goals.size() > 0) {
        auto goal = *random_choice(begin(goals), end(goals));
        auto state = find_path(map, initialState, goal, 1.0, cancelled);
        if (state.sim.robot_pos == goal) {
            return state;
        }
    }

    auto state = initialState;
    state.sim.is_ended = 1;
    return state;
}


search_state static inline
player_bfs(const map_info& map, const search_state& initialState, const r64 timelimit, const u8& cancelled) {
    auto started = appclock::now();

    queue<search_state> fringe;
    unordered_set<u64> visited;

    fringe.push(initialState);

    search_state best = initialState;

    while (fringe.size() > 0) {
        auto now = appclock::now();
        chrono::duration<r64> elapsed = now - started;
        if (elapsed.count() >= timelimit) {
            break;
        }

        auto current = fringe.front();
        fringe.pop();

        if (visited.find(current.sim.board_hash) != end(visited)) {
            continue;
        }
        visited.insert(current.sim.board_hash);

        // logger << "current: " << current.sim.robot_pos << " " << current.prog << endl;

        if (current.sim.score > best.sim.score) {
            best = current;
        }

        if (current.is_win) { // bfs
            break;
        }

        auto tm = timelimit / (map.lambdas_total * map.lambdas_total * map.lambdas_total + 2);

        for (auto child : plan_children(map, current, tm, cancelled)) {
            // logger << "  child " << child.sim.robot_pos << " " << child.prog << endl;
            fringe.push(child);
        }
    }

    return best;
}


typedef struct node {
    u64 board_hash;
    u32 visits;
    r64 acc_score;
    node* parent;
    vector<node*> children;
    program_t prog;
    u8 explored;
} node;


r64 static inline
mc_select_heuristic(const node& n) {
    #if 0
    return n.acc_score / n.visits + sqrt(2.0 * log(n.parent->visits) / n.visits);
    #else
    return n.acc_score / n.visits / 10000.0 + sqrt(2.0 * log(n.parent->visits) / n.visits);
    #endif
}


search_state static inline
mc_dive(const map_info& map, const search_state& initialState, const u8& cancelled) {
    auto state = initialState;

    unordered_set<u64> visited;
    visited.insert(state.sim.board_hash);

    while (!state.sim.is_ended && !cancelled) {
        unordered_set<pos> exclude;

        auto nextState = plan_random(map, state, cancelled);

        while (!nextState.sim.is_ended && visited.find(nextState.sim.board_hash) != end(visited) && !cancelled) {
            exclude.insert(nextState.sim.robot_pos);
            nextState = plan_random(map, state, cancelled, exclude);
        }

        state = nextState;
        visited.insert(state.sim.board_hash);
    }

    return state;
}


search_state static inline
player_mc(const map_info& map, const search_state& initialState, const r64 timelimit, const u8& cancelled) {
    auto started = appclock::now();

    static constexpr size_t tree_max_size = 500 * 1024 * 1024 / sizeof(node);

    vector<node> search_tree;
    unordered_map<u64, node*> visited;

    search_tree.reserve(tree_max_size);

    node root = {
        initialState.sim.board_hash,   // .board_hash
        0,                      // .visits
        0,                      // .acc_score
        nullptr,                // .parent
        {},                     // .children
        initialState.prog,      // .prog
        0,                      // .explored
    };

    if (search_tree.size() < tree_max_size) {
        search_tree.push_back(root);
        visited[initialState.sim.board_hash] = &search_tree.back();
    }

    memo_t memo;
    memo.add(initialState.prog, initialState);

    auto best = initialState;

    while (!cancelled) {

        auto now = appclock::now();
        chrono::duration<r64> elapsed = now - started;
        auto timeleft = timelimit - elapsed.count();
        if (timeleft <= 0) {
            break;
        }

        // select
        // logger << "select\n";
        node* selected = &search_tree[0];

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
                break;
            }

            r64 child_score = r64_min;
            node* best_child = nullptr;

            for (auto p : selected->children) {
                if (p != nullptr && !p->explored) {
                    auto score = mc_select_heuristic(*p);
                    if (score > child_score) {
                        child_score = score;
                        best_child = p;
                    }
                }
            }

            if (best_child != nullptr) {
                selected = best_child;
            }
            else {
                selected->explored = 1;
                break;
            }
        }

        r64 score = r64_min;

        if (selected->explored) {
            score = selected->acc_score / selected->visits;
        }
        else {
            // expand
            // logger << "expand for " << selected->prog << "\n";
            search_state state = {};

            auto it = memo.find(selected->prog);
            if (it != memo.end()) {
                state = it->second;
            }
            else {
                state.sim = runsim(map, initialState.sim, selected->prog, runsim_opts::no_abort);
                state.is_win = state.sim.is_ended && state.sim.robot_pos == map.lift_pos;
                state.prog = selected->prog;
                memo.add(state.prog, state);
            }

            // logger << "selected: " << state.sim.robot_pos << " " << state.prog << endl;

            const auto& selected_state = state;
            size_t child_index = 0;

            auto children = plan_children(map, selected_state, timeleft / 2, cancelled);

            selected->explored = children.size() == 0;

            for (auto& child : children) {
                // logger << "  child " << child.sim.robot_pos << " " << child.prog << endl;

                auto child_hash = child.sim.board_hash;

                auto it = visited.find(child_hash);
                if (it != end(visited) && it->second->prog.size() <= child.prog.size()) {
                    continue;
                }

                memo.add(child.prog, child);

                if (search_tree.size() < tree_max_size) {
                    node child_node = {
                        child_hash,         // .board_hash
                        0,                  // .visits
                        0,                  // .acc_score
                        selected,           // .parent
                        {nullptr},          // .children
                        child.prog,         // .prog
                        0,                  // .explored
                    };

                    search_tree.push_back(child_node);
                    auto& n = search_tree.back();
                    selected->children.push_back(&n);

                    visited[child_hash] = &n;
                }
            }

            // simulate
            // logger << "simulate\n";
            auto deep_state = mc_dive(map, selected_state, cancelled);
            score = deep_state.sim.score;

            if (score > best.sim.score) {
                best = deep_state;
                // logger << "best update: " << best_score << " " << best_state.prog << endl;
            }
        }

        // backprop
        // logger << "backprop\n";
        for (auto n = selected; n != nullptr; n = n->parent) {
            n->acc_score += score;
            n->visits++;
        }
    }

    return best;
}


s32 static inline
plan_distance(const search_state& a, const search_state& b) {
    return -(b.sim.score - a.sim.score);
}


s32 static inline
plan_heuristic(const map_info& map, const search_state& state) {
    // return state.sim.lambdas_collected + (state.is_win ? 0 : map.width + map.height) + state.prog.size();
    // return state.sim.lambdas_collected + (state.is_win ? 0 : 1);
    return -state.sim.score;
}


typedef struct plan_search_graph {
    typedef search_state Node;
    typedef u64 Location;
    typedef program_t Path;
    typedef s32 Distance;

    static constexpr Distance MaxDistance = s32_max;

    const map_info& map;
    const Node& initial;
    const Location root;
    unordered_map<Location, Node> tree;
    unordered_set<u64> visited;

    plan_search_graph(const map_info& map, const Node& initial) :
        map(map), initial(initial), root(initial.sim.board_hash) {

        tree[root] = initial;
    }

    Location stub_node(const pos& at) {
        Node node = initial;
        node.sim.robot_pos = at;
        node.sim.board[at.y * map.width + at.x] = cell::robot;
        node.sim.board_hash = board_hash(node.sim.board);
        node.prog = { action::abort };

        Location loc = node.sim.board_hash;

        tree[loc] = node;

        return loc;
    }

    u8 check_goal(const Location& at, const Location& goal) const {
        auto fromit = tree.find(at);
        #if 0
        auto goalit = tree.find(goal);
        if (fromit != end(tree) && goalit != end(tree)) {
            return fromit->second.sim.robot_pos == goalit->second.sim.robot_pos;
        }
        return 0;
        #else
            return fromit != end(tree) && fromit->second.is_win;
        #endif
    }

    vector<Location> children(const Location& from) {
        auto fromit = tree.find(from);
        if (fromit == end(tree)) {
            return {};
        }

        auto parent = fromit->second;

        visited.insert(parent.sim.board_hash);

        // logger << "from " << parent.sim.robot_pos << " " << parent.prog << endl;

        vector<Location> res;

        // auto tm = timelimit / (map.lambdas_total * map.lambdas_total * map.lambdas_total + 2);
        r64 tm = 0.1;
        u8 cancelled = 0; // TODO:

        for (auto child : plan_children(map, parent, tm, cancelled)) {
            // logger << "  child " << child.sim.robot_pos << " " << child.prog << endl;
            auto loc = child.sim.board_hash;
            if (visited.find(loc) == end(visited)) {
                tree[loc] = child;
                res.push_back(loc);
            }
        }

        return res;
    }

    Path path(const Node& from, const Node& to) const {
        auto it = begin(to.prog);
        for (auto x = begin(from.prog);
            x != end(from.prog) && it != end(to.prog) && *x == *it;
            x++, it++) {
        }

        return Path(it, end(to.prog));
    }

    Distance distance(const Location& from, const Location& to) const {
        auto fromit = tree.find(from);
        auto toit = tree.find(to);
        if (fromit != end(tree) && toit != end(tree)) {
            #if 0
            return path(fromit->second, toit->second).size();
            #else
            return abs(toit->second.sim.score - fromit->second.sim.score);
            #endif
        }
        return MaxDistance;
    }

    Distance path_estimate(const Location& from, const Location& goal) const {
        auto fromit = tree.find(from);
        #if 0
        auto goalit = tree.find(goal);
        if (fromit != end(tree) && goalit != end(tree)) {
            return manhattan_distance(fromit->second.sim.robot_pos, goalit->second.sim.robot_pos);
        }
        #else
        if (fromit != end(tree)) {
            auto max_score = map.lambdas_total * 25; //  - manhattan_distance(initial.sim.robot_pos, map.lift_pos);
            return max_score - fromit->second.sim.score;
        }
        #endif
        return MaxDistance;
    }

} plan_search_graph;


search_state static inline
player_astar(const map_info& map, const search_state& initialState, const r64 timelimit, const u8& cancelled) {
    auto started = appclock::now();

    auto best = initialState;

    plan_search_graph gen(map, initialState);
    auto goal_state = gen.stub_node(map.lift_pos);

    astar::search<plan_search_graph> find_path(gen);
    auto path = find_path(gen.root, goal_state, timelimit);

    if (path.size() > 0) {
        return gen.tree[path.back()];
    }

    return best;
}


typedef struct distance_order {
    pos origin;

    bool inline
    operator () (pos a, pos b) const {
        return manhattan_distance(a, origin) < manhattan_distance(b, origin);
    }
} distance_order;


search_state static inline
player_rand(const map_info& map, const search_state& initialState, const r64 timelimit, const u8& cancelled) {
    auto started = appclock::now();

    auto best = initialState;
    u64 dives = 0;

    while (!cancelled) {
        auto now = appclock::now();
        chrono::duration<r64> elapsed = now - started;
        auto timeleft = timelimit - elapsed.count();
        if (timeleft <= 0) {
            break;
        }

        auto tm = timeleft / 4;

        #if 0
        auto state = mc_dive(map, initialState, cancelled);

        #else
        auto state = initialState;
        // logger << "dive " << dives << " pos " << state.sim.robot_pos << endl;

        unordered_set<u64> visited;
        visited.insert(state.sim.board_hash);

        while (!state.sim.is_ended && !cancelled) {
            unordered_set<pos> exclude;

            auto goals = plan_goals(map, state);

            while (!cancelled) {
                goals = _difference(goals, exclude);

                if (goals.empty()) {
                    state.sim.is_ended = 1;
                    break;
                }

                distance_order distcomp = {state.sim.robot_pos};
                vector<pos> goalsvec(begin(goals), end(goals));
                sort(begin(goalsvec), end(goalsvec), distcomp);

                goalsvec.resize(min(size_t(10), goalsvec.size()));

                #if 0
                logger << "  from goals:";
                for (auto x : goalsvec) {
                    logger << " " << x;
                }
                logger << endl;
                #endif

                #if 1
                auto board = state.sim.board;
                for (size_t i = goalsvec.size(); i > 0; i--) {
                    const pos goal = goalsvec[i - 1];
                    switch (board[goal.y * map.width + goal.x]) {
                        case cell::lambda:
                        case cell::openlift:
                            goalsvec.push_back(goal);
                            break;
                        default:
                            break;
                    }
                }
                #endif

                auto goal = *random_choice(begin(goalsvec), end(goalsvec));
                // logger << "  picked " << goal << endl;

                auto nextState = (goal == state.sim.robot_pos) ? advance_search(map, state, action::wait) :
                    find_path(map, state, goal, 1.0, cancelled);
                // logger << "    find_path: " << nextState.sim.robot_pos << endl;
                if (nextState.sim.robot_pos != goal || visited.find(nextState.sim.board_hash) != end(visited)) {
                    // logger << "    exclude " << goal << endl;
                    exclude.insert(goal);
                    continue;
                }

                state = nextState;
                visited.insert(state.sim.board_hash);
                // logger << "  pos " << state.sim.robot_pos << endl;
                break;
            }
        }
        #endif

        dives++;

        if (state.sim.score > best.sim.score) {
            best = state;
        }
    }

    logger << "total dives: " << dives << endl;
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

    // state = player_bfs(map, state, timelimit, cancelled);
    // state = player_mc(map, state, timelimit, cancelled);
    state = player_rand(map, state, timelimit, cancelled);

    return state.prog;
}


} // namespace pl
}
