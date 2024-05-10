//
//  NSHTTPURLResponse+MPAdditions.h
//
//  Copyright 2018-2019 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

extern NSString * const kMoPubHTTPHeaderContentType;

@interface NSHTTPURLResponse (MPAdditions)

- (NSStringEncoding)stringEncodingFromContentType:(NSString *)contentType;

@end
