//
// Created by Unravel on 2022/3/15.
//

#include "util.h"
#include <algorithm>

namespace wfdg {
    namespace util {
        void mergeLineRanges(vector<pair<unsigned, unsigned>> &ranges) {
            vector<pair<unsigned, unsigned>> result;
            sort(ranges.begin(), ranges.end());
            size_t i = 0, sz = ranges.size();
            while (i < sz) {
                unsigned minx = ranges[i].first, maxx = ranges[i].second;
                ++i;
                while (i < sz) {
                    if (ranges[i].first <= maxx + 1) {
                        maxx = max(maxx, ranges[i].second);
                        ++i;
                    } else {
                        break;
                    }
                }
                result.emplace_back(minx, maxx);
            }
            ranges.assign(result.begin(), result.end());
        }
    }
}