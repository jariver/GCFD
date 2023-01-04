//
// Created by Administrator on 07/04/2020.
//

#ifndef ALCFD_CPP_USING_NAME_H
#define ALCFD_CPP_USING_NAME_H

#include <algorithm>
#include <cmath>
#include <cstring>
#include <ctime>
#include <deque>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using VecUint = std::vector<unsigned>;
using UpVecUint = std::unique_ptr<VecUint>;
using VecUpVecUint = std::vector<UpVecUint>;
using UpVecUpVecUint = std::unique_ptr<VecUpVecUint>;

using VecInt = std::vector<int>;
using UpVecInt = std::unique_ptr<std::vector<int>>;
using VecUpVecInt = std::vector<std::unique_ptr<std::vector<int>>>;
using UpVecUpVecInt = std::unique_ptr<std::vector<std::unique_ptr<std::vector<int>>>>;

using VecStr = std::vector<std::string>;
using VecVecStr = std::vector<VecStr>;
using VecVecUint = std::vector<VecUint>;
using VecVecVecUint = std::vector<VecVecUint>;

using UpStr = std::unique_ptr<std::string>;
using VecUpStr = std::vector<UpStr>;
using UpVecUpStr = std::unique_ptr<VecUpStr>;
using VecUpVecUpStr = std::vector<UpVecUpStr>;
using UpVecUpVecUpStr = std::unique_ptr<VecUpVecUpStr>;

using SetUint = std::set<unsigned>;
using VecSetUint = std::vector<SetUint>;
using DeqSetUint = std::deque<SetUint>;

using UsetUint = std::unordered_set<unsigned>;
using VecUsetUint = std::vector<UsetUint>;
using DeqUsetUint = std::deque<UsetUint>;

using SpVecUint = std::shared_ptr<VecUint>;
using VecSpVecUint = std::vector<SpVecUint>;
using SpVecSpVecUint = std::shared_ptr<VecSpVecUint>;
using SpVecVecUint = std::shared_ptr<VecVecUint>;

#endif  // ALCFD_CPP_USING_NAME_H
