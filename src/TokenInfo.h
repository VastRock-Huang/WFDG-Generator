//
// Created by Unravel on 2022/3/15.
//

#ifndef WFG_GENERATOR_TOKENINFO_H
#define WFG_GENERATOR_TOKENINFO_H


#include <string>
#include <unordered_map>
#include <vector>

using namespace std;


namespace wfg {
    class TokenInfo {
    private:
        string _funcName;
        unordered_map<string,vector<unsigned>> _idMap;
    public:
        TokenInfo(string &&funcName) : _funcName(funcName) {}

        void insertIdentifier(const string& id, unsigned lineNo);
    };
}


#endif //WFG_GENERATOR_TOKENINFO_H
