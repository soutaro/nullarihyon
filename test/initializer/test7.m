// Option: -filter Foo

#import "../objc/polyfill.h"

NS_ASSUME_NONNULL_BEGIN

@interface Test : NSObject
@end

@interface Foo : NSObject
@end

@implementation Test {
	NSString * _Nonnull _name;
}

// Output is filtered out
- (instancetype)init __attribute__((annotate("nlh_initializer"))) {
	self = [super init];
	return self;
}

@end

@implementation Foo {
	NSString * _Nonnull _name;
}

- (instancetype)init __attribute__((annotate("nlh_initializer"))) { // expected-warning{{Nonnull ivar should be initialized: _name}}
	self = [super init];
	return self;
}

@end

NS_ASSUME_NONNULL_END
