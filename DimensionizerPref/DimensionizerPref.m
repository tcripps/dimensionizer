//
//  DimensionizerPref.m
//  DimensionizerPreferencePane
//
//  Created by Travis Cripps on 2/8/08.
//  Copyright (c) 2008 Hivelogic. All rights reserved.
//

#import "DimensionizerPref.h"
#import "BundleUserDefaults.h"


@implementation DimensionizerPref


+ (void)initialize {
    BundleUserDefaults *ud = [[BundleUserDefaults alloc] initWithPersistentDomainName: [[NSBundle bundleForClass:[self class]] bundleIdentifier]];
    [[NSUserDefaultsController sharedUserDefaultsController] _setDefaults: ud];
    [ud release];
    
    [DimensionizerPref setupDefaults];
}


+ (void)setupDefaults {
    NSDictionary *_defaultPreferences = [NSDictionary dictionaryWithObjectsAndKeys:
                                         [NSNumber numberWithInt: HTML], PRIMARY_OUTPUT_FORMAT_KEY,
                                         [NSNumber numberWithInt: CSS], SECONDARY_OUTPUT_FORMAT_KEY,
                                         [NSArray array], PRIMARY_CUSTOM_FORMAT,
                                         [NSArray array], SECONDARY_CUSTOM_FORMAT,
                                         nil];
    
    [[NSUserDefaults standardUserDefaults] registerDefaults: _defaultPreferences];
    
    // Set the initial values in the shared user defaults controller
    //[[NSUserDefaultsController sharedUserDefaultsController] setInitialValues: _defaultPreferences];
}


- (void) willSelect {
	NSCharacterSet *set = [NSCharacterSet characterSetWithCharactersInString: @""];
    
	[primaryCustomFormatField setTokenizingCharacterSet: set];
	[primaryCustomFormatField setTokenStyle: NSPlainTextTokenStyle];
    
    [secondaryCustomFormatField setTokenizingCharacterSet: set];
	[secondaryCustomFormatField setTokenStyle: NSPlainTextTokenStyle];
    
    [predefinedHeightField setTokenizingCharacterSet: set];
    [predefinedHeightField setObjectValue: [NSArray arrayWithObject: HEIGHT_TOKEN]];
    
    [predefinedHeightField2 setTokenizingCharacterSet: set];
    [predefinedHeightField2 setObjectValue: [NSArray arrayWithObject: HEIGHT_TOKEN]];
    
    [predefinedNameField setTokenizingCharacterSet: set];
    [predefinedNameField setObjectValue: [NSArray arrayWithObject: NAME_TOKEN]];
    
    [predefinedNameField2 setTokenizingCharacterSet: set];
    [predefinedNameField2 setObjectValue: [NSArray arrayWithObject: NAME_TOKEN]];
    
    [predefinedWidthField setTokenizingCharacterSet: set];
    [predefinedWidthField setObjectValue: [NSArray arrayWithObject: WIDTH_TOKEN]];
    
    [predefinedWidthField2 setTokenizingCharacterSet: set];
    [predefinedWidthField2 setObjectValue: [NSArray arrayWithObject: WIDTH_TOKEN]];
}


- (void) mainViewDidLoad {
    [self registerAsObserver];
}

- (void) registerAsObserver {
	NSLog(@"Registering as an observer to 'values'.");
	@try {
		[[NSUserDefaultsController sharedUserDefaultsController] addObserver: self
																  forKeyPath: @"values"
																	 options: (NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld)
																	 context: NULL];
	} 
	@catch (id theException) {
		NSLog(@"Couldn't unregister as an observer to 'values': %@", theException);
	}
}

- (void) didUnselect {
    [self unregisterForChangeNotification];
}

- (void) unregisterForChangeNotification {
	NSLog(@"UN-registering as an observer to 'values'.");
    [[NSUserDefaultsController sharedUserDefaultsController] removeObserver: self forKeyPath:@"values"];
}


- (void) observeValueForKeyPath: (NSString *)keyPath
                       ofObject: (id)object
                         change: (NSDictionary *)change
                        context: (void *)context
{
    NSLog(@"observeValueForKeyPath called");
    if ([keyPath isEqual: @"values"]) {
        [[[NSUserDefaultsController sharedUserDefaultsController] defaults] synchronize];
    }
    
    // Be sure to call the super implementation if the superclass implements it.
    [super observeValueForKeyPath: keyPath
                         ofObject: object
                           change: change
                          context: context];
}


/*
- (void) dealloc {
    
    [super dealloc];
}
 */


/*
- (NSArray *) tokenField: (NSTokenField *)tokenField shouldAddObjects: (NSArray *)tokens atIndex: (unsigned)index {
	//NSLog(@"shouldAddObjects");
	NSEnumerator* enumerator = [tokens objectEnumerator];
	id anObject;
	
	int i = 0;
	while (anObject = [enumerator nextObject]) {
		// code to act on each element as it is returned
		//NSLog(@"%d: %@", i++, [anObject description]);
	}
    
	return tokens; //array
}
*/


- (NSString *) tokenField: (NSTokenField *)tokenField displayStringForRepresentedObject: (id)representedObject {
	//NSLog(@"displayStringForRepresentedObject");
	NSString *string;
    if ([(NSString *)representedObject isEqualToString: HEIGHT_TOKEN]) {
        string = @"height";
    } else if ([(NSString *)representedObject isEqualToString: WIDTH_TOKEN]) {
        string = @"width";
    } else if ([(NSString *)representedObject isEqualToString: NAME_TOKEN]) {
        string = @"name";
    } else {
        string = representedObject;
    }
    
	return string;
}

- (NSTokenStyle) tokenField: (NSTokenField *)tokenField styleForRepresentedObject: (id)representedObject {
	//NSLog(@"styleForRepresentedObject: Field %@ with value: %@", tokenField, representedObject);
    if ([(NSString *)representedObject isEqualToString: HEIGHT_TOKEN] || 
        [(NSString *)representedObject isEqualToString: WIDTH_TOKEN] ||
        [(NSString *)representedObject isEqualToString: NAME_TOKEN]) {
        return NSRoundedTokenStyle;
    }
	return NSPlainTextTokenStyle;
}


- (BOOL) tokenField: (NSTokenField *)tokenField writeRepresentedObjects: (NSArray *)objects toPasteboard: (NSPasteboard *)pboard {
    NSLog(@"writeRepresentedObjectstoPasteboard: %@", pboard);
    BOOL result = YES;
    
    NSMutableString *string = [NSMutableString stringWithCapacity: 100];
    
    NSEnumerator* enumerator = [objects objectEnumerator];
    id anObject;
    
    //int i = 0;
    while (anObject = [enumerator nextObject]) {
        //NSLog(@"%d: %@", i++, [anObject description]);
        [string appendString: (NSString *)anObject];
    }
    
    //NSLog(@"result: %@", string);
    
    result = [pboard setString: string forType: NSStringPboardType];
    //[pboard setValue: objects];
    
    return result;
}

- (NSArray *) tokenField: (NSTokenField *)tokenField readFromPasteboard: (NSPasteboard *)pboard {
    NSMutableArray *result = [NSMutableArray array];
    
    NSString *copiedString = [pboard stringForType: NSStringPboardType];
    // NSLog(@"tokenField readFromPasteboard: string from pasteboard: %@", copiedString);
    if (copiedString) {
        // Find the tokens
        int searchLocation = 0;
        int copyStart = 0;
        int stringLength = [copiedString length];
        while (searchLocation < stringLength) {
            NSRange searchRange = NSMakeRange(searchLocation, (stringLength - searchLocation));
            NSRange tokenStartMatchRange = [copiedString rangeOfString: @"^__" options: NSLiteralSearch range: searchRange];
            if (tokenStartMatchRange.location == NSNotFound) {
                // NSLog(@"tokenField readFromPasteboard: Did not find the token start string.  Adding remainder of string from position: %d to the result.", copyStart);
                
                // Put the remainder of the string into the result array.
                NSString *theMatch = [copiedString substringFromIndex: copyStart];
                [result addObject: theMatch];
                break;
            } else {
                // NSLog(@"tokenField readFromPasteboard: Found the token start string at: %d.  Will search for the token end string...", tokenStartMatchRange.location);
                
                NSRange tokenSearchRange = NSMakeRange(tokenStartMatchRange.location, (stringLength - tokenStartMatchRange.location));
                NSRange tokenEndRange = [copiedString rangeOfString: @"__^" options: NSLiteralSearch range: tokenSearchRange];
                if (tokenEndRange.location == NSNotFound) { // Didn't find the end of the token
                    // NSLog(@"tokenField readFromPasteboard: Did not find the token end string.  Adding remainder of string from position: %d to the result.", copyStart);
                    // Put the remainder of the string into the result array.
                    NSString *theMatch = [copiedString substringFromIndex: copyStart];
                    [result addObject: theMatch];
                    break;
                } else {
                    // NSLog(@"tokenField readFromPasteboard: Found the token end string at %d.  But is the fragment a token?.", tokenEndRange.location);
                    
                    NSRange matchRange = NSMakeRange(tokenStartMatchRange.location, ((tokenEndRange.location + tokenEndRange.length) - tokenStartMatchRange.location));
                    NSString *theToken = [copiedString substringWithRange: matchRange];
                    
                    if ([theToken isEqualToString: NAME_TOKEN] ||
                        [theToken isEqualToString: HEIGHT_TOKEN] ||
                        [theToken isEqualToString: WIDTH_TOKEN]) {
                        // We found a token!
                        // NSLog(@"tokenField readFromPasteboard Found a token.");
                        
                        NSString *preToken = [copiedString substringWithRange: NSMakeRange(copyStart, (tokenStartMatchRange.location - copyStart))];
                        [result addObject: preToken];
                        [result addObject: theToken];
                        
                        searchLocation = tokenEndRange.location + tokenEndRange.length;
                        copyStart = searchLocation;
                    } else {
                        // Not a token!
                        searchLocation = tokenStartMatchRange.location + 1;
                    }
                }
            }
        }
    }
    
    // NSLog(@"tokenField readFromPasteboard result: %@", result);
    return result;
}


@end
