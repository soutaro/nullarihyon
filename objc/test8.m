// Block expr is nonnull

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface Test8 : NSObject
@end

@implementation Test8

- (void)test:(void(^)())block {

}

- (void)test2 {
  [self test:^{}];
}

@end

NS_ASSUME_NONNULL_END
