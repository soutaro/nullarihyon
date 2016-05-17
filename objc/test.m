#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface TestClass : NSObject

@property (nonatomic) NSString *nonnullString;
@property (nonatomic, nullable) NSString *nullableString;

@end

@implementation TestClass

- (void)test {
  NSString *s = @"hoge";
  self.nonnullString = s;
  self.nullableString = s;

  NSString * _Nonnull t;
  t = self.nonnullString;
  t = self.nullableString;
}

@end

NS_ASSUME_NONNULL_END
