// Test for loop variables
// 
// Since most commonly used form of `for x in ...` is with NSArray or NSDictionary,
// we assume loop variables to be `_Nonnull` if no nullability is specified.

#import "polyfill.h"

@interface Test6 : NSObject

@end

@implementation Test6

- (void)loopTest:(NSArray<NSString *> *)array {
  for (NSString *x in array) { // expected-warning{{collection expression type 'NSArray<NSString *> *' may not respond to 'countByEnumeratingWithState:objects:count:'}}, expected-remark{{Variable nullability: nonnull}} 
    NSString * _Nonnull y = x; // ok
  }
}

@end

