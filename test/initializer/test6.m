#import "../objc/polyfill.h"

NS_ASSUME_NONNULL_BEGIN

@interface Base : NSObject

@property (nonatomic) NSString *base;

- (void)setup __attribute__((annotate("nlh_initializer")));

@end

@interface Derived : Base

@property (nonatomic) NSString *name;

@end

@implementation Derived

- (void)setup { // expected-warning{{Nonnull ivar should be initialized: _name}}
	[super setup];
}

@end

NS_ASSUME_NONNULL_END
