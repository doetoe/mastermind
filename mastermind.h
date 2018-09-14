// -*- eval: (google-set-c-style) -*-
#ifndef MASTERMIND_H_
#define MASTERMIND_H_

#include <string>
// #include <algorithm>
#include <vector>
#include <unordered_map>

/*
  Future improvements:
  - remove intents that will not give extra information
    for example remove colors that cannot be in the target, leaving one if
    empty spots are not allowed
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
std::string intersect(const std::string& s1, const std::string& s2);

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

  // Early on, we can a priori say that permuting some colors will not
  // change the information content:
  // - before the first intent, all colors are equivalent
  // - at every moment, all colors that haven't been used yet are equivalent
  // - etc.
  // This map keeps track of the color classes.
  // std::unordered_map<char, std::string> color_classes_;
  std::vector<std::string> color_class_list_;
  std::unordered_map<char, int> color_class_index_;
  const std::string& ColorClass(char color) const;

  void BuildColorClassIndex();
  
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

  // Return optimal intent candidate that is also a possible target.
  // If non of the optimal candidates is a possible target, just return
  // any of them
  std::string PickIntent(const std::vector<int>& optimal_intents) const;

 public:
  MasterMind(const std::string& colors, int num_positions)
      : colors_(colors),
        num_positions_(num_positions) {
    color_class_list_ = {colors_};
    for (int i = 0; i < colors_.size(); i++) {
      color_index_[colors_[i]] = i;
      color_class_index_[colors_[i]] = 0;
    }
    
    GenerateTargetCandidates();
    intent_candidates_ = target_candidates_; // deep copy
  }
  
  int num_positions() const { return num_positions_; }
  const std::string& colors() const { return colors_; }

  // Colors in the same class are equivalent if they can be freely permuted
  // without changing the entropy if all known information arises from an
  // evaluation of the specified intent. This function returns the equivalence
  // classes if all present information is based on the single evaluated
  // intent "intent".
  void ColorClasses(const std::string& intent,
                    std::vector<std::string>* classes) const;

  // Returns a unique representative of the passed intent
  // in such a way that intents are equivalent iff they have an equal
  // representative according to color equivalences in color_class_list_.
  std::string IntentClass(const std::string& intent) const;

  /*
  // Positions in the same class are equivalent if they can be freely permuted
  // without changing the entropy if all known information arises from an
  // evaluation of the specified intent.
  void PositionClasses(const std::string& intent,
                       std::vector<int>* classes) const;
  */
  
  // for a given target (hidden combination), return the number of black/white
  // for the given intent    
  static pair<int,int> Evaluate(
      const std::string& target, const std::string& intent);
  
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
  // possible candidate. Equivalences are used to speed this up.
  std::string Choose2ndIntent() const;
  
  // In the given state, return an intent of maximal entropy that actually is a
  // possible candidate
  std::string ChooseIntent() const;

  bool exist_equivalences() const {return color_class_list_.size() != colors_.size();}

  // Update the existing equivalence relation (the list of color classes) and
  // refine it by taking the information obtained from the new intent into
  // account, i.e. with the new intent, some colors will cease to be equivalent.
  void UpdateEquivalences(const string& intent);
  
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
