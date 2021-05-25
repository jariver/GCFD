//
// Created by Administrator on 07/20/2020.
//

#include <stack>
#include "re_sampling.h"

extern const bool filterInitialCombi;
extern const float CONFIDENCE_THRESHOLD;

// extern const unsigned TUPLE_PAIR_COUNT;
extern const int CLOCKS_PER_MSEC;
extern const bool re_pass_n_second;

VecUint find_certain_value_set(unsigned attr_id,
                               std::vector<Pli> &plis,
                               const VecSpRec &records,
                               const std::string &value) {
    VecUint ans;

    for (const UpVecUint &uptr_cluster: *plis[attr_id].partition) {
        if (records[(*uptr_cluster)[0]]->get_data(attr_id) == value) {
            for (unsigned record_id: *uptr_cluster) {
                ans.emplace_back(record_id);
            }
        }
    }
    return ans;
}

void ReSampling::backtrack(VecVecUint &res,
                           const VecVecUint &init_combination,
                           const VecSpRec &records,
                           const VecUint &nums,
                           unsigned k, int first, VecUint &curr,
                           const VecUpVecUint &rhs_pli) {
    // this is a change
    if (res.size() >= 1) return;

    if (re_pass_n_second && pass_n_second()) return;

    if (curr.size() == k) {
        unsigned left_pli_id = curr[0];
        unsigned right_pli_id = curr[1];
        unsigned left_rid = records[left_pli_id]->get_id();
        unsigned right_rid = records[right_pli_id]->get_id();
        if (left_rid > right_rid) std::swap(left_rid, right_rid);
        std::string tp_string = "L" + std::to_string(left_rid) + "R" + std::to_string(right_rid);
        if (this_used_tuple_pairs.find(tp_string) != this_used_tuple_pairs.end())
            return;

        if (filterInitialCombi) {
            for (const VecUint &combi: init_combination) {
                if (std::all_of(combi.begin(), combi.end(), [&](unsigned attr_id) {
                    VecStr left_data = records[left_pli_id]->get_data();
                    VecStr right_data = records[right_pli_id]->get_data();
                    return left_data[attr_id] == right_data[attr_id];
                })) {
                    return;
                }
            }
        }

        if (std::any_of(rhs_pli.begin(), rhs_pli.end(), [&](const UpVecUint &uptr_vec) {
            return std::find(uptr_vec->begin(), uptr_vec->end(), left_pli_id) != uptr_vec->end() &&
                   std::find(uptr_vec->begin(), uptr_vec->end(), right_pli_id) != uptr_vec->end();
        }))
            return;

        res.emplace_back(curr);
        this_used_tuple_pairs.insert(tp_string);
        return;
    }
    for (int i = first; i != nums.size(); ++i) {
        curr.emplace_back(nums[i]);
        backtrack(res, init_combination, records, nums, k, i + 1, curr, rhs_pli);
        curr.pop_back();
    }
}

void ReSampling::combinations(VecVecUint &res, const VecVecUint &init_combination, const VecSpRec &records,
                              const VecUint &nums, unsigned k, const VecUpVecUint &rhs_pli) {
    VecUint curr;
    backtrack(res, init_combination, records, nums, k, 0, curr, rhs_pli);
}

void ReSampling::re_sampling_tuple_pair_dfs(VecVecVecUint &lst_tuple_pair, const VecVecUint &init_combination,
                                            const VecStr &attributes, const VecSpRec &records,
                                            const std::vector<Rule> &rule_repository,
                                            const PredicatesSpace &predicates_space,
                                            const std::unordered_set<std::string> &used_tuple_pairs) {
    std::vector<Pli> plis;
    init_sampling::preprocess(plis, attributes, records);
    this_used_tuple_pairs = used_tuple_pairs;

    for (const Rule &rule: rule_repository) {
        // rule: CC=01, AC => CT
        std::vector<unsigned> lhs = rule.get_lhs();
        unsigned RHS = rule.get_rhs();

        // RHS_PLI
        VecUpVecUint rhs_pli;
        if (predicates_space.at(RHS).type == 0) {
            unsigned attr_id = predicates_space.at(RHS).attr_id;
            for (const UpVecUint &uptr_cluster: *plis[attr_id].partition) {
                rhs_pli.emplace_back(std::make_unique<VecUint>(*uptr_cluster));
            }
        } else {
            unsigned attr_id = predicates_space.at(RHS).attr_id;
            std::string value = predicates_space.at(RHS).value;
            rhs_pli.emplace_back(std::make_unique<VecUint>(find_certain_value_set(attr_id, plis, records, value)));
        }

        // construct LHS PLIs
        std::vector<Pli> current_plis;
        for (unsigned pred_id: lhs) {
            unsigned attr_id = predicates_space.at(pred_id).attr_id;
            if (predicates_space.at(pred_id).type == 0) {
                VecUpVecUint partition;
                for (const UpVecUint &cluster: *plis[attr_id].partition) {
                    VecUint cluster_ = *cluster;
                    partition.emplace_back(std::make_unique<VecUint>(cluster_));
                }
                current_plis.emplace_back(Pli(std::make_unique<VecUpVecUint>(std::move(partition)), attr_id));
            } else {
                std::string value = predicates_space.at(pred_id).value;
                auto cluster = std::make_unique<VecUint>(find_certain_value_set(attr_id, plis, records, value));
                VecUpVecUint partition;
                partition.emplace_back(std::move(cluster));
                Pli pli(std::make_unique<VecUpVecUint>(std::move(partition)), attr_id);
                current_plis.emplace_back(std::move(pli));
            }
        }

        // DFS for LHS
        std::stack<VecUint> myStack;
        std::stack<VecUint> indexStack;
        unsigned i = 0, j = 0;
        t_begin = clock();
        if (current_plis.size() == 1) {
            const auto &pli = current_plis[0];
            for (j = 0; j < pli.partition->size(); ++j) {
                if (re_pass_n_second && pass_n_second()) break;

                VecUint cluster = *pli.partition->at(j);
                VecVecUint ans;
                combinations(ans, init_combination, records, cluster, 2, rhs_pli);
                if (!ans.empty()) {
                    lst_tuple_pair.emplace_back(ans);
                    break;
                }
            }
        } else
            while (i < current_plis.size() && j < current_plis[i].partition->size()) {
                if (re_pass_n_second && pass_n_second()) break;

                if (myStack.empty() && i == 0) {
                    myStack.push(*current_plis[i].partition->at(j));
                    indexStack.push({i, j});
                    ++i;
                    j = 0;
                }
                VecUint now_pli = myStack.top();
                VecUint now_pli_2 = *current_plis[i].partition->at(j);
                VecUint inter;
                processing::intersection(inter, now_pli, now_pli_2);
                if (inter.size() >= 2) {
                    myStack.push(inter);
                    indexStack.push({i, j});
                    ++i;
                    if (i >= current_plis.size()) {
                        VecUint cluster = myStack.top();
                        myStack.pop();
                        indexStack.pop();
                        --i;
                        ++j;
                        VecVecUint ans;
//                        std::cout << "cluster.size(): " << cluster.size() << std::endl;
//                        std::cout << "rhs_pli.size(): " << rhs_pli.size() << std::endl;
//                        for (const auto &clu: rhs_pli) {
//                            std::cout << "\tcluster.size(): " << clu->size() << std::endl;
//                        }
                        combinations(ans, init_combination, records, cluster, 2, rhs_pli);
//                        std::cout << "combinations end." << std::endl;
                        if (!ans.empty()) {
                            for (const auto &ans_item: ans) {
                                for (unsigned id: ans_item) {
                                    std::cout << id << ' ';
                                }
                                std::cout << std::endl;
                            }
                            lst_tuple_pair.emplace_back(ans);
                            break;
                        }
                    }
                } else {
                    ++j;
                    if (j >= current_plis[i].partition->size()) {
                        i = indexStack.top()[0];
                        j = indexStack.top()[1] + 1;
                        myStack.pop();
                        indexStack.pop();
                    }
                }
            }
        std::cout << rule.get_information() << std::endl;
        std::cout << (clock() - t_begin) / CLOCKS_PER_MSEC << "ms" << std::endl;
    }
}

void ReSampling::re_sampling_tuple_pair_bfs(VecVecVecUint &lst_tuple_pair,
                                            const VecVecUint &init_combination,
                                            const VecStr &attributes,
                                            const VecSpRec &records,
                                            const std::vector<Rule> &rule_repository,
                                            const PredicatesSpace &predicates_space,
                                            const std::unordered_set<std::string> &used_tuple_pairs) {
    std::vector<Pli> plis;
    init_sampling::preprocess(plis, attributes, records);
    // pred_id in lhs: 1, 3, 5, 137
    // key: 137005003001    1*10^(3*0) + 3*10^(3*1) + 5*10^(3*2) + 137*10^(3*3)
    // key: 5003001
    // key: 3001
    // key: 1
    std::unordered_map<unsigned, std::shared_ptr<VecUpVecUint>> key_pli;
    this_used_tuple_pairs = used_tuple_pairs;

    for (const Rule &rule: rule_repository) {
        // rule: CC=01, AC => CT
        std::vector<unsigned> lhs = rule.get_lhs();
        std::sort(lhs.begin(), lhs.end(), [&](unsigned id_1, unsigned id_2) {
            return id_1 < id_2;
        });
        unsigned i = 0, key = 0;
        std::stack<unsigned> stack_keys;
        unsigned RHS = rule.get_rhs();
        std::shared_ptr<VecUpVecUint> pli = std::make_shared<VecUpVecUint>();
        for (unsigned j = 0; j != lhs.size(); ++j) {
            unsigned pred_id = lhs[j];
            key += pred_id * static_cast<unsigned >(pow(10, 3 * j));
            stack_keys.push(key);
        }
        i = lhs.size();
        while (!stack_keys.empty()) {
            key = stack_keys.top();
            if (key_pli.find(key) != key_pli.end()) {
                pli = key_pli.find(key)->second;
                break;
            } else {
                stack_keys.pop();
                key = 0;
                i--;
            }
        }

        clock_t t1 = clock();
        for (; i != lhs.size(); ++i) {
            unsigned pred_id = lhs[i];
            key += pred_id * static_cast<unsigned >(pow(10, 3 * i));
            if (predicates_space.at(pred_id).type == 0) {
                unsigned attr_id = predicates_space.at(pred_id).attr_id;
                if (i == 0) {
                    for (const UpVecUint &uptr_cluster: *plis[attr_id].partition) {
                        pli->emplace_back(std::make_unique<VecUint>(*uptr_cluster));
                        key_pli[key] = pli;
                    }
                } else {
                    std::shared_ptr<VecUpVecUint> now_pli = std::make_shared<VecUpVecUint>();
                    for (const UpVecUint &uptr_cluster: *plis[attr_id].partition) {
                        now_pli->emplace_back(std::make_unique<VecUint>(*uptr_cluster));
                    }

                    std::shared_ptr<VecUpVecUint> ans_set = std::make_shared<VecUpVecUint>();
                    for (const auto &cluster: *pli) {
                        for (const auto &cluster_: *now_pli) {
                            VecUint ans;
                            processing::intersection(ans, *cluster, *cluster_);
                            if (ans.size() >= 2)
                                ans_set->emplace_back(std::make_unique<VecUint>(ans));
                        }
                    }
                    pli = ans_set;
                    key_pli[key] = pli;
                }
            }
                // CC=01
            else {
                std::string value = predicates_space.at(pred_id).value;
                if (i == 0) {
                    pli->emplace_back(std::make_unique<VecUint>(
                            find_certain_value_set(predicates_space.at(pred_id).attr_id, plis, records, value)));
                    key_pli[key] = pli;
                } else {
                    std::shared_ptr<VecUpVecUint> now_pli = std::make_shared<VecUpVecUint>();
                    now_pli->emplace_back(std::make_unique<VecUint>(
                            find_certain_value_set(predicates_space.at(pred_id).attr_id, plis, records, value)));

                    std::shared_ptr<VecUpVecUint> ans_set = std::make_shared<VecUpVecUint>();
                    for (const auto &cluster: *pli) {
                        for (const auto &cluster_: *now_pli) {
                            VecUint ans;
                            processing::intersection(ans, *cluster, *cluster_);
                            if (ans.size() >= 2)
                                ans_set->emplace_back(std::make_unique<VecUint>(ans));
                        }
                    }
                    pli = ans_set;
                    key_pli[key] = pli;
                }
            }
        }
        clock_t t2 = clock();
        std::cout << (t2 - t1) / (CLOCKS_PER_SEC / 1000) << "ms" << std::endl;

        // LHS PLI {{0, 1, 2, 3}, {4, 5, 6}, {7, 8}}
        // RHS PLI {{0, 2, 4}, {5, 6}, {7, 8}}
        // ANS {{0, 1}, {0, 3}, {1, 2}, {1, 3}, {2, 3}}
        VecUpVecUint rhs_pli;
        if (predicates_space.at(RHS).type == 0) {
            unsigned attr_id = predicates_space.at(RHS).attr_id;
            for (const UpVecUint &uptr_cluster: *plis[attr_id].partition) {
                rhs_pli.emplace_back(std::make_unique<VecUint>(*uptr_cluster));
            }
        } else {
            unsigned attr_id = predicates_space.at(RHS).attr_id;
            std::string value = predicates_space.at(RHS).value;
            rhs_pli.emplace_back(std::make_unique<VecUint>(find_certain_value_set(attr_id, plis, records, value)));
        }
        for (const auto &cluster: *pli) {
            VecVecUint ans;
            combinations(ans, init_combination, records, *cluster, 2, rhs_pli);
            // std::cout << this_used_tuple_pairs.size() << std::endl;
            // std::cout << "ok" << std::endl;
            if (!ans.empty())
                lst_tuple_pair.emplace_back(ans);
        }
        clock_t t3 = clock();
        std::cout << (t3 - t2) / CLOCKS_PER_MSEC << "ms" << std::endl << std::endl;
    }
}

bool ReSampling::pass_n_second() {
    int n = 2000;
    return (clock() - t_begin) / CLOCKS_PER_MSEC >= n;
}

void ReSampling::re_sampling_tuple_pair_pliRecords(VecVecVecUint &lst_tuple_pair, const VecVecUint &init_combination,
                                                   const VecStr &attributes,
                                                   const VecSpRec &records, std::vector<Rule> &rule_repository,
                                                   const PredicatesSpace &predicates_space,
                                                   const std::unordered_set<std::string> &used_tuple_pairs,
                                                   std::unordered_map<std::string, SpVecVecUint> &map_pred_pli) {
    this_used_tuple_pairs = used_tuple_pairs;

    for (Rule &rule: rule_repository) {
        if (rule.cant_sample() || rule.get_whole_conf() >= CONFIDENCE_THRESHOLD) continue;
//        std::cout << rule.get_information() << ": ";
        // rule: CC=01, AC => CT
        bool cant_sample = true;
        int violation = 0, support = 0;
        std::vector<unsigned> lhs = rule.get_lhs();
        unsigned RHS = rule.get_rhs();

        clock_t t1 = clock();
        // BEGIN-1
        std::sort(lhs.begin(), lhs.end(), [&](unsigned lhs_pred_id, unsigned rhs_pred_id) {
            return lhs_pred_id < rhs_pred_id;
        });
        SpVecVecUint now_lst_tuple_pair = std::make_shared<VecVecUint>();
        SpVecVecUint last_lst_tuple_pair = std::make_shared<VecVecUint>();
        unsigned ii = 0;
        std::string key;
        for (unsigned i = 0; i != lhs.size(); i++) {
            key += processing::emplace_front(std::to_string(lhs[i]), 3, '0');
            if (map_pred_pli.find(key) != map_pred_pli.end()) {
                last_lst_tuple_pair = map_pred_pli[key];
                ii = i + 1;
            }
        }
        key = "";
        for (unsigned i = 0; i != ii; ++i) {
            key += processing::emplace_front(std::to_string(lhs[i]), 3, '0');
        }
        // END-1
        for (unsigned i = ii; i != lhs.size(); ++i) {
            unsigned pred_id = lhs[i];
            unsigned attr_id = predicates_space.at(pred_id).attr_id;

            if (predicates_space.at(pred_id).type == 0) {
                if (i == 0) {
                    for (const UpVecUint &vec: *plis[attr_id].partition) {
                        last_lst_tuple_pair->emplace_back(*vec);
                    }
                    // BEGIN-2
                    key += processing::emplace_front(std::to_string(lhs[i]), 3, '0');
                    map_pred_pli[key] = last_lst_tuple_pair;
                    // END-2
                } else {
                    now_lst_tuple_pair = std::make_shared<VecVecUint>();
                    for (const VecUint &now_vec: *last_lst_tuple_pair) {
                        std::unordered_map<unsigned, VecUint> new_map;
                        for (unsigned record_id: now_vec) {
                            int pli_id = (*(*pliRecords)[record_id])[attr_id];
                            if (pli_id != -1) {
                                new_map[pli_id].emplace_back(record_id);
                            }
                        }
                        for (const auto &pair: new_map) {
                            if (pair.second.size() >= 2)
                                now_lst_tuple_pair->emplace_back(pair.second);
                        }
                    }
                    last_lst_tuple_pair = now_lst_tuple_pair;
                    // BEGIN-3
                    key += processing::emplace_front(std::to_string(lhs[i]), 3, '0');
                    map_pred_pli[key] = last_lst_tuple_pair;
                    // END-3
                }
            }
                // CC = 01
            else {
                std::string value = predicates_space.at(pred_id).value;
                if (i == 0) {
                    last_lst_tuple_pair->emplace_back(find_certain_value_set(attr_id, plis, records, value));
                    // BEGIN-4
                    key += processing::emplace_front(std::to_string(lhs[i]), 3, '0');
                    map_pred_pli[key] = last_lst_tuple_pair;
                    // END-4
                } else {
                    now_lst_tuple_pair = std::make_shared<VecVecUint>();
                    for (const VecUint &now_vec: *last_lst_tuple_pair) {
                        std::unordered_map<unsigned, VecUint> new_map;
                        for (unsigned record_id: now_vec) {
                            int pli_id = (*(*pliRecords)[record_id])[attr_id];
                            if (pli_id != -1 && records[record_id]->at(attr_id) == value) {
                                new_map[pli_id].emplace_back(record_id);
                            }
                        }
                        for (const auto &pair: new_map) {
                            if (pair.second.size() >= 2)
                                now_lst_tuple_pair->emplace_back(pair.second);
                        }
                    }
                    last_lst_tuple_pair = now_lst_tuple_pair;
                    // BEGIN-5
                    key += processing::emplace_front(std::to_string(lhs[i]), 3, '0');
                    map_pred_pli[key] = last_lst_tuple_pair;
                    // END-5
                }
            }
        }
        now_lst_tuple_pair = last_lst_tuple_pair;

//        clock_t t2 = clock();
//        std::cout << "LHS: " << (t2 - t1) / (CLOCKS_PER_SEC / 1000) << "ms" << ", ";

        auto attr_id = predicates_space.at(RHS).attr_id;
        auto value = predicates_space.at(RHS).value;

        // this can be used to compute whole_support and whole_violaiton;
        int rule_sampling_count = 0;
        for (const VecUint &cluster: *now_lst_tuple_pair) {
            VecVecUint ans;
            std::unordered_map<unsigned, VecUint> new_map;
            for (unsigned record_id: cluster) {
                int pli_id = (*(*pliRecords)[record_id])[attr_id];
                new_map[pli_id].emplace_back(record_id);
            }
            if (new_map.size() > 1) {
                VecVecUint now_pli;
                now_pli.reserve(1024);
                for (const auto &pair: new_map) {
                    now_pli.emplace_back(pair.second);
                }
                int cluster_sampling_cout = 0;
                for (unsigned i = 0; i != now_pli.size(); ++i) {
                    if (cluster_sampling_cout >= 3) break;
                    for (unsigned j = i + 1; j != now_pli.size(); j++) {
                        if (cluster_sampling_cout++ >= 3) break;
                        VecUint curr;
                        curr.emplace_back(now_pli[i][0]);
                        curr.emplace_back(now_pli[j][0]);
                        unsigned left_pli_id = curr[0];
                        unsigned right_pli_id = curr[1];
                        unsigned left_rid = records[left_pli_id]->get_id();
                        unsigned right_rid = records[right_pli_id]->get_id();
                        if (left_rid > right_rid) std::swap(left_rid, right_rid);
                        std::string tp_string = "L" + std::to_string(left_rid) + "R" + std::to_string(right_rid);
                        if (this_used_tuple_pairs.find(tp_string) != this_used_tuple_pairs.end()) break;

                        bool is_init_combi = false;
                        if (filterInitialCombi) {
                            for (const VecUint &combi: init_combination) {
                                if (std::all_of(combi.begin(), combi.end(), [&](unsigned attr_id) {
                                    VecStr left_data = records[left_pli_id]->get_data();
                                    VecStr right_data = records[right_pli_id]->get_data();
                                    return left_data[attr_id] == right_data[attr_id];
                                })) {
                                    is_init_combi = true;
                                    break;
                                }
                            }
                        }
                        if (is_init_combi) break;

                        ans.emplace_back(curr);
                        this_used_tuple_pairs.insert(tp_string);
                        cant_sample = false;
                    }
                }
            }
            if (!ans.empty()) {
                if (rule_sampling_count++ >= 10) break;
                lst_tuple_pair.emplace_back(ans);
            }
            if (rule.get_whole_violation() == -1) {
                int maxSize = 0;
                for (const auto &pair: new_map) {
                    if (maxSize < pair.second.size()) maxSize = pair.second.size();
                }
                violation += static_cast<int>(cluster.size()) - maxSize;
            }
        }
        if (rule.get_whole_violation() == -1)
            rule.set_whole_violation(violation);

        if (rule.get_whole_supp() == -1) {
            for (const auto &vec: *now_lst_tuple_pair) {
                support += vec.size();
            }
            rule.set_whole_supp(support);
            rule.set_whole_conf(1 - (violation + 0.0) / support);
        }
        rule.set_cant_samle(cant_sample);

//        clock_t t3 = clock();
//        std::cout << "RHS: " << (t3 - t2) / (CLOCKS_PER_SEC / 1000) << "ms" << ", ";
    }
}

void ReSampling::build_plis(const VecStr &attributes, const VecSpRec &records) {
    init_sampling::preprocess(plis, attributes, records);
    init_sampling::create_pliRecords(pliRecords, plis, attributes, records);
}
