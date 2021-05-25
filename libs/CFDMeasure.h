//
// Created by Administrator on 9/14/20.
//

#ifndef ALCFD_CPP_CFDMEASURE_H
#define ALCFD_CPP_CFDMEASURE_H

#include "using_name.h"
#include "processing.h"
#include "predicates_space.h"
#include "init_sampling.h"
#include "rule.h"

class CFDMeasure {
public:
    static unsigned compute_support(const VecStr &attributes,
                                    const VecSpRec &records,
                                    const Rule &rule,
                                    const PredicatesSpace &predicates_space);

    static unsigned compute_violation(const VecStr &attributes,
                                      const VecSpRec &records,
                                      const Rule &rule,
                                      const PredicatesSpace &predicates_space);

    static void compute_confidence(const VecStr &attributes,
                                       const VecSpRec &records,
                                       const Rule &rule,
                                       const PredicatesSpace &predicates_space);

    static UpVecUpVecUint build_pli(UpVecUpStr lst);

    static void preprocess(std::vector<Pli> &plis, const VecStr &attributes, const VecSpRec &records);

    static UpVecUpVecUpStr transpose(const VecSpRec &records);

    static void create_pliRecords(UpVecUpVecInt &pliRecords, const std::vector<Pli> &plis, const VecStr &attributes,
                           const VecSpRec &records);
};


#endif //ALCFD_CPP_CFDMEASURE_H
