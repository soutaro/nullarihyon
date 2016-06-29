#import "../objc/polyfill.h"

NS_ASSUME_NONNULL_BEGIN

@interface Base : NSObject

@property (nonatomic, readonly) NSString *name;

- (void)setup1 __attribute__((annotate("nlh_initializer")));

@end

@interface Derived : Base

@property (nonatomic) NSString *address;

@end

@implementation Derived

- (void)setup2 __attribute__((annotate("nlh_initializer"))) { // expected-warning{{Nonnull ivar should be initialized: _address}}
	[self setup1];
}

@end

NS_ASSUME_NONNULL_END
