#import "polyfill.h"

@interface TestClass : NSObject

@property (nonatomic, nonnull) NSString *nonnullString;
@property (nonatomic, nullable) NSString *nullableString;

@end

@implementation TestClass

- (void)test {
  NSString * _Nullable x;

  self.nonnullString = x; // expected-warning{{-[TestClass setNonnullString:] expects nonnull argument}}
  self.nullableString = x; // no warning
}

- (void)test2 {
  NSString * _Nonnull x;

  x = self.nonnullString; // no warning
  x = self.nullableString; // expected-warning{{Nullability mismatch on assignment}}
}

@end

