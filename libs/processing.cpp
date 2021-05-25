//
// Created by Administrator on 2020/5/7.
//

#include "processing.h"

void processing::split(const std::string &s, VecStr &tokens, const std::string &delimiters) {
    std::string::size_type lastPos = s.find_first_not_of(delimiters, 0);
    std::string::size_type pos = s.find(delimiters, lastPos);
    while (std::string::npos != pos || std::string::npos != lastPos) {
        tokens.emplace_back(s.substr(lastPos, pos - lastPos));
        lastPos = s.find_first_not_of(delimiters, pos);
        pos = s.find(delimiters, lastPos);
    }
}

VecVecStr processing::read_csv(const std::string &file_path, const std::string &separator = ",") {
    std::string data = processing::readfile(file_path.c_str());

    VecVecStr read_from_csv;

    VecStr lines;
    split(data, lines, "\n");

    for (const auto &line : lines) {
        VecStr row;
        split(line, row, separator);
        read_from_csv.emplace_back(row);
    }
    return read_from_csv;
}

char *processing::readfile(const char *filename) {
    char *tmp;
    FILE *fp = fopen(filename, "rb");
    if (fp != nullptr) {
        fseek(fp, 0, SEEK_END);
        int file_size = ftell(fp);
        rewind(fp);
        tmp = new char[file_size + 1]();
        fread(tmp, sizeof(char), file_size, fp);
        fclose(fp);
        return tmp;
    } else {
        std::cout << "file open failed!\t" << filename << std::endl;
        return nullptr;
    }
}

bool processing::write_file(const char *filename, const char *contents) {
    FILE *fp = fopen(filename, "wb");
    if (fp != nullptr) {
        // fseek(fp, 0, SEEK_END);
        int contents_size = strlen(contents);
        fwrite(contents, sizeof(char), contents_size, fp);
        fclose(fp);
        return true;
    } else {
        std::cout << "file open failed!\t" << filename << std::endl;
        return false;
    }
}

bool processing::write_file(const std::string &filename, const std::string &contents) {
    // std::ofstream file_write(filename, std::ios::out | std::ios::app);
    std::ofstream file_write(filename, std::ios::out | std::ios::trunc);
    if (file_write.is_open()) {
        file_write << contents << std::endl;
        file_write.close();
        return true;
    } else {
        std::cout << "file open failed!\t" << filename << std::endl;
        return false;
    }
}

void processing::print_records(const char *filename, const VecSpRec &vec) {
    char *tmp = new char[200000 * 200]();
    unsigned index = 0;
    for (const auto &record: vec) {
        for (auto item = record->cbegin(); item != record->cend(); ++item) {
            // strcat(tmp, item.c_str());
            for (std::string::size_type i = 0; i != item->size(); ++i) {
                tmp[index++] = (*item)[i];
            }
            if (item != record->cend() - 1)
                tmp[index++] = ',';
        }
        // strcat(tmp, "\n");
        tmp[index++] = '\n';
    }
    std::cout << "Records have " << strlen(tmp) << " chars." << std::endl;
    processing::write_file(filename, tmp);
    delete[] tmp;
}

void backtrack(VecVecUint &res, const VecUint &nums, unsigned k, int first, VecUint &curr) {
    if (curr.size() == k) {
        res.emplace_back(curr);
        return;
    }
    for (int i = first; i != nums.size(); ++i) {
        curr.emplace_back(nums[i]);
        backtrack(res, nums, k, i + 1, curr);
        curr.pop_back();
    }
}

void backtrack(VecVecStr &res, const VecStr &nums, unsigned k, int first, VecStr &curr) {
    if (curr.size() == k) {
        res.emplace_back(curr);
        return;
    }
    for (int i = first; i != nums.size(); ++i) {
        curr.emplace_back(nums[i]);
        backtrack(res, nums, k, i + 1, curr);
        curr.pop_back();
    }
}

void processing::combinations(VecVecUint &res, const VecUint &nums, unsigned k) {
    VecUint curr;
    backtrack(res, nums, k, 0, curr);
}

void processing::combinations(VecVecStr &res, const VecStr &nums, unsigned int k) {
    VecStr curr;
    backtrack(res, nums, k, 0, curr);
}

void processing::products(VecVecUint &res, const VecUint &nums_1, const VecUint &nums_2) {
    for (unsigned left: nums_1) {
        for (unsigned right: nums_2) {
            VecUint vec{left, right};
            res.emplace_back(vec);
        }
    }
}

void processing::random_sampling(VecVecUint &ret, const VecVecUint &vec, unsigned size) {
    std::uniform_int_distribution<unsigned> u(0, vec.size() - 1);
    std::default_random_engine e;
    bool *is_created = new bool[vec.size()]();
    for (unsigned i = 0; i != size;) {
        unsigned random_num = u(e);
        if (is_created[random_num]) {
            continue;
        } else {
            is_created[random_num] = 1;
            ret.emplace_back(vec[random_num]);
            ++i;
        }
    }
    delete[] is_created;
}

void processing::intersection(VecUint &ret, const VecUint &vec1, const VecUint &vec2) {
    ret.clear();
    std::set<unsigned> set1(vec1.begin(), vec1.end());
    std::set<unsigned> set2(vec2.begin(), vec2.end());
    std::set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(),
                          std::insert_iterator<VecUint>(ret, ret.begin()));
}

std::string processing::emplace_front(std::string str, int length, char ch) {
    str.reserve(length + 1);
    while (str.size() < length) {
        str.insert(str.begin(), ch);
    }
    return str;
}


