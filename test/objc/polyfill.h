#import "polyfill0.h"

typedef unsigned NSUInteger;
typedef signed NSInteger;
typedef _Bool BOOL;

#define YES __objc_yes
#define NO __objc_no

#define nil ((void *)0)

#define NS_ASSUME_NONNULL_BEGIN _Pragma("clang assume_nonnull begin")
#define NS_ASSUME_NONNULL_END   _Pragma("clang assume_nonnull end")

NS_ASSUME_NONNULL_BEGIN

typedef struct {} NSFastEnumerationState;

@protocol NSFastEnumeration

- (NSUInteger)countByEnumeratingWithState:(NSFastEnumerationState *)state objects:(id __unsafe_unretained [])buffer count:(NSUInteger)len;

@end

@interface NSString : NSObject<NSCopying>
@end

@interface NSNumber : NSObject

+ (instancetype)numberWithInt:(NSInteger)integer;
+ (instancetype)numberWithBool:(BOOL)bool;
+ (instancetype)numberWithDouble:(double)d;

@end

@interface NSArray<Element> : NSObject<NSFastEnumeration>

+ (instancetype)arrayWithObjects:(const id _Nonnull [])objects count:(NSUInteger)count;
- (Element)objectAtIndexedSubscript:(NSUInteger)index;

@end

@interface NSDictionary<Key, Element> : NSObject<NSFastEnumeration>

+ (instancetype)dictionaryWithObjects:(const id _Nonnull [])objects
                              forKeys:(const id<NSCopying> _Nonnull [])keys
                                count:(NSUInteger)count;

- (nullable Element)objectForKeyedSubscript:(Key)key;

@end

NS_ASSUME_NONNULL_END
