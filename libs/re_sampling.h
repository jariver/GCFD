//
// Created by Administrator on 07/20/2020.
// re_sampling_tuple_pairs_global

#ifndef ALCFD_CPP_RE_SAMPLING_H
#define ALCFD_CPP_RE_SAMPLING_H

#include <utility>

#include "using_name.h"
#include "processing.h"
#include "predicates_space.h"
#include "init_sampling.h"
#include "rule.h"

class ReSampling {
public:
    ReSampling() = default;

    void build_plis(const VecStr &attributes, const VecSpRec &records);

    void backtrack(VecVecUint &res, const VecVecUint &init_combination, const VecSpRec &records, const VecUint &nums,
                   unsigned k, int first, VecUint &curr, const VecUpVecUint &rhs_pli);

    void combinations(VecVecUint &res, const VecVecUint &init_combination, const VecSpRec &records, const VecUint &nums,
                      unsigned k, const VecUpVecUint &rhs_pli);

    void re_sampling_tuple_pair_pliRecords(VecVecVecUint &lst_tuple_pair,
                                           const VecVecUint &init_combination,
                                           const VecStr &attributes,
                                           const VecSpRec &records,
                                           std::vector<Rule> &rule_repository,
                                           const PredicatesSpace &predicates_space,
                                           const std::unordered_set<std::string> &used_tuple_pairs,
                                           std::unordered_map<std::string, SpVecVecUint> &map_pred_pli);

    void re_sampling_tuple_pair_dfs(VecVecVecUint &lst_tuple_pair,
                                    const VecVecUint &init_combination,
                                    const VecStr &attributes, const VecSpRec &records,
                                    const std::vector<Rule> &rule_repository,
                                    const PredicatesSpace &predicates_space,
                                    const std::unordered_set<std::string> &used_tuple_pairs);

    void re_sampling_tuple_pair_bfs(VecVecVecUint &lst_tuple_pair,
                                    const VecVecUint &init_combination,
                                    const VecStr &attributes,
                                    const VecSpRec &records,
                                    const std::vector<Rule> &rule_repository,
                                    const PredicatesSpace &predicates_space,
                                    const std::unordered_set<std::string> &used_tuple_pairs);

    bool pass_n_second();

private:
    std::vector<Pli> plis;

    UpVecUpVecInt pliRecords;

    std::unordered_set<std::string> this_used_tuple_pairs;

    clock_t t_begin = 0;
};


#endif //ALCFD_CPP_RE_SAMPLING_H
