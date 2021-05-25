//
// Created by Administrator on 2020/5/7.
//

#ifndef ALCFD_CPP_ALCFD_H
#define ALCFD_CPP_ALCFD_H

#include <memory>

#include "libs/using_name.h"
#include "libs/using_name_sp.h"
#include "libs/processing.h"
#include "libs/rule.h"
#include "libs/record.h"
#include "libs/init_sampling.h"
#include "libs/re_sampling.h"

extern const int SMALL_MIN_SUPPORT_COUNT;

class al_cfd {
public:
    al_cfd() {
        all_lst_tp_label_boss.reserve(5);
        all_lst_tp_label_tt = std::make_shared<VecSpTp>();
        all_lst_tp_label_boss.emplace_back(all_lst_tp_label_tt);
        all_lst_tp_label_tf = std::make_shared<VecSpTp>();
        all_lst_tp_label_boss.emplace_back(all_lst_tp_label_tf);
        all_lst_tp_label_ffi = std::make_shared<VecSpTp>();
        all_lst_tp_label_boss.emplace_back(all_lst_tp_label_ffi);
        all_lst_tp_label_ffii = std::make_shared<VecSpTp>();
        all_lst_tp_label_boss.emplace_back(all_lst_tp_label_ffii);
        all_lst_tp_label_ffiii = std::make_shared<VecSpTp>();
        all_lst_tp_label_boss.emplace_back(all_lst_tp_label_ffiii);

        rule_repository.reserve(10000);
        rule_repository_top_k.reserve(1000);
        trash_repository_set.reserve(300000);
    };

    void run();

    void assess(const std::vector<std::string> &lhs, const std::string &rhs);

private:
    VecSpTp lst_tuple_pairs;
    VecSpTp lst_origin_tuple_pairs;
    VecSpTp re_globle_lst_tuple_pairs;
    VecSpTp re_origin_tuple_pairs;
    VecSpVecSpTp all_lst_tp_label_boss;
    SpVecSpTp all_lst_tp_label_tt;
    SpVecSpTp all_lst_tp_label_tf;
    SpVecSpTp all_lst_tp_label_ffi;
    SpVecSpTp all_lst_tp_label_ffii;
    SpVecSpTp all_lst_tp_label_ffiii;

    std::unordered_set<Record> all_sampled_records_set;
    VecSpRec all_sampled_records;
    std::vector<Pli> sampled_plis;
    UpVecUpVecInt sampled_pliRecords;

    std::vector<std::vector<std::string>> frequency_items;
    PredicatesSpace predicates_space;
    std::vector<Rule> rule_repository_top_k;
    std::vector<Rule> rule_repository;
    std::vector<Rule> origin_rules;
    std::vector<Rule> learned_rules;
    std::map<unsigned, unsigned> map_attr_freq;
    std::vector<std::pair<unsigned, unsigned>> map_attr_freq_by_value;
    std::vector<unsigned> zero_freq_pred;
    std::unordered_set<Rule> trash_repository_set;
    std::vector<unsigned> lst_relative_predicates;
    std::set<unsigned> error_attributes;
    std::set<unsigned> last_error_attributes;
    std::unordered_set<unsigned> freq_rhs;
    std::vector<std::string> attributes;
    ReSampling reSampling;
    std::map<std::string, unsigned> map_attr_id;
    VecSpRec records;
    std::map<unsigned, SpRec> map_id_record;
    std::vector<std::shared_ptr<Record>> clean_records;
    std::map<unsigned, std::shared_ptr<Record>> map_id_clean_record;
    // 0 3 8 128 == +1 ==> 1 4 9 129
    // 1 * 1000^(n-1) + 4 * 1000^(n-2) + 9 * 1000^(n-3) + 129 * 1000^(n-3)
    // 1 * 1000^3 + 4 * 1000^2 + 9 * 1000^1 + 129 * 1000^0 = 1004009128
    // (pred_id + 1) * 1000^(len-1-i)
    std::unordered_map<std::string, SpVecVecUint> map_pred_pli;
    std::vector<Pli> plis;
    std::vector<Pli> clean_plis;
    UpVecUpVecInt pliRecords;
    UpVecUpVecInt clean_pliRecords;
    std::unordered_set<std::string> used_tuple_pairs;
    std::unordered_set<std::string> used_tuple_pairs_for_learning;
    VecVecUint init_combination;
    int SUPPORT_COUNT = SMALL_MIN_SUPPORT_COUNT;

    // output
    std::ofstream out_result;
    int round_count = 0;
    int resampling_time = 0;
    int refinement_time = 0;

    std::vector<std::string> assess_lhs;
    std::string assess_rhs;

    void read_data();

    void read_data_clean();

    void dedup();

    void print_records();

    void execute();

    void init_sampling();

    void init_label();

    void init_learning();

    void re_sampling(unsigned epoch, bool doing);

//    void create_freq_pred_in_rule();

    void re_sampling_global();

    void re_sampling_predicted(unsigned epoch);

    void re_label(unsigned epoch);

    bool auto_ajustment(unsigned epoch);

    void result();

    void create_origin_cfds(const std::string &file_name);

    void compute_recall();

    void predict(unsigned epoch);

    std::string label();

    void get_label();

    void get_label_from_clean_data();

    void get_dirty_records_from_clean_data();

    void create_map_id_record(int total = -1);

    void compose_tuple_pair();

    void compose_tuple_pair_auto();

    void compose_tuple_pair_xxxx();

    void set_relative_predicates(Record &record);

    void set_relative_predicates(TuplePair &tp);

    void print_labeled_tuple_pair(unsigned epoch);

    void re_print_labeled_tuple_pair(unsigned epoch);

    bool minimal_rule(Rule &rule);

    void compute_support();

    void compute_whole_support();

    void compute_whole_support(Rule &rule);

    void compute_whole_vio(Rule &rule);

    int compute_support(Rule &rule);

    int compute_error_support(Rule &rule);

    bool lr_have_no_same_attr(const Rule &rule);

    bool verify_by_true(const Rule &rule);

    bool support_by_n_true(Rule &rule, unsigned epoch);

    void add_trash_repository_set(const Rule &rule);

    // void add_rule_repository(const Rule &rule);

    void add_most_general_rule(Rule &rule);

    bool add_most_general_rule(const Rule &rule, std::vector<Rule> &rule_repo);

    bool in_trash_repository_set(const Rule &rule);

    bool in_rule_repository(const Rule &rule);

    bool is_same_rule(const Rule &rule1, const Rule &rule2);

    template<typename T>
    void append_predicate_into_rule(const T &tuplePair, const Rule &rule, std::vector<bool> feature, std::vector<bool> &hasAppended, unsigned epoch);

    void print_rule_repository(const std::string &filename);

    void print_result(const std::string &filename);

    static bool have_some_value(const SpRec &left_record, const SpRec &right_record);

    void add_sampled_records(const VecSpTp &tuple_pairs);

    static void init_parameters();
};


#endif //ALCFD_CPP_ALCFD_H
