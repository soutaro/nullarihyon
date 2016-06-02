typedef unsigned NSUInteger;

__attribute__((objc_root_class))
@interface NSObject

+ (instancetype)alloc;
- (instancetype)init;

@end

@protocol NSObjectProtocol
@end

@protocol NSCopying
@end

@interface NSString : NSObject<NSCopying>
@end

@interface NSNumber : NSObject
@end

@interface NSArray<Element> : NSObject

@end

