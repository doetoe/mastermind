// -*- eval: (google-set-c-style) -*-

#include <cassert>
#include <cstring>
#include <cmath>
#include <iostream>
#include <set>
#include <vector>
#include <tuple>
// #include <array>
#include <algorithm>
using namespace std;

#include "mastermind.h"

/*
  Future improvements:
  - allow empty spots in guesses (i.e. a color that is not in the target)
  - identify equivalent intents
  - remove intents that will not give extra information
    for example remove colors that cannot be in the target, leaving one if
    empty spots are not allowed
  - when the best intents include possible configurations, pick those (the
    reason is that a wrong intent may give you enough information to guess
    the target, you will still have to waste an intent.
  - The greedy approach implemented here is not necessarily optimal: an
    option of maximal entropy is chosen, but the outcomes may each have
    inferior follow up entropies to a non-maximal entropy intent.
 */

// The number of partitions of n in at most k terms of size at most mx
static void partitions(int n, int k, int mx,
                       vector<vector<int>>* result, const vector<int>& prefix) {
  if (n > mx * k)
    return;
  else if (n == 0)
    result->push_back(vector<int>(prefix));
  else {
    for (int i = 1; i <= min(n, mx); i++)
    {
      vector<int> new_prefix(prefix);
      new_prefix.push_back(i);
      partitions(n - i, k - 1, i, result, new_prefix);
    }
  }
}

void partitions(int n, int k, vector<vector<int>>* result) {
  std::vector<int> prefix;
  return partitions(n, k, n, result, prefix);
}


void MasterMind::BuildColorClassIndex() {
  color_class_index_.clear();
  int i = 0;
  for (auto cls: color_class_list_) {
    for (auto color: cls) {
      color_class_index_[color] = i;
    }
    i++;
  } 
}
  
// NOTE: it would actually be more efficient if we would separately have
// a vector<string> of classes, and an unordered_map<char, int> referencing
// them, but not significantly.
void MasterMind::ColorClasses(const string& intent,
                              vector<string>* classes) const {
  classes->clear();

  vector<int> counter(colors_.size(), 0);
  for (auto color: intent)
    counter[color_index_.at(color)]++;
  int i = 0;
  for (auto count: counter) {
    if (count >= classes->size()) {
      classes->resize(count + 1);
    }
    char color = colors_[i];
    (*classes)[count].push_back(color);
    i++;
  }
  remove_if(classes->begin(), classes->end(),
            [](auto s){return s.empty();});
}

const string& MasterMind::ColorClass(char color) const {
  static string emptystring("");
  if (color_class_index_.count(color) == 1)
    return color_class_list_[color_class_index_.at(color)];
  else
    return emptystring;
}


// s1 and s2 are strings without repetitions, with their elements in
// a fixed but unknown order. Returns their intersection as a string
// without changing the order
string intersect(const string& s1, const string& s2) {
  string ret("");
  int i1 = 0, i2 = 0;
  for (auto c1: s1) {
    for (i2 = i1; i2 < s2.size(); i2++) {
      if (c1 == s2[i2]) {
        ret.push_back(c1);
        i2++;
        break;
      }
    }
  }
  return ret;
}

// Update the existing equivalence relation (the list of color classes) and
// refine it by taking the information obtained from the new intent into
// account, i.e. with the new intent, some colors will cease to be equivalent.
void MasterMind::UpdateEquivalences(const string& intent)
{
  if (!exist_equivalences()) // cannot refine further
    return;
  vector<string> new_classes;
  vector<string> intent_classes;
  ColorClasses(intent, &intent_classes);
  for (auto cls1: color_class_list_) {
    for (auto cls2: intent_classes) {
      string intersection = intersect(cls1, cls2);
      if (!intersection.empty())
      {
        new_classes.push_back(intersection);
      }
    }
  }
  swap(color_class_list_, new_classes);
  BuildColorClassIndex();
}

string MasterMind::IntentClass(const string& intent) const {
  string repr;
  vector<unordered_map<char, char>> mappings(color_class_list_.size());
  for (auto color: intent) {
    int cls = color_class_index_.at(color);
    if (mappings[cls].count(color) == 0) {
      int len = mappings[cls].size();
      // cls[mappings[cls].size()] doesn't work!
      // LHS creates new entry before RHS is executed!
      mappings[cls][color] = color_class_list_[cls][len];
    }
    repr.push_back(mappings[cls][color]);    
  }
  
  return repr;
}

std::string MasterMind::cc2string(const ColorComb& cc) const {
  string ret;
  for (auto i: cc) {
    ret.push_back(colors_[i]);
  }
  return ret;
}

MasterMind::ColorComb MasterMind::string2cc(const std::string& s) const {
  MasterMind::ColorComb ret;
  for (auto c: s) {
    ret.push_back(color_index_.at(c));
  }
  return ret;
}


void MasterMind::GenerateTargetCandidates(int length, const string& prefix) {
  if (length < 0)
    GenerateTargetCandidates(num_positions_, prefix);
  else if (length == 0)
    target_candidates_.push_back(prefix);
  else
    for (auto it = colors_.begin(); it < colors_.end(); ++it)
      GenerateTargetCandidates(length - 1, prefix + *it);
}
  
int MasterMind::EvaluationNumerical_(
    const string& target, const string& intent) const {
  int black, white;
  tie(black, white) = Evaluate(target, intent);
  return EvaluationIndex_(black, white);
}  

/*
pair<int,int> MasterMind::Evaluate(const string& target, const string& intent) const {
  int black = 0;
  multiset<char> target_colors;
  multiset<char> intent_colors;
  for (int i = 0; i < num_positions_; i++) {
    char target_color = target[i];
    char intent_color = intent[i];
    if (target_color == intent_color) {
      black++;
    } else {
      target_colors.insert(target_color);
      intent_colors.insert(intent_color);
    }
  }
  return {black, IntersectionSize(target_colors, intent_colors)};
}
*/

pair<int,int> MasterMind::Evaluate(const string& target, const string& intent) {
  int black = 0, white = 0;
  // present_colors[c] = number of c in intent - number of c in target so far 
  unordered_map<char, int> present_colors;
  for (int i = 0; i < target.size(); i++) {
    char target_color = target[i];
    char intent_color = intent[i];
    if (target_color == intent_color) {
      black++;
    } else {
      if (present_colors[intent_color] < 0)
        white++;
      present_colors[intent_color]++;
      if (present_colors[target_color] > 0)
        white++;
      present_colors[target_color]--;
    }
  }
  return {black, white};
}

// The events are the evaluations, 14 of them for four positions.
// For a given intent, the space of targets is partitioned by the outcomes.
// The entropy of that partition (event space) is returned, where all
// targets are assumed to be equally likely.
double MasterMind::Entropy(const string& intent) const {
  vector<int> counter(NumResults(), 0);
  for (auto target: target_candidates_)
    counter[EvaluationNumerical_(target, intent)]++;
  double S = 0;
  double N = target_candidates_.size();
  // The information content of an event A with probability p = p(A) is
  // i(A) = log2(1/p) = -log2(p)
  // The expected information content is called the entropy.
  auto weighted_information_content = [](double p) { return -p * log2(p); };
  for (auto count: counter) {
    double p = count / N;
    S += p != 0 ? weighted_information_content(p) : 0;
  }
  return S;
}

// If the candidate of the given index, with the specified entropy,
// improves on the maximal entropy, replace the optimal_intents
// by this one. If it is equal, it is added to the optimal_intents.
// Returns the new max_entropy.
static double updateOptimalIntents(
    int intent_index, double intent_entropy,
    double max_entropy, vector<int>* optimal_intents) {
  if (intent_entropy >= max_entropy) {
    if (intent_entropy > max_entropy) {
      optimal_intents->clear();
      max_entropy = intent_entropy;  
    }
    optimal_intents->push_back(intent_index);
  }
  return max_entropy;
}
  
// For n colors, equivalence classes of starting positions correspond to
// partitions of the number of positions in at most n summands.
// The best partition is returned.
vector<int> MasterMind::ChooseInitialIntent() const {
  vector<vector<int>> intent_classes;
  partitions(num_positions_, colors_.size(), &intent_classes);
  vector<int> optimal_intents;
  double max_entropy = -1;
  for (int i = 0; i < intent_classes.size(); ++i) {
    // create intent from partition
    string intent;
    vector<int>& intent_class = intent_classes[i];
    int j = 0;
    for (auto num: intent_class) {        
      intent += string(num, colors_[j]);
      j++;
    }

    max_entropy = updateOptimalIntents(
        i, Entropy(intent), max_entropy, &optimal_intents);

    /*
    double entropy = Entropy(intent);
    // cout << intent << ": " << entropy << endl;
    if (entropy >= max_entropy) {
      if (entropy > max_entropy) {
        optimal_intents.clear();
        max_entropy = entropy;  
      }
      optimal_intents.push_back(i);
    }
    */
  }
  
  return intent_classes[optimal_intents.front()];    
}

string MasterMind::PickIntent(const vector<int>& optimal_intents) const {
  for (auto i: optimal_intents) { 
    auto it = find(target_candidates_.begin(), target_candidates_.end(),
                   intent_candidates_[i]);
    if (it != target_candidates_.end())
      return intent_candidates_[i];
  }
  return intent_candidates_[optimal_intents.front()];
}


// Refactor this for repeated code.
string MasterMind::Choose2ndIntent() const {
  assert(!intent_candidates_.empty());
  unordered_map<string, double> cached_intents;
  vector<int> optimal_intents;
  double max_entropy = -1;
  for (int i = 0; i < intent_candidates_.size(); ++i) {
    string intent_class = IntentClass(intent_candidates_[i]);
    if (cached_intents.count(intent_class) == 0)
      cached_intents[intent_class] = Entropy(intent_class);

    max_entropy = updateOptimalIntents(
        i, cached_intents[intent_class], max_entropy, &optimal_intents);

    /*    
    double entropy = cached_intents[intent_class];
    if (entropy >= max_entropy) {
      if (entropy > max_entropy) {
        optimal_intents.clear();
        max_entropy = entropy;  
      }
      optimal_intents.push_back(i);
    }
    */
  }

  return PickIntent(optimal_intents);
}

string MasterMind::ChooseIntent() const {
  assert(!intent_candidates_.empty());
  vector<int> optimal_intents;
  double max_entropy = -1;
  for (int i = 0; i < intent_candidates_.size(); ++i) {
    string intent = intent_candidates_[i];

    max_entropy = updateOptimalIntents(
        i, Entropy(intent), max_entropy, &optimal_intents);

    /*    
    double entropy = Entropy(intent);
    if (entropy >= max_entropy) {
      if (entropy > max_entropy) {
        optimal_intents.clear();
        max_entropy = entropy;  
      }
      optimal_intents.push_back(i);
    }
    */
  }
  return PickIntent(optimal_intents);
}

double MasterMind::Update(const string& intent, int black, int white) {
  int result = EvaluationIndex_(black, white);
  decltype(target_candidates_) new_candidates;
  remove_copy_if(target_candidates_.begin(), target_candidates_.end(),
                 inserter(new_candidates, new_candidates.end()),
                 [&intent, result, this](const string& target)
                 { return EvaluationNumerical_(target, intent) != result; });
  swap(target_candidates_, new_candidates);
  UpdateEquivalences(intent);
  return log2(static_cast<double>(new_candidates.size()) / target_candidates_.size());
}

// int main_play(int argc, char *argv[]) {
int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::printf("Usage: mastermind colors positions\n");
    exit(0);
  }

  MasterMind game_assistant(argv[1], atoi(argv[2]));

  vector<int> intent_class = game_assistant.ChooseInitialIntent();
  
  cout << "You could try any string with the following grouping of colors: ";
  int last = intent_class.back();
  intent_class.pop_back();
  for (auto num: intent_class)
    cout << num << ",";
  cout << last << endl;
  
  while (true) {
    cout << "intent black white> ";
    string intent;
    int black, white;
    cin >> intent >> black >> white;
    
    printf("The entropy (expected information gain) of your intent is %.2f bits\n",
           game_assistant.Entropy(intent));
    
    double information = game_assistant.Update(intent, black, white);
    printf("You gained %.2f bits of information\n", information);

    if (game_assistant.num_candidates() == 1)
      break;
    
    printf("There are %d possible targets left\n", game_assistant.num_candidates());
    cout << "do you want a hint (y/n) ";
    char hint;
    cin >> hint;
    if (hint == 'y') {
      string proposal = game_assistant.exist_equivalences() ?
	game_assistant.Choose2ndIntent() :
	game_assistant.ChooseIntent();
      printf("You could try %s (entropy %.2f bits)\n",
	     proposal.c_str(), game_assistant.Entropy(proposal));
    }
  }

  printf("The only possibility is %s\n", game_assistant.target_candidates_begin()->c_str());
}

/////////////////////////////  TESTS  /////////////////////////////////////

int main_test_constructor(int argc, char *argv[]) {
  if (argc < 3) {
    std::printf("Usage: mastermind colors npos\n");
    exit(0);
  }

  MasterMind colors(argv[1], atoi(argv[2]));

  // printf("try %s\n", g_colors.substr(0,4).c_str());
  // char result[5];
  // scanf("%4s", result);
  // printf("%s\n", result);
  string result;
  getline(cin, result);
  printf("%s\n", result.c_str());
  return 0;
}

int main_test_evaluation(int argc, char *argv[]) {
// int main(int argc, char *argv[]) {
  if (argc > 1) {
    std::printf("Usage: mastermind\n");
    exit(0);
  }

  MasterMind colors("rgbcmyko", 4);

  // printf("try %s\n", g_colors.substr(0,4).c_str());
  // char result[5];
  // scanf("%4s", result);
  // printf("%s\n", result);
  string target;
  string intent;
  getline(cin, target);
  getline(cin, intent);
  auto bw = colors.Evaluate(target, intent);
  printf("%s with %s: black: %d, white: %d\n",
         target.c_str(), intent.c_str(), bw.first, bw.second);
  return 0;
}

int main_test_candidates(int argc, char *argv[]) {
// int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::printf("Usage: mastermind colors length\n");
    exit(0);
  }

  MasterMind colors(argv[1], atoi(argv[2]));
  for (auto it = colors.target_candidates_begin();
       it < colors.target_candidates_end(); ++it)
    printf("%s\n", it->c_str());
  return 0;
}

int main_test_color_classes(int argc, char *argv[]) {
// int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::printf("Usage: mastermind colors intent\n");
    exit(0);
  }

  MasterMind colors(argv[1], strlen(argv[2]));

  vector<string> classes;
  colors.ColorClasses(argv[2], &classes);
  for (auto cc: classes)
    printf("%s\n", cc.c_str());

  colors.UpdateEquivalences(argv[2]);
  printf("representative: %s\n", colors.IntentClass(argv[2]).c_str());
  return 0;
}

int main_test_entropy(int argc, char *argv[]) {
// int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::printf("Usage: mastermind colors intent\n");
    exit(0);
  }

  MasterMind colors(argv[1], strlen(argv[2]));
  printf("The entropy of %s when there are %lu colors is %f\n",
         argv[2], strlen(argv[1]), colors.Entropy(string(argv[2])));
  return 0;
}

int main_test_choose(int argc, char *argv[]) {
// int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::printf("Usage: mastermind colors length\n");
    exit(0);
  }

  MasterMind colors(argv[1], atoi(argv[2]));
  printf("You could try %s\n", colors.ChooseIntent().c_str());
  return 0;
}

int main_partitions(int argc, char *argv[]) {
// int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::printf("Usage: mastermind n k\n");
    exit(0);
  }

  vector<vector<int>> partition_list;
  partitions(atoi(argv[1]), atoi(argv[2]), &partition_list);

  for (auto part: partition_list) {
    for (auto elt: part)
      cout << elt << " + ";
    cout << endl;
  }    
  return 0;
}

int main_test_choose_initial(int argc, char *argv[]) {
// int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::printf("Usage: mastermind colors length\n");
    exit(0);
  }

  MasterMind colors(argv[1], atoi(argv[2]));
  vector<int> intent_class = colors.ChooseInitialIntent();
  
  cout << "You could try any string with the following grouping of colors: ";
  int last = intent_class.back();
  intent_class.pop_back();
  for (auto num: intent_class)
    cout << num << ",";
  cout << last << endl;
  return 0;
}

int main_test_update(int argc, char *argv[]) {
// int main(int argc, char *argv[]) {
  if (argc != 5) {
    std::printf("Usage: mastermind colors intent black white\n");
    exit(0);
  }

  MasterMind colors(argv[1], strlen(argv[2]));
  int black = atoi(argv[3]), white = atoi(argv[4]);
  
  printf("There are %d possible targets\n", colors.num_candidates());
  printf("The entropy (expected information gain) of your intent is %.2f bits\n",
         colors.Entropy(argv[2]));
  
  double information = colors.Update(argv[2], black, white);
  printf("There are %d possible targets left\n", colors.num_candidates());
  printf("You gained %.2f bits of information\n", information);
  for (auto it = colors.target_candidates_begin();
       it != colors.target_candidates_end(); ++it)
    cout << *it << ' ';
  cout << endl;
  return 0;
}

int MasterMind::test_to_from_string(const string& colors,
                                    const string& colorcomb_s,
                                    const string& colorstring) {
  MasterMind cc2string(colors, colorcomb_s.size());
  MasterMind string2cc(colors, colorstring.size());
  ColorComb colorcomb;
  for (auto c: colorcomb_s) {
    colorcomb.push_back(c - '0');
  }
  auto s = cc2string.cc2string(colorcomb);
  printf("%s corresponds to %s\n", colorcomb_s.c_str(), s.c_str());

  auto cc = string2cc.string2cc(colorstring);
  string cc_s;
  for (auto i: cc) {
    cc_s.push_back(i + '0');
  }
  
  printf("%s corresponds to %s\n", colorstring.c_str(), cc_s.c_str());

  return 0;
}

int main_test_to_from_string(int argc, char *argv[]) {
// int main(int argc, char *argv[]) {
  if (argc < 4) {
    std::printf("Usage: mastermind colors colornum colorstring\n");
    exit(0);
  }

  return MasterMind::test_to_from_string(argv[1], argv[2], argv[3]);
}

int main_test_intersect(int argc, char *argv[]) {
// int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::printf("Usage: mastermind string1 string2\n");
    exit(0);
  }

  printf("%s\n", intersect(argv[1], argv[2]).c_str());
  return 0;
}

