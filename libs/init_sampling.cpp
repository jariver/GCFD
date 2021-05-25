//
// Created by Administrator on 06/21/2020.
//

#include "init_sampling.h"

extern const unsigned TUPLE_PAIR_COUNT;
extern const int CLOCKS_PER_MSEC;
extern const bool init_pass_n_second;
extern const float MIN_WHOLE_SUPPORT_RATE;

//void dedup(VecUint &vec, const VecSpRec &records) {
//    std::unordered_set<Record> unorderedSet;
//    for (auto it = vec.begin(); it != vec.end();) {
//        unsigned record_id = *it;
//        if (unorderedSet.find(*records[record_id]) != unorderedSet.end()) {
//            it = vec.erase(it);
//        } else {
//            unorderedSet.emplace(*records[record_id]);
//            ++it;
//        }
//    }
//}

//void dedup(std::vector<Pli> &plis, const VecSpRec &records) {
//    for (const auto &pli: plis) {
//        for (auto it = pli.partition->begin(); it != pli.partition->end();) {
//            dedup(**it, records);
//            if ((*it)->size() < 2) it = pli.partition->erase(it);
//            else ++it;
//        }
//    }
//}

//void init_sampling::random_sampling(VecSpRec &sub_records, const VecSpRec &records) {
//    double sub_rate = 0.1;
//    VecSpRec new_records;
//    for (const SpRec &rec: records) {
//        new_records.emplace_back(std::make_unique<Record>(*rec));
//    }
//    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
//    std::shuffle(new_records.begin(), new_records.end(), std::default_random_engine(seed));
//    for (unsigned i = 0; i != static_cast<unsigned >(new_records.size() * sub_rate); ++i) {
//        sub_records.emplace_back(std::make_unique<Record>(*new_records[i]));
//    }
//}

void init_sampling::execute(VecVecUint &lst_tuple_pair,
                            VecPre &vec_predicates,
                            VecVecUint &combination,
                            std::vector<Pli> &plis,
                            UpVecUpVecInt &pliRecords,
                            const VecStr &attributes,
                            const VecSpRec &records,
                            std::unordered_map<std::string, SpVecVecUint> &map_pred_pli) {
//    VecSpRec sub_records;
//    random_sampling(sub_records, records);

    clock_t t0 = clock();
    preprocess(plis, attributes, records);
    clock_t t1 = clock();
    std::cout << "Pre-processing cost: " << (t1 - t0) / CLOCKS_PER_MSEC << "ms" << std::endl;

//    clock_t t2 = clock();
//    create_frequency_items(vec_predicates, plis, attributes, records);
//    clock_t t3 = clock();
//    std::cout << "Creating frequency items cost: " << (t3 - t2) / CLOCKS_PER_MSEC << "ms" << std::endl;

    // sampling_tuple_pairs(lst_tuple_pair, plis, attributes, records);
    // sampling_tuple_pairs_dfs(lst_tuple_pair, combination, plis, attributes, records);
    sampling_tuple_pairs_pliRecords(lst_tuple_pair, combination, plis, pliRecords, attributes, records, map_pred_pli);
    clock_t t4 = clock();
    std::cout << "Sampling tuple pairs cost: " << (t4 - t1) / CLOCKS_PER_MSEC << "ms" << std::endl << std::endl;
}

void init_sampling::preprocess(std::vector<Pli> &plis,
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

    // dedup(plis, records);

//    std::cout << plis.size() << std::endl;
//    for(auto &pli : plis) {
//        std::cout << pli.partition->size() << std::endl;
//        for(auto &cluster : *pli.partition) {
//            for(auto num : *cluster) {
//                std::cout << num << ' ';
//            }
//            std::cout << std::endl;
//        }
//        std::cout << std::endl;
//    }
}

void init_sampling::sampling_tuple_pairs_pliRecords(VecVecUint &lst_tuple_pair,
                                                    VecVecUint &init_combination,
                                                    std::vector<Pli> &plis,
                                                    UpVecUpVecInt &pliRecords,
                                                    const VecStr &attributes,
                                                    const VecSpRec &records,
                                                    std::unordered_map<std::string, SpVecVecUint> &map_pred_pli) {
    VecVecUint combination;

    unsigned sampling_cout = 0;

    std::sort(plis.begin(), plis.end(),
              [](const Pli &lhs, const Pli &rhs) {
                  return lhs.partition->size() > rhs.partition->size();
              });

    create_pliRecords(pliRecords, plis, attributes, records);

    std::vector<unsigned> best_attr_list;
    best_attr_list.reserve(attributes.size() + 1);
    std::cout << "\nThe number of clusters of every pli: ";
    for (const auto &pli : plis) {
        std::cout << pli.partition->size() << ' ';
        // if (best_attr_list.size() < plis.size() * 0.7) {
        if (best_attr_list.size() < plis.size() * 0.7) {
            best_attr_list.emplace_back(pli.attr);
        }
    }
    std::sort(plis.begin(), plis.end(),
              [](const Pli &lhs, const Pli &rhs) {
                  return lhs.attr < rhs.attr;
              });
    std::cout << std::endl << "The best attributes: ";
    for (const auto &item : best_attr_list) {
        std::cout << item << ' ';
    }
    std::cout << std::endl;

    for (int i = 1; i != best_attr_list.size(); ++i) {
        // Combinations
        processing::combinations(combination, best_attr_list, best_attr_list.size() - i);
    }

    std::cout << "\nThe best attributes' combinations are: \n";

    for (auto &com_attrs: combination) {
        if (sampling_cout >= TUPLE_PAIR_COUNT) break;

        for (unsigned j = 0; j != com_attrs.size(); ++j) {
            std::cout << com_attrs[j] << ' ';
        }
        std::cout << std::endl;

        clock_t t0 = clock();
        bool sampled = false;

        // BEGIN-1
        std::sort(com_attrs.begin(), com_attrs.end(), [&](unsigned lhs_attr_id, unsigned rhs_attr_id) {
            return lhs_attr_id < rhs_attr_id;
        });
        SpVecVecUint now_lst_tuple_pair = std::make_shared<VecVecUint>();
        SpVecVecUint last_lst_tuple_pair = std::make_shared<VecVecUint>();
        unsigned jj = 0;
        std::string key;
        for (unsigned j = 0; j != com_attrs.size(); j++) {
            key += processing::emplace_front(std::to_string(com_attrs[j]), 3, '0');
            if (map_pred_pli.find(key) != map_pred_pli.end()) {
                last_lst_tuple_pair = map_pred_pli[key];
                jj = j + 1;
            }
        }
        key = "";
        for (unsigned j = 0; j != jj; ++j) {
            key += processing::emplace_front(std::to_string(com_attrs[j]), 3, '0');
        }
        // END-1
        for (unsigned j = jj; j != com_attrs.size(); ++j) {
            unsigned attr_id = com_attrs[j];
            if (j == 0) {
                for (const UpVecUint &vec: *plis[attr_id].partition) {
                    last_lst_tuple_pair->emplace_back(*vec);
                }
                // BEGIN-2
                key += processing::emplace_front(std::to_string(com_attrs[j]), 3, '0');
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
                key += processing::emplace_front(std::to_string(com_attrs[j]), 3, '0');
                map_pred_pli[key] = last_lst_tuple_pair;
                // END-3
            }
        }
        now_lst_tuple_pair = last_lst_tuple_pair;

        decltype(now_lst_tuple_pair) ans_lst_tuple_pair = std::make_shared<VecVecUint>();;
        int length = now_lst_tuple_pair->size();
        for (int index = 0; index != length; ++index) {
            if ((*now_lst_tuple_pair)[index].size() > 2) {
                VecUint copy_;
                for (unsigned int it : (*now_lst_tuple_pair)[index]) {
                    copy_.emplace_back(it);
                    if (copy_.size() >= 2) break;
                }
                VecVecUint res;
                processing::combinations(res, copy_, 2);
                for (VecUint &inter_set : res) {
                    ans_lst_tuple_pair->emplace_back(inter_set.begin(), inter_set.end());
                }
            } else if ((*now_lst_tuple_pair)[index].size() == 2) {
                ans_lst_tuple_pair->emplace_back((*now_lst_tuple_pair)[index]);
            }
        }

        unsigned now_sampling_count = 0;
        for (const auto &tuple_pair: *ans_lst_tuple_pair) {
            if (std::find(lst_tuple_pair.begin(), lst_tuple_pair.end(), tuple_pair) == lst_tuple_pair.end()) {
                lst_tuple_pair.emplace_back(tuple_pair);
                sampled = true;
                if (now_sampling_count++ >= 1) break;
                sampling_cout++;
            }
        }
        if (sampled) init_combination.emplace_back(com_attrs);

        clock_t t1 = clock();
//            for (unsigned num: com_attrs) {
//                std::cout << num << ' ';
//            }
//            std::cout << std::endl;
//            std::cout << "this combination sampling cost: " << (t1 - t0) / CLOCKS_PER_MSEC << "ms" << std::endl;
        if (sampling_cout >= TUPLE_PAIR_COUNT) break;
    }
}

UpVecUpVecUpStr init_sampling::transpose(const VecSpRec &records) {
    UpVecUpVecUpStr ret = std::make_unique<VecUpVecUpStr>();
    for (size_t j = 0; j != records[0]->size(); ++j) {
        UpVecUpStr col = std::make_unique<VecUpStr>();
        for (size_t i = 0; i != records.size(); ++i) {
            UpStr tmp_str(new std::string(records.at(i)->at(j)));
            // UpStr tmp_str = std::make_unique<std::string>(records.at(i)->at(j));
            // (*col)[records.at(i)->get_id()] = std::move(tmp_str);
            col->emplace_back(std::move(tmp_str));
        }
        ret->emplace_back(std::move(col));
    }
    return ret;
}

UpVecUpVecUint init_sampling::build_pli(UpVecUpStr lst) {
    UpVecUpVecUint list_pli = std::make_unique<VecUpVecUint>();
    std::unordered_map<std::string, std::set<unsigned>> hashes;
    hashes.reserve(1024);
    for (unsigned i = 0; i != lst->size(); ++i) {
        hashes[*(*lst)[i]].insert(i);
    }
    for (const auto &item : hashes) {
        auto cluster = std::make_unique<VecUint>(item.second.cbegin(), item.second.cend());
        list_pli->emplace_back(std::move(cluster));
//        if (item.second.size() >= 2) {
//            auto cluster = std::make_unique<VecUint>(item.second.cbegin(), item.second.cend());
//            list_pli->emplace_back(std::move(cluster));
//        }
    }
    return list_pli;
}

void init_sampling::create_frequency_items(VecPre &vec_predicates,
                                           const std::vector<Pli> &plis,
                                           const VecStr &attributes,
                                           const VecSpRec &records) {
    for (const auto &pli : plis) {
        for (const auto &cluster : *pli.partition) {
            // if (cluster->size() >= FREQUENCY_THRESHOLD) {
            if (cluster->size() >= records.size() * MIN_WHOLE_SUPPORT_RATE) {
                vec_predicates.emplace_back(1, attributes[pli.attr], pli.attr,
                                            records[cluster->at(0)]->at(pli.attr));
            }
        }
    }
}

void init_sampling::create_pliRecords(UpVecUpVecInt &pliRecords,
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