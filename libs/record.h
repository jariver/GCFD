//
// Created by Administrator on 2020/5/8.
//

#ifndef ALCFD_CPP_RECORD_H
#define ALCFD_CPP_RECORD_H

#include "predicates_space.h"
#include <memory>
#include <iostream>
#include <utility>
#include <iomanip>

class Record {
    friend bool operator!=(const Record &lhs, const Record &rhs);

    friend bool operator==(const Record &lhs, const Record &rhs);

    friend std::ostream &operator<<(std::ostream &os, const Record &record);

public:
    Record() = default;

    Record(const unsigned &id_, std::vector<std::string> data_) : id(id_), data(std::move(data_)) {}

    std::string operator[](unsigned);

    std::string at(unsigned i) const {
        return data.at(i);
    }

    std::size_t size() const {
        return data.size();
    }

    void set_data(unsigned i, std::string str) {
        data[i] = std::move(str);
    }

    void set_label(bool label_) {
        label = label_;
    }

    int get_label() const {
        return label;
    }

    auto begin() const {
        return data.begin();
    }

    auto cbegin() const {
        return data.cbegin();
    }

    auto end() const {
        return data.end();
    }

    auto cend() const {
        return data.cend();
    }

    unsigned get_id() const {
        return id;
    }

    std::vector<std::string> get_data() const {
        return data;
    }

    std::string get_data(unsigned index) const {
        return data[index];
    }

    void emplace_back_error(unsigned attr) {
        error_attr.emplace_back(attr);
        error_value.emplace_back(data[attr]);
    }

    std::vector<unsigned> get_error_attr() const{
        return error_attr;
    }

    unsigned get_error_attr(unsigned index) const {
        return error_attr[index];
    }

    unsigned get_error_size() const {
        return error_attr.size();
    }

    std::vector<std::string> get_error_value() const {
        return error_value;
    }

    std::string get_error_value(unsigned index) const {
        return error_value[index];
    }

    void set_feature(const PredicatesSpace &predicates_space);

    unsigned get_feature_size() const {
        return feature.size();
    }

    std::vector<bool> get_feature() const {
        return feature;
    }

    bool get_feature(unsigned index) const {
        return feature[index];
    }

    std::vector<unsigned> get_non_pred() const {
        return non_relative_predicates;
    }

    void set_feature_information(const PredicatesSpace &predicates_space);

    std::vector<std::string> get_information() {
        return feature_information;
    }

    void emplace_back_non_pred(unsigned index);

private:
    const unsigned id = 0;
    std::vector<std::string> data;
    int label = -1;
    std::vector<unsigned> error_attr;
    std::vector<std::string> error_value;
    std::vector<bool> feature;
    std::vector<std::string> feature_information;
    std::vector<unsigned> non_relative_predicates;
};

namespace std {
    template<>
    struct hash<Record> {
        std::size_t operator()(const Record &record) const {
            std::string tmp_str;
            for (unsigned i = 0; i != record.size(); ++i) {
                tmp_str += record.at(i);
            }
            return std::hash<std::string>()(tmp_str);
        }
    };
}

using SpRec = std::shared_ptr<Record>;
using VecSpRec = std::vector<SpRec>;

#endif //ALCFD_CPP_RECORD_H
