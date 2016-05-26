__attribute__((objc_root_class))
@interface NSObject

+ (instancetype)alloc;
- (instancetype)init;

@end

@interface NSString : NSObject
@end

@interface NSNumber : NSObject
@end

@interface NSArray<Element> : NSObject
@end
