//
// Created by Administrator on 2020/5/7.
//

#ifndef ALCFD_CPP_PREDICATE_H
#define ALCFD_CPP_PREDICATE_H

#include <utility>
#include <vector>
#include <string>

struct Predicate {
    unsigned type;  // 0 or 1
    std::string attr;
    std::string value;    // gender, "" or "male"
    unsigned attr_id;

    Predicate(unsigned type_, std::string attr_, unsigned attr_id_, std::string value_ = "") :
            type(type_), attr(std::move(attr_)), attr_id(attr_id_), value(std::move(value_)) {};
};


#endif //ALCFD_CPP_PREDICATE_H
