//
// Created by Unravel on 2022/3/15.
//

#ifndef WFG_GENERATOR_UTIL_H
#define WFG_GENERATOR_UTIL_H

#include <string>
#include <vector>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <set>
#include <map>

using namespace std;

namespace wfdg {
    namespace util {
        template<typename T>
        string numToString(T num) {
            return to_string(num);
        }

        template<typename T>
        string vecToString(const vector<T> &vec,
                           const function<string(decltype(vec.front()))> &toStrFunc = numToString<T>) {
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
        string numPairToString(const pair<T, U> &p) {
            return "(" + to_string(p.first) + ", " + to_string(p.second) + ")";
        }

        template<typename T, typename U>
        string pairToString(const pair<T, U> &p, const function<string(decltype(p.first))> &tFunc,
                            const function<string(decltype(p.second))> &uFunc) {
            return "(" + tFunc(p.first) + ", " + uFunc(p.second) + ")";
        }

        template<typename T, typename U, typename H>
        string hashmapToString(const unordered_map<T, U, H> &hashmap,
                               const function<string(decltype(hashmap.begin()->first))> &tFunc,
                               const function<string(decltype(hashmap.begin()->second))> &uFunc) {
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

        template<typename T, typename U>
        string mapToString(const map<T, U> &m, const function<string(decltype(m.begin()->first))> &tFunc,
                           const function<string(decltype(m.begin()->second))> &uFunc) {
            string str = "[";
            size_t i = 0, sz = m.size();
            for (auto &item: m) {
                str += tFunc(item.first) + ":" + uFunc(item.second);
                if (++i != sz) {
                    str += ", ";
                }
            }
            return str += "]";
        }

        template<typename T, typename H>
        string hashsetToString(const unordered_set<T, H> &s,
                               const function<string(decltype(*s.begin()))> &toStrFunc = numToString<T>) {
            string str = "[";
            size_t i = 0, sz = s.size();
            for (auto &item: s) {
                str += toStrFunc(item);
                if (++i != sz) {
                    str += ", ";
                }
            }
            return str += "]";
        }


        template<typename T>
        string setToString(const set<T> &s,
                           const function<string(decltype(*s.begin()))> &toStrFunc = numToString<T>) {
            string str = "[";
            size_t i = 0, sz = s.size();
            for (auto &item: s) {
                str += toStrFunc(item);
                if (++i != sz) {
                    str += ", ";
                }
            }
            return str += "]";
        }

        inline int numInRange(unsigned num, const pair<unsigned, unsigned> &range) {
            return num < range.first ? -1 : (num > range.second ? 1 : 0);
        }

        void mergeLineRanges(vector<pair<unsigned, unsigned>> &ranges);

        template<typename T>
        inline void hash_combine(std::size_t &seed, T const &v) {
            seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }

        struct pair_hash {
            template<typename T1, typename T2>
            std::size_t operator()(const std::pair<T1, T2> &p) const {
                size_t seed = 0;
                hash_combine(seed, p.first);
                hash_combine(seed, p.second);
                return seed;
            }
        };
    }
}


#endif //WFG_GENERATOR_UTIL_H
