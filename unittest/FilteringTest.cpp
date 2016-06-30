#include <gtest/gtest.h>
#include <iostream>
#include <set>

#include <FilteringClause.h>

#include "TestHelper.h"

TEST(Filtering, clauses) {
    std::set<std::string> subjects;
    subjects.insert("NSString");
    subjects.insert("NSObject");
    subjects.insert("OBHViewControllerObject");
    
    TextFilteringClause textClause1("NSString");
    ASSERT_TRUE(textClause1.testClassName(subjects));
    
    TextFilteringClause textClause2("NSURLSession");
    ASSERT_FALSE(textClause2.testClassName(subjects));
    
    RegexpFilteringClause regexpClause1(std::regex("Object"));
    ASSERT_TRUE(regexpClause1.testClassName(subjects));

    RegexpFilteringClause regexpClause2(std::regex("XYZZY"));
    ASSERT_FALSE(regexpClause2.testClassName(subjects));

    RegexpFilteringClause regexpClause3(std::regex("(^NS)"));
    ASSERT_TRUE(regexpClause3.testClassName(subjects));
}

TEST(Filtering, empty_filter) {
    std::set<std::string> subjects;
    subjects.insert("NSString");
    subjects.insert("NSObject");
    subjects.insert("OBHViewControllerObject");
    
    Filter filter;
    ASSERT_TRUE(filter.testClassName(subjects));
}

TEST(Filtering, filter_pass) {
    std::set<std::string> subjects;
    subjects.insert("NSString");
    subjects.insert("NSObject");
    subjects.insert("OBHViewControllerObject");
    
    Filter filter;
    filter.addClause(std::shared_ptr<TextFilteringClause>{ new TextFilteringClause("NSString") });
    filter.addClause(std::shared_ptr<TextFilteringClause>{ new TextFilteringClause("XYZZY") });
    
    ASSERT_TRUE(filter.testClassName(subjects));
}

TEST(Filtering, filter_reject) {
    std::set<std::string> subjects;
    subjects.insert("NSString");
    subjects.insert("NSObject");
    subjects.insert("OBHViewControllerObject");
    
    Filter filter;
    filter.addClause(std::shared_ptr<TextFilteringClause>{ new TextFilteringClause("NS") });
    filter.addClause(std::shared_ptr<TextFilteringClause>{ new TextFilteringClause("XYZZY") });
    
    ASSERT_FALSE(filter.testClassName(subjects));
}