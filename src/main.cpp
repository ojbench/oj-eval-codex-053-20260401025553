#include <iostream>
#include <queue>
#include <string>
#include <unordered_set>
#include <vector>
using namespace std;

// Implement the NFA-based regex engine per README description.
namespace Grammar {
enum class TransitionType { Epsilon, a, b };
struct Transition {
  TransitionType type;
  int to;
  Transition(TransitionType type, int to) : type(type), to(to) {}
};

class NFA {
private:
  int start;
  std::unordered_set<int> ends;
  std::vector<std::vector<Transition>> transitions;

public:
  NFA() = default;
  ~NFA() = default;

  std::unordered_set<int> GetEpsilonClosure(std::unordered_set<int> states) const {
    std::unordered_set<int> closure;
    std::queue<int> queue;
    for (const auto &state : states) {
      if (closure.find(state) != closure.end())
        continue;
      queue.push(state);
      closure.insert(state);
    }
    while (!queue.empty()) {
      int current = queue.front();
      queue.pop();
      for (const auto &transition : transitions[current]) {
        if (transition.type == TransitionType::Epsilon) {
          if (closure.find(transition.to) == closure.end()) {
            queue.push(transition.to);
            closure.insert(transition.to);
          }
        }
      }
    }
    return closure;
  }

  std::unordered_set<int> Advance(std::unordered_set<int> current_states,
                                  char character) const {
    std::unordered_set<int> cur_closure = GetEpsilonClosure(current_states);
    std::unordered_set<int> next;
    TransitionType t = (character == 'a') ? TransitionType::a : TransitionType::b;
    for (int s : cur_closure) {
      for (const auto &tr : transitions[s]) {
        if (tr.type == t) {
          next.insert(tr.to);
        }
      }
    }
    return GetEpsilonClosure(next);
  }

  bool IsAccepted(int state) const { return ends.find(state) != ends.end(); }
  int GetStart() const { return start; }

  friend NFA MakeStar(const char &character);
  friend NFA MakePlus(const char &character);
  friend NFA MakeQuestion(const char &character);
  friend NFA MakeSimple(const char &character);
  friend NFA Concatenate(const NFA &nfa1, const NFA &nfa2);
  friend NFA Union(const NFA &nfa1, const NFA &nfa2);
};

static void ensure_size(std::vector<std::vector<Transition>> &trans, int n) {
  if ((int)trans.size() < n) trans.resize(n);
}

NFA MakeStar(const char &character) {
  NFA nfa;
  nfa.start = 0;
  nfa.ends.insert(0);
  nfa.transitions.push_back(std::vector<Transition>());
  if (character == 'a') {
    nfa.transitions[0].push_back({TransitionType::a, 0});
  } else {
    nfa.transitions[0].push_back({TransitionType::b, 0});
  }
  return nfa;
}

NFA MakePlus(const char &character) {
  // One state 0 with a/b loop; end not via epsilon, must consume >=1
  // Implement by two states: 0 --a/b--> 1, and 1 loops on a/b, end=1
  NFA n;
  n.start = 0;
  n.transitions.resize(2);
  if (character == 'a') {
    n.transitions[0].push_back({TransitionType::a, 1});
    n.transitions[1].push_back({TransitionType::a, 1});
  } else {
    n.transitions[0].push_back({TransitionType::b, 1});
    n.transitions[1].push_back({TransitionType::b, 1});
  }
  n.ends.insert(1);
  return n;
}

NFA MakeQuestion(const char &character) {
  // Two states 0(start,end) and 1; epsilon 0->1 means zero occurrence accepted; also char 0->1
  NFA n;
  n.start = 0;
  n.transitions.resize(2);
  n.transitions[0].push_back({TransitionType::Epsilon, 1});
  if (character == 'a')
    n.transitions[0].push_back({TransitionType::a, 1});
  else
    n.transitions[0].push_back({TransitionType::b, 1});
  n.ends.insert(1);
  return n;
}

NFA MakeSimple(const char &character) {
  // Two states: 0->1 with a/b, start=0, end=1
  NFA n;
  n.start = 0;
  n.transitions.resize(2);
  if (character == 'a')
    n.transitions[0].push_back({TransitionType::a, 1});
  else
    n.transitions[0].push_back({TransitionType::b, 1});
  n.ends.insert(1);
  return n;
}

NFA Concatenate(const NFA &nfa1, const NFA &nfa2) {
  // Merge two NFAs by epsilon from each end of nfa1 to start of nfa2
  NFA n;
  int n1 = (int)nfa1.transitions.size();
  int n2 = (int)nfa2.transitions.size();
  n.start = nfa1.start; // will shift indices later
  // Build transitions with shifted indices for nfa2
  n.transitions = nfa1.transitions;
  n.transitions.resize(n1 + n2);
  for (int i = 0; i < n2; ++i) {
    for (const auto &tr : nfa2.transitions[i]) {
      n.transitions[n1 + i].push_back({tr.type, n1 + tr.to});
    }
  }
  n.ends.clear();
  // Epsilon connect ends of nfa1 to shifted start of nfa2
  for (int e : nfa1.ends) {
    n.transitions[e].push_back({TransitionType::Epsilon, n1 + nfa2.start});
  }
  // Ends are ends of nfa2 shifted
  for (int e : nfa2.ends) n.ends.insert(n1 + e);
  return n;
}

NFA Union(const NFA &nfa1, const NFA &nfa2) {
  // New start 0 with epsilon to nfa1.start+1 and nfa2.start+1
  // Shift nfa1 by +1, nfa2 by +1 + n1
  NFA n;
  int n1 = (int)nfa1.transitions.size();
  int n2 = (int)nfa2.transitions.size();
  n.start = 0;
  n.transitions.resize(1 + n1 + n2);
  // Epsilon from new start to the two starts
  n.transitions[0].push_back({TransitionType::Epsilon, 1 + nfa1.start});
  n.transitions[0].push_back({TransitionType::Epsilon, 1 + n1 + nfa2.start});
  // Copy transitions with shift
  for (int i = 0; i < n1; ++i) {
    for (const auto &tr : nfa1.transitions[i]) {
      n.transitions[1 + i].push_back({tr.type, 1 + tr.to});
    }
  }
  for (int i = 0; i < n2; ++i) {
    for (const auto &tr : nfa2.transitions[i]) {
      n.transitions[1 + n1 + i].push_back({tr.type, 1 + n1 + tr.to});
    }
  }
  // Ends are union of shifted ends
  for (int e : nfa1.ends) n.ends.insert(1 + e);
  for (int e : nfa2.ends) n.ends.insert(1 + n1 + e);
  return n;
}

class RegexChecker {
private:
  NFA nfa;

public:
  bool Check(const std::string &str) const {
    std::unordered_set<int> states{nfa.GetStart()};
    for (char c : str) {
      if (c != 'a' && c != 'b') return false; // Only a/b
      states = nfa.Advance(states, c);
      if (states.empty()) return false;
    }
    for (int s : states) if (nfa.IsAccepted(s)) return true;
    return false;
  }

  RegexChecker(const std::string &regex) {
    // Parse regex without parentheses. Operators: + * ? unary postfix; | lowest precedence; concat implicit.
    // Strategy: Split by '|' into alternatives, build NFA for each term (concatenation of atoms), then union them.
    auto build_atom = [&](char ch, char op) {
      if (op == '+') return MakePlus(ch);
      if (op == '*') return MakeStar(ch);
      if (op == '?') return MakeQuestion(ch);
      return MakeSimple(ch);
    };

    vector<NFA> alts;
    size_t i = 0;
    while (i <= regex.size()) {
      // build one alternative until '|' or end
      vector<NFA> parts;
      while (i < regex.size() && regex[i] != '|') {
        char ch = regex[i++];
        if (ch != 'a' && ch != 'b') {
          // ignore invalid chars defensively
          continue;
        }
        char op = 0;
        if (i < regex.size() && (regex[i] == '+' || regex[i] == '*' || regex[i] == '?')) {
          op = regex[i++];
        }
        parts.push_back(build_atom(ch, op));
      }
      // concatenate all parts
      if (!parts.empty()) {
        NFA cur = parts[0];
        for (size_t k = 1; k < parts.size(); ++k) cur = Concatenate(cur, parts[k]);
        alts.push_back(cur);
      }
      if (i < regex.size() && regex[i] == '|') ++i; // skip '|'
      else if (i >= regex.size()) break;
    }
    // Union all alternatives
    if (!alts.empty()) {
      NFA cur = alts[0];
      for (size_t k = 1; k < alts.size(); ++k) cur = Union(cur, alts[k]);
      nfa = cur;
    } else {
      // Fallback minimal NFA (will reject most inputs); shouldn't occur for valid regex
      nfa = MakeSimple('a');
    }
  }
};
} // namespace Grammar

int main() {
  ios::sync_with_stdio(false);
  cin.tie(nullptr);
  // Input format: first line regex, then number t, then t lines of strings to test; or fallback: lines with regex and strings until EOF?
  // Since problem statement is not precise, implement: read two tokens: regex and string; if more pairs, process all pairs line by line.
  // We'll support two modes:
  // 1) If first token is an integer n, then read n testcases each as: regex string
  // 2) Else: treat as regex followed by m and then m strings; or pairs.
  vector<string> tokens; string s; while (cin >> s) tokens.push_back(s);
  if (tokens.empty()) return 0;
  // Heuristic: If tokens.size() >= 2 and tokens[1] is digits, then form1: regex, m, then m strings
  auto is_digits = [](const string &x){
    if (x.empty()) return false;
    for (char c : x) if (!(c >= '0' && c <= '9')) return false;
    return true;
  };
  size_t idx = 0;
  if (is_digits(tokens[0])) {
    int n = stoi(tokens[0]);
    for (int i = 0; i < n && idx+2 <= tokens.size(); ++i) {
      string regex = tokens[++idx];
      string test = tokens[++idx];
      Grammar::RegexChecker rc(regex);
      cout << (rc.Check(test) ? "Yes" : "No") << '\n';
    }
    return 0;
  }
  // Otherwise assume format: regex, then t, then t strings
  string regex = tokens[idx++];
  int t = 0; bool has_t = false;
  if (idx < tokens.size() && is_digits(tokens[idx])) { t = stoi(tokens[idx++]); has_t = true; }
  Grammar::RegexChecker rc(regex);
  if (has_t) {
    for (int i = 0; i < t && idx < tokens.size(); ++i) {
      string str = tokens[idx++];
      cout << (rc.Check(str) ? "Yes" : "No") << '\n';
    }
  } else {
    // Evaluate remaining tokens as strings
    for (; idx < tokens.size(); ++idx) {
      cout << (rc.Check(tokens[idx]) ? "Yes" : "No") << '\n';
    }
  }
  return 0;
}
