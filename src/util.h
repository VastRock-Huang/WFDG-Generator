//
// Created by Unravel on 2022/3/15.
//

#ifndef WFG_GENERATOR_UTIL_H
#define WFG_GENERATOR_UTIL_H

#include <string>
#include <vector>
#include <utility>
#include <unordered_map>

using namespace std;

namespace wfg {
    struct Util {
        template<typename T>
        static string numVecToString(const vector<T> &vec) {
            string str = "[";
            size_t sz = vec.size();
            for (size_t i = 0; i < sz; ++i) {
                str += to_string(vec[i]);
                if (i != sz - 1) {
                    str += ", ";
                }
            }
            return str += "]";
        }

        template<typename T, typename U>
        static string numPairToString(const pair<T, U> p) {
            return "(" + to_string(p.first) + ", " + to_string(p.second) + ")";
        }

        template<typename T, typename U>
        static string numPairVecToString(const vector<pair<T, U>> &vec) {
            string str = "[";
            size_t sz = vec.size();
            for (size_t i = 0; i < sz; ++i) {
                str += numPairToString(vec[i]);
                if (i != sz - 1) {
                    str += ", ";
                }
            }
            return str += "]";
        }

        template<typename T>
        static string objVecToString(const vector<T> &vec) {
            string str = "[";
            size_t sz = vec.size();
            for (size_t i = 0; i < sz; ++i) {
                str += vec[i].toString();
                if (i != sz - 1) {
                    str += ", ";
                }
            }
            return str += "]";
        }

        template<typename T>
        static string str_NumVecMapToString(const unordered_map<string, vector<T>>& strNumMap) {
            string str = "[";
            size_t sz = strNumMap.size();
            size_t i = 0;
            for(auto & item: strNumMap) {
                str += item.first + ":" + numVecToString(item.second);
                if(++i != sz) {
                    str += ", ";
                }
            }
            return str += "]";
        }

        static int numInRange(unsigned num,const pair<unsigned, unsigned>& range) {
            if(num < range.first) {
                return -1;
            }
            if(num > range.second){
                return 1;
            }
            return 0;
        }

    };
}


#endif //WFG_GENERATOR_UTIL_H
