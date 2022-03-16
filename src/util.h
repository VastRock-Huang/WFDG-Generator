//
// Created by Unravel on 2022/3/15.
//

#ifndef WFG_GENERATOR_UTIL_H
#define WFG_GENERATOR_UTIL_H

#include <string>
#include <vector>
#include <utility>

using namespace std;

namespace wfg{
    struct Util {
        template<typename T>
        static string NumVecToString(const vector<T>& vec) {
            string str = "[";
            size_t sz = vec.size();
            for(size_t i = 0; i < sz; ++i) {
                str += to_string(vec[i]);
                if(i != sz -1) {
                    str += ", ";
                }
            }
            return str += "]";
        }

        template<typename T, typename U>
        static string NumPairVecToString(const vector<pair<T,U>>& vec) {
            string str = "[";
            size_t sz = vec.size();
            for(size_t i =0;i<sz;++i) {
                str += "(" + to_string(vec[i].first) + ", " + to_string(vec[i].second) + ")";
                if(i != sz -1) {
                    str += ", ";
                }
            }
            return str += "]";
        }
    };
}


#endif //WFG_GENERATOR_UTIL_H
