//
// Created by Administrator on 07/06/2020.
//

#ifndef ALCFD_CPP_USING_NAME_SP_H
#define ALCFD_CPP_USING_NAME_SP_H

#include "tuple_pair.h"

using SpTp = std::shared_ptr<TuplePair>;
using VecSpTp = std::vector<SpTp>;
using SpVecSpTp = std::shared_ptr<VecSpTp>;
using VecSpVecSpTp = std::vector<SpVecSpTp>;

#endif //ALCFD_CPP_USING_NAME_SP_H
