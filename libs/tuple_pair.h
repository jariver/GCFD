//
// Created by Administrator on 2020/5/1.
//

#ifndef ALCFD_CPP_TUPLEPAIR_H
#define ALCFD_CPP_TUPLEPAIR_H

#include <utility>

#include "predicates_space.h"
#include "using_name.h"
#include "record.h"

class TuplePair {
    friend std::ostream &operator<<(std::ostream &os, const TuplePair &tp);

public:
    static const unsigned TT = 0;
    static const unsigned TF = 1;
    static const unsigned FFI = 2;
    static const unsigned FFII = 3;
    static const unsigned FFIII = 4;

    TuplePair(SpRec lhs, SpRec rhs) : left_data(std::move(lhs)), right_data(std::move(rhs)) {};

    void create_feature_information(const PredicatesSpace &predicates_space);

    std::vector<std::string> get_information() {
        return feature_information;
    }

    void set_similarity(float similarity) {
        this->_similarity = similarity;
    }

    float get_similarity() {
        return this->_similarity;
    }

    void set_label(unsigned label) {
        _label = label;
    }

    int get_label() const {
        return _label;
    }

    void set_prediction(unsigned prediction) {
        _prediction = prediction;
    }

    int get_prediction() const {
        return _prediction;
    }

    const SpRec &get_left_data_ptr() const {
        return left_data;
    }

    const SpRec &get_right_data_ptr() const {
        return right_data;
    }

    unsigned get_error_size() const {
        return error_attr.size();
    }

    std::vector<unsigned> get_error_attr() const {
        return error_attr;
    }

    unsigned get_error_attr(unsigned index) const {
        return error_attr[index];
    }

    std::vector<std::string> get_error_value() const {
        return error_value;
    }

    std::string get_error_value(unsigned index) const {
        return error_value[index];
    }

    std::vector<std::string> get_corr_value() const {
        return correct_value;
    }

    std::string get_corr_value(unsigned index) const {
        return correct_value[index];
    }

    void emplace_back_error(unsigned err_id, const std::string &err_value, const std::string &corr_value = "");

    void emplace_back_relative_pred(unsigned index);

    void emplace_back_relative_pred_2(unsigned index);

    void emplace_back_non_pred(unsigned index);

    std::vector<unsigned> get_relative_pred() const {
        return relative_predicates;
    }

    std::vector<unsigned> get_relative_pred_2() const {
        return relative_predicates_2;
    }

    std::vector<unsigned> get_non_pred() const {
        return non_relative_predicates;
    }

    void set_feature(const PredicatesSpace &predicatesSpace);

    unsigned get_feature_size() const {
        return feature.size();
    }

    std::vector<bool> get_feature() const {
        return feature;
    }

    bool get_feature(unsigned index) const {
        return feature[index];
    }

    void add_violate_cfds_count() {
        violate_cfds_count++;
    }

    void reset_violate_cfds_count() {
        violate_cfds_count = 0;
    }

    unsigned ViolateCFDsCount() const {
        return violate_cfds_count;
    }

private:
    SpRec left_data;
    SpRec right_data;
    std::vector<bool> feature;
    std::vector<std::string> feature_information;
    int _prediction = -1;
    int _label = -1;
    unsigned violate_cfds_count = 0;
    std::vector<unsigned> error_attr;
    std::vector<std::string> error_value, correct_value;
    std::vector<unsigned> relative_predicates, relative_predicates_2, non_relative_predicates;
    float _similarity{};
};

#endif //ALCFD_CPP_TUPLEPAIR_H
