//
// Created by Administrator on 2020/5/7.
//

#ifndef ALCFD_CPP_PREDICATES_SPACE_H
#define ALCFD_CPP_PREDICATES_SPACE_H

#include "predicate.h"
#include <vector>
#include <memory>

using VecPre = std::vector<Predicate>;
using UpVecPre = std::unique_ptr<VecPre>;

class PredicatesSpace {
public:
    PredicatesSpace() = default;

    Predicate at(const unsigned i) const {
        return predicates->at(i);
    }

    auto begin() {
        return predicates->begin();
    }

    auto end() {
        return predicates->end();
    }

    auto begin() const {
        return predicates->cbegin();
    }

    auto end() const {
        return predicates->cend();
    }

    unsigned size() const {
        return predicates->size();
    }

    void clear() const{
        predicates->clear();
    }

    void emplace_back(Predicate &&predicate) {
        predicates->emplace_back(std::move(predicate));
    }

private:
    UpVecPre predicates = std::make_unique<VecPre>();
};


#endif //ALCFD_CPP_PREDICATES_SPACE_H
