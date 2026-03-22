/*
 * ===========================================================================
 * Exercise 13: DAG (Directed Acyclic Graph) Representation  --  C++
 * ===========================================================================
 *
 * CONCEPT:
 *   A Directed Acyclic Graph (DAG) represents expressions so that identical
 *   subexpressions share a single node, enabling common subexpression
 *   elimination (CSE).  Unlike a syntax tree where every occurrence of a+b
 *   produces a separate subtree, a DAG reuses the same node whenever the
 *   same computation appears again.
 *
 * ALGORITHM:
 *   For each TAC statement  result = left op right:
 *     1. Find or create leaf nodes for `left` and `right`.
 *     2. Search existing interior nodes for a match on (op, left, right).
 *        If found, reuse it (CSE).  Otherwise, create a new interior node.
 *     3. Attach `result` as a label on the (possibly shared) node.
 *   Optimised TAC is generated via post-order traversal, emitting each
 *   node at most once.
 *
 * TIME COMPLEXITY:  O(n^2) worst case (linear scan for existing nodes),
 *                   reducible to O(n) with hashing.
 * SPACE COMPLEXITY: O(n) for DAG nodes and labels.
 *
 * EXAMPLE:
 *   Input:  t1=a+b, t2=a+b, t3=t1*t2, x=t3
 *   DAG reuses the a+b node for both t1 and t2.
 *   Optimised: t1=a+b, t3=t1*t1, x=t3
 * ===========================================================================
 */

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <cctype>

/* ---------- DAG Node ----------------------------------------------------- */

struct DAGNode {
    int                      id;
    bool                     is_leaf;
    std::string              leaf_value;   /* only for leaves           */
    std::string              op;           /* operator for interior     */
    int                      left;         /* child index, -1 if none   */
    int                      right;        /* child index, -1 if none   */
    std::vector<std::string> labels;       /* variable names on node    */

    DAGNode() : id(-1), is_leaf(true), left(-1), right(-1) {}
};

/* ---------- DAG Builder -------------------------------------------------- */

class DAGBuilder {
public:
    void build(const std::vector<std::string> &tac_lines) {
        nodes_.clear();
        name_to_node_.clear();
        next_id_ = 0;

        for (const auto &line : tac_lines) {
            std::istringstream iss(line);
            std::string result, eq, left, op, right;
            iss >> result >> eq >> left;

            if (iss >> op >> right) {
                /* Binary statement */
                int left_idx  = get_or_create_leaf(left);
                int right_idx = get_or_create_leaf(right);
                int existing  = find_existing(op, left_idx, right_idx);
                if (existing >= 0) {
                    add_label(existing, result);
                } else {
                    int idx = create_interior(op, left_idx, right_idx);
                    add_label(idx, result);
                }
            } else {
                /* Simple copy */
                int src_idx = get_or_create_leaf(left);
                add_label(src_idx, result);
            }
        }
    }

    void print_dag() const {
        for (const auto &node : nodes_) {
            if (node.is_leaf) {
                std::cout << "    n" << node.id << ": leaf  value="
                          << node.leaf_value << "  labels=[";
            } else {
                std::cout << "    n" << node.id << ": '" << node.op
                          << "'   left=n" << node.left
                          << "  right=n" << node.right << "  labels=[";
            }
            for (size_t j = 0; j < node.labels.size(); ++j) {
                if (j > 0) std::cout << ", ";
                std::cout << node.labels[j];
            }
            std::cout << "]\n";
        }
    }

    std::vector<std::string> generate_optimised() const {
        std::vector<std::string> output;
        std::unordered_set<int>  emitted;

        /* Post-order visit, emit each node once. */
        for (const auto &node : nodes_)
            if (!node.is_leaf)
                visit(node.id, emitted, output);

        /* Emit copy assignments for non-temporary aliases. */
        for (const auto &node : nodes_) {
            if (node.labels.size() > 1) {
                const std::string &primary = node.labels[0];
                for (size_t j = 1; j < node.labels.size(); ++j) {
                    if (!is_temporary(node.labels[j]))
                        output.push_back(node.labels[j] + " = " + primary);
                }
            }
        }
        return output;
    }

private:
    std::vector<DAGNode>                     nodes_;
    std::unordered_map<std::string, int>     name_to_node_;
    int                                      next_id_ = 0;

    static bool is_temporary(const std::string &name) {
        if (name.size() < 2 || name[0] != 't') return false;
        for (size_t i = 1; i < name.size(); ++i)
            if (!std::isdigit(static_cast<unsigned char>(name[i]))) return false;
        return true;
    }

    int get_or_create_leaf(const std::string &name) {
        auto it = name_to_node_.find(name);
        if (it != name_to_node_.end()) return it->second;

        DAGNode node;
        node.id = next_id_++;
        node.is_leaf = true;
        node.leaf_value = name;
        node.labels.push_back(name);
        int idx = static_cast<int>(nodes_.size());
        nodes_.push_back(node);
        name_to_node_[name] = idx;
        return idx;
    }

    int find_existing(const std::string &op, int left, int right) const {
        for (int i = 0; i < static_cast<int>(nodes_.size()); ++i) {
            const auto &n = nodes_[i];
            if (!n.is_leaf && n.op == op && n.left == left && n.right == right)
                return i;
        }
        return -1;
    }

    int create_interior(const std::string &op, int left, int right) {
        DAGNode node;
        node.id = next_id_++;
        node.is_leaf = false;
        node.op = op;
        node.left = left;
        node.right = right;
        int idx = static_cast<int>(nodes_.size());
        nodes_.push_back(node);
        return idx;
    }

    void add_label(int idx, const std::string &label) {
        nodes_[idx].labels.push_back(label);
        name_to_node_[label] = idx;
    }

    void visit(int idx, std::unordered_set<int> &emitted,
               std::vector<std::string> &output) const {
        if (emitted.count(idx)) return;
        const DAGNode &node = nodes_[idx];
        if (node.is_leaf) { emitted.insert(idx); return; }

        visit(node.left,  emitted, output);
        visit(node.right, emitted, output);
        emitted.insert(idx);

        const std::string &left_name  = nodes_[node.left].labels[0];
        const std::string &right_name = nodes_[node.right].labels[0];
        output.push_back(node.labels[0] + " = " + left_name + " " +
                         node.op + " " + right_name);
    }
};

/* ---------- driver ------------------------------------------------------- */

static void process_test_case(int case_num, const std::string &description,
                               const std::vector<std::string> &tac_lines) {
    std::cout << "  --- Test Case " << case_num << ": " << description << " ---\n";
    std::cout << "  Input Three-Address Code:\n";
    for (const auto &line : tac_lines)
        std::cout << "    " << line << "\n";
    std::cout << "\n";

    DAGBuilder builder;
    builder.build(tac_lines);

    std::cout << "  DAG Nodes:\n";
    builder.print_dag();
    std::cout << "\n";

    auto optimised = builder.generate_optimised();
    std::cout << "  Optimised Three-Address Code:\n";
    for (const auto &line : optimised)
        std::cout << "    " << line << "\n";
    std::cout << "\n";
}

int main() {
    std::cout << "=================================================================\n";
    std::cout << "   DAG (Directed Acyclic Graph) REPRESENTATION  --  C++\n";
    std::cout << "=================================================================\n\n";

    process_test_case(1, "Common subexpression a + b", {
        "t1 = a + b", "t2 = a + b", "t3 = t1 * t2", "x = t3"
    });

    process_test_case(2, "Multiple common subexpressions", {
        "t1 = a + b", "t2 = c + d", "t3 = a + b", "t4 = c + d",
        "t5 = t1 * t2", "t6 = t3 * t4", "y = t5 + t6"
    });

    process_test_case(3, "No common subexpressions", {
        "t1 = a + b", "t2 = c - d", "t3 = t1 * t2", "z = t3"
    });

    process_test_case(4, "Chained computation with reuse", {
        "t1 = a + b", "t2 = a + b", "t3 = t1 + t2",
        "t4 = t3 + t1", "w = t4"
    });

    std::cout << "=================================================================\n";
    std::cout << "  All test cases processed successfully.\n";
    std::cout << "=================================================================\n";
    return 0;
}
