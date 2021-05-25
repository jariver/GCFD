//
// Created by Administrator on 06/21/2020.
//

#ifndef ALCFD_CPP_INIT_SAMPLING_H
#define ALCFD_CPP_INIT_SAMPLING_H

#include "using_name.h"
#include "processing.h"
#include "predicates_space.h"
#include <chrono>

extern const unsigned FREQUENCY_THRESHOLD;

struct Pli {
    Pli(UpVecUpVecUint pli_, unsigned attr_) : partition(std::move(pli_)), attr(attr_) {}

    UpVecUpVecUint partition;
    unsigned attr;
};

class init_sampling {
public:
    static void execute(VecVecUint &lst_tuple_pair,
                        VecPre &vec_predicates,
                        VecVecUint &combination,
                        std::vector<Pli> &plis,
                        UpVecUpVecInt &pliRecords,
                        const VecStr &attributes,
                        const VecSpRec &records,
                        std::unordered_map<std::string, SpVecVecUint> &map_pred_pli);

    static void preprocess(std::vector<Pli> &plis,
                           const VecStr &attributes,
                           const VecSpRec &records);

    static void create_frequency_items(VecPre &vec_predicates,
                                       const std::vector<Pli> &plis,
                                       const VecStr &attributes,
                                       const VecSpRec &records);

    static void sampling_tuple_pairs_pliRecords(VecVecUint &lst_tuple_pair,
                                                VecVecUint &combination,
                                                std::vector<Pli> &plis,
                                                UpVecUpVecInt &pliRecords,
                                                const VecStr &attributes,
                                                const VecSpRec &records,
                                                std::unordered_map<std::string, SpVecVecUint> &map_pred_pli);


    static UpVecUpVecUpStr transpose(const VecSpRec &records);

    static UpVecUpVecUint build_pli(UpVecUpStr lst);

    static void create_pliRecords(UpVecUpVecInt &pliRecords,
                                  const std::vector<Pli> &plis,
                                  const VecStr &attributes,
                                  const VecSpRec &records);
};


#endif //ALCFD_CPP_INIT_SAMPLING_H
