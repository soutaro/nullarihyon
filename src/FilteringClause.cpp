#include "FilteringClause.h"

bool TextFilteringClause::testClassName(const std::set<std::string> &subjects) {
    return subjects.find(_Text) != subjects.end();
}

bool RegexpFilteringClause::testClassName(const std::set<std::string> &subjects) {
    for (auto subject : subjects) {
        std::smatch m;
        if (std::regex_search(subject, m, _Regexp)) {
            return true;
        }
    }
    return false;
}

bool Filter::testClassName(const std::set<std::string> &subjects) {
    if (_Clauses.empty()) {
        return true;
    } else {
        for (auto clause : _Clauses) {
            if (clause->testClassName(subjects)) {
                return true;
            }
        }
        
        return false;
    }
}