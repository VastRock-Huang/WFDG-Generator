//
// Created by Unravel on 2022/3/15.
//

#ifndef WFG_GENERATOR_UTIL_H
#define WFG_GENERATOR_UTIL_H

#include <string>
#include <vector>
#include <utility>
#include <unordered_map>
#include <functional>

using namespace std;

namespace wfg {
    struct Util {
        template<typename T>
        static string numToString(T num) {
            return move(to_string(num));
        }

        template<typename T>
        static string vecToString(const vector<T> &vec, function<string(decltype(*vec.begin()))> toStrFunc) {
            string str = "[";
            size_t sz = vec.size();
            for (size_t i = 0; i < sz; ++i) {
                str += toStrFunc(vec[i]);
                if (i != sz - 1) {
                    str += ", ";
                }
            }
            return str += "]";
        }

        template<typename T, typename U>
        static string numPairToString(const pair<T, U>& p) {
            return "(" + to_string(p.first) + ", " + to_string(p.second) + ")";
        }

        template<typename T, typename U>
        static string
        mapToString(const unordered_map<T, U>& hashmap, function<string(decltype(hashmap.begin()->first))> tFunc,
                    function<string(decltype(hashmap.begin()->second))> uFunc) {
            string str = "[";
            size_t i = 0, sz = hashmap.size();
            for (auto &item: hashmap) {
                str += tFunc(item.first) + ":" + uFunc(item.second);
                if (++i != sz) {
                    str += ", ";
                }
            }
            return str += "]";
        }

        static int numInRange(unsigned num, const pair<unsigned, unsigned> &range) {
            if (num < range.first) {
                return -1;
            }
            if (num > range.second) {
                return 1;
            }
            return 0;
        }

    };
}


#endif //WFG_GENERATOR_UTIL_H
