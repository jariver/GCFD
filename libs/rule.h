//
// Created by Administrator on 2020/5/17.
//

#ifndef ALCFD_CPP_RULE_H
#define ALCFD_CPP_RULE_H

#include <utility>
#include <algorithm>
#include "predicates_space.h"

class Rule {
    friend bool operator==(const Rule &rule1, const Rule &rule2);

public:
    Rule() = default;

    Rule(std::vector<unsigned> lhs, unsigned rhs) : LHS(std::move(lhs)), RHS(rhs) {};

    Rule(const std::vector<std::string> &lhs, const std::string &rhs, const PredicatesSpace &predicatesSpace);

    void create_information(const PredicatesSpace &predicatesSpace);

    std::string get_information() const {
        return information;
    }

    std::vector<unsigned> get_lhs() const {
        return LHS;
    }

    std::vector<unsigned> &get_lhs_ref() {
        return LHS;
    }

    unsigned get_rhs() const {
        return RHS;
    }

    void set_rhs(unsigned new_rhs) {
        RHS = new_rhs;
    }

    void set_support(int su) {
        support = su;
    }

    void set_error_support(int error_su) {
        error_support = error_su;
    }

    void set_whole_violation(int violatino) {
        whole_violation = violatino;
    }

    void set_whole_conf(float conf) {
        whole_conf = conf;
    }

    void set_whole_supp(int supp) {
        whole_supp = supp;
    }

    int get_support() const {
        return support;
    }

    int get_error_support() const {
        return error_support;
    }

    int get_whole_violation() const {
        return whole_violation;
    }

    float get_whole_conf() const {
        return whole_conf;
    }

    int get_whole_supp() const {
        return whole_supp;
    }

    std::vector<unsigned>::iterator erase_lhs(std::vector<unsigned>::iterator it) {
        return LHS.erase(it);
    }

    bool cant_sample() const {
        return cant_sample_;
    }

    void set_cant_samle(bool cant) {
        cant_sample_ = cant;
    }

private:
    std::vector<unsigned> LHS;
    unsigned RHS = 0;
    int support_tp = -1;
    int support_rc = -1;
    int support = -1;
    int error_support = -1;
    int whole_violation = -1;
    int whole_supp = -1;
    float whole_conf = -1;
    bool cant_sample_ = false;

    std::string information = "";
};

namespace std {
    template<>
    struct hash<Rule> {
        std::size_t operator()(const Rule &rule) const {
            std::string tmp_str = rule.get_information();
            return std::hash<std::string>()(tmp_str);
        }
    };
}

#endif //ALCFD_CPP_RULE_H
