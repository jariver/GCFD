//
// Created by Administrator on 2020/5/7.
//

#ifndef ALCFD_CPP_PROCESSING_H
#define ALCFD_CPP_PROCESSING_H

#include "using_name.h"
#include "record.h"

class processing {
public:
    processing() = default;

    static char *readfile(const char *filename);

    static void split(const std::string &s, VecStr &tokens, const std::string &delimiters = " ");

    static VecVecStr read_csv(const std::string &file_path, const std::string &separator);

    static bool write_file(const char *filename, const char *contents);

    static bool write_file(const std::string &filename, const std::string &contents);

    static void print_records(const char *filename, const VecSpRec &vec);

    static void combinations(VecVecUint &res, const VecUint &nums, unsigned k);

    static void combinations(VecVecStr &res, const VecStr &nums, unsigned k);

    static void products(VecVecUint &res, const VecUint &nums_1, const VecUint &nums_2);

    static void random_sampling(VecVecUint &ret, const VecVecUint &vec, unsigned size);

    static void intersection(VecUint &ret, const VecUint &vec1, const VecUint &vec2);

    static std::string emplace_front(std::string str, int length, char ch);
};


#endif //ALCFD_CPP_PROCESSING_H
