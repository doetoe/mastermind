// -*- eval: (google-set-c-style) -*-
#ifndef MASTERMIND_H_
#define MASTERMIND_H_

#include <string>
// #include <algorithm>
#include <vector>

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

/*
template <typename T>
struct Counter {
  using value_type = T;
  int counter = 0;
  void push_back(const T& t) {counter++;}
};

template <typename S>
int IntersectionSize(const S& s1, const S& s2) {
  Counter<typename S::value_type> counter;
  std::set_intersection(s1.begin(), s1.end(),
                        s2.begin(), s2.end(),
                        std::back_inserter(counter));
  return counter.counter;
}
*/

void partitions(int n, int k, std::vector<std::vector<int>>* result);

class MasterMind {
 private:
  std::string colors_;
  int num_positions_;
  unordered_map<char, int> color_index_;
  using ColorComb = std::vector<char>;
  std::string cc2string(const ColorComb& cc) const;
  ColorComb string2cc(const std::string& s) const;
  
  // candidates for targets that are still possible
  //std::vector<ColorComb> target_candidates_;
  std::vector<std::string> target_candidates_;
  // candidates for (high information yielding) intents.
  //std::vector<ColorComb> intent_candidates_;
  std::vector<std::string> intent_candidates_;

  // generates all possible targets in target_candidates_
  void GenerateTargetCandidates(int length=-1, const std::string& prefix="");
  
  // convert a possible evaluation (number of black/white) to an integer value
  // numbered from 0,..,N-1 where N = num_results(). Note that N-2 corresponds
  // to all but one black, and a single white, which doesn't actually occur.
  int EvaluationIndex_(int black, int white) const {
    return (black * (2 * num_positions_ + 3 - black)) / 2 + white;
  }  
  int EvaluationNumerical_(const std::string& target, const std::string& intent) const;

  // The total number of possible evaluations of an intent (counting all but one
  // black, and a single white). 
  int NumResults() const { return EvaluationIndex_(num_positions_ + 1, 0); }
  
 public:
  MasterMind(const std::string& colors, int num_positions)
      : colors_(colors),
        num_positions_(num_positions) {
    for (int i = 0; i < colors_.size(); i++)
      color_index_[colors_[i]] = i;

    GenerateTargetCandidates();
    intent_candidates_ = target_candidates_; // deep copy
  }
  
  int num_positions() const { return num_positions_; }
  const std::string& colors() const { return colors_; }

  // Colors in the same class are equivalent if they can be freely permuted
  // without changing the entropy if all known information arises from an
  // evaluation of the specified intent.
  void ColorClasses(const std::string& intent,
                    std::vector<std::string>* classes) const;

  // Positions in the same class are equivalent if they can be freely permuted
  // without changing the entropy if all known information arises from an
  // evaluation of the specified intent.
  void PositionClasses(const std::string& intent,
                       std::vector<int>* classes) const;
  
  // MasterMind(const std::string& colors, const std::string& target)
  //     : colors_(colors),
  //       target_(target) {
  //   for (int i = 0; i < colors_.size(); i++) {
  //     color_index_[colors_[i]] = i;
  //     auto color = target_color_counter.find(target_color);
  //     if (color != target_color_counter.end())
  //       *color += 1;
  //     else
  //       target_color_counter[color] = 1;
  //   }
  //   // init();
  // }

  // for a given target (hidden combination), return the number of black/white
  // for the given intent    
  pair<int,int> Evaluate(const std::string& target, const std::string& intent) const;
  
  // The events are the evaluations, 14 of them for four positions.
  // For a given intent, the space of targets is partitioned by the outcomes.
  // The entropy of that partition (event space) is returned, where all
  // targets are assumed to be equally likely.
  double Entropy(const std::string& intent) const;
  
  // For n colors, equivalence classes of starting positions correspond to
  // partitions of the number of positions in at most n summands.
  // The best partition is returned.
  std::vector<int> ChooseInitialIntent() const;

  // In the given state, return an intent of maximal entropy that actually is a
  // possible candidate
  std::string ChooseIntent() const;
  
  // Updates the candidate lists assuming the passed intent resulted in the
  // specified numbers of black and white
  // Returns the information gained.
  double Update(const std::string& intent, int black, int white);
  
  auto target_candidates_begin() const { return target_candidates_.cbegin(); }
  auto target_candidates_end() const { return target_candidates_.cend(); }

  int num_candidates() const {
    return target_candidates_end() - target_candidates_begin();
  }
  
  auto intent_candidates_begin() const { return intent_candidates_.cbegin(); }
  auto intent_candidates_end() const { return intent_candidates_.cend(); }

  // Tests
  static int test_to_from_string(const std::string& colors,
                                 const std::string& colorcomb,
                                 const std::string& colorstring);
};

#endif // MASTERMIND_H_
