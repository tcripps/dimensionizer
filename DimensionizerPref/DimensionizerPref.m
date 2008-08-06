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

/*!
 @function
 @abstract   Sets up the User Defaults dictionary with the default values it should expect.
 @discussion The user defaults controller will only write the values that are different from these default values to the user defaults database.
 */
+ (void)setupDefaults {
    NSDictionary *_defaultPreferences = [NSDictionary dictionaryWithObjectsAndKeys:
                                         [NSNumber numberWithInt: HTML], PRIMARY_OUTPUT_FORMAT_KEY,
                                         [NSNumber numberWithInt: CSS], SECONDARY_OUTPUT_FORMAT_KEY,
                                         [NSArray array], PRIMARY_CUSTOM_FORMAT,
                                         [NSArray array], SECONDARY_CUSTOM_FORMAT,
                                         nil];
    
    [[NSUserDefaults standardUserDefaults] registerDefaults: _defaultPreferences];
}

/*!
 @function
 @abstract   Callback function that is executed when the main view is set up and prepared to be displayed.
 @discussion We use this as a hook to set up the tokens and tokenizing.
 */
- (void) mainViewDidLoad {
	//NSLog(@"Main view did load.");
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

/*!
 @function
 @abstract   Registers this controller as an observer of the "values" key of the shared user defaults controller.
 @discussion We do this so we will get notifications when the user defaults values change.
 */
- (void) registerAsObserver {
	//NSLog(@"Registering as an observer to 'values'.");
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver: self
															  forKeyPath: @"values"
																 options: (NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld)
																 context: NULL];
}

/*!
 @function
 @abstract   Callback function that is executed when the main application is about to display the preference pane’s main view.
 @discussion 
 */
- (void) willSelect {
	//NSLog(@"Will select.");
	
}

/*!
 @function
 @abstract   Notification callback function received when the main application has just displayed the preference pane’s main view.
 @discussion We're using it to register as an observer to the shared user defaults object.
 */
- (void) didSelect {
	//NSLog(@"Did select.");
    [self registerAsObserver];
}

/*!
 @function
 @abstract   Notification callback function received when the main application has just stopped displaying the preference pane’s main view.
 @discussion We're using it to unregister as an observer to the shared user defaults object.
 */
- (void) didUnselect {
	//NSLog(@"Did unselect.");
    [self unregisterForChangeNotification];
}

- (void) unregisterForChangeNotification {
	//NSLog(@"UN-registering as an observer to 'values'.");
	@try {
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver: self forKeyPath:@"values"];
	}
	@catch (NSException * e) {
		NSLog(@"Couldn't unregister as an observer to 'values': %@", e);
	}
}

/*!
 @function
 @abstract   Notification callback function received when the value of the observed object changes.
 @discussion In this case, we're using it to know when the user defaults values have changed, so we can synchronize the user defaults.
 @param      keyPath to observe
 @param      object whose keyPath to observe
 @param      change dictionary
 @param      context of the changes
 */
- (void) observeValueForKeyPath: (NSString *)keyPath
                       ofObject: (id)object
                         change: (NSDictionary *)change
                        context: (void *)context
{
    //NSLog(@"observeValueForKeyPath called");
    if ([keyPath isEqualToString: @"values"]) {
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

/*!
    @function
    @abstract   NSTokenField delegate method that determines the display string for a token.
    @discussion 
    @param      tokenField that is invoking the method
	@param      representedObject the token to represent
    @result     the string that should be displayed for the token
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

/*!
 @function
 @abstract   NSTokenField delegate method that determines the token style for a token.
 @discussion 
 @param      tokenField that is invoking the method
 @param      representedObject the token to represent
 @result     the token style that should be displayed for the token
 */
- (NSTokenStyle) tokenField: (NSTokenField *)tokenField styleForRepresentedObject: (id)representedObject {
	//NSLog(@"styleForRepresentedObject: Field %@ with value: %@", tokenField, representedObject);
    if ([(NSString *)representedObject isEqualToString: HEIGHT_TOKEN] || 
        [(NSString *)representedObject isEqualToString: WIDTH_TOKEN] ||
        [(NSString *)representedObject isEqualToString: NAME_TOKEN]) {
        return NSRoundedTokenStyle;
    }
	return NSPlainTextTokenStyle;
}

/*!
 @function
 @abstract   NSTokenField delegate method that places a representation of the tokens onto the pasteboard.
 @discussion We use this method to serialize the tokens into a format that we can parse.
 @param      tokenField that is invoking the method
 @param      objects to write to the pasteboard
 @param		 pboard to which the objects' representation should be written
 @result     the result of putting the representation of the objects onto the pasteboard
 */
- (BOOL) tokenField: (NSTokenField *)tokenField writeRepresentedObjects: (NSArray *)objects toPasteboard: (NSPasteboard *)pboard {
    //NSLog(@"writeRepresentedObjectstoPasteboard: %@", pboard);
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

/*!
 @function
 @abstract   NSTokenField delegate method that reads the pasteboard's representation of tokens and deserializes it into real tokens.
 @discussion We use this method to unserialize the tokens from the format we serialized them to with the 'writeRepresentedObjects'... function.
 @param      tokenField that is invoking the method
 @param		 pboard to which the objects' representation should be written
 @result     the array of tokens from the pasteboard representation
 */
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
