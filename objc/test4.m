#import <Foundation/Foundation.h>

@interface Test4 : NSObject

@end

@implementation Test4

- (NSString * _Nonnull)test {
  NSString * (^block)() = ^NSString *() {
    NSString *a = @"";
    return a;
  };

  return @"hoge";
}

@end
