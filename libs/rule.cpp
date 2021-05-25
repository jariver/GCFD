//
// Created by Administrator on 2020/5/17.
//

#include <unordered_map>
#include "rule.h"

void Rule::create_information(const PredicatesSpace &predicatesSpace) {
    std::sort(LHS.begin(), LHS.end());
    information += "(";
    for (unsigned pred_id : LHS) {
        auto predicate = predicatesSpace.at(pred_id);
        if (predicate.type == 0) {
            information += predicate.attr;
            information += ",";
        } else if (predicate.type == 1) {
            information += predicate.attr;
            information += "=";
            information += predicate.value;
            information += ",";
        }
    }

    information += ")=>";

    auto predicate = predicatesSpace.at(RHS);
    if (predicate.type == 0) {
        information += predicate.attr;
    } else if (predicate.type == 1) {
        information += predicate.attr;
        information += "=";
        information += predicate.value;
    }
}

bool operator==(const Rule &rule1, const Rule &rule2) {
    return rule1.get_information() == rule2.get_information();
}

Rule::Rule(const std::vector<std::string> &lhs, const std::string &rhs, const PredicatesSpace &predicatesSpace) {
    std::unordered_map<std::string, unsigned> unorderedMap;

    for (unsigned i = 0; i != predicatesSpace.size(); ++i) {
        Predicate predicate = predicatesSpace.at(i);
        if (predicate.type == 0) {
            unorderedMap[predicate.attr] = i;
        } else if (predicate.type == 1) {
            unorderedMap[predicate.attr + "=" + predicate.value] = i;
        }
    }

    for (const std::string &str: lhs) {
        LHS.emplace_back(unorderedMap[str]);
    }
    RHS = unorderedMap[rhs];
}
