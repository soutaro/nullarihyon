#ifndef FilteringClause_h
#define FilteringClause_h

#include <string>
#include <regex>

class FilteringClause {
public:
    virtual bool testClassName(const std::set<std::string> &subjects) {
        return false;
    }
    virtual ~FilteringClause() {};
};

class TextFilteringClause : public FilteringClause {
    std::string _Text;
    
public:
    TextFilteringClause(std::string text) : _Text(text) {};
    
    bool testClassName(const std::set<std::string> &subjects) override;
};

class RegexpFilteringClause : public FilteringClause {
    std::regex _Regexp;
public:
    RegexpFilteringClause(std::regex regexp) : _Regexp(regexp) {};
    
    bool testClassName(const std::set<std::string> &subjects) override;
};

class Filter {
    std::vector<std::shared_ptr<FilteringClause>> _Clauses;
    
public:
    void addClause(std::shared_ptr<FilteringClause> clause) {
        _Clauses.push_back(clause);
    }
    
    bool testClassName(const std::set<std::string> &subjects);
};

#endif /* FilteringClause_h */
