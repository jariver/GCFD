//
// Created by Administrator on 9/14/20.
//

#include "CFDMeasure.h"

extern const int CLOCKS_PER_MSEC;

VecUint find_certain_value_set_(unsigned attr_id,
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

UpVecUpVecUpStr CFDMeasure::transpose(const VecSpRec &records) {
    UpVecUpVecUpStr ret = std::make_unique<VecUpVecUpStr>();
    for (size_t j = 0; j != records[0]->size(); ++j) {
        UpVecUpStr col = std::make_unique<VecUpStr>();
        for (size_t i = 0; i != records.size(); ++i) {
            UpStr tmp_str(new std::string(records.at(i)->at(j)));
            col->emplace_back(std::move(tmp_str));
        }
        ret->emplace_back(std::move(col));
    }
    return ret;
}

UpVecUpVecUint CFDMeasure::build_pli(UpVecUpStr lst) {
    UpVecUpVecUint list_pli = std::make_unique<VecUpVecUint>();
    std::unordered_map<std::string, std::set<unsigned>> hashes;
    hashes.reserve(1024);
    for (unsigned i = 0; i != lst->size(); ++i) {
        hashes[*(*lst)[i]].insert(i);
    }
    for (const auto &item : hashes) {
        if (item.second.size() >= 2) {
            auto cluster = std::make_unique<VecUint>(item.second.cbegin(), item.second.cend());
            list_pli->emplace_back(std::move(cluster));
        }
    }
    return list_pli;
}

void CFDMeasure::preprocess(std::vector<Pli> &plis,
                            const VecStr &attributes,
                            const VecSpRec &records) {
    clock_t t0 = clock();
    auto trans_records = transpose(records);
    clock_t t1 = clock();
    std::cout << "transpose cost: " << (t1 - t0) / CLOCKS_PER_MSEC << "ms" << std::endl;

    for (unsigned i = 0; i != trans_records->size(); ++i) {
        plis.emplace_back(std::move(Pli(std::move(build_pli(std::move(trans_records->at(i)))), i)));
    }
    std::cout << "build plis cost: " << (clock() - t1) / CLOCKS_PER_MSEC << "ms" << std::endl;
}

void CFDMeasure::create_pliRecords(UpVecUpVecInt &pliRecords,
                                   const std::vector<Pli> &plis,
                                   const VecStr &attributes,
                                   const VecSpRec &records) {
    pliRecords = std::make_unique<VecUpVecInt>();
    for (unsigned i = 0; i != records.size(); ++i) {
        pliRecords->emplace_back(std::make_unique<VecInt>(attributes.size(), -1));
    }

    for (const Pli &pli: plis) {
        for (unsigned i = 0; i != pli.partition->size(); ++i) {
            for (unsigned record_id: *(*pli.partition)[i]) {
                (*(*pliRecords)[record_id])[pli.attr] = i;
            }
        }
    }
}

unsigned CFDMeasure::compute_support(const VecStr &attributes,
                                     const VecSpRec &records,
                                     const Rule &rule,
                                     const PredicatesSpace &predicates_space) {

    unsigned support = 0;
    unsigned support_2 = 0;

    std::vector<Pli> plis;
    CFDMeasure::preprocess(plis, attributes, records);
    UpVecUpVecInt pliRecords;
    CFDMeasure::create_pliRecords(pliRecords, plis, attributes, records);

    clock_t t0 = clock();
    auto lhs = rule.get_lhs();
    auto rhs = rule.get_rhs();

    VecVecUint now_lst_tuple_pair;
    VecVecUint last_lst_tuple_pair;
    for (int i = 0; i != lhs.size(); ++i) {
        unsigned pred_id = lhs[i];
        unsigned attr_id = predicates_space.at(pred_id).attr_id;

        if (predicates_space.at(pred_id).type == 0) {
            if (i == 0) {
                for (const UpVecUint &vec: *plis[attr_id].partition) {
                    last_lst_tuple_pair.emplace_back(*vec);
                }
            } else {
                now_lst_tuple_pair.clear();
                for (const VecUint &now_vec: last_lst_tuple_pair) {
                    std::unordered_map<unsigned, VecUint> new_map;
                    for (unsigned record_id: now_vec) {
                        int pli_id = (*(*pliRecords)[record_id])[attr_id];
                        if (pli_id != -1) {
                            new_map[pli_id].emplace_back(record_id);
                        }
                    }
                    for (const auto &pair: new_map) {
                        if (pair.second.size() >= 2)
                            now_lst_tuple_pair.emplace_back(pair.second);
                    }
                }
                last_lst_tuple_pair = now_lst_tuple_pair;
            }
        }
            // CC = 01
        else {
            std::string value = predicates_space.at(pred_id).value;
            if (i == 0) {
                last_lst_tuple_pair.emplace_back(find_certain_value_set_(attr_id, plis, records, value));
            } else {
                now_lst_tuple_pair.clear();
                for (const VecUint &now_vec: last_lst_tuple_pair) {
                    std::unordered_map<unsigned, VecUint> new_map;
                    for (unsigned record_id: now_vec) {
                        int pli_id = (*(*pliRecords)[record_id])[attr_id];
                        if (pli_id != -1 && records[record_id]->at(attr_id) == value) {
                            new_map[pli_id].emplace_back(record_id);
                        }
                    }
                    for (const auto &pair: new_map) {
                        if (pair.second.size() >= 2)
                            now_lst_tuple_pair.emplace_back(pair.second);
                    }
                }
                last_lst_tuple_pair = now_lst_tuple_pair;
            }
        }
    }
    now_lst_tuple_pair = last_lst_tuple_pair;

    unsigned attr_id = predicates_space.at(rhs).attr_id;
    std::string value = predicates_space.at(rhs).value;

    if (predicates_space.at(rhs).type == 0) {
        for (const auto &vec: now_lst_tuple_pair) {
            support_2 += vec.size();
            support += (vec.size() * (vec.size() - 1)) / 2;
        }
    } else if (predicates_space.at(rhs).type == 1) {
        now_lst_tuple_pair.clear();
        for (const VecUint &now_vec: last_lst_tuple_pair) {
            std::unordered_map<unsigned, VecUint> new_map;
            for (unsigned record_id: now_vec) {
                int pli_id = (*(*pliRecords)[record_id])[attr_id];
                if (pli_id != -1 && records[record_id]->at(attr_id) == value) {
                    new_map[pli_id].emplace_back(record_id);
                }
            }
            for (const auto &pair: new_map) {
                if (pair.second.size() >= 2)
                    now_lst_tuple_pair.emplace_back(pair.second);
            }
        }
        last_lst_tuple_pair = now_lst_tuple_pair;

        for (const auto &vec: now_lst_tuple_pair) {
            support_2 += vec.size();
            support += (vec.size() * (vec.size() - 1)) / 2;
        }
    }

    clock_t t1 = clock();
    std::cout << (t1 - t0) / CLOCKS_PER_MSEC << "ms." << std::endl;
    std::cout << "hold(#tuples): " << support_2;

    return support;
}

unsigned CFDMeasure::compute_violation(const VecStr &attributes, const VecSpRec &records, const Rule &rule,
                                       const PredicatesSpace &predicates_space) {
    int violation = 0;

    std::vector<Pli> plis;
    CFDMeasure::preprocess(plis, attributes, records);
    UpVecUpVecInt pliRecords;
    CFDMeasure::create_pliRecords(pliRecords, plis, attributes, records);

    auto lhs = rule.get_lhs();
    auto rhs = rule.get_rhs();

    VecVecUint now_lst_tuple_pair;
    VecVecUint last_lst_tuple_pair;
    for (int i = 0; i != lhs.size(); ++i) {
        unsigned pred_id = lhs[i];
        unsigned attr_id = predicates_space.at(pred_id).attr_id;

        if (predicates_space.at(pred_id).type == 0) {
            if (i == 0) {
                for (const UpVecUint &vec: *plis[attr_id].partition) {
                    last_lst_tuple_pair.emplace_back(*vec);
                }
            } else {
                now_lst_tuple_pair.clear();
                for (const VecUint &now_vec: last_lst_tuple_pair) {
                    std::unordered_map<unsigned, VecUint> new_map;
                    for (unsigned record_id: now_vec) {
                        int pli_id = (*(*pliRecords)[record_id])[attr_id];
                        if (pli_id != -1) {
                            new_map[pli_id].emplace_back(record_id);
                        }
                    }
                    for (const auto &pair: new_map) {
                        if (pair.second.size() >= 2)
                            now_lst_tuple_pair.emplace_back(pair.second);
                    }
                }
                last_lst_tuple_pair = now_lst_tuple_pair;
            }
        }
            // CC = 01
        else {
            std::string value = predicates_space.at(pred_id).value;
            if (i == 0) {
                last_lst_tuple_pair.emplace_back(find_certain_value_set_(attr_id, plis, records, value));
            } else {
                now_lst_tuple_pair.clear();
                for (const VecUint &now_vec: last_lst_tuple_pair) {
                    std::unordered_map<unsigned, VecUint> new_map;
                    for (unsigned record_id: now_vec) {
                        int pli_id = (*(*pliRecords)[record_id])[attr_id];
                        if (pli_id != -1 && records[record_id]->at(attr_id) == value) {
                            new_map[pli_id].emplace_back(record_id);
                        }
                    }
                    for (const auto &pair: new_map) {
                        if (pair.second.size() >= 2)
                            now_lst_tuple_pair.emplace_back(pair.second);
                    }
                }
                last_lst_tuple_pair = now_lst_tuple_pair;
            }
        }
    }
    now_lst_tuple_pair = last_lst_tuple_pair;

    auto attr_id = predicates_space.at(rhs).attr_id;
    auto value = predicates_space.at(rhs).value;

    if (predicates_space.at(rhs).type == 0) {
        for (const VecUint &now_vec: last_lst_tuple_pair) {
            std::unordered_map<unsigned, VecUint> new_map;
            for (unsigned record_id: now_vec) {
                int pli_id = (*(*pliRecords)[record_id])[attr_id];
                new_map[pli_id].emplace_back(record_id);
            }
            int maxSize = 0;
            for (const auto &pair: new_map) {
                if (maxSize < pair.second.size()) maxSize = pair.second.size();
            }
            violation += static_cast<int>(now_vec.size()) - maxSize;
        }
    } else if (predicates_space.at(rhs).type == 1) {
        for (const VecUint &now_vec: last_lst_tuple_pair) {
            std::unordered_map<unsigned, VecUint> new_map;
            for (unsigned record_id: now_vec) {
                int pli_id = (*(*pliRecords)[record_id])[attr_id];
                new_map[pli_id].emplace_back(record_id);
            }
            int maxSize = 0;
            for (const auto &pair: new_map) {
                if (value == records[pair.second[0]]->at(attr_id)) maxSize = pair.second.size();
                // if (maxSize < pair.second.size()) maxSize = pair.second.size();
            }
            violation += static_cast<int>(now_vec.size()) - maxSize;
        }
    }

    return violation;
}

void CFDMeasure::compute_confidence(const VecStr &attributes, const VecSpRec &records, const Rule &rule,
                                    const PredicatesSpace &predicates_space) {
    unsigned violation = compute_violation(attributes, records, rule, predicates_space);
    unsigned support = compute_support(attributes, records, rule, predicates_space);
    // double hold_rate = support / ((records.size() * (records.size() - 1)) / 2.0);
    std::cout << std::endl;
    std::cout << "vio(#records): " << violation << "\nhold(#tps): " << support;// << "\nhold_rate: " << hold_rate;
    std::cout << std::endl << std::endl;
}
