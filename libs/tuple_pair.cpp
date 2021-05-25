//
// Created by Administrator on 2020/5/6.
//

#include "tuple_pair.h"

void TuplePair::create_feature_information(const PredicatesSpace &predicates_space) {
    for (int i = 0; i < predicates_space.size(); i++) {
        Predicate predicate = predicates_space.at(i);
        std::string attr = predicate.attr;
        std::string info;
        if (feature[i] == 1) {
            if (predicate.type == 0)
                info = "s.{" + attr + "} = r.{" + attr + "}";
            else
                info = attr + " = " + predicate.value;
        } else {
            if (predicate.type == 0)
                info = "s.{" + attr + "} != r.{" + attr + "}";
            else
                info = attr + " != " + predicate.value;
        }
        feature_information.emplace_back(std::move(info));
    }
}

std::ostream &operator<<(std::ostream &os, const TuplePair &tp) {
    os << *tp.left_data << std::endl;
    os << *tp.right_data << std::endl;
    return os;
}

void TuplePair::set_feature(const PredicatesSpace &predicatesSpace) {
    for (const Predicate &predicate: predicatesSpace) {
        if (left_data->at(predicate.attr_id) == right_data->at(predicate.attr_id)) {
            if(predicate.type == 0) feature.emplace_back(1);
            else if(left_data->at(predicate.attr_id) == predicate.value) feature.emplace_back(1);
            else feature.emplace_back(0);
        } else {
            feature.emplace_back(0);
        }
    }
    create_feature_information(predicatesSpace);
}

void TuplePair::emplace_back_error(unsigned err_id, const std::string &err_value, const std::string &corr_value) {
    error_attr.emplace_back(err_id);
    error_value.emplace_back(err_value);
    correct_value.emplace_back(corr_value);
}

void TuplePair::emplace_back_relative_pred_2(unsigned index) {
    relative_predicates_2.emplace_back(index);
}

void TuplePair::emplace_back_non_pred(unsigned index) {
    non_relative_predicates.emplace_back(index);
}

void TuplePair::emplace_back_relative_pred(unsigned index) {
    relative_predicates.emplace_back(index);
}
