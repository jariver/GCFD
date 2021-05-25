//
// Created by Administrator on 2020/5/8.
//

#include "record.h"

void Record::set_feature(const PredicatesSpace &predicates_space) {
    for (int i = 0; i != predicates_space.size(); ++i) {
        Predicate predicate = predicates_space.at(i);
        unsigned attr_id = predicate.attr_id;
        if (predicate.type == 0) {
            feature.push_back(false);
        } else if (predicate.type == 1) {
            std::string attr_value = predicate.value;
            feature.push_back(data[attr_id] == attr_value);
        }
    }
    set_feature_information(predicates_space);
}

void Record::set_feature_information(const PredicatesSpace &predicates_space) {
    for (int i = 0; i != predicates_space.size(); ++i) {
        Predicate predicate = predicates_space.at(i);
        std::string attr = predicate.attr;
        std::string info;
        if (this->feature[i] == 1) {
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
        this->feature_information.push_back(info);
    }
}

std::string Record::operator[](unsigned i) {
    if (i >= data.size()) {
        throw std::out_of_range("Index cross the line.");
    } else return data[i];
}

bool operator==(const Record &lhs, const Record &rhs) {
    std::string lhs_str, rhs_str;
    for (unsigned i = 0; i != lhs.size(); ++i) {
        lhs_str += lhs.at(i);
        rhs_str += rhs.at(i);
    }
    return lhs_str == rhs_str;
}

std::ostream &operator<<(std::ostream &os, const Record &record) {
    os << record.id << ' ';
    for (const auto &item : record.data) {
        os << std::left << std::setw(item.size() > 12 ? 24 : 12) << item << ' ';
    }
    return os;
}

void Record::emplace_back_non_pred(unsigned index) {
    non_relative_predicates.emplace_back(index);
}

bool operator!=(const Record &lhs, const Record &rhs) {
    return !(lhs == rhs);
}

