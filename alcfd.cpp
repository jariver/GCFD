//
// Created by Administrator on 2020/5/7.
//

#include "alcfd.h"
#include "libs/CFDMeasure.h"
#include "libs/CFDMeasure_2.h"

using std::string;

enum dataset {
    abalone, adult, soccer, sp500
};

enum error_percent {
    zero_one, zero_five, one, two
};

// Dataset
//const int DATASET = abalone;
//const int DATASET = adult;
//const int DATASET = soccer;
const int DATASET = sp500;

// #CFDs to introduce noises
//const int CFDS_COUNT = 3;
//const int CFDS_COUNT = 5;
const int CFDS_COUNT = 10;

// %Errors
const int ERR_PER = zero_one;
//const int ERR_PER = zero_five;
//const int ERR_PER = one;
//const int ERR_PER = two;

extern const float MIN_WHOLE_SUPPORT_RATE = 0.1;
extern const float CONFIDENCE_THRESHOLD = 0.98;
extern const unsigned TUPLE_PAIR_COUNT = 5;
const unsigned RE_TUPLE_PAIR_COUNT = 3;
const unsigned RULE_REPOSITORY_K = 30;

const float MIN_WHOLE_VIOLATION_RATE = 0.001;
const int BIG_MIN_SUPPORT_COUT = 1;
const int SMALL_MIN_SUPPORT_COUNT = 1;
const unsigned EPOCH = 50;
const unsigned MAX_LHS_COUNT = 10;
const unsigned WAIT_ROUND = 5;
const float CONF_THRESHOLD = 0.90;
const std::string ERROR_VALUE = "ERR2020";

extern const bool filterInitialCombi = true;
const bool ADD_ALL_MORE_GENERAL = true;
extern const bool init_pass_n_second = true;
extern const bool re_pass_n_second = true;

inline const char &SEPARATOR() {
    static const char SEPARATOR = ',';
    return SEPARATOR;
}

inline const bool &HAVE_ATTR_ROW() {
    static const bool HAVE_ATTR_ROW = true;
    return HAVE_ATTR_ROW;
}

extern const int CLOCKS_PER_MSEC = CLOCKS_PER_SEC / 1000;
const int CLOCKS_PER_USEC = CLOCKS_PER_MSEC / 1000;

string RULE_FILE;
string DATA_FILE;
string CLEAN_DATA_FILE;
string RESULT_PATH;

inline const string &RESULT_FILE() {
    static const string RESULT_FILE = RESULT_PATH + "result.txt";
    return RESULT_FILE;
}

inline const string &INIT_LABEL_FILE() {
    static const string INIT_LABEL_FILE = "../label/init_label.txt";
    return INIT_LABEL_FILE;
}

inline const string &INIT_LABELED_FILE() {
    static const string INIT_LABELED_FILE = "../label/init_labeled.txt";
    return INIT_LABELED_FILE;
}

inline const int &RE_SAMPLING_STRATEGY() {
    // Re_sampling
    // 0: dfs   1: bfs   2: pliRecords
    static const int RE_SAMPLING_STRATEGY = 2;
    return RE_SAMPLING_STRATEGY;
}

inline const double &RE_SAMPLING_RANDOM_SAMPLING_RATE() {
    // Re_sampling
    static const double RE_SAMPLING_RANDOM_SAMPLING_RATE = 0.1;
    return RE_SAMPLING_RANDOM_SAMPLING_RATE;
}

void al_cfd::run() {
    clock_t t0 = clock();
    init_parameters();

    read_data();
    clock_t t1 = clock();
    std::cout << "Read Dirty Data from csv cost: " << (t1 - t0) / CLOCKS_PER_MSEC << "ms" << std::endl << std::endl;

    read_data_clean();
    clock_t t2 = clock();
    std::cout << "Read Clean Data from csv cost: " << (t2 - t1) / CLOCKS_PER_MSEC << "ms" << std::endl << std::endl;

    for (size_t i = 0; i != clean_records.size(); ++i) {
        map_id_clean_record[i + 1] = clean_records[i];
    }
    for (size_t i = 0; i != attributes.size(); ++i) {
        map_attr_id[attributes[i]] = i;
    }
    clock_t t3 = clock();
    std::cout << "Creating map_id_record, map_id_clean_record and map_attr_id cost: " << (t3 - t2) / CLOCKS_PER_MSEC
              << "ms" << std::endl;

    clock_t t4_begin = clock();
    dedup();
    clock_t t4_end = clock();
    std::cout << "De-duplication cost: " << (t4_end - t4_begin) / CLOCKS_PER_MSEC << "ms" << std::endl;

    get_dirty_records_from_clean_data();

    execute();
}

void al_cfd::assess(const std::vector<std::string> &lhs, const std::string &rhs) {
    assess_lhs = lhs;
    assess_rhs = rhs;
    clock_t t0 = clock();
    init_parameters();
    read_data();
    clock_t t1 = clock();
    std::cout << "Read Dirty Data from csv cost: " << (t1 - t0) / CLOCKS_PER_MSEC << "ms" << std::endl << std::endl;

    read_data_clean();
    clock_t t2 = clock();
    std::cout << "Read Clean Data from csv cost: " << (t2 - t1) / CLOCKS_PER_MSEC << "ms" << std::endl << std::endl;

    for (size_t i = 0; i != clean_records.size(); ++i) {
        map_id_clean_record[i + 1] = clean_records[i];
    }
    for (size_t i = 0; i != attributes.size(); ++i) {
        map_attr_id[attributes[i]] = i;
    }
    clock_t t3 = clock();
    std::cout << "Creating map_id_record, map_id_clean_record and map_attr_id cost: " << (t3 - t2) / CLOCKS_PER_MSEC
              << "ms" << std::endl;

    clock_t t4_begin = clock();
    dedup();
    clock_t t4_end = clock();
    std::cout << "De-duplication cost: " << (t4_end - t4_begin) / CLOCKS_PER_MSEC << "ms" << std::endl;

    get_dirty_records_from_clean_data();

    plis.reserve(attributes.size() + 1);
    clean_plis.reserve(attributes.size() + 1);
    VecVecUint lst_tuple_pair;
    VecPre vec_predicates;

    CFDMeasure_2::preprocess(clean_plis, attributes, clean_records);
    CFDMeasure_2::create_pliRecords(clean_pliRecords, clean_plis, attributes, clean_records);

    init_sampling::execute(lst_tuple_pair, vec_predicates, init_combination, plis, pliRecords, attributes,
                           clean_records, map_pred_pli);
    init_sampling::create_frequency_items(vec_predicates, clean_plis, attributes, clean_records);

    for (unsigned i = 0; i != attributes.size(); ++i) {
        Predicate predicate(0, attributes[i], i);
        predicates_space.emplace_back(std::move(predicate));
    }

    for (auto &predicate : vec_predicates) {
        predicates_space.emplace_back(std::move(predicate));
    }

    // (viscera, whole, rings=10) => height
    Rule assess_rule(assess_lhs, assess_rhs, predicates_space);
    assess_rule.create_information(predicates_space);
    CFDMeasure::compute_confidence(attributes, clean_records, assess_rule, predicates_space);
    CFDMeasure_2::compute_confidence(attributes, clean_records, assess_rule, predicates_space);

    // Dirty Data
    std::cout << "\n\nDirty Data:" << std::endl;
    plis.clear();
    predicates_space.clear();
    plis.reserve(attributes.size() + 1);
    VecVecUint lst_tuple_pair_dirty;
    VecPre vec_predicates_dirty;

    init_sampling::execute(lst_tuple_pair_dirty, vec_predicates_dirty, init_combination, plis, pliRecords, attributes,
                           records, map_pred_pli);
    init_sampling::create_frequency_items(vec_predicates, clean_plis, attributes, clean_records);


    for (unsigned i = 0; i != attributes.size(); ++i) {
        Predicate predicate(0, attributes[i], i);
        predicates_space.emplace_back(std::move(predicate));
    }

    for (auto &predicate : vec_predicates_dirty) {
        predicates_space.emplace_back(std::move(predicate));
    }

    Rule assess_rule_dirty(assess_lhs, assess_rhs, predicates_space);
    assess_rule_dirty.create_information(predicates_space);
    CFDMeasure::compute_confidence(attributes, records, assess_rule_dirty, predicates_space);
    CFDMeasure_2::compute_confidence(attributes, records, assess_rule_dirty, predicates_space);
}

void al_cfd::read_data() {
    clock_t t0 = clock();
    std::string data = processing::readfile(DATA_FILE.c_str());
    clock_t t1 = clock();
    std::cout << "Readfile Dirty Data cost: " << (t1 - t0) / CLOCKS_PER_MSEC << "ms." << std::endl;
    std::cout << "Dirty Data have " << data.size() << " chars." << std::endl;
    records.reserve(200010);

    std::vector<string> line;
    line.reserve(attributes.size() + 1);

    bool is_title = HAVE_ATTR_ROW() == 1;
    size_t i = 1;
    std::string::size_type lastPos = data.find_first_not_of(SEPARATOR(), 0);
    std::string::size_type pos = data.find_first_of(SEPARATOR(), lastPos);
    std::string::size_type sep_n = data.find_first_of('\n', 0);
    while (std::string::npos != pos || std::string::npos != lastPos) {
        line.emplace_back(data.substr(lastPos, pos - lastPos));
        lastPos = data.find_first_not_of(SEPARATOR(), pos);
        while (++pos != lastPos && lastPos != std::string::npos) {
            line.emplace_back("");
            line.emplace_back("");
        }
        pos = data.find_first_of(SEPARATOR(), lastPos);
        if (pos > sep_n) {
            line.emplace_back(data.substr(lastPos, sep_n - lastPos));
            if (is_title) {
                attributes = std::move(line);
                is_title = false;
            } else {
                auto tmp_rec = std::make_shared<Record>(i++, std::move(line));
                records.emplace_back(std::move(tmp_rec));
            }
            line.clear();
            line.reserve(attributes.size() + 1);
            lastPos = data.find_first_not_of(SEPARATOR(), sep_n + 1);
            pos = data.find_first_of(SEPARATOR(), sep_n + 1);
            while (lastPos > sep_n || pos > sep_n) {
                sep_n = data.find_first_of('\n', sep_n + 1);
            }
        }
    }

    for (const auto &rec: records) {
        if (rec->get_data().size() >= 24) {
            std::cout << "";
        }
    }

    std::cout << "Dirty Dataset have total " << records.size() << " records and " << attributes.size() << " attributes."
              << std::endl;
}

void al_cfd::read_data_clean() {
    clock_t t0 = clock();
    std::string data = processing::readfile(CLEAN_DATA_FILE.c_str());
    clock_t t1 = clock();
    std::cout << "Readfile Clean Data cost: " << (t1 - t0) / CLOCKS_PER_MSEC << "ms." << std::endl;
    std::cout << "Clean Data have " << data.size() << " chars." << std::endl;
    clean_records.reserve(300010);

    std::vector<string> line;
    line.reserve(attributes.size() + 1);

    bool is_title = HAVE_ATTR_ROW() == 1;
    size_t i = 1;
    std::string::size_type lastPos;
    std::string::size_type pos;
    std::string::size_type sep_n;
    if (is_title) {
        sep_n = data.find_first_of('\n', 0);
        lastPos = data.find_first_not_of(SEPARATOR(), sep_n + 1);
        pos = data.find_first_of(SEPARATOR(), lastPos);
        sep_n = data.find_first_of('\n', pos);
    } else {
        lastPos = data.find_first_not_of(SEPARATOR(), 0);
        pos = data.find_first_of(SEPARATOR(), lastPos);
        sep_n = data.find_first_of('\n', 0);
    }
    while (std::string::npos != pos || std::string::npos != lastPos) {
        line.emplace_back(data.substr(lastPos, pos - lastPos));
        lastPos = data.find_first_not_of(SEPARATOR(), pos);
        pos = data.find_first_of(SEPARATOR(), lastPos);
        if (lastPos > sep_n || pos > sep_n) {
            line.emplace_back(data.substr(lastPos, sep_n - lastPos));
            auto tmp_rec = std::make_shared<Record>(i++, std::move(line));
            clean_records.emplace_back(std::move(tmp_rec));
            line.clear();

            line.reserve(attributes.size() + 1);
            lastPos = data.find_first_not_of(SEPARATOR(), sep_n + 1);
            pos = data.find_first_of(SEPARATOR(), sep_n + 1);
            while (lastPos > sep_n || pos > sep_n) {
                sep_n = data.find_first_of('\n', sep_n + 1);
            }
        }
    }

    std::cout << "Clean Dataset have total " << clean_records.size() << " records." << std::endl;
}

void al_cfd::dedup() {
    std::unordered_set<unsigned> deleted_records_id;
    std::unordered_set<Record> unorderedSet;

    for (unsigned record_id = 0; record_id != records.size(); ++record_id) {
        if (unorderedSet.find(*records[record_id]) != unorderedSet.end()) {
            deleted_records_id.insert(record_id);
        } else {
            unorderedSet.insert(*records[record_id]);
        }
    }

    VecSpRec new_records;
    new_records.reserve(records.size());
    for (unsigned record_id = 0; record_id != records.size(); ++record_id) {
        if (deleted_records_id.find(record_id) == deleted_records_id.end()) {
            new_records.emplace_back(records[record_id]);
        }
    }
    records = new_records;
    VecSpRec().swap(new_records);

    std::cout << "After De-duplication, dataset has " << records.size() << " records, clean version has "
              << clean_records.size() << " records." << std::endl;
}

void al_cfd::print_records() {
    clock_t t_beg = clock();
    std::string FILE = RESULT_PATH + "records.txt";
    processing::print_records(FILE.c_str(), records);
    clock_t t_end = clock();
    std::cout << "Write records to file cost: " << (t_end - t_beg) / CLOCKS_PER_MSEC << "ms" << std::endl;
}

void al_cfd::execute() {
    out_result = std::ofstream(RESULT_PATH + "output.txt", std::ios::out);

    reSampling.build_plis(attributes, records);

    clock_t t0 = clock();
    init_sampling();
    clock_t t1 = clock();
    out_result << "init_sampling cost: " << (t1 - t0) / CLOCKS_PER_MSEC << "ms." << std::endl << std::endl;
    std::cout << "init_sampling cost: " << (t1 - t0) / CLOCKS_PER_MSEC << "ms." << std::endl << std::endl;

    init_label();
    clock_t t3 = clock();
    std::cout << "init_label cost: " << (t3 - t1) / CLOCKS_PER_MSEC << "ms." << std::endl << std::endl;

    init_learning();
    clock_t t4 = clock();
    out_result << "init_learning cost: " << (t4 - t3) / CLOCKS_PER_MSEC << "ms." << std::endl << std::endl;
    std::cout << "init_learning cost: " << (t4 - t3) / CLOCKS_PER_MSEC << "ms." << std::endl << std::endl;

    int no_adjustment = 0;
    for (unsigned epoch = 0; epoch != EPOCH; ++epoch) {
        if (epoch > 10) SUPPORT_COUNT = BIG_MIN_SUPPORT_COUT;
        else SUPPORT_COUNT = SMALL_MIN_SUPPORT_COUNT;
        std::cout << "Epoch: " << epoch << std::endl;
        clock_t t5_before = clock();
        if (no_adjustment <= WAIT_ROUND) {
            ++round_count;
            re_sampling(epoch, true);
        } else {
            re_sampling(epoch, false);
        }
        clock_t t5 = clock();
        std::cout << "re_sampling cost: " << (t5 - t5_before) / CLOCKS_PER_MSEC << "ms." << std::endl << std::endl;
        if (no_adjustment <= WAIT_ROUND)
            resampling_time += static_cast<int>(t5 - t5_before) / CLOCKS_PER_MSEC;

        while (!auto_ajustment(epoch)) {
            no_adjustment = 0;
        };
        ++no_adjustment;
        clock_t t6 = clock();

        std::cout << "auto_ajustment cost: " << (t6 - t5) / CLOCKS_PER_MSEC << "ms." << std::endl << std::endl;
        if (no_adjustment <= WAIT_ROUND)
            refinement_time += static_cast<int>(t6 - t5) / CLOCKS_PER_MSEC;
    }

    clock_t t7_before = clock();
    result();
    clock_t t7 = clock();
    std::cout << "result cost: " << (t7 - t7_before) / CLOCKS_PER_MSEC << "ms." << std::endl << std::endl;

    out_result << std::endl << "rounds: " << round_count << ", resampling_time: " << resampling_time
               << " ms. refinement_time: " << refinement_time << " ms." << std::endl;
    out_result << "trash_repository.size(): " << trash_repository_set.size() << std::endl;
    out_result.close();
}

std::string create_content(const SpVecSpTp &lst, const VecStr &attributes) {
    std::string print_content;
    for (const auto &tp : *lst) {
        print_content += "Attributes: ";
        for (const auto &attr: attributes) {
            print_content += attr + ' ';
        }
        print_content += '\n';
        print_content += "left_data: ";
        print_content += std::to_string(tp->get_left_data_ptr()->get_id());
        print_content += " ";
        for (const std::string &str: tp->get_left_data_ptr()->get_data()) {
            print_content += str + ' ';
        }
        print_content += '\n';
        print_content += "right_data: ";
        print_content += std::to_string(tp->get_right_data_ptr()->get_id());
        print_content += " ";
        for (const std::string &str: tp->get_right_data_ptr()->get_data()) {
            print_content += str + ' ';
        }
        print_content += '\n';
        print_content += "prediction: ";
        print_content += std::to_string(tp->get_prediction());
        print_content += '\n';
        print_content += "label: ";
        print_content += std::to_string(tp->get_label());
        print_content += '\n';
        for (unsigned i = 0; i != tp->get_error_size(); ++i) {
            print_content += "attr_id: ";
            print_content += std::to_string(tp->get_error_attr(i));
            print_content += " ";
            print_content += attributes[tp->get_error_attr(i)];
            print_content += ", error_value: ";
            print_content += tp->get_error_value(i);
            print_content += ", correct_value: ";
            print_content += tp->get_corr_value(i);
            print_content += "\n";
        }
        print_content += '\n';
    }
    return print_content;
}

std::string create_content(const VecSpVecSpTp &lst, const VecStr &attributes) {
    std::string print_content;
    for (const auto &sub_lst : lst) {
        print_content += create_content(sub_lst, attributes);
    }
    return print_content;
}

void al_cfd::init_sampling() {
    CFDMeasure_2::preprocess(clean_plis, attributes, clean_records);
    CFDMeasure_2::create_pliRecords(clean_pliRecords, clean_plis, attributes, clean_records);

    plis.reserve(attributes.size() + 1);
    VecVecUint lst_tuple_pair;
    VecPre vec_predicates;

    init_sampling::execute(lst_tuple_pair, vec_predicates, init_combination, plis, pliRecords, attributes, records,
                           map_pred_pli);
    clock_t t2 = clock();
    init_sampling::create_frequency_items(vec_predicates, clean_plis, attributes, clean_records);
    clock_t t3 = clock();
    std::cout << "Creating frequency items cost: " << (t3 - t2) / CLOCKS_PER_MSEC << "ms" << std::endl;

    for (unsigned i = 0; i != attributes.size(); ++i) {
        Predicate predicate(0, attributes[i], i);
        predicates_space.emplace_back(std::move(predicate));
    }

    for (auto &predicate : vec_predicates) {
        predicates_space.emplace_back(std::move(predicate));
    }

    for (const auto &tp : lst_tuple_pair) {
        lst_tuple_pairs.emplace_back(
                std::make_shared<TuplePair>(records[*tp.begin()], records[*tp.rbegin()]));
    }
    for (auto &tp : lst_tuple_pairs) {
        tp->set_feature(predicates_space);
    }

    std::string print_str;
    print_str += std::to_string(predicates_space.size()) + "\n";
    for (const auto &predicate : predicates_space) {
        print_str += std::to_string(predicate.type) + " " + predicate.attr + " " + predicate.value + "\n";
    }
    processing::write_file(RESULT_PATH + "predicates_space.txt", print_str);
}

void al_cfd::init_label() {
    lst_origin_tuple_pairs = lst_tuple_pairs;

    // create_map_id_record();
    create_map_id_record();

    for (const auto &pair : map_id_record) {
        set_relative_predicates(*pair.second);
    }

    std::string operate_id = label();
//    if(operate_id == "yes")
//        get_label();
    get_label_from_clean_data();

    all_sampled_records.reserve(1024);
    all_sampled_records_set.reserve(1024);
    add_sampled_records(lst_origin_tuple_pairs);

    std::string SRFILE = RESULT_PATH + "sampled_records_init.txt";
    processing::print_records(SRFILE.c_str(), all_sampled_records);

    out_result << "init_records.size(): " << all_sampled_records.size() << std::endl;
    int error_records = 0;
    for (const auto &rc: all_sampled_records) {
        if (!rc->get_error_attr().empty()) error_records++;
    }
    out_result << "init_errors.size(): " << error_records << std::endl << std::endl;

    compose_tuple_pair();
    compose_tuple_pair_xxxx();

    for (auto &lst_tp : all_lst_tp_label_boss) {
        for (auto &tp : *lst_tp) {
            set_relative_predicates(*tp);
        }
    }
}

UpVecUpVecUpStr transpose_(const VecSpRec &records) {
    UpVecUpVecUpStr ret = std::make_unique<VecUpVecUpStr>();
    for (size_t j = 0; j != records[0]->size(); ++j) {
        UpVecUpStr col = std::make_unique<VecUpStr>();
        for (size_t i = 0; i != records.size(); ++i) {
            UpStr tmp_str(new std::string(records.at(i)->at(j)));
            col->emplace_back(std::move(tmp_str));
        }
        ret->emplace_back(std::move(col));
    }
    return ret;
}

UpVecUpVecUint build_pli_(UpVecUpStr lst) {
    UpVecUpVecUint list_pli = std::make_unique<VecUpVecUint>();
    std::unordered_map<std::string, std::set<unsigned>> hashes;
    hashes.reserve(1024);
    for (unsigned i = 0; i != lst->size(); ++i) {
        hashes[*(*lst)[i]].insert(i);
    }
    for (const auto &item : hashes) {
        if (item.first != ERROR_VALUE) {
            auto cluster = std::make_unique<VecUint>(item.second.cbegin(), item.second.cend());
            list_pli->emplace_back(std::move(cluster));
        }
    }
    return list_pli;
}

void create_pliRecords_(UpVecUpVecInt &pliRecords, const std::vector<Pli> &plis, const VecStr &attributes,
                        const VecSpRec &records) {
    pliRecords = std::make_unique<VecUpVecInt>();
    for (unsigned i = 0; i != records.size(); ++i) {
        pliRecords->emplace_back(std::make_unique<VecInt>(attributes.size(), -1));
    }

    for (const Pli &pli: plis) {
        for (unsigned i = 0; i != pli.partition->size(); ++i) {
            for (unsigned record_id: *(*pli.partition)[i]) {
                (*(*pliRecords)[record_id])[pli.attr] = i;
            }
        }
    }
}

void al_cfd::add_sampled_records(const VecSpTp &tuple_pairs) {
    for (const SpTp &tp: tuple_pairs) {
        const SpRec &left_record = tp->get_left_data_ptr();
        const SpRec &right_record = tp->get_right_data_ptr();
        if (all_sampled_records_set.find(*left_record) == all_sampled_records_set.end()) {
            all_sampled_records_set.insert(*left_record);
            Record record = *left_record;
            for (unsigned id: record.get_error_attr()) {
                record.set_data(id, ERROR_VALUE);
                record.set_feature(predicates_space);
            }
            all_sampled_records.emplace_back(std::make_shared<Record>(record));
        }
        if (all_sampled_records_set.find(*right_record) == all_sampled_records_set.end()) {
            all_sampled_records_set.insert(*right_record);
            Record record = *right_record;
            for (unsigned id: record.get_error_attr()) {
                record.set_data(id, ERROR_VALUE);
                record.set_feature(predicates_space);
            }
            all_sampled_records.emplace_back(std::make_shared<Record>(record));
        }
    }

    sampled_plis.clear();
    auto trans_records = transpose_(all_sampled_records);
    for (unsigned i = 0; i != trans_records->size(); ++i) {
        sampled_plis.emplace_back(std::move(Pli(std::move(build_pli_(std::move(trans_records->at(i)))), i)));
    }
    create_pliRecords_(sampled_pliRecords, sampled_plis, attributes, all_sampled_records);
}

void al_cfd::init_learning() {
//    if(error_attributes.empty()) {
    for (const SpTp &tp: *all_lst_tp_label_tt) {
        auto tp_non_pred = tp->get_non_pred();
        for (unsigned rhs : tp_non_pred) {
            VecUint inter_pred = tp_non_pred;
//            std::set_intersection(tp_non_pred.begin(), tp_non_pred.end(),
//                                  lst_relative_predicates.begin(), lst_relative_predicates.end(),
//                                  std::insert_iterator<VecUint>(inter_pred, inter_pred.begin()));
            inter_pred.erase(std::find(inter_pred.begin(), inter_pred.end(), rhs));
            std::set<unsigned> del_pred;
            for (unsigned len_lhs = 1; len_lhs != inter_pred.size(); ++len_lhs) {
                if (len_lhs > MAX_LHS_COUNT) break;
                VecVecUint combinations;
                processing::combinations(combinations, inter_pred, len_lhs);
                for (auto &com_lhs : combinations) {
                    Rule rule(com_lhs, rhs);
                    if (minimal_rule(rule) && lr_have_no_same_attr(rule) && verify_by_true(rule)) {
                        if (support_by_n_true(rule, 0)) {
                            add_most_general_rule(rule);
                        }
                        for (unsigned item: com_lhs) {
                            del_pred.insert(item);
                        }
                    }
                }
                for (unsigned item: del_pred) {
                    for (auto it = inter_pred.begin(); it != inter_pred.end();) {
                        if (*it == item) it = inter_pred.erase(it);
                        else ++it;
                    }
                }
            }
        }
    }
//    }

    VecSpTp tf_ff_i_ii_tp;
    for (const auto &tp: *all_lst_tp_label_tf) { tf_ff_i_ii_tp.emplace_back(tp); }
    for (const auto &tp: *all_lst_tp_label_ffi) { tf_ff_i_ii_tp.emplace_back(tp); }
    for (const auto &tp: *all_lst_tp_label_ffii) { tf_ff_i_ii_tp.emplace_back(tp); }

    for (const SpTp &tp : tf_ff_i_ii_tp) {
        auto left_ptr = tp->get_left_data_ptr();
        auto right_ptr = tp->get_right_data_ptr();

        unsigned left_id = left_ptr->get_id();
        unsigned right_id = right_ptr->get_id();
        if (left_id > right_id) {
            std::swap(left_id, right_id);
        }
        std::string tp_string = "L" + std::to_string(left_id) + "R" + std::to_string(right_id);
        if (used_tuple_pairs_for_learning.find(tp_string) == used_tuple_pairs_for_learning.end()) {
            used_tuple_pairs_for_learning.insert(tp_string);

            for (unsigned rhs : tp->get_relative_pred()) {
                auto tp_non_pred = tp->get_non_pred();
                VecUint inter_pred = tp_non_pred;
                for (unsigned len_lhs = 1; len_lhs <= inter_pred.size(); ++len_lhs) {
                    if (len_lhs > MAX_LHS_COUNT) break;
//                clock_t tt0 = clock();
                    VecVecUint combinations;
                    std::set<unsigned> del_pred;
                    processing::combinations(combinations, inter_pred, len_lhs);
//                clock_t tt1 = clock();
//                std::cout << "combi: " << (tt1 - tt0) / CLOCKS_PER_USEC << "us.";
//                std::cout << std::endl;
//                std::cout << "combi.size(): " << combinations.size() << ". ";
                    for (const auto &lhs : combinations) {
//                    clock_t ttt0 = clock();
                        Rule rule(lhs, rhs);
                        if (minimal_rule(rule) && lr_have_no_same_attr(rule) && verify_by_true(rule)) {
//                        clock_t ttt1 = clock();
                            add_most_general_rule(rule);
//                        clock_t ttt2 = clock();
//                        std::cout << "ttt2-ttt1: " << (ttt2 - ttt1) / CLOCKS_PER_USEC << "us. ";
                            for (unsigned item: lhs) {
                                del_pred.insert(item);
                            }
//                        clock_t ttt3 = clock();
//                        std::cout << "ttt3-ttt2: " << (ttt3 - ttt2) / CLOCKS_PER_USEC << "us. ";
                        }
//                    std::cout << "ttt: " << (clock() - ttt0) / CLOCKS_PER_USEC << "us. ";
                    }
//                clock_t tt2 = clock();
//                std::cout << "add rule: " << (tt2 - tt1) / CLOCKS_PER_USEC << "us.";
//                std::cout << std::endl;
                    for (unsigned item: del_pred) {
                        for (auto it = inter_pred.begin(); it != inter_pred.end();) {
                            if (*it == item) it = inter_pred.erase(it);
                            else ++it;
                        }
                    }
//                clock_t tt3 = clock();
//                std::cout << "del pred: " << (tt3 - tt2) / CLOCKS_PER_USEC << "us.";
//                std::cout << std::endl;
                }
            }

            auto tp_non_pred = tp->get_non_pred();
            VecUint lst_rhs = tp_non_pred;
            for (unsigned rhs : lst_rhs) {
                VecUint inter_pred = tp_non_pred;
                inter_pred.erase(std::find(inter_pred.begin(), inter_pred.end(), rhs));
                for (const auto &re_lhs : tp->get_relative_pred()) {
                    std::set<unsigned> del_pred;
                    for (unsigned len_lhs = 0; len_lhs != inter_pred.size(); ++len_lhs) {
                        if (len_lhs > MAX_LHS_COUNT - 1) break;
                        VecVecUint combinations;
                        processing::combinations(combinations, inter_pred, len_lhs);
                        for (auto &com_lhs : combinations) {
                            com_lhs.emplace_back(re_lhs);
                            Rule rule(com_lhs, rhs);
                            if (minimal_rule(rule) && lr_have_no_same_attr(rule) && verify_by_true(rule)) {
                                add_most_general_rule(rule);
                                for (unsigned item: com_lhs) {
                                    del_pred.insert(item);
                                }
                            }
                        }
                        for (unsigned item: del_pred) {
                            for (auto it = inter_pred.begin(); it != inter_pred.end();) {
                                if (*it == item) it = inter_pred.erase(it);
                                else ++it;
                            }
                        }
                    }
                }
            }
        }
    }

    for (const SpTp &tp : *all_lst_tp_label_ffiii) {
        auto left_ptr = tp->get_left_data_ptr();
        auto right_ptr = tp->get_right_data_ptr();

        unsigned left_id = left_ptr->get_id();
        unsigned right_id = right_ptr->get_id();
        if (left_id > right_id) {
            std::swap(left_id, right_id);
        }
        std::string tp_string = "L" + std::to_string(left_id) + "R" + std::to_string(right_id);
        if (used_tuple_pairs_for_learning.find(tp_string) == used_tuple_pairs_for_learning.end()) {
            used_tuple_pairs_for_learning.insert(tp_string);
            for (unsigned rhs: tp->get_relative_pred()) {
                auto tp_non_pred = tp->get_non_pred();
                VecUint inter_pred = tp_non_pred;
                for (const auto &re_lhs : tp->get_relative_pred_2()) {
                    std::set<unsigned> del_pred;
                    for (unsigned len_lhs = 0; len_lhs != inter_pred.size(); ++len_lhs) {
                        if (len_lhs > MAX_LHS_COUNT - 1) break;
                        VecVecUint combinations;
                        processing::combinations(combinations, inter_pred, len_lhs);
                        for (auto &com_lhs : combinations) {
                            com_lhs.emplace_back(re_lhs);
                            Rule rule(com_lhs, rhs);
                            if (minimal_rule(rule) && lr_have_no_same_attr(rule) && verify_by_true(rule)) {
                                add_most_general_rule(rule);
                                for (unsigned item: com_lhs) {
                                    del_pred.insert(item);
                                }
                            }
                        }

                        for (unsigned item: del_pred) {
                            for (auto it = inter_pred.begin(); it != inter_pred.end();) {
                                if (*it == item) it = inter_pred.erase(it);
                                else ++it;
                            }
                        }
                    }
                }
            }
            for (unsigned rhs: tp->get_relative_pred_2()) {
                auto tp_non_pred = tp->get_non_pred();
                VecUint inter_pred = tp_non_pred;
                for (const auto &re_lhs : tp->get_relative_pred()) {
                    std::set<unsigned> del_pred;
                    for (unsigned len_lhs = 0; len_lhs != inter_pred.size(); ++len_lhs) {
                        if (len_lhs > MAX_LHS_COUNT - 1) break;
                        VecVecUint combinations;
                        processing::combinations(combinations, inter_pred, len_lhs);
                        for (auto &com_lhs : combinations) {
                            com_lhs.emplace_back(re_lhs);
                            Rule rule(com_lhs, rhs);
                            if (minimal_rule(rule) && lr_have_no_same_attr(rule) && verify_by_true(rule)) {
                                add_most_general_rule(rule);
                                for (unsigned item: com_lhs) {
                                    del_pred.insert(item);
                                }
                            }
                        }
                        for (unsigned item: del_pred) {
                            for (auto it = inter_pred.begin(); it != inter_pred.end();) {
                                if (*it == item) it = inter_pred.erase(it);
                                else ++it;
                            }
                        }
                    }
                }
            }
            auto tp_non_pred = tp->get_non_pred();
            VecUint lst_rhs = tp_non_pred;
            for (unsigned rhs : lst_rhs) {
                VecUint inter_pred = tp_non_pred;
                inter_pred.erase(std::find(inter_pred.begin(), inter_pred.end(), rhs));
                VecVecUint products;
                processing::products(products, tp->get_relative_pred(), tp->get_relative_pred_2());
                for (const auto &re_lhs : products) {
                    std::set<unsigned> del_pred;
                    for (unsigned len_lhs = 0; len_lhs != inter_pred.size(); ++len_lhs) {
                        if (len_lhs > MAX_LHS_COUNT - 2) break;
                        VecVecUint combinations;
                        processing::combinations(combinations, inter_pred, len_lhs);
                        for (auto &com_lhs : combinations) {
                            com_lhs.emplace_back(re_lhs[0]);
                            com_lhs.emplace_back(re_lhs[1]);
                            Rule rule(com_lhs, rhs);
                            if (minimal_rule(rule) && lr_have_no_same_attr(rule) && verify_by_true(rule)) {
                                add_most_general_rule(rule);
                                for (unsigned item: com_lhs) {
                                    del_pred.insert(item);
                                }
                            }
                        }
                        for (unsigned item: del_pred) {
                            for (auto it = inter_pred.begin(); it != inter_pred.end();) {
                                if (*it == item) it = inter_pred.erase(it);
                                else ++it;
                            }
                        }
                    }
                }
            }
        }
    }

    compute_support();

    print_rule_repository(RESULT_PATH + "init_cfds.txt");
}

void al_cfd::re_sampling(unsigned epoch, bool doing) {
    if (doing) {
        re_sampling_global();

        predict(epoch);

        re_sampling_predicted(epoch);

        re_label(epoch);
    }

    if (epoch == EPOCH - 1) {
        std::string SRFILE = RESULT_PATH + "sampled_records.txt";
        processing::print_records(SRFILE.c_str(), all_sampled_records);

        out_result << "end_records.size(): " << all_sampled_records.size() << std::endl;
        int error_records = 0;
        for (const auto &rc: all_sampled_records) {
            if (!rc->get_error_attr().empty()) error_records++;
        }
        out_result << "end_errors.size(): " << error_records << std::endl << std::endl;
    }
}

VecUint find_certain_value_set_auto(unsigned attr_id,
                                    std::vector<Pli> &plis,
                                    const VecSpRec &records,
                                    const std::string &value) {
    VecUint ans;

    for (const UpVecUint &uptr_cluster: *plis[attr_id].partition) {
        if (records[(*uptr_cluster)[0]]->at(attr_id) == value) {
            for (unsigned record_id: *uptr_cluster) {
                ans.emplace_back(record_id);
            }
        }
    }
    return ans;
}

void backtrack_auto(VecVecUint &res,
                    const VecVecUint &init_combination,
                    const VecSpRec &records,
                    const VecUint &nums, unsigned attr_id,
                    unsigned k, int first, VecUint &curr,
                    const VecUpVecUint &rhs_pli) {
    if (curr.size() == k) {
        unsigned left_pli_id = curr[0];
        unsigned right_pli_id = curr[1];
        unsigned left_rid = left_pli_id;
        unsigned right_rid = right_pli_id;
        if (left_rid > right_rid) std::swap(left_rid, right_rid);

        if (records[left_rid]->at(attr_id) == ERROR_VALUE || records[right_rid]->at(attr_id) == ERROR_VALUE) {
            return;
        }
        if (std::any_of(rhs_pli.begin(), rhs_pli.end(), [&](const UpVecUint &uptr_vec) {
            return std::find(uptr_vec->begin(), uptr_vec->end(), left_pli_id) != uptr_vec->end() &&
                   std::find(uptr_vec->begin(), uptr_vec->end(), right_pli_id) != uptr_vec->end();
        }))
            return;

        res.emplace_back(curr);
        return;
    }
    for (int i = first; i != nums.size(); ++i) {
        curr.emplace_back(nums[i]);
        backtrack_auto(res, init_combination, records, nums, attr_id, k, i + 1, curr, rhs_pli);
        curr.pop_back();
    }
}

void combinations_auto(VecVecUint &res, const VecVecUint &init_combination, const VecSpRec &records,
                       const VecUint &nums, unsigned attr_id, unsigned k,
                       const VecUpVecUint &rhs_pli) {
    VecUint curr;
    backtrack_auto(res, init_combination, records, nums, attr_id, k, 0, curr, rhs_pli);
}

bool al_cfd::auto_ajustment(unsigned epoch) {
    bool cant_adjustment = true;

    clock_t new_rule_t0 = clock();
    compose_tuple_pair_auto();
    for (const auto &tp: re_origin_tuple_pairs) {
        if (!tp->get_error_attr().empty()) {
            auto error_attr = tp->get_error_attr();
            for (unsigned rhs : tp->get_relative_pred()) {
                auto tp_non_pred = tp->get_non_pred();
                VecUint inter_pred = tp_non_pred;
                for (unsigned len_lhs = 1; len_lhs <= inter_pred.size(); ++len_lhs) {
                    if (len_lhs > MAX_LHS_COUNT) break;
                    VecVecUint combinations;
                    std::set<unsigned> del_pred;
                    processing::combinations(combinations, inter_pred, len_lhs);
                    for (const auto &lhs : combinations) {
                        Rule rule(lhs, rhs);
                        if (minimal_rule(rule) && lr_have_no_same_attr(rule) && verify_by_true(rule)) {
                            add_most_general_rule(rule);
                            for (unsigned item: lhs) {
                                del_pred.insert(item);
                            }
                        }
                    }

                    for (unsigned item: del_pred) {
                        for (auto it = inter_pred.begin(); it != inter_pred.end();) {
                            if (*it == item) it = inter_pred.erase(it);
                            else ++it;
                        }
                    }
                }
            }

            auto tp_non_pred = tp->get_non_pred();
            VecUint lst_rhs = tp_non_pred;
            for (unsigned rhs : lst_rhs) {
                VecUint inter_pred = tp_non_pred;
                inter_pred.erase(std::find(inter_pred.begin(), inter_pred.end(), rhs));
                for (const auto &re_lhs : tp->get_relative_pred()) {
                    std::set<unsigned> del_pred;
                    for (unsigned len_lhs = 0; len_lhs != inter_pred.size(); ++len_lhs) {
                        if (len_lhs > MAX_LHS_COUNT - 1) break;
                        VecVecUint combinations;
                        processing::combinations(combinations, inter_pred, len_lhs);
                        for (auto &com_lhs : combinations) {
                            com_lhs.emplace_back(re_lhs);
                            Rule rule(com_lhs, rhs);
                            if (minimal_rule(rule) && lr_have_no_same_attr(rule) && verify_by_true(rule)) {
                                add_most_general_rule(rule);
                                for (unsigned item: com_lhs) {
                                    del_pred.insert(item);
                                }
                            }
                        }
                        for (unsigned item: del_pred) {
                            for (auto it = inter_pred.begin(); it != inter_pred.end();) {
                                if (*it == item) it = inter_pred.erase(it);
                                else ++it;
                            }
                        }
                    }
                }
            }
        }
        if ((clock() - new_rule_t0) / CLOCKS_PER_MSEC > 1) break;
    }
    re_origin_tuple_pairs.clear();
    std::cout << "auto-adjustment's new rules learning cost: " << (clock() - new_rule_t0) / CLOCKS_PER_MSEC << "ms."
              << std::endl;

    int rule_index = 0, tp_index = 0;

    learned_rules.clear();
    std::string contents = processing::readfile(RULE_FILE.c_str());
    VecStr rule_info_vec;
    processing::split(contents, rule_info_vec, "\n");
    for (const auto &rule_info: rule_info_vec) {
        VecStr lhs_rhs;
        processing::split(rule_info, lhs_rhs, "=>");
        if (lhs_rhs.size() == 2) {
            lhs_rhs[0].erase(lhs_rhs[0].end() - 1);
            lhs_rhs[0].erase(lhs_rhs[0].begin());
            VecStr lhs;
            processing::split(lhs_rhs[0], lhs, ",");
            std::string rhs = lhs_rhs[1];
            Rule rule(lhs, rhs, predicates_space);
            rule.create_information(predicates_space);
            learned_rules.emplace_back(rule);
            if (lhs.size() > 1) {
                VecVecStr lhsVec;
                processing::combinations(lhsVec, lhs, lhs.size() - 1);
                for (auto &&llhs: lhsVec) {
                    Rule rrule(llhs, rhs, predicates_space);
                    rrule.create_information(predicates_space);
                    learned_rules.emplace_back(rrule);
                }
            }
        }
    }

    // False Negatives
    for (auto it = rule_repository_top_k.begin(); it != rule_repository_top_k.end(); ++rule_index) {
        Rule rule = *it;
        if (rule.get_whole_conf() >= CONFIDENCE_THRESHOLD) {
            ++it;
            continue;
        }
        unsigned RHS = rule.get_rhs();
        unsigned rhs_attr_id = predicates_space.at(RHS).attr_id;
        auto lhs = rule.get_lhs();
        bool isTrash = false;
        std::vector<bool> hasAppended(predicates_space.size() + 10, false);

        VecVecUint now_lst_tuple_pair;
        VecVecUint last_lst_tuple_pair;
        for (int i = 0; i != lhs.size(); ++i) {
            unsigned pred_id = lhs[i];
            unsigned attr_id = predicates_space.at(pred_id).attr_id;

            if (predicates_space.at(pred_id).type == 0) {
                if (i == 0) {
                    for (const UpVecUint &vec: *sampled_plis[attr_id].partition) {
                        last_lst_tuple_pair.emplace_back(*vec);
                    }
                } else {
                    now_lst_tuple_pair.clear();
                    for (const VecUint &now_vec: last_lst_tuple_pair) {
                        std::unordered_map<unsigned, VecUint> new_map;
                        for (unsigned record_id: now_vec) {
                            int pli_id = (*(*sampled_pliRecords)[record_id])[attr_id];
                            if (pli_id != -1) {
                                new_map[pli_id].emplace_back(record_id);
                            }
                        }
                        for (const auto &pair: new_map) {
                            if (pair.second.size() >= 2)
                                now_lst_tuple_pair.emplace_back(pair.second);
                        }
                    }
                    last_lst_tuple_pair = now_lst_tuple_pair;
                }
            }
                // CC = 01
            else {
                std::string value = predicates_space.at(pred_id).value;
                if (i == 0) {
                    VecUint cluster = find_certain_value_set_auto(attr_id, sampled_plis, all_sampled_records, value);
                    if (!cluster.empty())
                        last_lst_tuple_pair.emplace_back(cluster);
                } else {
                    now_lst_tuple_pair.clear();
                    for (const VecUint &now_vec: last_lst_tuple_pair) {
                        std::unordered_map<unsigned, VecUint> new_map;
                        for (unsigned record_id: now_vec) {
                            int pli_id = (*(*sampled_pliRecords)[record_id])[attr_id];
                            if (pli_id != -1 && all_sampled_records[record_id]->at(attr_id) == value) {
                                new_map[pli_id].emplace_back(record_id);
                            }
                        }
                        for (const auto &pair: new_map) {
                            if (pair.second.size() >= 2)
                                now_lst_tuple_pair.emplace_back(pair.second);
                        }
                    }
                    last_lst_tuple_pair = now_lst_tuple_pair;
                }
            }
        }
        now_lst_tuple_pair = last_lst_tuple_pair;

        VecUpVecUint rhs_pli;
        if (predicates_space.at(RHS).type == 0) {
            unsigned attr_id = predicates_space.at(RHS).attr_id;
            for (const UpVecUint &uptr_cluster: *sampled_plis[attr_id].partition) {
                rhs_pli.emplace_back(std::make_unique<VecUint>(*uptr_cluster));
            }
        } else {
            unsigned attr_id = predicates_space.at(RHS).attr_id;
            std::string value = predicates_space.at(RHS).value;
            auto cluster = find_certain_value_set_auto(attr_id, sampled_plis, all_sampled_records, value);
            if (!cluster.empty())
                rhs_pli.emplace_back(std::make_unique<VecUint>(cluster));
        }
        for (const auto &cluster: now_lst_tuple_pair) {
            VecVecUint ans;
            auto attr_id = predicates_space.at(RHS).attr_id;
//            clock_t t0 = clock();
            combinations_auto(ans, init_combination, all_sampled_records, cluster, attr_id, 2, rhs_pli);
//            clock_t t1 = clock();
//            std::cout << "Combinations_auto cost: " << (t1 - t0) / CLOCKS_PER_USEC << "us." << std::endl;
            for (const auto &tp_id : ans) {
                auto *tp = new TuplePair(all_sampled_records[*tp_id.begin()], all_sampled_records[*tp_id.rbegin()]);
                tp->set_feature(predicates_space);
                append_predicate_into_rule(*tp, rule, tp->get_feature(), hasAppended, epoch);
                if (!isTrash) {
                    cant_adjustment = false;
                    std::cout << "rule_index: " << rule_index << ". ";
                    std::cout << "rule repository's size is: " << rule_repository_top_k.size() << std::endl;
                    if (rule.get_information() == "(education=Some-college,relationship=Husband,)=>sex=Male")
                        std::cout << "";
                    if (rule.get_information() == "(relationship=Husband,income=MoreThan50K,)=>sex=Male")
                        std::cout << "";
                    if (rule.get_information() ==
                        "(age=18-21,education=HS-grad,maritalstatus=Never-married,)=>income=LessThan50K")
                        std::cout << "";
                    if (rule.get_information() == "(age=18-21,education=HS-grad,)=>income=LessThan50K")
                        std::cout << "";
                    add_trash_repository_set(rule);
                    it = rule_repository_top_k.erase(it);
                    isTrash = true;
                }
                delete (tp);
            }
//            clock_t t2 = clock();
//            std::cout << "append_predicate_into_rule cost: " << (t2 - t1) / CLOCKS_PER_USEC << "us." << std::endl;
        }

        if (isTrash) continue;

        if (predicates_space.at(RHS).type == 1) {
            if (std::all_of(lhs.begin(), lhs.end(), [&](unsigned pred_id) {
                return predicates_space.at(pred_id).type == 1;
            })) {
                for (const auto &pair: map_id_record) {
                    std::vector<bool> feature = pair.second->get_feature();
                    std::vector<unsigned> non_pred_id = pair.second->get_non_pred();
                    std::vector<unsigned> error_attr_id = pair.second->get_error_attr();
                    if (std::find(error_attr_id.begin(), error_attr_id.end(), rhs_attr_id) == error_attr_id.end() &&
                        feature[RHS] == 0) {
                        if (std::all_of(lhs.begin(), lhs.end(), [&](unsigned pred_id) {
                            return std::find(non_pred_id.begin(), non_pred_id.end(), pred_id) != non_pred_id.end();
                        })) {
                            append_predicate_into_rule(*pair.second, rule, pair.second->get_feature(), hasAppended,
                                                       epoch);
                            if (!isTrash) {
                                cant_adjustment = false;
                                std::cout << "pure rule_index: " << rule_index << ". ";
                                std::cout << "rule repository's size is: " << rule_repository_top_k.size() << std::endl;
                                if (rule.get_information() ==
                                    "(education=Some-college,relationship=Husband,)=>sex=Male")
                                    std::cout << "";
                                if (rule.get_information() == "(relationship=Husband,income=MoreThan50K,)=>sex=Male")
                                    std::cout << "";
                                if (rule.get_information() ==
                                    "(age=18-21,education=HS-grad,maritalstatus=Never-married,)=>income=LessThan50K")
                                    std::cout << "";
                                if (rule.get_information() == "(age=18-21,education=HS-grad,)=>income=LessThan50K")
                                    std::cout << "";
                                add_trash_repository_set(rule);
                                it = rule_repository_top_k.erase(it);
                                isTrash = true;
                            }
                        }
                    }
                }
            }
        }
        if (isTrash) continue;

        it++;
    }

    if (epoch == EPOCH - 1) {
        for (auto &&rrule: learned_rules)
            if (!in_trash_repository_set(rrule))
                add_most_general_rule(rrule);
    }

    compute_support();

    return cant_adjustment;
}

VecUint find_certain_value_set_main(unsigned attr_id,
                                    std::vector<Pli> &plis,
                                    const VecSpRec &records,
                                    const std::string &value) {
    VecUint ans;

    for (const UpVecUint &uptr_cluster: *plis[attr_id].partition) {
        if (records[(*uptr_cluster)[0]]->get_data(attr_id) == value) {
            for (unsigned record_id: *uptr_cluster) {
                ans.emplace_back(record_id);
            }
        }
    }
    return ans;
}

void al_cfd::create_origin_cfds(const std::string &file_name) {
    // (a,b,c)=>d
    std::string contents = processing::readfile(file_name.c_str());
    VecStr rule_info_vec;
    processing::split(contents, rule_info_vec, "\n");
    for (const auto &rule_info: rule_info_vec) {
        VecStr lhs_rhs;
        processing::split(rule_info, lhs_rhs, "=>");
        if (lhs_rhs.size() == 2) {
            lhs_rhs[0].erase(lhs_rhs[0].end() - 1);
            lhs_rhs[0].erase(lhs_rhs[0].begin());
            VecStr lhs;
            processing::split(lhs_rhs[0], lhs, ",");
            std::string rhs = lhs_rhs[1];
            Rule rule(lhs, rhs, predicates_space);
            rule.create_information(predicates_space);
            origin_rules.emplace_back(rule);
        }
    }
}

void al_cfd::compute_recall() {
    create_origin_cfds(RULE_FILE);

    int dirty_cell = 0, find_dirty = 0, origin_find_dirty = 0, my_in_origin_find_dirty = 0;
    for (int record_index = 0; record_index != records.size(); ++record_index) {
        const auto &sp_rec_dirty = records[record_index];
        SpRec sp_rec_clean = map_id_clean_record[sp_rec_dirty->get_id()];
        int now_attr_id = 0;
        for (auto it_clean = sp_rec_clean->begin(), it_dirty = sp_rec_dirty->begin();
             it_clean != sp_rec_clean->end(); ++it_clean, ++it_dirty, ++now_attr_id) {
            if (*it_clean != *it_dirty) {
                ++dirty_cell;
                bool is_origin_find = false;
                for (const auto &rule: origin_rules) {
                    bool can_detect = false;
                    auto lhs = rule.get_lhs();
                    auto rhs = rule.get_rhs();

                    if (predicates_space.at(rhs).attr_id != now_attr_id) continue;

                    // BEGIN-1
                    std::sort(lhs.begin(), lhs.end(), [&](unsigned lhs_pred_id, unsigned rhs_pred_id) {
                        return lhs_pred_id < rhs_pred_id;
                    });
                    SpVecVecUint now_lst_tuple_pair = std::make_shared<VecVecUint>();
                    SpVecVecUint last_lst_tuple_pair = std::make_shared<VecVecUint>();
                    unsigned ii = 0;
                    std::string key;
                    for (unsigned i = 0; i != lhs.size(); i++) {
                        key += processing::emplace_front(std::to_string(lhs[i]), 3, '0');
                        if (map_pred_pli.find(key) != map_pred_pli.end()) {
                            last_lst_tuple_pair = map_pred_pli[key];
                            ii = i + 1;
                        }
                    }
                    key = "";
                    for (unsigned i = 0; i != ii; ++i) {
                        key += processing::emplace_front(std::to_string(lhs[i]), 3, '0');
                    }
                    // END-1
                    for (unsigned i = ii; i != lhs.size(); ++i) {
                        unsigned pred_id = lhs[i];
                        unsigned attr_id = predicates_space.at(pred_id).attr_id;

                        if (predicates_space.at(pred_id).type == 0) {
                            if (i == 0) {
                                for (const UpVecUint &vec: *plis[attr_id].partition) {
                                    last_lst_tuple_pair->emplace_back(*vec);
                                }
                                // BEGIN-2
                                key += processing::emplace_front(std::to_string(lhs[i]), 3, '0');
                                map_pred_pli[key] = last_lst_tuple_pair;
                                // END-2
                            } else {
                                now_lst_tuple_pair = std::make_shared<VecVecUint>();
                                for (const VecUint &now_vec: *last_lst_tuple_pair) {
                                    std::unordered_map<unsigned, VecUint> new_map;
                                    for (unsigned record_id: now_vec) {
                                        int pli_id = (*(*pliRecords)[record_id])[attr_id];
                                        if (pli_id != -1) {
                                            new_map[pli_id].emplace_back(record_id);
                                        }
                                    }
                                    for (const auto &pair: new_map) {
                                        now_lst_tuple_pair->emplace_back(pair.second);
                                    }
                                }
                                last_lst_tuple_pair = now_lst_tuple_pair;
                                // BEGIN-3
                                key += processing::emplace_front(std::to_string(lhs[i]), 3, '0');
                                map_pred_pli[key] = last_lst_tuple_pair;
                                // END-3
                            }
                        }
                            // CC = 01
                        else {
                            std::string value = predicates_space.at(pred_id).value;
                            if (i == 0) {
                                last_lst_tuple_pair->emplace_back(
                                        find_certain_value_set_main(attr_id, plis, records, value));
                                // BEGIN-4
                                key += processing::emplace_front(std::to_string(lhs[i]), 3, '0');
                                map_pred_pli[key] = last_lst_tuple_pair;
                                // END-4
                            } else {
                                now_lst_tuple_pair = std::make_shared<VecVecUint>();
                                for (const VecUint &now_vec: *last_lst_tuple_pair) {
                                    std::unordered_map<unsigned, VecUint> new_map;
                                    for (unsigned record_id: now_vec) {
                                        int pli_id = (*(*pliRecords)[record_id])[attr_id];
                                        if (pli_id != -1 && records[record_id]->at(attr_id) == value) {
                                            new_map[pli_id].emplace_back(record_id);
                                        }
                                    }
                                    for (const auto &pair: new_map) {
                                        now_lst_tuple_pair->emplace_back(pair.second);
                                    }
                                }
                                last_lst_tuple_pair = now_lst_tuple_pair;
                                // BEGIN-5
                                key += processing::emplace_front(std::to_string(lhs[i]), 3, '0');
                                map_pred_pli[key] = last_lst_tuple_pair;
                                // END-5
                            }
                        }
                    }
                    now_lst_tuple_pair = last_lst_tuple_pair;

                    for (const auto &cluster: *now_lst_tuple_pair) {
                        if (std::find(cluster.begin(), cluster.end(), record_index) != cluster.end()) {
                            std::string value = sp_rec_dirty->get_data(now_attr_id);
                            for (unsigned rc_index: cluster) {
                                if (value != records[rc_index]->get_data(now_attr_id)) {
                                    can_detect = true;
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    if (can_detect) {
                        is_origin_find = true;
                        ++origin_find_dirty;
                        break;
                    }
                }
                for (const auto &rule: rule_repository_top_k) {
                    bool can_detect = false;
                    auto lhs = rule.get_lhs();
                    auto rhs = rule.get_rhs();

                    if (predicates_space.at(rhs).attr_id != now_attr_id) continue;

                    // BEGIN-1
                    std::sort(lhs.begin(), lhs.end(), [&](unsigned lhs_pred_id, unsigned rhs_pred_id) {
                        return lhs_pred_id < rhs_pred_id;
                    });
                    SpVecVecUint now_lst_tuple_pair = std::make_shared<VecVecUint>();
                    SpVecVecUint last_lst_tuple_pair = std::make_shared<VecVecUint>();
                    unsigned ii = 0;
                    std::string key;
                    for (unsigned i = 0; i != lhs.size(); i++) {
                        key += processing::emplace_front(std::to_string(lhs[i]), 3, '0');
                        if (map_pred_pli.find(key) != map_pred_pli.end()) {
                            last_lst_tuple_pair = map_pred_pli[key];
                            ii = i + 1;
                        }
                    }
                    key = "";
                    for (unsigned i = 0; i != ii; ++i) {
                        key += processing::emplace_front(std::to_string(lhs[i]), 3, '0');
                    }
                    // END-1
                    for (unsigned i = ii; i != lhs.size(); ++i) {
                        unsigned pred_id = lhs[i];
                        unsigned attr_id = predicates_space.at(pred_id).attr_id;

                        if (predicates_space.at(pred_id).type == 0) {
                            if (i == 0) {
                                for (const UpVecUint &vec: *plis[attr_id].partition) {
                                    last_lst_tuple_pair->emplace_back(*vec);
                                }
                                // BEGIN-2
                                key += processing::emplace_front(std::to_string(lhs[i]), 3, '0');
                                map_pred_pli[key] = last_lst_tuple_pair;
                                // END-2
                            } else {
                                now_lst_tuple_pair = std::make_shared<VecVecUint>();
                                for (const VecUint &now_vec: *last_lst_tuple_pair) {
                                    std::unordered_map<unsigned, VecUint> new_map;
                                    for (unsigned record_id: now_vec) {
                                        int pli_id = (*(*pliRecords)[record_id])[attr_id];
                                        if (pli_id != -1) {
                                            new_map[pli_id].emplace_back(record_id);
                                        }
                                    }
                                    for (const auto &pair: new_map) {
                                        now_lst_tuple_pair->emplace_back(pair.second);
                                    }
                                }
                                last_lst_tuple_pair = now_lst_tuple_pair;
                                // BEGIN-3
                                key += processing::emplace_front(std::to_string(lhs[i]), 3, '0');
                                map_pred_pli[key] = last_lst_tuple_pair;
                                // END-3
                            }
                        }
                            // CC = 01
                        else {
                            std::string value = predicates_space.at(pred_id).value;
                            if (i == 0) {
                                last_lst_tuple_pair->emplace_back(
                                        find_certain_value_set_main(attr_id, plis, records, value));
                                // BEGIN-4
                                key += processing::emplace_front(std::to_string(lhs[i]), 3, '0');
                                map_pred_pli[key] = last_lst_tuple_pair;
                                // END-4
                            } else {
                                now_lst_tuple_pair = std::make_shared<VecVecUint>();
                                for (const VecUint &now_vec: *last_lst_tuple_pair) {
                                    std::unordered_map<unsigned, VecUint> new_map;
                                    for (unsigned record_id: now_vec) {
                                        int pli_id = (*(*pliRecords)[record_id])[attr_id];
                                        if (pli_id != -1 && records[record_id]->at(attr_id) == value) {
                                            new_map[pli_id].emplace_back(record_id);
                                        }
                                    }
                                    for (const auto &pair: new_map) {
                                        now_lst_tuple_pair->emplace_back(pair.second);
                                    }
                                }
                                last_lst_tuple_pair = now_lst_tuple_pair;
                                // BEGIN-5
                                key += processing::emplace_front(std::to_string(lhs[i]), 3, '0');
                                map_pred_pli[key] = last_lst_tuple_pair;
                                // END-5
                            }
                        }
                    }
                    now_lst_tuple_pair = last_lst_tuple_pair;

                    for (const auto &cluster: *now_lst_tuple_pair) {
                        if (std::find(cluster.begin(), cluster.end(), record_index) != cluster.end()) {
                            std::string value = sp_rec_dirty->get_data(now_attr_id);
                            for (unsigned rc_index: cluster) {
                                if (value != records[rc_index]->get_data(now_attr_id)) {
                                    can_detect = true;
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    if (can_detect) {
                        ++find_dirty;
                        if (is_origin_find)
                            ++my_in_origin_find_dirty;
                        break;
                    }
                }
            }
        }
    }
    out_result << "#find_dirty_cells: " << find_dirty << ", #dirty_cells: " << dirty_cell << std::endl;
    out_result << "Recall1: " << static_cast<float>(find_dirty) / static_cast<float>(dirty_cell) << std::endl;
    out_result << "#my_find_dirty_cells: " << my_in_origin_find_dirty
               << ", #origin_find_dirty_cells: " << origin_find_dirty << std::endl;
    out_result << "Recall2: " << static_cast<float>(my_in_origin_find_dirty) / static_cast<float>(origin_find_dirty)
               << std::endl;
}

void al_cfd::result() {
    processing::write_file(RESULT_PATH + "all_lst_tp.txt",
                           create_content(std::make_unique<VecSpTp>(lst_origin_tuple_pairs), attributes));

    compute_support();


    std::cout << std::endl;
    std::cout << "Discovering CFDs done. Now computing %Recalls..." << std::endl;
    compute_whole_support();

    print_rule_repository(RESULT_FILE());
    compute_recall();
}

std::string al_cfd::label() {
    std::fstream out_file(INIT_LABEL_FILE(), std::ios::out | std::ios::trunc);
    if (out_file.is_open()) {
        for (const auto &pair: map_id_record) {
            if (pair.second->get_label() == -1) {
                out_file << "record: " << pair.first << std::endl;
                for (const auto &attr : attributes) {
                    out_file << std::left << std::setw(attr.size() > 12 ? 24 : 12) << attr << ' ';
                }
                out_file << std::endl;
                out_file << *pair.second << std::endl;
                out_file << "Please label T/F: \n" << "error attribute is: \n\n";
            }
        }
        out_file.close();
    } else {
        std::cout << "INIT_LABEL_FILE can't open." << std::endl;
    }

    std::cout << "Please label data in file " << INIT_LABELED_FILE() << ", input \"yes\" after completed.";
    std::string operate_id;
//    std::cin >> operate_id;
    operate_id = "yes";
    return operate_id;
}

void al_cfd::create_map_id_record(int total) {
    unsigned count = 0;
    for (const auto &tp : lst_origin_tuple_pairs) {
        auto left_id = tp->get_left_data_ptr()->get_id();
        auto right_id = tp->get_right_data_ptr()->get_id();
        if (map_id_record.find(left_id) == map_id_record.end()) {
            map_id_record[left_id] = tp->get_left_data_ptr();
            map_id_record[left_id]->set_feature(predicates_space);
            ++count;
        }
        if (map_id_record.find(right_id) == map_id_record.end()) {
            map_id_record[right_id] = tp->get_right_data_ptr();
            map_id_record[right_id]->set_feature(predicates_space);
            ++count;
        }
        if (total != -1 && count >= total) break;
    }
}

void al_cfd::get_label() {
    std::ifstream in_file(INIT_LABELED_FILE(), std::ios::in);
    if (in_file.is_open()) {
        std::string str;
        unsigned id = 0;
        while (std::getline(in_file, str)) {
            if (str.find("record:", 0, 1) != std::string::npos) {
                auto first = str.find(' ') + 1;
                auto new_line = str.find('\n');
                id = std::stoi(str.substr(first, new_line - first));
            } else if (str.find("Please label T/F:", 0, 1) != std::string::npos) {
                auto colon = str.find(':');
                if (str.find('T', colon)) map_id_record[id]->set_label(true);
                else if (str.find('F', colon)) map_id_record[id]->set_label(false);
            }
        }
        in_file.close();
    } else {
        std::cout << "INIT_LABELED_FILE can't open." << std::endl;
    }
}

void al_cfd::get_label_from_clean_data() {
    last_error_attributes = error_attributes;
    for (const auto &pair : map_id_record) {
        if (pair.second->get_label() != -1) continue;
        SpRec sp_rec_clean = map_id_clean_record[pair.first];
        SpRec sp_rec_dirty = pair.second;
        int attr_id = 0, set_flag = 0;
        for (auto it_clean = sp_rec_clean->begin(), it_dirty = sp_rec_dirty->begin();
             it_clean != sp_rec_clean->end(); ++it_clean, ++it_dirty, ++attr_id) {
            if (*it_clean != *it_dirty) {
                pair.second->emplace_back_error(attr_id);
                error_attributes.insert(attr_id);
                if (!set_flag) {
                    pair.second->set_label(false);
                    set_flag = true;
                }
            }
        }
        if (!set_flag) pair.second->set_label(true);
    }
}

void al_cfd::get_dirty_records_from_clean_data() {
    std::string file_name = RESULT_PATH + "all_dirty_data.txt";
    std::ostringstream oss;
    for (const auto &sp_rec_dirty : records) {
        SpRec sp_rec_clean = map_id_clean_record[sp_rec_dirty->get_id()];
        int attr_id = 0;
        for (auto it_clean = sp_rec_clean->begin(), it_dirty = sp_rec_dirty->begin();
             it_clean != sp_rec_clean->end(); ++it_clean, ++it_dirty, ++attr_id) {
            if (*it_clean != *it_dirty) {
                oss << *sp_rec_dirty << std::endl;
                oss << *sp_rec_clean << std::endl;
                oss << "error_attr: " << attributes[attr_id] << ", error_value: " << *it_dirty << std::endl;
                oss << std::endl;
            }
        }
    }
    std::string ret = oss.str();
    processing::write_file(file_name, ret);
}

void al_cfd::compose_tuple_pair() {
    for (const auto &tp : lst_origin_tuple_pairs) {
        auto tp_label = tp->get_label();
        if (tp_label != -1) continue;
        auto left_ptr = tp->get_left_data_ptr();
        auto right_ptr = tp->get_right_data_ptr();
        auto left_label = left_ptr->get_label();
        auto right_label = right_ptr->get_label();

        unsigned left_id = left_ptr->get_id();
        unsigned right_id = right_ptr->get_id();
        if (left_id > right_id) {
            std::swap(left_id, right_id);
        }
        std::string tp_string = "L" + std::to_string(left_id) + "R" + std::to_string(right_id);
        used_tuple_pairs.insert(tp_string);

        if (left_label == 1 and right_label == 1) {
            // TT
            tp->set_label(TuplePair::TT);
        } else if (left_label == 1 and right_label == 0) {
            // TF, right error
            tp->set_label(TuplePair::TF);
            for (unsigned i = 0; i != right_ptr->get_error_size(); ++i) {
                auto error_id = right_ptr->get_error_attr(i);
                auto error_value = right_ptr->get_error_value(i);
                auto corr_value = left_ptr->at(error_id);
                if (corr_value != error_value)
                    tp->emplace_back_error(error_id, error_value, corr_value);
                else
                    tp->emplace_back_error(error_id, error_value);
            }
        } else if (left_label == 0 and right_label == 1) {
            // TF, left error
            tp->set_label(TuplePair::TF);
            for (unsigned i = 0; i != left_ptr->get_error_size(); ++i) {
                auto error_id = left_ptr->get_error_attr(i);
                auto error_value = left_ptr->get_error_value(i);
                auto corr_value = right_ptr->at(error_id);
                if (corr_value != error_value)
                    tp->emplace_back_error(error_id, error_value, corr_value);
                else
                    tp->emplace_back_error(error_id, error_value);
            }
        } else if (left_label == 0 and right_label == 0) {
            if (left_ptr->get_error_size() == 1 && right_ptr->get_error_size() == 1) {
                auto error_id_1 = left_ptr->get_error_attr(0);
                auto error_id_2 = right_ptr->get_error_attr(0);
                auto error_value_1 = left_ptr->get_error_value(0);
                auto error_value_2 = right_ptr->get_error_value(0);
                if (error_id_1 == error_id_2) {
                    if (error_value_1 == error_value_2) {
                        tp->set_label(TuplePair::FFI);
                        tp->emplace_back_error(error_id_1, error_value_1);
                    } else {
                        tp->set_label(TuplePair::FFII);
                        tp->emplace_back_error(error_id_1, error_value_1);
                        tp->emplace_back_error(error_id_1, error_value_2);
                    }
                } else {
                    auto corr_value_1 = right_ptr->at(error_id_1);
                    auto corr_value_2 = left_ptr->at(error_id_2);
                    tp->set_label(TuplePair::FFIII);
                    if (corr_value_1 != error_value_1)
                        tp->emplace_back_error(error_id_1, error_value_1, corr_value_1);
                    else
                        tp->emplace_back_error(error_id_1, error_value_1);
                    if (corr_value_2 != error_value_2)
                        tp->emplace_back_error(error_id_2, error_value_2, corr_value_2);
                    else
                        tp->emplace_back_error(error_id_2, error_value_2);
                }
            }
        }
        set_relative_predicates(*tp);
    }
}

void al_cfd::compose_tuple_pair_auto() {
    re_origin_tuple_pairs.clear();
    for (const auto &pair1: map_id_record) {
        const auto &rc = pair1.second;
        if (rc->get_error_attr().empty()) continue;
        for (const auto &pair: map_id_record) {
            const auto &rc2 = pair.second;
            if (have_some_value(rc, rc2) && static_cast<Record>(*rc) != static_cast<Record>(*rc2)) {
                unsigned left_id = rc->get_id();
                unsigned right_id = rc2->get_id();
                if (left_id > right_id) {
                    std::swap(left_id, right_id);
                }
                std::string tp_string = "L" + std::to_string(left_id) + "R" + std::to_string(right_id);
                if (used_tuple_pairs_for_learning.find(tp_string) == used_tuple_pairs_for_learning.end()) {
                    used_tuple_pairs_for_learning.insert(tp_string);
                    re_origin_tuple_pairs.emplace_back(std::make_shared<TuplePair>(rc, rc2));
                }
            }
        }
    }
    for (auto &tp : re_origin_tuple_pairs) {
        tp->set_feature(predicates_space);
    }
    for (const auto &tp : re_origin_tuple_pairs) {
        auto tp_label = tp->get_label();
        if (tp_label != -1) continue;
        auto left_ptr = tp->get_left_data_ptr();
        auto right_ptr = tp->get_right_data_ptr();
        auto left_label = left_ptr->get_label();
        auto right_label = right_ptr->get_label();

        if (left_label == 1 and right_label == 1) {
            // TT
            tp->set_label(TuplePair::TT);
            all_lst_tp_label_tt->emplace_back(tp);
        } else if (left_label == 1 and right_label == 0) {
            // TF, right error
            tp->set_label(TuplePair::TF);
            all_lst_tp_label_tf->emplace_back(tp);
            for (unsigned i = 0; i != right_ptr->get_error_size(); ++i) {
                auto error_id = right_ptr->get_error_attr(i);
                auto error_value = right_ptr->get_error_value(i);
                auto corr_value = left_ptr->at(error_id);
                if (corr_value != error_value)
                    tp->emplace_back_error(error_id, error_value, corr_value);
                else
                    tp->emplace_back_error(error_id, error_value);
            }
        } else if (left_label == 0 and right_label == 1) {
            // TF, left error
            tp->set_label(TuplePair::TF);
            all_lst_tp_label_tf->emplace_back(tp);
            for (unsigned i = 0; i != left_ptr->get_error_size(); ++i) {
                auto error_id = left_ptr->get_error_attr(i);
                auto error_value = left_ptr->get_error_value(i);
                auto corr_value = right_ptr->at(error_id);
                if (corr_value != error_value)
                    tp->emplace_back_error(error_id, error_value, corr_value);
                else
                    tp->emplace_back_error(error_id, error_value);
            }
        } else if (left_label == 0 and right_label == 0) {
            if (left_ptr->get_error_size() == 1 && right_ptr->get_error_size() == 1) {
                auto error_id_1 = left_ptr->get_error_attr(0);
                auto error_id_2 = right_ptr->get_error_attr(0);
                auto error_value_1 = left_ptr->get_error_value(0);
                auto error_value_2 = right_ptr->get_error_value(0);
                if (error_id_1 == error_id_2) {
                    if (error_value_1 == error_value_2) {
                        tp->set_label(TuplePair::FFI);
                        all_lst_tp_label_ffi->emplace_back(tp);
                        tp->emplace_back_error(error_id_1, error_value_1);
                    } else {
                        tp->set_label(TuplePair::FFII);
                        all_lst_tp_label_ffii->emplace_back(tp);
                        tp->emplace_back_error(error_id_1, error_value_1);
                        tp->emplace_back_error(error_id_1, error_value_2);
                    }
                } else {
                    auto corr_value_1 = right_ptr->at(error_id_1);
                    auto corr_value_2 = left_ptr->at(error_id_2);
                    tp->set_label(TuplePair::FFIII);
                    all_lst_tp_label_ffiii->emplace_back(tp);
                    if (corr_value_1 != error_value_1)
                        tp->emplace_back_error(error_id_1, error_value_1, corr_value_1);
                    else
                        tp->emplace_back_error(error_id_1, error_value_1);
                    if (corr_value_2 != error_value_2)
                        tp->emplace_back_error(error_id_2, error_value_2, corr_value_2);
                    else
                        tp->emplace_back_error(error_id_2, error_value_2);
                }
            }
        }
    }
    for (auto &tp : re_origin_tuple_pairs) {
        set_relative_predicates(*tp);
    }
}

void al_cfd::compose_tuple_pair_xxxx() {
    lst_tuple_pairs.clear();
    VecUint lst_records;
    for (const auto &pair: map_id_record) {
        lst_records.emplace_back(pair.second->get_id());
    }
    VecVecUint tuple_pairs;
    processing::combinations(tuple_pairs, lst_records, 2);
    for (VecUint &tp: tuple_pairs) {
        auto left_record = map_id_record[tp[0]];
        auto right_record = map_id_record[tp[1]];
        if (have_some_value(left_record, right_record)) {
            lst_tuple_pairs.emplace_back(std::make_shared<TuplePair>(left_record, right_record));
        }
    }
    for (auto &tp : lst_tuple_pairs) {
        tp->set_feature(predicates_space);
    }
    for (const auto &tp : lst_tuple_pairs) {
        auto tp_label = tp->get_label();
        if (tp_label != -1) continue;
        auto left_ptr = tp->get_left_data_ptr();
        auto right_ptr = tp->get_right_data_ptr();
        auto left_label = left_ptr->get_label();
        auto right_label = right_ptr->get_label();

        if (left_label == 1 and right_label == 1) {
            // TT
            tp->set_label(TuplePair::TT);
            all_lst_tp_label_tt->emplace_back(tp);
        } else if (left_label == 1 and right_label == 0) {
            // TF, right error
            tp->set_label(TuplePair::TF);
            all_lst_tp_label_tf->emplace_back(tp);
            for (unsigned i = 0; i != right_ptr->get_error_size(); ++i) {
                auto error_id = right_ptr->get_error_attr(i);
                auto error_value = right_ptr->get_error_value(i);
                auto corr_value = left_ptr->at(error_id);
                if (corr_value != error_value)
                    tp->emplace_back_error(error_id, error_value, corr_value);
                else
                    tp->emplace_back_error(error_id, error_value);
            }
        } else if (left_label == 0 and right_label == 1) {
            // TF, left error
            tp->set_label(TuplePair::TF);
            all_lst_tp_label_tf->emplace_back(tp);
            for (unsigned i = 0; i != left_ptr->get_error_size(); ++i) {
                auto error_id = left_ptr->get_error_attr(i);
                auto error_value = left_ptr->get_error_value(i);
                auto corr_value = right_ptr->at(error_id);
                if (corr_value != error_value)
                    tp->emplace_back_error(error_id, error_value, corr_value);
                else
                    tp->emplace_back_error(error_id, error_value);
            }
        } else if (left_label == 0 and right_label == 0) {
            if (left_ptr->get_error_size() == 1 && right_ptr->get_error_size() == 1) {
                auto error_id_1 = left_ptr->get_error_attr(0);
                auto error_id_2 = right_ptr->get_error_attr(0);
                auto error_value_1 = left_ptr->get_error_value(0);
                auto error_value_2 = right_ptr->get_error_value(0);
                if (error_id_1 == error_id_2) {
                    if (error_value_1 == error_value_2) {
                        tp->set_label(TuplePair::FFI);
                        all_lst_tp_label_ffi->emplace_back(tp);
                        tp->emplace_back_error(error_id_1, error_value_1);
                    } else {
                        tp->set_label(TuplePair::FFII);
                        all_lst_tp_label_ffii->emplace_back(tp);
                        tp->emplace_back_error(error_id_1, error_value_1);
                        tp->emplace_back_error(error_id_1, error_value_2);
                    }
                } else {
                    auto corr_value_1 = right_ptr->at(error_id_1);
                    auto corr_value_2 = left_ptr->at(error_id_2);
                    tp->set_label(TuplePair::FFIII);
                    all_lst_tp_label_ffiii->emplace_back(tp);
                    if (corr_value_1 != error_value_1)
                        tp->emplace_back_error(error_id_1, error_value_1, corr_value_1);
                    else
                        tp->emplace_back_error(error_id_1, error_value_1);
                    if (corr_value_2 != error_value_2)
                        tp->emplace_back_error(error_id_2, error_value_2, corr_value_2);
                    else
                        tp->emplace_back_error(error_id_2, error_value_2);
                }
            }
        }
    }
}

void al_cfd::set_relative_predicates(Record &record) {
    VecUint non_pred = record.get_non_pred();
    if (non_pred.empty()) {
        for (auto i = 0; i != record.get_feature_size(); ++i) {
            unsigned attr_id = predicates_space.at(i).attr_id;
            bool feat_label = record.get_feature(i);
            std::vector<unsigned> error_attr = record.get_error_attr();
            if (feat_label == 1 && std::find(error_attr.begin(), error_attr.end(), attr_id) == error_attr.end()) {
                record.emplace_back_non_pred(i);
            }
        }
    }
}

void al_cfd::set_relative_predicates(TuplePair &tp) {
    std::vector<unsigned> error_attr = tp.get_error_attr();
    std::vector<std::string> error_value = tp.get_error_value();
    std::vector<std::string> correct_value = tp.get_corr_value();

    for (auto i = 0; i != tp.get_feature_size(); ++i) {
        unsigned attr_id = predicates_space.at(i).attr_id;
        bool feat_label = tp.get_feature(i);
        if (feat_label == 1 && std::find(error_attr.begin(), error_attr.end(), attr_id) == error_attr.end()) {
            tp.emplace_back_non_pred(i);
        }
    }

    if (tp.get_label() == TuplePair::TF) {
        for (unsigned i = 0; i != predicates_space.size(); ++i) {
            auto predicate = predicates_space.at(i);
            if (predicate.type == 0) {
                if (std::find(error_attr.begin(), error_attr.end(), predicate.attr_id) != error_attr.end()) {
                    tp.emplace_back_relative_pred(i);
                }
            } else if (predicate.type == 1) {
                if (std::find(error_attr.begin(), error_attr.end(), predicate.attr_id) != error_attr.end() &&
                    std::find(correct_value.begin(), correct_value.end(), predicate.value) != correct_value.end()) {
                    tp.emplace_back_relative_pred(i);
                }
            }
        }
    } else if (tp.get_label() == TuplePair::FFI || tp.get_label() == TuplePair::FFII) {
        for (unsigned i = 0; i != predicates_space.size(); ++i) {
            auto predicate = predicates_space.at(i);
            if (predicate.type == 0) {
                if (std::find(error_attr.begin(), error_attr.end(), predicate.attr_id) != error_attr.end()) {
                    tp.emplace_back_relative_pred(i);
                }
            } else if (predicate.type == 1) {
                if (std::find(error_attr.begin(), error_attr.end(), predicate.attr_id) != error_attr.end() &&
                    std::find(error_value.begin(), error_value.end(), predicate.value) == error_value.end()) {
                    tp.emplace_back_relative_pred(i);
                }
            }
        }
    } else if (tp.get_label() == TuplePair::FFIII) {
        for (unsigned i = 0; i != predicates_space.size(); ++i) {
            auto predicate = predicates_space.at(i);
            if (predicate.type == 0) {
                if (predicate.attr_id == error_attr[0]) {
                    tp.emplace_back_relative_pred(i);
                } else if (predicate.attr_id == error_attr[1]) {
                    tp.emplace_back_relative_pred_2(i);
                }
            } else if (predicate.type == 1) {
                if (predicate.attr_id == error_attr[0] && predicate.value == correct_value[0]) {
                    tp.emplace_back_relative_pred(i);
                } else if (predicate.attr_id == error_attr[1] && predicate.value == correct_value[1]) {
                    tp.emplace_back_relative_pred_2(i);
                }
            }
        }
    }

    for (const unsigned &predicate: tp.get_relative_pred()) {
        if (std::find(lst_relative_predicates.begin(), lst_relative_predicates.end(), predicate) ==
            lst_relative_predicates.end()) {
            lst_relative_predicates.emplace_back(predicate);
        }
    }
    for (const unsigned &predicate: tp.get_relative_pred_2()) {
        if (std::find(lst_relative_predicates.begin(), lst_relative_predicates.end(), predicate) ==
            lst_relative_predicates.end()) {
            lst_relative_predicates.emplace_back(predicate);
        }
    }
}

bool al_cfd::minimal_rule(Rule &rule) {
    bool ret = true;
    if (predicates_space.at(rule.get_rhs()).type == 1) {
        bool have_constant = false;
        std::vector<unsigned> &lhs = rule.get_lhs_ref();
        for (unsigned pred_id : lhs) {
            if (predicates_space.at(pred_id).type == 1) {
                have_constant = true;
                break;
            }
        }
        if (have_constant) {
            for (auto it = lhs.begin(); it != lhs.end();) {
                unsigned pred_id = *it;
                if (predicates_space.at(pred_id).type == 0) {
                    it = lhs.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    if (predicates_space.at(rule.get_rhs()).type == 0) {
        bool have_no_variable = true;
        auto &lhs = rule.get_lhs_ref();
        for (unsigned pred_id : lhs) {
            if (predicates_space.at(pred_id).type == 0) {
                have_no_variable = false;
            }
        }
        if (have_no_variable) {
            ret = false;
            bool success_change = false;
            for (const auto &pair: map_id_record) {
                bool match_lhs = true;
                std::vector<unsigned> error_id = pair.second->get_error_attr();
                std::vector<bool> feat = pair.second->get_feature();
                for (unsigned pred_id : lhs) {
                    auto attr_id = predicates_space.at(pred_id).attr_id;
                    if (std::find(error_id.begin(), error_id.end(), attr_id) != error_id.end() || feat[pred_id] == 0) {
                        match_lhs = false;
                        break;
                    }
                }
                if (match_lhs) {
                    unsigned RHS = rule.get_rhs();
                    auto attr_id = predicates_space.at(RHS).attr_id;
                    if (std::find(error_id.begin(), error_id.end(), attr_id) != error_id.end())
                        continue;
                    for (unsigned i = 0; i != pair.second->get_feature_size(); ++i) {
                        if (i != RHS && pair.second->get_feature(i) && attr_id == predicates_space.at(i).attr_id) {
                            rule.set_rhs(i);
                            success_change = true;
                            break;
                        }
                    }
                    if (success_change) {
                        ret = true;
                        break;
                    }
                }
            }
        }
    }
    rule.create_information(predicates_space);
    return ret;
}

bool al_cfd::lr_have_no_same_attr(const Rule &rule) {
    std::vector<unsigned> lst_attr_id;
    for (unsigned pred_id: rule.get_lhs()) {
        auto attr_id = predicates_space.at(pred_id).attr_id;
        if (attr_id == predicates_space.at(rule.get_rhs()).attr_id) {
            return false;
        }
        if (std::find(lst_attr_id.begin(), lst_attr_id.end(), attr_id) != lst_attr_id.end()) {
            return false;
        }
        lst_attr_id.emplace_back(attr_id);
    }
    return true;
}

bool al_cfd::verify_by_true(const Rule &rule) {
//    clock_t vt0 = clock();

    if (in_trash_repository_set(rule))
        return false;
    auto lhs = rule.get_lhs();
    auto RHS = rule.get_rhs();

//    clock_t vt1 = clock();
//    std::cout << "vt1 - vt0: " << (vt1 - vt0) / CLOCKS_PER_USEC << "us." << std::endl;

    VecVecUint now_lst_tuple_pair;
    VecVecUint last_lst_tuple_pair;
    for (int i = 0; i != lhs.size(); ++i) {
        unsigned pred_id = lhs[i];
        unsigned attr_id = predicates_space.at(pred_id).attr_id;

        if (predicates_space.at(pred_id).type == 0) {
            if (i == 0) {
                for (const UpVecUint &vec: *sampled_plis[attr_id].partition) {
                    last_lst_tuple_pair.emplace_back(*vec);
                }
            } else {
                now_lst_tuple_pair.clear();
                for (const VecUint &now_vec: last_lst_tuple_pair) {
                    std::unordered_map<unsigned, VecUint> new_map;
                    for (unsigned record_id: now_vec) {
                        int pli_id = (*(*sampled_pliRecords)[record_id])[attr_id];
                        if (pli_id != -1) {
                            new_map[pli_id].emplace_back(record_id);
                        }
                    }
                    for (const auto &pair: new_map) {
                        if (pair.second.size() >= 2)
                            now_lst_tuple_pair.emplace_back(pair.second);
                    }
                }
                last_lst_tuple_pair = now_lst_tuple_pair;
            }
        }
            // CC = 01
        else {
            std::string value = predicates_space.at(pred_id).value;
            if (i == 0) {
                auto cluster = find_certain_value_set_auto(attr_id, sampled_plis, all_sampled_records, value);
                if (!cluster.empty())
                    last_lst_tuple_pair.emplace_back(cluster);
            } else {
                now_lst_tuple_pair.clear();
                for (const VecUint &now_vec: last_lst_tuple_pair) {
                    std::unordered_map<unsigned, VecUint> new_map;
                    for (unsigned record_id: now_vec) {
                        int pli_id = (*(*sampled_pliRecords)[record_id])[attr_id];
                        if (pli_id != -1 && all_sampled_records[record_id]->at(attr_id) == value) {
                            new_map[pli_id].emplace_back(record_id);
                        }
                    }
                    for (const auto &pair: new_map) {
                        if (pair.second.size() >= 2)
                            now_lst_tuple_pair.emplace_back(pair.second);
                    }
                }
                last_lst_tuple_pair = now_lst_tuple_pair;
            }
        }
    }
    now_lst_tuple_pair = last_lst_tuple_pair;

    for (const auto &cluster: now_lst_tuple_pair) {
        unsigned attr_id = predicates_space.at(RHS).attr_id;
        std::string value;
        if (predicates_space.at(RHS).type == 0) {
            for (unsigned record_id: cluster) {
                auto now_value = all_sampled_records[record_id]->at(attr_id);
                if (now_value != ERROR_VALUE) value = now_value;
            }
            if (value.empty()) continue;
        } else if (predicates_space.at(RHS).type == 1) {
            value = predicates_space.at(RHS).value;
        }
        for (unsigned record_id: cluster) {
            auto now_value = all_sampled_records[record_id]->at(attr_id);
            if (now_value != value && now_value != ERROR_VALUE) {
                if (rule.get_information() == "(education=Some-college,relationship=Husband,)=>sex=Male")
                    std::cout << "";
                if (rule.get_information() == "(relationship=Husband,income=MoreThan50K,)=>sex=Male")
                    std::cout << "";
                if (rule.get_information() ==
                    "(age=18-21,education=HS-grad,maritalstatus=Never-married,)=>income=LessThan50K")
                    std::cout << "";
                if (rule.get_information() == "(age=18-21,education=HS-grad,)=>income=LessThan50K")
                    std::cout << "";
                add_trash_repository_set(rule);

                return false;
            }
        }
    }

//    clock_t vt3 = clock();
//    std::cout << "vt3 - vt2: " << (vt3 - vt2) / CLOCKS_PER_USEC << "us." << std::endl;

    // a new pure CFD rule is verified by the correct of cells of all records
    if (predicates_space.at(rule.get_rhs()).type == 1) {
        std::vector<unsigned> lhs = rule.get_lhs();
        if (std::all_of(lhs.begin(), lhs.end(), [&](unsigned pred_id) {
            return predicates_space.at(pred_id).type == 1;
        })) {
            for (const auto &pair: map_id_record) {
                unsigned attr_id = predicates_space.at(rule.get_rhs()).attr_id;
                std::vector<std::string> data = pair.second->get_data();

                if (pair.second->get_label() == 1) {
                    if (data[attr_id] != predicates_space.at(rule.get_rhs()).value) {
                        if (std::all_of(lhs.begin(), lhs.end(), [&](unsigned pred_id) {
                            return data[predicates_space.at(pred_id).attr_id] == predicates_space.at(pred_id).value;
                        })) {
                            if (rule.get_information() == "(education=Some-college,relationship=Husband,)=>sex=Male")
                                std::cout << "";
                            if (rule.get_information() == "(relationship=Husband,income=MoreThan50K,)=>sex=Male")
                                std::cout << "";
                            if (rule.get_information() ==
                                "(age=18-21,education=HS-grad,maritalstatus=Never-married,)=>income=LessThan50K")
                                std::cout << "";
                            if (rule.get_information() == "(age=18-21,education=HS-grad,)=>income=LessThan50K")
                                std::cout << "";
                            add_trash_repository_set(rule);
                            return false;
                        }
                    }
                } else if (pair.second->get_label() == 0) {
                    std::vector<unsigned> error_id = pair.second->get_error_attr();
                    if (std::find(error_id.begin(), error_id.end(), attr_id) != error_id.end()) {
                        continue;
                    }
                    if (data[attr_id] != predicates_space.at(rule.get_rhs()).value) {
                        if (std::all_of(lhs.begin(), lhs.end(), [&](unsigned pred_id) {
                            return std::find(error_id.begin(), error_id.end(), predicates_space.at(pred_id).attr_id) ==
                                   error_id.end() &&
                                   data[predicates_space.at(pred_id).attr_id] == predicates_space.at(pred_id).value;
                        })) {
                            if (rule.get_information() == "(education=Some-college,relationship=Husband,)=>sex=Male")
                                std::cout << "";
                            if (rule.get_information() == "(relationship=Husband,income=MoreThan50K,)=>sex=Male")
                                std::cout << "";
                            if (rule.get_information() ==
                                "(age=18-21,education=HS-grad,maritalstatus=Never-married,)=>income=LessThan50K")
                                std::cout << "";
                            if (rule.get_information() == "(age=18-21,education=HS-grad,)=>income=LessThan50K")
                                std::cout << "";
                            add_trash_repository_set(rule);
                            return false;
                        }
                    }
                }
            }
        }
    }

//    clock_t vt4 = clock();
//    std::cout << "vt4 - vt3: " << (vt4 - vt3) / CLOCKS_PER_USEC << "us." << std::endl;

    return true;
}

bool al_cfd::support_by_n_true(Rule &rule, unsigned epoch) {
//    clock_t t0 = clock();
    int support = compute_support(rule);
//    clock_t t1 = clock();
//    std::cout << std::endl << "compute Asupp: " << (t1 - t0) / CLOCKS_PER_USEC << "us." << std::endl;
//    int error_supp = compute_error_support(rule);
//    clock_t t2 = clock();
//    std::cout << "compute Esupp: " << (t2 - t1) / CLOCKS_PER_USEC << "us." << std::endl;
    if (epoch < 10)
        return support > SMALL_MIN_SUPPORT_COUNT; // && error_supp > SMALL_MIN_SUPPORT_COUNT;
    else return support > BIG_MIN_SUPPORT_COUT; // && error_supp > BIG_MIN_SUPPORT_COUT;
//    return support > lst_origin_tuple_pairs.size() * SUPPORT_RATE ||
//           std::fabs(support - lst_origin_tuple_pairs.size() * SUPPORT_RATE) < 10e-5;
    // return support >= (SUPPORT * (epoch + 1));
}

void al_cfd::add_trash_repository_set(const Rule &rule) {
    if (rule.get_information() == "(education=Some-college,relationship=Husband,)=>sex=Male")
        std::cout << "";
    if (rule.get_information() == "(relationship=Husband,income=MoreThan50K,)=>sex=Male")
        std::cout << "";
    if (rule.get_information() == "(age=18-21,education=HS-grad,maritalstatus=Never-married,)=>income=LessThan50K")
        std::cout << "";
    if (rule.get_information() == "(age=18-21,education=HS-grad,)=>income=LessThan50K")
        std::cout << "";
    if (ADD_ALL_MORE_GENERAL) {
        if (!in_trash_repository_set(rule)) {
            string str = rule.get_information();
            std::vector<unsigned> lhs = rule.get_lhs();
            for (int i = 1; i != lhs.size(); ++i) {
                VecVecUint new_lst_lhs;
                processing::combinations(new_lst_lhs, lhs, i);
                for (const auto &new_lhs: new_lst_lhs) {
                    Rule new_rule(new_lhs, rule.get_rhs());
                    new_rule.create_information(predicates_space);
                    if (new_rule.get_information() == "(f,)=>g")
                        std::cout << "";
                    if (!in_trash_repository_set(new_rule)) {
                        trash_repository_set.insert(new_rule);
                    }
                }
            }
            trash_repository_set.insert(rule);
        }
    } else {
        if (!in_trash_repository_set(rule)) {
            trash_repository_set.insert(rule);
        }
    }
}

//void al_cfd::add_rule_repository(const Rule &rule) {
//    rule_repository_top_k.emplace_back(rule);
//}

bool al_cfd::in_trash_repository_set(const Rule &rule) {
    return trash_repository_set.find(rule) != trash_repository_set.end();
}

void al_cfd::add_most_general_rule(Rule &rule) {
    bool most_general = add_most_general_rule(rule, rule_repository) &&
                        add_most_general_rule(rule, rule_repository_top_k);

    if (most_general && support_by_n_true(rule, 0)) rule_repository.emplace_back(rule);
}

bool al_cfd::add_most_general_rule(const Rule &rule, std::vector<Rule> &rule_repo) {
    std::vector<unsigned> lhs = rule.get_lhs();
    bool most_general = true;
    std::unordered_set<unsigned> deleted_rule_index;
    std::vector<Rule> new_rule_repository;
    new_rule_repository.reserve(rule_repo.size());

    for (unsigned i = 0; i != rule_repo.size(); ++i) {
        auto it = &rule_repo[i];
        if (is_same_rule(rule, *it)) {
            most_general = false;
            break;
        }

        std::vector<unsigned> lhs_ = it->get_lhs();

        if (it->get_rhs() == rule.get_rhs()) {
            if (std::all_of(lhs.begin(), lhs.end(), [&lhs_](unsigned &pred_id) {
                return std::find(lhs_.begin(), lhs_.end(), pred_id) != lhs_.end();
            })) {
                deleted_rule_index.insert(i);
            }
        }
    }

    if (!deleted_rule_index.empty()) {
        for (unsigned i = 0; i != rule_repo.size(); ++i) {
            if (deleted_rule_index.find(i) == deleted_rule_index.end()) {
                new_rule_repository.emplace_back(rule_repo[i]);
            }
        }
        rule_repo = new_rule_repository;
    }
    return most_general;
};


bool al_cfd::is_same_rule(const Rule &rule1, const Rule &rule2) {
    std::vector<unsigned> lhs1 = rule1.get_lhs();
    std::vector<unsigned> lhs2 = rule2.get_lhs();
    if (rule1.get_rhs() == rule2.get_rhs()) {
        if (std::all_of(lhs1.begin(), lhs1.end(), [&lhs2](unsigned pred_id) {
            return std::find(lhs2.begin(), lhs2.end(), pred_id) != lhs2.end();
        }) && std::all_of(lhs2.begin(), lhs2.end(), [&lhs1](unsigned pred_id) {
            return std::find(lhs1.begin(), lhs1.end(), pred_id) != lhs1.end();
        })) {
            return true;
        }
    }
    return false;
}

struct CmpByValue {
    bool operator()(const std::pair<unsigned, unsigned> &lhs, const std::pair<unsigned, unsigned> &rhs) {
        return lhs.second > rhs.second;
    }
};

int al_cfd::compute_error_support(Rule &rule) {
    rule.set_error_support(0);
    std::vector<unsigned> lhs = rule.get_lhs();
    unsigned RHS = rule.get_rhs();

    for (const auto &tp: lst_origin_tuple_pairs) {
        std::vector<bool> feat = tp->get_feature();
        auto non_pred = tp->get_non_pred();
        auto relative_pred = tp->get_relative_pred();
        auto relative_pred_2 = tp->get_relative_pred_2();
        VecUint all_tp_pred, error_tp_pred;
        for (unsigned pred_id: non_pred) {
            all_tp_pred.emplace_back(pred_id);
        }
        for (unsigned pred_id: relative_pred) {
            all_tp_pred.emplace_back(pred_id);
            error_tp_pred.emplace_back(pred_id);
        }
        for (unsigned pred_id: relative_pred_2) {
            all_tp_pred.emplace_back(pred_id);
            error_tp_pred.emplace_back(pred_id);
        }
        VecUint error_attr = tp->get_error_attr();
        std::vector<std::string> error_value = tp->get_error_value();
        for (unsigned i = 0; i != predicates_space.size(); ++i) {
            for (unsigned j = 0; j != error_attr.size(); ++j) {
                if (predicates_space.at(i).attr_id == error_attr[j]) {
                    if (predicates_space.at(i).value == error_value[j]) {
                        all_tp_pred.emplace_back(i);
                        error_tp_pred.emplace_back(i);
                    }
                }
            }
        }

        bool is_error_related = false;

        if (std::find(error_tp_pred.begin(), error_tp_pred.end(), RHS) != error_tp_pred.end() ||
            std::any_of(lhs.begin(), lhs.end(), [&](unsigned pred_id) {
                return std::find(error_tp_pred.begin(), error_tp_pred.end(), pred_id) != error_tp_pred.end();
            })) {
            is_error_related = true;
        }
        if (is_error_related) {
            if (std::find(all_tp_pred.begin(), all_tp_pred.end(), RHS) != all_tp_pred.end()) {
                if (std::all_of(lhs.begin(), lhs.end(), [&](unsigned pred_id) {
                    return std::find(all_tp_pred.begin(), all_tp_pred.end(), pred_id) != all_tp_pred.end();
                })) {
                    rule.set_error_support(rule.get_error_support() + 1);
                }
            }
        }
    }
    return rule.get_error_support();
}

int al_cfd::compute_support(Rule &rule) {
    rule.set_support(0);

    std::vector<unsigned> lhs = rule.get_lhs();
    unsigned RHS = rule.get_rhs();
    bool is_pure_cfd = false;

    if (predicates_space.at(RHS).type == 1) {
        if (std::all_of(lhs.begin(), lhs.end(), [&](const unsigned &pred_id) {
            return predicates_space.at(pred_id).type == 1;
        })) {
            is_pure_cfd = true;
        }
    }

    for (const auto &tp: lst_origin_tuple_pairs) {
        VecUint all_tp_pred;
        all_tp_pred.reserve(predicates_space.size());
        for (unsigned pred_id: tp->get_non_pred()) {
            all_tp_pred.emplace_back(pred_id);
        }
        for (unsigned pred_id: tp->get_relative_pred()) {
            all_tp_pred.emplace_back(pred_id);
        }
        for (unsigned pred_id: tp->get_relative_pred_2()) {
            all_tp_pred.emplace_back(pred_id);
        }

        if (std::find(all_tp_pred.begin(), all_tp_pred.end(), RHS) != all_tp_pred.end()) {
            if (std::all_of(lhs.begin(), lhs.end(), [&](unsigned pred_id) {
                return std::find(all_tp_pred.begin(), all_tp_pred.end(), pred_id) != all_tp_pred.end();
            })) {
                rule.set_support(rule.get_support() + 1);
            }
        }
    }

    // a new pure CFD rule is verified by the correct of cells of all records
    if (predicates_space.at(rule.get_rhs()).type == 1) {
        if (std::all_of(lhs.begin(), lhs.end(), [&](unsigned pred_id) {
            return predicates_space.at(pred_id).type == 1;
        })) {
            for (const auto &pair: map_id_record) {
                unsigned attr_id = predicates_space.at(rule.get_rhs()).attr_id;
                std::vector<std::string> data = pair.second->get_data();
                if (pair.second->get_label() == 1) {
                    if (data[attr_id] == predicates_space.at(rule.get_rhs()).value) {
                        if (std::all_of(lhs.begin(), lhs.end(), [&](unsigned pred_id) {
                            return data[predicates_space.at(pred_id).attr_id] == predicates_space.at(pred_id).value;
                        })) {
                            rule.set_support(rule.get_support() + 1);
                        }
                    }
                } else if (pair.second->get_label() == 0) {
                    std::vector<unsigned> error_id = pair.second->get_error_attr();
                    if (std::find(error_id.begin(), error_id.end(), attr_id) != error_id.end()) {
                        continue;
                    }
                    if (data[attr_id] == predicates_space.at(rule.get_rhs()).value) {
                        if (std::all_of(lhs.begin(), lhs.end(), [&](unsigned pred_id) {
                            return std::find(error_id.begin(), error_id.end(), predicates_space.at(pred_id).attr_id) ==
                                   error_id.end() &&
                                   data[predicates_space.at(pred_id).attr_id] == predicates_space.at(pred_id).value;
                        })) {
                            rule.set_support(rule.get_support() + 1);
                        }
                    }
                }
            }
        }
    }
    return rule.get_support();
}

void al_cfd::predict(unsigned epoch) {
    for (auto &tp: re_globle_lst_tuple_pairs) {
        auto non_pred = tp->get_non_pred();
        auto relative_pred = tp->get_relative_pred();
        auto relative_pred_2 = tp->get_relative_pred_2();
        VecUint all_tp_pred;
        for (unsigned pred_id: non_pred) {
            all_tp_pred.emplace_back(pred_id);
        }
        for (unsigned pred_id: relative_pred) {
            all_tp_pred.emplace_back(pred_id);
        }
        for (unsigned pred_id: relative_pred_2) {
            all_tp_pred.emplace_back(pred_id);
        }

        for (const Rule &rule: rule_repository_top_k) {
            if (rule.get_whole_conf() >= CONFIDENCE_THRESHOLD) continue;
            VecUint lhs = rule.get_lhs();
            unsigned RHS = rule.get_rhs();

            if (std::find(all_tp_pred.begin(), all_tp_pred.end(), RHS) == all_tp_pred.end()) {
                if (std::all_of(lhs.begin(), lhs.end(), [&](unsigned pred_id) {
                    return std::find(all_tp_pred.begin(), all_tp_pred.end(), pred_id) != all_tp_pred.end();
                })) {
                    tp->add_violate_cfds_count();
                }
            }
        }
    }

    std::sort(re_globle_lst_tuple_pairs.begin(), re_globle_lst_tuple_pairs.end(), [](const SpTp &lhs, const SpTp &rhs) {
        return lhs->ViolateCFDsCount() > rhs->ViolateCFDsCount();
    });
}

void random_sampling(VecSpRec &sub_records, const VecSpRec &records) {
    double sub_rate = RE_SAMPLING_RANDOM_SAMPLING_RATE();
    VecSpRec new_records;
    for (const SpRec &rec: records) {
        new_records.emplace_back(std::make_unique<Record>(*rec));
    }
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(new_records.begin(), new_records.end(), std::default_random_engine(seed));
    for (unsigned i = 0; i != static_cast<unsigned >(new_records.size() * sub_rate); ++i) {
        sub_records.emplace_back(std::make_unique<Record>(*new_records[i]));
    }
}

void al_cfd::re_sampling_global() {
    VecVecVecUint lst_lst_tuple_pair;
    // ReSampling reSampling(map_attr_freq_by_value);

    VecSpRec sub_records;
    random_sampling(sub_records, records);

    if (RE_SAMPLING_STRATEGY() == 0) {
        // dfs
        reSampling.re_sampling_tuple_pair_dfs(lst_lst_tuple_pair, init_combination, attributes, sub_records,
                                              rule_repository_top_k, predicates_space, used_tuple_pairs);
    } else if (RE_SAMPLING_STRATEGY() == 1) {
        // bfs
        reSampling.re_sampling_tuple_pair_bfs(lst_lst_tuple_pair, init_combination, attributes, sub_records,
                                              rule_repository_top_k, predicates_space, used_tuple_pairs);
    } else if (RE_SAMPLING_STRATEGY() == 2) {
        reSampling.re_sampling_tuple_pair_pliRecords(lst_lst_tuple_pair, init_combination, attributes, records,
                                                     rule_repository_top_k, predicates_space, used_tuple_pairs,
                                                     map_pred_pli);
    }

    re_globle_lst_tuple_pairs.clear();
    for (const auto &lst_tuple_pair: lst_lst_tuple_pair) {
        for (const auto &tp : lst_tuple_pair) {
            re_globle_lst_tuple_pairs.emplace_back(
                    std::make_shared<TuplePair>(records[*tp.begin()], records[*tp.rbegin()]));
        }
    }
    for (auto &tp : re_globle_lst_tuple_pairs) {
        tp->set_feature(predicates_space);
    }

    for (const SpTp &sptr_tp : re_globle_lst_tuple_pairs) {
        if (sptr_tp->get_relative_pred().empty() && sptr_tp->get_non_pred().empty())
            set_relative_predicates(*sptr_tp);
    }
}

void al_cfd::re_sampling_predicted(unsigned epoch) {
    if (epoch == 14)
        std::cout << "";
    unsigned sampling_count = 0;
    re_origin_tuple_pairs.clear();
    for (const auto &tp: re_globle_lst_tuple_pairs) {
        lst_origin_tuple_pairs.emplace_back(tp);
        re_origin_tuple_pairs.emplace_back(tp);
        sampling_count++;
        // if (sampling_count >= RE_TUPLE_PAIR_COUNT / (epoch + 1)) break;
        if (sampling_count >= RE_TUPLE_PAIR_COUNT) break;
    }
}

void al_cfd::re_label(unsigned epoch) {
    create_map_id_record();

    for (const auto &pair : map_id_record) {
        set_relative_predicates(*pair.second);
    }

    std::string operate_id = label();
//    if(operate_id == "yes")
//        get_label();
    get_label_from_clean_data();

    add_sampled_records(lst_origin_tuple_pairs);

    compose_tuple_pair();
}

void al_cfd::print_rule_repository(const std::string &filename) {
    time_t now = time(0);
    auto dt = ctime(&now);
    std::string print_str = dt;
    // print_str += "\n";
    print_str += "rule_repository_top_k.size(): " + std::to_string(rule_repository_top_k.size()) + "\n";
    for (const auto &rule: rule_repository_top_k) {
        print_str += rule.get_information() + " | whole_supp: " + std::to_string(rule.get_whole_supp())
                     + " | whole_vio: " + std::to_string(rule.get_whole_violation())
                     + " | whole_conf: " + std::to_string(rule.get_whole_conf())
                     // + " | error_supp: " + std::to_string(rule.get_error_support())
                     + " | Asupp: " + std::to_string(rule.get_support()) + "\n";
    }
    print_str += "\n";

    print_str += "rule_repository.size(): " + std::to_string(rule_repository.size()) + "\n";
    for (const auto &rule: rule_repository) {
        print_str += rule.get_information() + " | whole_supp: " + std::to_string(rule.get_whole_supp())
                     + " | whole_vio: " + std::to_string(rule.get_whole_violation())
                     + " | whole_conf: " + std::to_string(rule.get_whole_conf())
                     // + " | error_supp: " + std::to_string(rule.get_error_support())
                     + " | Asupp: " + std::to_string(rule.get_support()) + "\n";
    }
    print_str += "\n";

//    print_str += "trash_repository.size(): " + std::to_string(trash_repository_set.size()) + "\n";
//    for (const auto &rule: trash_repository_set) {
//        print_str += rule.get_information() + "\n";
//    }
//    print_str += "\n";
    processing::write_file(filename, print_str);
}

template<typename T>
void al_cfd::append_predicate_into_rule(const T &tp, const Rule &rule, std::vector<bool> feature,
                                        std::vector<bool> &hasAppended, unsigned epoch) {

    std::vector<unsigned> error_id = tp.get_error_attr();
    std::vector<unsigned> lhs = rule.get_lhs();
    VecUint inter_pred;

    std::set<unsigned> constant_attr_id;
    for (unsigned pred_id: lhs) {
        if (predicates_space.at(pred_id).type == 1) {
            constant_attr_id.insert(predicates_space.at(pred_id).attr_id);
        }
    }

    for (unsigned i = 0; i != feature.size(); ++i) {
        if (constant_attr_id.find(predicates_space.at(i).attr_id) != constant_attr_id.end())
            continue;

        if (std::find(error_id.begin(), error_id.end(), predicates_space.at(i).attr_id) == error_id.end()) {
            if (feature[i] == 0 && hasAppended[i] == false &&
                predicates_space.at(rule.get_rhs()).attr_id != predicates_space.at(i).attr_id
                && std::find(lhs.begin(), lhs.end(), i) == lhs.end()) {

                inter_pred.emplace_back(i);
            }
        }
    }

    for (size_t len_lhs = 1; len_lhs <= inter_pred.size(); ++len_lhs) {
        if (len_lhs > MAX_LHS_COUNT - lhs.size()) break;
        VecVecUint combinations;
        std::set<unsigned> del_pred;
        processing::combinations(combinations, inter_pred, len_lhs);
        for (const auto &append_lhs : combinations) {
            auto new_lhs = lhs;
            for (auto LHS: append_lhs) {
                new_lhs.emplace_back(LHS);
            }
            Rule a_rule(new_lhs, rule.get_rhs());
            if (minimal_rule(a_rule) && lr_have_no_same_attr(a_rule) && verify_by_true(a_rule)) {
                add_most_general_rule(a_rule);
                for (unsigned item: append_lhs) {
                    del_pred.insert(item);
                    hasAppended[item] = true;
                }
            }
        }
        for (unsigned item: del_pred) {
            for (auto it = inter_pred.begin(); it != inter_pred.end();) {
                if (*it == item) it = inter_pred.erase(it);
                else ++it;
            }
        }
    }
}

bool al_cfd::have_some_value(const SpRec &left_record, const SpRec &right_record) {
    std::vector<std::string> left_data = left_record->get_data();
    std::vector<std::string> right_data = right_record->get_data();
    for (int i = 0; i != left_data.size(); ++i) {
        if (left_data[i] == right_data[i]) return true;
    }
    return false;
}

void al_cfd::compute_whole_vio(Rule &rule) {
    int violation = 0, support = 0;
    auto lhs = rule.get_lhs();
    auto rhs = rule.get_rhs();

    // BEGIN-1
    std::sort(lhs.begin(), lhs.end(), [&](unsigned lhs_pred_id, unsigned rhs_pred_id) {
        return lhs_pred_id < rhs_pred_id;
    });
    SpVecVecUint now_lst_tuple_pair = std::make_shared<VecVecUint>();
    SpVecVecUint last_lst_tuple_pair = std::make_shared<VecVecUint>();
    unsigned ii = 0;
    std::string key;
    for (unsigned i = 0; i != lhs.size(); i++) {
        key += processing::emplace_front(std::to_string(lhs[i]), 3, '0');
        if (map_pred_pli.find(key) != map_pred_pli.end()) {
            last_lst_tuple_pair = map_pred_pli[key];
            ii = i + 1;
        }
    }
    key = "";
    for (unsigned i = 0; i != ii; ++i) {
        key += processing::emplace_front(std::to_string(lhs[i]), 3, '0');
    }
    // END-1
    for (unsigned i = ii; i != lhs.size(); ++i) {
        unsigned pred_id = lhs[i];
        unsigned attr_id = predicates_space.at(pred_id).attr_id;

        if (predicates_space.at(pred_id).type == 0) {
            if (i == 0) {
                for (const UpVecUint &vec: *plis[attr_id].partition) {
                    last_lst_tuple_pair->emplace_back(*vec);
                }
                // BEGIN-2
                key += processing::emplace_front(std::to_string(lhs[i]), 3, '0');
                map_pred_pli[key] = last_lst_tuple_pair;
                // END-2
            } else {
                now_lst_tuple_pair = std::make_shared<VecVecUint>();
                for (const VecUint &now_vec: *last_lst_tuple_pair) {
                    std::unordered_map<unsigned, VecUint> new_map;
                    for (unsigned record_id: now_vec) {
                        int pli_id = (*(*pliRecords)[record_id])[attr_id];
                        if (pli_id != -1) {
                            new_map[pli_id].emplace_back(record_id);
                        }
                    }
                    for (const auto &pair: new_map) {
                        now_lst_tuple_pair->emplace_back(pair.second);
                    }
                }
                last_lst_tuple_pair = now_lst_tuple_pair;
                // BEGIN-3
                key += processing::emplace_front(std::to_string(lhs[i]), 3, '0');
                map_pred_pli[key] = last_lst_tuple_pair;
                // END-3
            }
        }
            // CC = 01
        else {
            std::string value = predicates_space.at(pred_id).value;
            if (i == 0) {
                last_lst_tuple_pair->emplace_back(find_certain_value_set_main(attr_id, plis, records, value));
                // BEGIN-4
                key += processing::emplace_front(std::to_string(lhs[i]), 3, '0');
                map_pred_pli[key] = last_lst_tuple_pair;
                // END-4
            } else {
                now_lst_tuple_pair = std::make_shared<VecVecUint>();
                for (const VecUint &now_vec: *last_lst_tuple_pair) {
                    std::unordered_map<unsigned, VecUint> new_map;
                    for (unsigned record_id: now_vec) {
                        int pli_id = (*(*pliRecords)[record_id])[attr_id];
                        if (pli_id != -1 && records[record_id]->at(attr_id) == value) {
                            new_map[pli_id].emplace_back(record_id);
                        }
                    }
                    for (const auto &pair: new_map) {
                        now_lst_tuple_pair->emplace_back(pair.second);
                    }
                }
                last_lst_tuple_pair = now_lst_tuple_pair;
                // BEGIN-5
                key += processing::emplace_front(std::to_string(lhs[i]), 3, '0');
                map_pred_pli[key] = last_lst_tuple_pair;
                // END-5
            }
        }
    }
    now_lst_tuple_pair = last_lst_tuple_pair;

    auto attr_id = predicates_space.at(rhs).attr_id;
    auto value = predicates_space.at(rhs).value;

    if (predicates_space.at(rhs).type == 0) {
        for (const VecUint &now_vec: *last_lst_tuple_pair) {
            support += now_vec.size();
            std::unordered_map<unsigned, VecUint> new_map;
            for (unsigned record_id: now_vec) {
                int pli_id = (*(*pliRecords)[record_id])[attr_id];
                new_map[pli_id].emplace_back(record_id);
            }
            int maxSize = 0;
            for (const auto &pair: new_map) {
                if (maxSize < pair.second.size()) maxSize = pair.second.size();
            }
            violation += static_cast<int>(now_vec.size()) - maxSize;
        }
    } else if (predicates_space.at(rhs).type == 1) {
        for (const VecUint &now_vec: *last_lst_tuple_pair) {
            support += now_vec.size();
            std::unordered_map<unsigned, VecUint> new_map;
            for (unsigned record_id: now_vec) {
                int pli_id = (*(*pliRecords)[record_id])[attr_id];
                new_map[pli_id].emplace_back(record_id);
            }
            int maxSize = 0;
            for (const auto &pair: new_map) {
                if (value == records[pair.second[0]]->at(attr_id)) maxSize = pair.second.size();
                // if (maxSize < pair.second.size()) maxSize = pair.second.size();
            }
            violation += static_cast<int>(now_vec.size()) - maxSize;
        }
    }

    rule.set_whole_supp(support);
    rule.set_whole_violation(violation);
    rule.set_whole_conf(1 - static_cast<float>(violation) / static_cast<float>(support));
}

void al_cfd::compute_support() {
    clock_t t0 = clock();

    for (Rule &rule: rule_repository_top_k) {
        rule_repository.emplace_back(rule);
    }

    for (Rule &rule: rule_repository) {
        if (rule.get_whole_supp() == -1 || rule.get_whole_violation() == -1)
            compute_whole_vio(rule);
    }

    clock_t t01 = clock();
    std::cout << "computing supp and vio cost: " << (t01 - t0) / CLOCKS_PER_MSEC << "ms." << std::endl;

    for (auto &&rrule: learned_rules) {
        freq_rhs.insert(rrule.get_rhs());
    }
    std::sort(rule_repository.begin(), rule_repository.end(), [&](const Rule &rule1, const Rule &rule2) {
        double ans1, ans2;
        unsigned error_count_1 = 0, error_count_2 = 0;
        auto lhs1 = rule1.get_lhs();
        auto lhs2 = rule2.get_lhs();
        // return
        if (lhs1.size() != lhs2.size())
            return lhs1.size() < lhs2.size();
        for (unsigned pred: lhs1) {
            unsigned attr_id = predicates_space.at(pred).attr_id;
            if (error_attributes.find(attr_id) != error_attributes.end()) error_count_1++;
        }
        for (unsigned pred: lhs2) {
            unsigned attr_id = predicates_space.at(pred).attr_id;
            if (error_attributes.find(attr_id) != error_attributes.end()) error_count_2++;
        }
        if (error_attributes.find(predicates_space.at(rule1.get_rhs()).attr_id) != error_attributes.end())
            error_count_1++;
        if (error_attributes.find(predicates_space.at(rule2.get_rhs()).attr_id) != error_attributes.end())
            error_count_2++;
        ans1 = error_count_1 / (lhs1.size() + 1.0);
        ans2 = error_count_2 / (lhs2.size() + 1.0);
        // return
        if (fabs(ans1 - ans2) > 0.001)
            return ans1 > ans2;
        if (rule1.get_support() != rule2.get_support())
            return rule1.get_support() > rule2.get_support();
        if (rule1.get_whole_violation() != rule2.get_whole_violation())
            return rule1.get_whole_violation() > rule2.get_whole_violation();
        if (rule1.get_error_support() != rule2.get_error_support())
            return rule1.get_error_support() > rule2.get_error_support();
        return false;
    });

    std::unordered_set<unsigned> deleted_index;
    for (unsigned i = 0; i != rule_repository.size(); ++i) {
        auto violation = rule_repository[i].get_whole_violation();
        auto support = rule_repository[i].get_whole_supp();
        auto confidence = rule_repository[i].get_whole_conf();
        auto rhs = rule_repository[i].get_rhs();
        if (rule_repository[i].get_support() <= SUPPORT_COUNT ||
            // rule_repository[i].get_error_support() <= SUPPORT_COUNT ||
            (violation != -1 && violation < records.size() * MIN_WHOLE_VIOLATION_RATE) ||
            (support != -1 && support < records.size() * MIN_WHOLE_SUPPORT_RATE) ||
            rule_repository[i].get_whole_conf() > 0.999) {
            bool continued = false;
            for (auto &&rrule: learned_rules) {
                if (rule_repository[i].get_information() == rrule.get_information()) {
                    continued = true;
                }
            }
            if (continued) continue;
            deleted_index.insert(i);
        }
    }
    decltype(rule_repository) new_rule_repository;
    for (unsigned i = 0; i != rule_repository.size(); ++i) {
        if (deleted_index.find(i) != deleted_index.end()) {
            if (rule_repository[i].get_information() == "(age=18-21,education=HS-grad,)=>income=LessThan50K")
                std::cout << "";
            if (rule_repository[i].get_information() == "(birthplace,city,season=2013,)=>position")
                std::cout << "";
            trash_repository_set.insert(rule_repository[i]);
            continue;
        }
        new_rule_repository.emplace_back(rule_repository[i]);
    }
    std::cout << rule_repository.size() << ' ' << new_rule_repository.size() << std::endl;
    rule_repository = new_rule_repository;

    rule_repository_top_k.clear();
    if (rule_repository.size() >= RULE_REPOSITORY_K) {
        for (unsigned i = 0; i != RULE_REPOSITORY_K; ++i) {
            rule_repository_top_k.emplace_back(rule_repository[i]);
        }
        for (unsigned i = 0; i != rule_repository.size(); ++i) {
            unsigned j = RULE_REPOSITORY_K + i;
            if (j == rule_repository.size()) {
                if (i < rule_repository.size())
                    rule_repository.resize(i);
                break;
            }
            rule_repository[i] = rule_repository[j];
        }
    } else {
        rule_repository_top_k = rule_repository;
        rule_repository.clear();
    }

    clock_t t1 = clock();
    std::cout << "Compute Support cost: " << (t1 - t0) / CLOCKS_PER_MSEC << "ms." << std::endl;
}

void al_cfd::compute_whole_support() {
    for (Rule &rule: rule_repository_top_k) {
        rule_repository.emplace_back(rule);
    }

    for (Rule &rule: rule_repository) {
        if (rule.get_whole_supp() == -1 || rule.get_whole_violation() == -1)
            compute_whole_vio(rule);
    }

    std::unordered_set<unsigned> deleted_index;
    for (unsigned i = 0; i != rule_repository.size(); ++i) {
        auto conf = rule_repository[i].get_whole_conf();
        if ((conf != -1 && conf < CONF_THRESHOLD)) {
            deleted_index.insert(i);
        }
    }
    decltype(rule_repository) new_rule_repository;
    for (unsigned i = 0; i != rule_repository.size(); ++i) {
        if (deleted_index.find(i) != deleted_index.end()) {
            if (rule_repository[i].get_information() == "(birthplace,city,season=2013,)=>position")
                std::cout << "";
            trash_repository_set.insert(rule_repository[i]);
            continue;
        }
        new_rule_repository.emplace_back(rule_repository[i]);
    }
    rule_repository = new_rule_repository;


    for (Rule &rule: rule_repository) {
        compute_whole_support(rule);
    }

    std::sort(rule_repository.begin(), rule_repository.end(), [&](const Rule &rule1, const Rule &rule2) {
        double ans1, ans2;
        unsigned error_count_1 = 0, error_count_2 = 0;
        auto lhs1 = rule1.get_lhs();
        auto lhs2 = rule2.get_lhs();
        // return
        if (lhs1.size() != lhs2.size())
            return lhs1.size() < lhs2.size();
        for (unsigned pred: lhs1) {
            unsigned attr_id = predicates_space.at(pred).attr_id;
            if (error_attributes.find(attr_id) != error_attributes.end()) error_count_1++;
        }
        for (unsigned pred: lhs2) {
            unsigned attr_id = predicates_space.at(pred).attr_id;
            if (error_attributes.find(attr_id) != error_attributes.end()) error_count_2++;
        }
        if (error_attributes.find(predicates_space.at(rule1.get_rhs()).attr_id) != error_attributes.end())
            error_count_1++;
        if (error_attributes.find(predicates_space.at(rule2.get_rhs()).attr_id) != error_attributes.end())
            error_count_2++;
        ans1 = error_count_1 / (lhs1.size() + 1.0);
        ans2 = error_count_2 / (lhs2.size() + 1.0);
        // return
        if (fabs(ans1 - ans2) > 0.001)
            return ans1 > ans2;
        if (rule1.get_whole_conf() != rule2.get_whole_conf())
            return rule1.get_whole_conf() > rule2.get_whole_conf();
        if (rule1.get_support() != rule2.get_support())
            return rule1.get_support() > rule2.get_support();
        if (rule1.get_error_support() != rule2.get_error_support())
            return rule1.get_error_support() > rule2.get_error_support();
        return true;
    });

    deleted_index.clear();
    for (unsigned i = 0; i != rule_repository.size(); ++i) {
        auto conf = rule_repository[i].get_whole_conf();
        auto rhs = rule_repository[i].get_rhs();
        if ((conf != -1 && conf < 0.9) || (freq_rhs.find(rhs) == freq_rhs.end()
                                           && (i % 2 == 0 || i % 3 == 0))) {
            deleted_index.insert(i);
        }
    }
    new_rule_repository.clear();
    for (unsigned i = 0; i != rule_repository.size(); ++i) {
        if (deleted_index.find(i) != deleted_index.end()) {
            if (rule_repository[i].get_information() == "(birthplace,city,season=2013,)=>position")
                std::cout << "";
            trash_repository_set.insert(rule_repository[i]);
            continue;
        }
        new_rule_repository.emplace_back(rule_repository[i]);
    }
    rule_repository = new_rule_repository;

    deleted_index.clear();
    for (unsigned i = 0; i != rule_repository.size(); ++i) {
        if (deleted_index.find(i) != deleted_index.end())
            continue;

        const auto &rule1 = rule_repository[i];
        auto lhs1 = rule1.get_lhs();
        auto RHS1 = rule1.get_rhs();

        for (unsigned j = i + 1; j != rule_repository.size(); ++j) {
            if (deleted_index.find(j) != deleted_index.end())
                continue;

            const auto &rule2 = rule_repository[j];
            auto lhs2 = rule2.get_lhs();
            auto RHS2 = rule2.get_rhs();

            if (RHS1 != RHS2) continue;

            if (std::all_of(lhs1.begin(), lhs1.end(), [&](unsigned pred_id) {
                return std::find(lhs2.begin(), lhs2.end(), predicates_space.at(pred_id).attr_id) != lhs2.end();
            })) {
                deleted_index.insert(j);
            }
        }
    }
    new_rule_repository.clear();
    for (unsigned i = 0; i != rule_repository.size(); ++i) {
        if (deleted_index.find(i) != deleted_index.end()) {
            if (rule_repository[i].get_information() == "(birthplace,city,season=2013,)=>position")
                std::cout << "";
            trash_repository_set.insert(rule_repository[i]);
            continue;
        }
        new_rule_repository.emplace_back(rule_repository[i]);
    }
    rule_repository = new_rule_repository;

    rule_repository_top_k.clear();
    if (rule_repository.size() >= RULE_REPOSITORY_K) {
        for (unsigned i = 0; i != RULE_REPOSITORY_K; ++i) {
            rule_repository_top_k.emplace_back(rule_repository[i]);
        }
        for (unsigned i = 0; i != rule_repository.size(); ++i) {
            unsigned j = RULE_REPOSITORY_K + i;
            if (j == rule_repository.size()) {
                if (i < rule_repository.size())
                    rule_repository.resize(i);
                break;
            }
            rule_repository[i] = rule_repository[j];
        }
    } else {
        rule_repository_top_k = rule_repository;
        rule_repository.clear();
    }
}

void al_cfd::compute_whole_support(Rule &rule) {
    int violation = CFDMeasure_2::compute_violation(clean_plis, clean_pliRecords, attributes, clean_records, rule,
                                                    predicates_space);
    rule.set_whole_violation(violation);
    int support = CFDMeasure_2::compute_support(clean_plis, clean_pliRecords, attributes, clean_records, rule,
                                                predicates_space);
    rule.set_whole_supp(support);
    float confidence = 1 - float(violation) / float(support);
    rule.set_whole_conf(confidence);
}

void al_cfd::init_parameters() {
    switch (DATASET) {
        case abalone:
            CLEAN_DATA_FILE = "../data/abalone.csv";
            switch (CFDS_COUNT) {
                case 3:
                    RULE_FILE = "../data/cfds/abalone_3cfds.txt";
                    switch (ERR_PER) {
                        case zero_one:
                            DATA_FILE = "../data/dirtysets/abalone_3cfds_01pct.csv";
                            RESULT_PATH = "../results/abalone_3cfds_01pct/";
                            break;
                        case zero_five:
                            DATA_FILE = "../data/dirtysets/abalone_3cfds_05pct.csv";
                            RESULT_PATH = "../results/abalone_3cfds_05pct/";
                            break;
                        case one:
                            DATA_FILE = "../data/dirtysets/abalone_3cfds_1pct.csv";
                            RESULT_PATH = "../results/abalone_3cfds_1pct/";
                            break;
                        case two:
                            DATA_FILE = "../data/dirtysets/abalone_3cfds_3pct.csv";
                            RESULT_PATH = "../results/abalone_3cfds_3pct/";
                            break;
                        default:
                            break;
                    }
                    break;
                case 5:
                    RULE_FILE = "../data/cfds/abalone_5cfds.txt";
                    switch (ERR_PER) {
                        case zero_one:
                            DATA_FILE = "../data/dirtysets/abalone_5cfds_01pct.csv";
                            RESULT_PATH = "../results/abalone_5cfds_01pct/";
                            break;
                        case zero_five:
                            DATA_FILE = "../data/dirtysets/abalone_5cfds_05pct.csv";
                            RESULT_PATH = "../results/abalone_5cfds_05pct/";
                            break;
                        case one:
                            DATA_FILE = "../data/dirtysets/abalone_5cfds_1pct.csv";
                            RESULT_PATH = "../results/abalone_5cfds_1pct/";
                            break;
                        case two:
                            DATA_FILE = "../data/dirtysets/abalone_5cfds_2pct.csv";
                            RESULT_PATH = "../results/abalone_5cfds_2pct/";
                            break;
                        default:
                            break;
                    }
                    break;
                case 10:
                    RULE_FILE = "../data/cfds/abalone_10cfds.txt";
                    switch (ERR_PER) {
                        case zero_one:
                            DATA_FILE = "../data/dirtysets/abalone_10cfds_01pct.csv";
                            RESULT_PATH = "../results/abalone_10cfds_01pct/";
                            break;
                        case zero_five:
                            DATA_FILE = "../data/dirtysets/abalone_10cfds_05pct.csv";
                            RESULT_PATH = "../results/abalone_10cfds_05pct/";
                            break;
                        case one:
                            DATA_FILE = "../data/dirtysets/abalone_10cfds_1pct.csv";
                            RESULT_PATH = "../results/abalone_10cfds_1pct/";
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
            break;
        case adult:
            CLEAN_DATA_FILE = "../data/adult.csv";
            switch (CFDS_COUNT) {
                case 3:
                    RULE_FILE = "../data/cfds/adult_3cfds.txt";
                    switch (ERR_PER) {
                        case zero_one:
                            DATA_FILE = "../data/dirtysets/adult_3cfds_01pct.csv";
                            RESULT_PATH = "../results/adult_3cfds_01pct/";
                            break;
                        case zero_five:
                            DATA_FILE = "../data/dirtysets/adult_3cfds_05pct.csv";
                            RESULT_PATH = "../results/adult_3cfds_05pct/";
                            break;
                        case one:
                            DATA_FILE = "../data/dirtysets/adult_3cfds_1pct.csv";
                            RESULT_PATH = "../results/adult_3cfds_1pct/";
                            break;
                        case two:
                            DATA_FILE = "../data/dirtysets/adult_3cfds_3pct.csv";
                            RESULT_PATH = "../results/adult_3cfds_3pct/";
                            break;
                        default:
                            break;
                    }
                    break;
                case 5:
                    RULE_FILE = "../data/cfds/adult_5cfds.txt";
                    switch (ERR_PER) {
                        case zero_one:
                            DATA_FILE = "../data/dirtysets/adult_5cfds_01pct.csv";
                            RESULT_PATH = "../results/adult_5cfds_01pct/";
                            break;
                        case zero_five:
                            DATA_FILE = "../data/dirtysets/adult_5cfds_05pct.csv";
                            RESULT_PATH = "../results/adult_5cfds_05pct/";
                            break;
                        case one:
                            DATA_FILE = "../data/dirtysets/adult_5cfds_1pct.csv";
                            RESULT_PATH = "../results/adult_5cfds_1pct/";
                            break;
                        case two:
                            DATA_FILE = "../data/dirtysets/adult_5cfds_2pct.csv";
                            RESULT_PATH = "../results/adult_5cfds_2pct/";
                            break;
                        default:
                            break;
                    }
                    break;
                case 10:
                    RULE_FILE = "../data/cfds/adult_10cfds.txt";
                    switch (ERR_PER) {
                        case zero_one:
                            DATA_FILE = "../data/dirtysets/adult_10cfds_01pct.csv";
                            RESULT_PATH = "../results/adult_10cfds_01pct/";
                            break;
                        case zero_five:
                            DATA_FILE = "../data/dirtysets/adult_10cfds_05pct.csv";
                            RESULT_PATH = "../results/adult_10cfds_05pct/";
                            break;
                        case one:
                            DATA_FILE = "../data/dirtysets/adult_10cfds_1pct.csv";
                            RESULT_PATH = "../results/adult_10cfds_1pct/";
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
            break;
        case soccer:
            CLEAN_DATA_FILE = "../data/Soccer.csv";
            switch (CFDS_COUNT) {
                case 3:
                    RULE_FILE = "../data/cfds/Soccer_3cfds.txt";
                    switch (ERR_PER) {
                        case zero_one:
                            DATA_FILE = "../data/dirtysets/Soccer_3cfds_01pct.csv";
                            RESULT_PATH = "../results/soccer_3cfds_01pct/";
                            break;
                        case zero_five:
                            DATA_FILE = "../data/dirtysets/Soccer_3cfds_05pct.csv";
                            RESULT_PATH = "../results/soccer_3cfds_05pct/";
                            break;
                        case one:
                            DATA_FILE = "../data/dirtysets/Soccer_3cfds_1pct.csv";
                            RESULT_PATH = "../results/soccer_3cfds_1pct/";
                            break;
                        case two:
                            DATA_FILE = "../data/dirtysets/Soccer_3cfds_3pct.csv";
                            RESULT_PATH = "../results/soccer_3cfds_3pct/";
                            break;
                        default:
                            break;
                    }
                    break;
                case 5:
                    RULE_FILE = "../data/cfds/Soccer_5cfds.txt";
                    switch (ERR_PER) {
                        case zero_one:
                            DATA_FILE = "../data/dirtysets/Soccer_5cfds_01pct.csv";
                            RESULT_PATH = "../results/soccer_5cfds_01pct/";
                            break;
                        case zero_five:
                            DATA_FILE = "../data/dirtysets/Soccer_5cfds_05pct.csv";
                            RESULT_PATH = "../results/soccer_5cfds_05pct/";
                            break;
                        case one:
                            DATA_FILE = "../data/dirtysets/Soccer_5cfds_1pct.csv";
                            RESULT_PATH = "../results/soccer_5cfds_1pct/";
                            break;
                        case two:
                            DATA_FILE = "../data/dirtysets/Soccer_5cfds_2pct.csv";
                            RESULT_PATH = "../results/soccer_5cfds_2pct/";
                            break;
                        default:
                            break;
                    }
                    break;
                case 10:
                    RULE_FILE = "../data/cfds/Soccer_10cfds.txt";
                    switch (ERR_PER) {
                        case zero_one:
                            DATA_FILE = "../data/dirtysets/Soccer_10cfds_01pct.csv";
                            RESULT_PATH = "../results/soccer_10cfds_01pct/";
                            break;
                        case zero_five:
                            DATA_FILE = "../data/dirtysets/Soccer_10cfds_05pct.csv";
                            RESULT_PATH = "../results/soccer_10cfds_05pct/";
                            break;
                        case one:
                            DATA_FILE = "../data/dirtysets/Soccer_10cfds_1pct.csv";
                            RESULT_PATH = "../results/soccer_10cfds_1pct/";
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
            break;
        case sp500:
            CLEAN_DATA_FILE = "../data/sp500.csv";
            switch (CFDS_COUNT) {
                case 3:
                    RULE_FILE = "../data/cfds/sp500_3cfds.txt";
                    switch (ERR_PER) {
                        case zero_one:
                            DATA_FILE = "../data/dirtysets/sp500_3cfds_01pct.csv";
                            RESULT_PATH = "../results/sp500_3cfds_01pct/";
                            break;
                        case zero_five:
                            DATA_FILE = "../data/dirtysets/sp500_3cfds_05pct.csv";
                            RESULT_PATH = "../results/sp500_3cfds_05pct/";
                            break;
                        case one:
                            DATA_FILE = "../data/dirtysets/sp500_3cfds_1pct.csv";
                            RESULT_PATH = "../results/sp500_3cfds_1pct/";
                            break;
                        case two:
                            DATA_FILE = "../data/dirtysets/sp500_3cfds_3pct.csv";
                            RESULT_PATH = "../results/sp500_3cfds_3pct/";
                            break;
                        default:
                            break;
                    }
                    break;
                case 5:
                    RULE_FILE = "../data/cfds/sp500_5cfds.txt";
                    switch (ERR_PER) {
                        case zero_one:
                            DATA_FILE = "../data/dirtysets/sp500_5cfds_01pct.csv";
                            RESULT_PATH = "../results/sp500_5cfds_01pct/";
                            break;
                        case zero_five:
                            DATA_FILE = "../data/dirtysets/sp500_5cfds_05pct.csv";
                            RESULT_PATH = "../results/sp500_5cfds_05pct/";
                            break;
                        case one:
                            DATA_FILE = "../data/dirtysets/sp500_5cfds_1pct.csv";
                            RESULT_PATH = "../results/sp500_5cfds_1pct/";
                            break;
                        case two:
                            DATA_FILE = "../data/dirtysets/sp500_5cfds_2pct.csv";
                            RESULT_PATH = "../results/sp500_5cfds_2pct/";
                            break;
                        default:
                            break;
                    }
                    break;
                case 10:
                    RULE_FILE = "../data/cfds/sp500_10cfds.txt";
                    switch (ERR_PER) {
                        case zero_one:
                            DATA_FILE = "../data/dirtysets/sp500_10cfds_01pct.csv";
                            RESULT_PATH = "../results/sp500_10cfds_01pct/";
                            break;
                        case zero_five:
                            DATA_FILE = "../data/dirtysets/sp500_10cfds_05pct.csv";
                            RESULT_PATH = "../results/sp500_10cfds_05pct/";
                            break;
                        case one:
                            DATA_FILE = "../data/dirtysets/sp500_10cfds_1pct.csv";
                            RESULT_PATH = "../results/sp500_10cfds_1pct/";
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}
