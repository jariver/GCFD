### **Introduction**

- The whole algorithm is implemented in JetBrains Clion on Ubuntu 18.04 with C++14.
- Be sure to run the code in Linux environment instead of Windows, otherwise there may be some mistakes. If you use CLion to build, you can delete the "cmake-build-debug" directory and Reload CMake Project.
- In our experiments, we use the clean version of the dataset for an automatic labeling, instead of manual labeling. If run a dataset without a clean version, need to make some changes to the code.
<br>

### **File Directory Information**

- data: clean version of all datasets.
- data/dirtysets: dirtysets with different noises introduced by different CFDs.
- data/cfds: all CFDs are used to introduce noises into datasets.
<br>

- label: label files. (In our experiment, we used clean version instead of manual labeling, so the folder was not used in our experiment)
- libs: various module programs
<br>

- results: result files
- results/*/init_cfds.txt: results of initial learning.
- results/*/result.txt: result of whole process. (Due to the large number of Trash Repositories, the file size may be too large, so we did not output the content of Trash Repository in this code.)
- results/*/sampled_records_init.txt: results of initial sampling.
- results/*/sampled_records.txt: all sampled records of whole process.
<br>
  
- alcfd.cpp: core program file
- main.cpp: program execution entry
- output.txt: including the final number of rounds, runntimes of each module, label informations, recall, etc
<br>

### **Parameter Setting Information**

All parameters are set in the file alcfd.cpp:

```cpp
const int DATASET;        // dataset
const int CFDS_COUNT;     // #CFDs are used to introduce noises
const int ERR_PER;        // %Error for introducing noises of every CFD
const float MIN_WHOLE_SUPPORT_RATE;   // support threshold
const float CONFIDENCE_THRESHOLD;     // confidence threshold
const unsigned TUPLE_PAIR_COUNT;      // #tuples in initial sampling module
const unsigned RE_TUPLE_PAIR_COUNT;   // #tuples in every re_sampling module
const unsigned RULE_REPOSITORY_K;     // top-k in rule repository
```
