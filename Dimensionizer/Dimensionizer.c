/*
 *  Dimensionizer.c
 *  Dimensionizer
 *
 *  Created by Travis Cripps on 1/16/08.
 *  Copyright 2008 Hivelogic. All rights reserved.
 *
 */

#include "Dimensionizer.h"



// -----------------------------------------------------------------------------
//  Compilation directives
// -----------------------------------------------------------------------------

#define LOG_ENTRY_POINTS            true    // set this TRUE to log routine entry points to the console
#define LOG_RECORDS                 true
#define FINDER_ONLY                 true    // set this TRUE if you want your CMM to only appear in the Finder




// -----------------------------------------------------------------------------
//  Defines
// -----------------------------------------------------------------------------

// Predefined menu format strings
#define MENU_FORMAT_STRING "%@: %d w x %d h (%0.0f dpi)"
#define HTML_FORMAT_STRING "<img src=\"%@\" height=\"%d\" width=\"%d\" alt=\"\" />"
#define CSS_FORMAT_STRING "url(%@);\nheight: %dpx;\nwidth: %dpx;"

// Image info dictionary key names
#define IMAGE_NAME_KEY CFSTR("ImageName")
#define IMAGE_HEIGHT_KEY CFSTR("ImageHeight")
#define IMAGE_WIDTH_KEY CFSTR("ImageWidth")
#define IMAGE_RESOLUTION_KEY CFSTR("ImageResolution")




// -----------------------------------------------------------------------------
//  Globals
// -----------------------------------------------------------------------------

SInt32 gNumCommandIDs = 0;

Boolean gHasAttributeAndModifierKeys = false;

EventHandlerUPP gMenuEventHandlerUPP = NULL;
EventHandlerRef gMenuEventHandlerRef = NULL;

/* Holds a dictionary of dictionaries containing image information. 
 * The outer dictionary is keyed by menu command id.
 */
CFMutableDictionaryRef gImageFileInfoDict = NULL;

/* Controls the logging output level. */
enum LogLevel gLogLevel = LogLevelDebug;





// -----------------------------------------------------------------------------
//  Exported function implementations
// -----------------------------------------------------------------------------

/*****************************************************
 *
 * Routine:  DimensionizerCMPlugIn_Factory(thisInstance, iid, ppv)
 *
 * Purpose:  Implementation of the factory function for this type.
 *
 * Inputs:   allocator - CFAllocatorRef that will allocate the memory for the instance of the plugin
 *           typeID - CFUUIDRef of the requested type of plugin
 *
 * Returns:  void * - a pointer to the allocated instance of the CM plugin
 */
void *DimensionizerCMPlugIn_Factory(CFAllocatorRef allocator, CFUUIDRef typeID) {
#pragma unused (allocator)
    
#if LOG_ENTRY_POINTS
    printf("DimensionizerCMPlugIn_Factory(%p, %p)\n", allocator, typeID); fflush(stdout);
#endif LOG_ENTRY_POINTS

    void *result = NULL;

    // If correct type is being requested, allocate an
    // instance of TestType and return the IUnknown interface.
    if (CFEqual(typeID, kContextualMenuTypeID)) {
        result = (void *)DimensionizerCMPlugIn_Alloc(kDimensionizerCMPlugIn_FactoryID);
    }
    return result;
} /* DimensionizerCMPlugIn_Factory */







// -----------------------------------------------------------------------------
//  COM interface
// -----------------------------------------------------------------------------

/*****************************************************
 *
 * Routine:  DimensionizerCMPlugIn_QueryInterface(thisInstance, iid, ppv)
 *
 * Purpose:  Implementation of the IUnknown QueryInterface function.
 *
 * Inputs:   thisInstance - pointer to the instance of the plugin
 *           iid - REFIID of the interface id being requested
 *           ppv - LPVOID pointer to pass back the instance of the plugin if the iid matches the UUID of our CM plugin.
 *
 * Returns:  HRESULT - the resonse code result of the query
 */
static HRESULT DimensionizerCMPlugIn_QueryInterface(void *thisInstance, REFIID iid, LPVOID *ppv) {
    
#if LOG_ENTRY_POINTS
    printf("DimensionizerCMPlugIn_QueryInterface(%p, %p, %p)\n", thisInstance, &iid, ppv);
    fflush(stdout);
#endif

    HRESULT result = S_OK;  // assume success
    
    // Create a CoreFoundation UUIDRef for the requested interface.
    CFUUIDRef interfaceID = CFUUIDCreateFromUUIDBytes(NULL, iid);

    // Test the requested ID against the valid interfaces.
    if (CFEqual(interfaceID, kContextualMenuInterfaceID)) {
        // If our interface was requested, bump the ref count, set the ppv parameter equal to the 
        // instance, and return good status.
        DimensionizerCMPlugIn_AddRef(thisInstance);

        *ppv = thisInstance;
        CFRelease(interfaceID);
    } else if (CFEqual(interfaceID, IUnknownUUID)) {
        // If the IUnknown interface was requested, same as above.
        DimensionizerCMPlugIn_AddRef(thisInstance);

        *ppv = thisInstance;
        CFRelease(interfaceID);
    } else {
        // Requested interface unknown, bail with result.
        *ppv = NULL;
        CFRelease(interfaceID);
        result = E_NOINTERFACE;
    }
    return result;
}


/*****************************************************
 *
 * Routine:  DimensionizerCMPlugIn_AddRef(thisInstance)
 *
 * Purpose:  Implementation of reference counting for this type. Whenever an interface
 *           is requested, bump the refCount for the instance. NOTE: returning the
 *           refcount is a convention but is not required so don't rely on it.
 *
 * Inputs:   thisInstance - pointer to the instance of the plugin
 *
 * Returns:  ULONG - the reference count of the instance of the plugin
 */
static ULONG DimensionizerCMPlugIn_AddRef(void *thisInstance) {
    
#if LOG_ENTRY_POINTS
    printf("DimensionizerCMPlugIn_AddRef(%p)\n",  thisInstance);
    fflush(stdout);
#endif
    
    ((DimensionizerCMPlugIn_ptr)thisInstance)->refCount += 1;
    return ((DimensionizerCMPlugIn_ptr)thisInstance)->refCount;
} /* DimensionizerCMPlugIn_AddRef */


/*****************************************************
 *
 * Routine:  DimensionizerCMPlugIn_Release(thisInstance)
 *
 * Purpose:  When an interface is released, decrement the refCount.
 *           If the refCount goes to zero, deallocate the instance.
 *
 * Inputs:   thisInstance - pointer to the instance of the plugin
 *
 * Returns:  ULONG - the reference count of the instance of the plugin
 */
static ULONG DimensionizerCMPlugIn_Release(void *thisInstance) {

#if LOG_ENTRY_POINTS
    printf("DimensionizerCMPlugIn_Release(%p)\n", thisInstance);
    fflush(stdout);
#endif

    ULONG result = 0;
    
    ((DimensionizerCMPlugIn_ptr)thisInstance)->refCount -= 1;
    if (((DimensionizerCMPlugIn_ptr)thisInstance)->refCount == 0) {
        DimensionizerCMPlugIn_Dealloc((DimensionizerCMPlugIn_ptr)thisInstance);
    } else {
        result = ((DimensionizerCMPlugIn_ptr)thisInstance)->refCount;
    }
    return result;
} /* DimensionizerCMPlugIn_Release */





// -----------------------------------------------------------------------------
//  COM utility functions
// -----------------------------------------------------------------------------

/*****************************************************
 *
 * Routine:  DimensionizerCMPlugIn_Alloc(factoryID)
 *
 * Purpose:  Utility function that allocates a new instance.
 *
 * Inputs:   factoryID - The CFUUID factory ID of the plugin
 *
 * Returns:  DimensionizerCMPlugIn_ptr - A pointer to the instance we create
 */
static DimensionizerCMPlugIn_ptr DimensionizerCMPlugIn_Alloc(CFUUIDRef factoryID) {
    
#if LOG_ENTRY_POINTS
    printf("DimensionizerCMPlugIn_Alloc(%p)\n", factoryID);
#endif
    
    // Allocate memory for the new instance.
    DimensionizerCMPlugIn_ptr newInstance = (DimensionizerCMPlugIn_ptr)malloc(sizeof(DimensionizerCMPlugIn_rec));
    
    // Point to the function table.
    newInstance->cmInterface = &gDimensionizerCMInterface;
    
    // Retain and keep an open instance refcount for each factory.
    newInstance->factoryID = CFRetain(factoryID);
    CFPlugInAddInstanceForFactory(factoryID);
    
    // This function returns the IUnknown interface so set the refCount to one.
    newInstance->refCount = 1;
    return newInstance;
}   /* DimensionizerCMPlugIn_Alloc */


/*****************************************************
 *
 * Routine:  DimensionizerCMPlugIn_Dealloc(thisInstance)
 *
 * Purpose:  Utility function that deallocates the instance when the refCount goes to zero.
 *
 * Inputs:   thisInstance - DimensionizerCMPlugIn_ptr to the instance whose refCount we inspect
 *
 * Returns:  void
 */
static void DimensionizerCMPlugIn_Dealloc(DimensionizerCMPlugIn_ptr thisInstance)
{
#if LOG_ENTRY_POINTS
    printf("DimensionizerCMPlugIn_Dealloc(%p)\n", thisInstance);
#endif LOG_ENTRY_POINTS
    CFUUIDRef factoryID = thisInstance->factoryID;
    
    free(thisInstance);
    if (factoryID) {
        CFPlugInRemoveInstanceForFactory(factoryID);
        CFRelease(factoryID);
    }
}   /* DimensionizerCMPlugIn_Dealloc */






// -----------------------------------------------------------------------------
//  Contextual menus interface
// -----------------------------------------------------------------------------

/*****************************************************
 *
 * Routine:  DimensionizerCMPlugIn_ExamineContext(thisInstance, context, outCommandPairs)
 *
 * Purpose:  The implementation of the ExamineContext interface function.
 *
 * Inputs:   thisInstance - pointer to the instance of the plugin
 *           context - AEDesc event context
 *           outCommandPairs - AEDescList of menu commands to be output
 *
 * Returns:  OSStatus - the error code
 */
static OSStatus DimensionizerCMPlugIn_ExamineContext(void *thisInstance, const AEDesc *context, AEDescList *outCommandPairs) {

#if LOG_ENTRY_POINTS
    printf("DimensionizerCMPlugIn_ExamineContext(%p, %p, %p)\n", thisInstance, context, outCommandPairs);
    fflush(stdout);
#endif
    
    if (gLogLevel >= LogLevelDebug) {
        LogContext(context);
    }

    OSStatus result = noErr;
    
#if FINDER_ONLY
    ProcessInfoRec tPIR;
    ProcessSerialNumber tPSN = {0, kCurrentProcess};
    
    tPIR.processInfoLength = sizeof(ProcessInfoRec);
    tPIR.processName = nil;
    tPIR.processAppSpec = nil;
    
    OSStatus status = GetProcessInformation(&tPSN, &tPIR);
    if (noErr == status) {
        if (tPIR.processSignature != 'MACS' || tPIR.processType != 'FNDR') {
            LogString(LogLevelWarn, CFSTR("Host process is not the Finder!\n"));
            return noErr;
        }
    } else {
        LogString(LogLevelError, CFSTR("Error getting process information: %ld.  Could not determine if host process is the Finder.\n"), status);
    }
#endif
    
    /*
    // Install Carbon (menu) event handler
    const EventTypeSpec menuEvents[] = {
        {kEventClassMenu, kEventMenuPopulate},
    };

    if (!gMenuEventHandlerUPP) {
        gMenuEventHandlerUPP = NewEventHandlerUPP(MenuEvent_Handle);
    }
    InstallApplicationEventHandler(gMenuEventHandlerUPP, GetEventTypeCount(menuEvents), menuEvents, NULL, &gMenuEventHandlerRef);
     */

    // Initialize the command id sequence
    gNumCommandIDs = 0;
    gImageFileInfoDict = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    
    // Verify that we've got an up-to-date CMM
    verify_noerr(Gestalt(gestaltContextualMenuAttr, &result));
    if ((result & (1 << gestaltContextualMenuHasAttributeAndModifierKeys)) != 0) {
        gHasAttributeAndModifierKeys = true;
        LogString(LogLevelInfo, CFSTR("DimensionizerCMPlugIn_ExamineContext: CMM supports Attributes and Modifiers keys.\n"));
    } else {
        gHasAttributeAndModifierKeys = false;
        LogString(LogLevelInfo, CFSTR("DimensionizerCMPlugIn_ExamineContext: CMM does not support Attributes and Modifiers keys.\n"));
    }

    // Make sure the descriptor isn't null.
    if (context) {
        //LogString(LogLevelDebug, CFSTR("DimensionizerCMPlugIn_ExamineContext: Raw AEDesc type: '%4.4s'\n"), (Ptr) &context->descriptorType);
        
        if (DescIsOfTypeOrCanBeCoercedToType(context, typeAEList)) {
            CreateMenuWithWithContext(context, outCommandPairs);
        }
    }

    if (gLogLevel >= LogLevelDebug) {
        LogContext(outCommandPairs);
    }

    fflush(stdout);

    return noErr;
}   /* DimensionizerCMPlugIn_ExamineContext */


/*****************************************************
 *
 * Routine:  DimensionizerCMPlugIn_HandleSelection(thisInstance, context, commandID)
 *
 * Purpose:  The implementation of the HandleSelection test interface function.
 *
 * Inputs:   thisInstance - pointer to the instance of the plugin
 *           context - AEDesc event context
 *           commandID -SInt32 menu command ID of the chosen command
 *
 * Returns:  OSStatus - the error code
 */
static OSStatus DimensionizerCMPlugIn_HandleSelection(void *thisInstance, AEDesc *context, SInt32 commandID) {

#if LOG_ENTRY_POINTS
    printf("\nDimensionizerCMPlugIn_->DimensionizerCMPlugIn_HandleSelection(instance: %p, context: %p, commandID: 0x%08lX)\n",
            thisInstance, context, commandID);
#endif LOG_ENTRY_POINTS
    
    if (LogLevelDebug < gLogLevel) {
        LogContext(context);
    }
    LogString(LogLevelDebug, CFSTR("DimensionizerCMPlugIn_HandleSelection: commandID: %d\n"), commandID);
    LogString(LogLevelVerbose, CFSTR("DimensionizerCMPlugIn_HandleSelection: Raw AEDesc type: '%4.4s'\n"), (Ptr)&context->descriptorType);

    
    OSStatus result = noErr;
    
    // Sequence the command ids
    gNumCommandIDs = 0;
    Boolean isMainCommandType = (commandID % 2 != 0);
    SInt32 realCommandID = isMainCommandType ? commandID : commandID - 1;
    LogString(LogLevelDebug, CFSTR("DimensionizerCMPlugIn_HandleSelection: real commandID: %d\n"), realCommandID);
    
    CFNumberRef menuCommandID = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &realCommandID);
    if (gImageFileInfoDict && CFDictionaryContainsKey(gImageFileInfoDict, menuCommandID)) {
        LogString(LogLevelVerbose, CFSTR("DimensionizerCMPlugIn_HandleSelection: gImageFileInfoDict: %@\n"), gImageFileInfoDict);
        
        CFDictionaryRef imageInfoDict = CFDictionaryGetValue(gImageFileInfoDict, menuCommandID);
        if (imageInfoDict) {
            enum OutputFormat theFormat;
            CFStringRef theOutputString;
            int preferredFormat = -1;
            
            if (isMainCommandType) {
                LogString(LogLevelDebug, CFSTR("DimensionizerCMPlugIn_HandleSelection: getting primary output type preference.\n"));
                preferredFormat = OutputFormatPreferenceForKey(PRIMARY_OUTPUT_FORMAT_KEY);
                if (preferredFormat == -1) {
                    theFormat = HTML;
                } else {
                    theFormat = preferredFormat;
                }
            } else {
                LogString(LogLevelDebug, CFSTR("DimensionizerCMPlugIn_HandleSelection: getting secondary output type preference.\n"));
                preferredFormat = OutputFormatPreferenceForKey(SECONDARY_OUTPUT_FORMAT_KEY);
                if (preferredFormat == -1) {
                    theFormat = CSS;
                } else {
                    theFormat = preferredFormat;
                }
            }
            
            if (theFormat == Custom) {
                theOutputString = CreateCustomOutputFormatStringFromImageInfoDictWithOutputType(imageInfoDict, isMainCommandType);
            } else {
                theOutputString = CreateStringFromImageInfoDictWithOutputFormat(imageInfoDict, theFormat);
            }
            
            LogString(LogLevelDebug, CFSTR("DimensionizerCMPlugIn_HandleSelection: theOutputString: %@\n"), theOutputString);
            
            if (theOutputString) {
                PasteboardRef theClipboard;
                result = PasteboardCreate(kPasteboardClipboard, &theClipboard);
                if (result == noErr) {
                    result = AddStringToPasteboard(theClipboard, theOutputString);
                    LogString(LogLevelWarn, CFSTR("Error adding the output string to the clipboard: %d\n"), result);
                }
                
                CFRelease(theOutputString);
            }
        } else {
            LogString(LogLevelInfo, CFSTR("DimensionizerCMPlugIn_HandleSelection: Could not get dictionary for commandID key.\n"));
        }
        
    }
    
    CFRelease(menuCommandID);

    return noErr;
}   /* DimensionizerCMPlugIn_HandleSelection */


/*****************************************************
 *
 * Routine:  DimensionizerCMPlugIn_PostMenuCleanup(thisInstance)
 *
 * Purpose:  The implementation of the PostMenuCleanup test interface function.
 *
 * Inputs:   thisInstance - pointer to the instance of the plugin
 *
 * Returns:  void
 */
static void DimensionizerCMPlugIn_PostMenuCleanup(void *thisInstance) {

#if LOG_ENTRY_POINTS
    printf("DimensionizerCMPlugIn_PostMenuCleanup(instance: %p)\n", thisInstance);
#endif LOG_ENTRY_POINTS
    
    if (gMenuEventHandlerRef) {
        RemoveEventHandler(gMenuEventHandlerRef);
	}
    
    if (gMenuEventHandlerUPP) {
        DisposeEventHandlerUPP(gMenuEventHandlerUPP);
	}
    
    if (gImageFileInfoDict) {
        CFRelease(gImageFileInfoDict);
    }
    
}   /* DimensionizerCMPlugIn_PostMenuCleanup */






// -----------------------------------------------------------------------------
//  Main functions
// -----------------------------------------------------------------------------

/*****************************************************
*
* Routine:  CreateMenuWithWithContext(context, commandList)
*
* Purpose:  Create a menu with the item(s) in the context and put into the commandList
*
* Inputs:   context - AEDesc from the menu event
*           commandList - AEDescList of commands to which we append our menu command(s)
*
* Returns:  OSStatus - error code
*/
static OSStatus CreateMenuWithWithContext(const AEDesc *context, AEDescList *commandList)
{
    OSStatus result = noErr;
    
#if LOG_ENTRY_POINTS
    printf("CreateMenuWithWithContext(context: %p, commandList: %p)\n", context, commandList);
#endif LOG_ENTRY_POINTS    
    
    AEDescList theSubMenuCommands = { typeNull, NULL };
    
    // The first thing we should do is create an AEDescList of subcommands.
    // Set up the AEDescList.
    result = AECreateList(NULL, 0, false, &theSubMenuCommands);
    require_noerr(result, CreateMenuWithWithContext_Complete_fail);
    
    long numItems;
    result = AECountItems(context, &numItems);
    if (noErr == result) {
        LogString(LogLevelInfo, CFSTR("CreateMenuWithWithContext: Received %ld item(s) in the context.\n"), numItems);
        
        CFArrayRef imageInfoDictsArray = GetImageInfoForQualifiedItems(context);
        if (imageInfoDictsArray) { CFRetain(imageInfoDictsArray); }
        
        CFIndex numImages = CFArrayGetCount(imageInfoDictsArray);
        LogString(LogLevelInfo, CFSTR("CreateMenuWithWithContext: %ld items are images.\n"), numImages);
        
        if (numImages == 1) {
            CFDictionaryRef anImageInfoDict = (CFDictionaryRef)CFArrayGetValueAtIndex(imageInfoDictsArray, 0);
            if (anImageInfoDict) {
                CFStringRef menuCommandName = CreateStringFromImageInfoDictWithOutputFormat(anImageInfoDict, Menu);
                LogString(LogLevelDebug, CFSTR("CreateMenuWithWithContext: menuCommandName: %@\n"), menuCommandName);
                
                // Add the menu item to the contextual menu.
                SInt32 commandIDNumber = ++gNumCommandIDs;
                result = InsertCommandIntoCommandListWithOptionsSubmenu(menuCommandName, commandIDNumber, commandList, kMenuItemAttrDynamic, kMenuNoModifiers, NULL);
                
                // Add alt menu item
                SInt32 altCommandIDNumber = ++gNumCommandIDs;
                result = InsertCommandIntoCommandListWithOptionsSubmenu(menuCommandName, altCommandIDNumber, commandList, kMenuItemAttrDynamic, kMenuOptionModifier, NULL);
                
                // Add this image info dict to the global image file info dict keyed by the menu item's command id.
                CFNumberRef menuCommandID = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &commandIDNumber);
                CFDictionarySetValue(gImageFileInfoDict, menuCommandID, anImageInfoDict);
                
                // Clean up.
                CFRelease(menuCommandID);
                CFRelease(menuCommandName);
                
                LogString(LogLevelVerbose, CFSTR("CreateMenuWithWithContext: created menu item.\n"));
            }
        } else if (numImages > 1) {
            CFIndex i;
            for (i = 0; i < numImages; i++) {
                CFDictionaryRef anImageInfoDict = (CFDictionaryRef)CFArrayGetValueAtIndex(imageInfoDictsArray, i);
                if (anImageInfoDict) {
                    CFStringRef menuCommandName = CreateStringFromImageInfoDictWithOutputFormat(anImageInfoDict, Menu);
                    LogString(LogLevelDebug, CFSTR("CreateMenuWithWithContext: menuCommandName: %@\n"), menuCommandName);
                                        
                    // Add the menu item to the contextual menu.
                    SInt32 commandIDNumber = 1000 + (++gNumCommandIDs);
                    result = InsertCommandIntoCommandListWithOptionsSubmenu(menuCommandName, commandIDNumber, &theSubMenuCommands, kMenuItemAttrDynamic, kMenuNoModifiers, NULL);
                    
                    // Add alt menu item
                    SInt32 altCommandIDNumber = 1000 + (++gNumCommandIDs);
                    result = InsertCommandIntoCommandListWithOptionsSubmenu(menuCommandName, altCommandIDNumber, &theSubMenuCommands, kMenuItemAttrDynamic, kMenuOptionModifier, NULL);
					
					// Have to add a hidden item in order to break the dynamic menu item sequence.  Retarded hack.
					result = InsertCommandIntoCommandListWithOptionsSubmenu(CFSTR("Hidden Item"), 0, &theSubMenuCommands, kMenuItemAttrHidden, kMenuNoModifiers, NULL);
                    
                    // Add this image info dict to the global image file info dict keyed by the menu item's command id.
                    CFNumberRef menuCommandID = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &commandIDNumber);
                    CFDictionarySetValue(gImageFileInfoDict, menuCommandID, anImageInfoDict);
                    
                    // Clean up.
                    CFRelease(menuCommandID);
                    CFRelease(menuCommandName);
                    
                    LogString(LogLevelVerbose, CFSTR("CreateMenuWithWithContext: created menu item: %d.\n"), i);
                }
            }
            
            LogString(LogLevelDebug, CFSTR("CreateMenuWithWithContext: created %d submenu items.\n"), numImages);
            
            // Now, we need to create the supercommand which will "own" the subcommands.
            // The supercommand lives in the root command list.
            CFStringRef superCommandString = CFStringCreateWithFormat(kCFAllocatorDefault, 
                                                                      NULL, 
                                                                      CFSTR("%d images selected."), 
                                                                      numImages);
            
            result = InsertCommandIntoCommandListWithOptionsSubmenu(superCommandString, 0, commandList, 0, kMenuNoModifiers, &theSubMenuCommands);
            CFRelease(superCommandString);
        }
        
        if (imageInfoDictsArray) {
            CFRelease(imageInfoDictsArray);
        }
    }
    

// clean up after ourself
CreateMenuWithWithContext_fail:    ;
    AEDisposeDesc(&theSubMenuCommands);
    //AEDisposeDesc(&theSupercommand);
    
    
CreateMenuWithWithContext_Complete_fail:
        return result;

} /* CreateMenuWithWithContext */


/*****************************************************
 *
 * Routine:  GetImageInfoForQualifiedItems(inContext)
 *
 * Purpose:  Creates an Array of CFDictionaryRefs containing information about each image
 *           file that the FSRefs in the inContext point to.
 *
 * Inputs:   inContext - AEDesc the context of the menu event
 *
 * Returns:  CFArrayRef - the array of CFDictionaryRefs for the images
 */
static CFArrayRef GetImageInfoForQualifiedItems(const AEDesc *inContext) {
    long numItems, i;
    OSStatus result = AECountItems(inContext, &numItems);
    if (noErr == result) {
        LogString(LogLevelDebug, CFSTR("GetImageInfoForQualifiedItems: Got %ld item(s)!\n"), numItems);
        
        CFMutableArrayRef imageInfoDicts = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
        
        for (i = 1; i <= numItems; i++) {
            AEKeyword theAEKeyword;
            AEDesc theAEDesc;
            
            result = AEGetNthDesc(inContext, i, typeWildCard, &theAEKeyword, &theAEDesc);
            if (noErr != result) { continue; }
            
            FSRef theFSRef;
            Size dataSize = AEGetDescDataSize(&theAEDesc);
            /*
            if (AECoerceDesc(inContext, typeFSRef, &theAEDesc) == noErr) {
                LogString(LogLevelDebug, CFSTR("GetImageInfoForQualifiedItems: Item %ld coerced to an FSRef type.\n"), i);
                
                result = AEGetDescData(&theAEDesc, &theFSRef, dataSize);
                if (noErr != result) {
                    LogString(LogLevelWarn, CFSTR("GetImageInfoForQualifiedItems: Could not resolve FSRef for item %d.\n"), i);
                    continue; 
                }
            } else {
                LogString(LogLevelWarn, CFSTR("GetImageInfoForQualifiedItems: Could not coerce item %d to an FSRef type.\n"), i);
            }
            */
            
            if (theAEDesc.descriptorType == typeFSRef) {
                LogString(LogLevelDebug, CFSTR("GetImageInfoForQualifiedItems: Item %ld is an FSRef.\n"), i);
                
                result = AEGetDescData(&theAEDesc, &theFSRef, dataSize);
                if (noErr != result) {
                    LogString(LogLevelWarn, CFSTR("GetImageInfoForQualifiedItems: Could not resolve FSRef for item %d.\n"), i);
                    continue; 
                }
            } else if (theAEDesc.descriptorType == typeAlias) {
                LogString(LogLevelDebug, CFSTR("GetImageInfoForQualifiedItems: Item %ld is an alias.\n"), i);
                
                AliasHandle aliasHdl = (AliasHandle)NewHandle(dataSize);
                if (aliasHdl) {
                    Boolean wasChanged;
                    result = AEGetDescData(&theAEDesc, *aliasHdl, dataSize);
                    if (noErr == result) {
                        result = FSResolveAlias(NULL, aliasHdl, &theFSRef, &wasChanged);
                        if (noErr != result) { 
                            LogString(LogLevelWarn, CFSTR("GetImageInfoForQualifiedItems: Could not resolve alias for item %d.  Error code: %d\n"), i, result);
                            DisposeHandle((Handle)aliasHdl);
                            continue; 
                        }
                        
                        CFURLRef url = CFURLCreateFromFSRef(kCFAllocatorDefault, &theFSRef);
                        if (url) {
                            LogString(LogLevelDebug, CFSTR("GetImageInfoForQualifiedItems: Alias resolved to URL for FSRef: %@.\n"), url);
                            CFRelease(url);
                        } else {
                          LogString(LogLevelDebug, CFSTR("GetImageInfoForQualifiedItems: Alias resolved to URL for FSRef: %@.\n"), url);
                        }
                        
                    }
                    DisposeHandle((Handle)aliasHdl);
                }                
            } else {
                // Not a type we're interested in.
                continue;
            }
            
            FSSpec fileSpec;
            result = FSGetCatalogInfo(&theFSRef, kFSCatInfoNone, NULL, NULL, &fileSpec, NULL);
            if (noErr == result) {
                LogString(LogLevelVerbose, CFSTR("GetImageInfoForQualifiedItems: Got an FSSpec for item %d.\n"), i);
                
                //char path[256];
                
                //convert back to a path
                //fss2path(path, &fileSpec);
                //printf("FSSpec path... %s\n", path);
                
                if (QTUtil_IsImageFile(&fileSpec)) {
                    LogString(LogLevelDebug, CFSTR("GetImageInfoForQualifiedItems: Item %d is an image!\n"), i);
                    
                    CFDictionaryRef imageInfoDict = CreateImageInfoDictionaryWithFSRef(&theFSRef);
                    if (imageInfoDict) {
                        CFArrayAppendValue(imageInfoDicts, imageInfoDict);
                        CFRelease(imageInfoDict);
                    } else {
                        LogString(LogLevelWarn, CFSTR("GetImageInfoForQualifiedItems: Failed to get image info dictionary info for file %d.\n"), i);
                    }
                }
            } else {
                LogString(LogLevelInfo, CFSTR("GetImageInfoForQualifiedItems: Failed to get catalog info for file %d.  Error: %d.\n"), i, result);
            }
            
            AEDisposeDesc(&theAEDesc);
        }
        
        return imageInfoDicts;
    }
    
    return NULL;
} /* GetImageInfoForQualifiedItems */


/*****************************************************
 *
 * Routine:  CreateImageInfoDictionaryWithFSRef(theFSRef)
 *
 * Purpose:  Creates a CFDictionaryRef containing information about the image if
 *           theFSRef points to a file in an image format that QuickTime understands.
 *
 * Inputs:   theFSRef - FSRefPtr pointing to an FSRef for which we will try to get
 *                      image information.
 *
 * Returns:  CFDictionaryRef - the formatted CFStringRef in the desired format
 */
static CFDictionaryRef CreateImageInfoDictionaryWithFSRef(FSRefPtr theFSRef) {
    CFMutableDictionaryRef imageDict = NULL;
    FSSpec fileSpec;
    
    if (FSGetCatalogInfo(theFSRef, kFSCatInfoNone, NULL, NULL, &fileSpec, NULL) == noErr) {
        ImageDescriptionPtr imageDesc = QTUtil_GetImageDescription(&fileSpec);
        
        if (imageDesc) {
            CFURLRef imageURL = CFURLCreateFromFSRef(kCFAllocatorDefault, theFSRef);
            CFStringRef imageName;
            
            if (imageURL) {
                imageName = CFURLCopyLastPathComponent(imageURL);
                LogString(LogLevelDebug, CFSTR("CreateImageInfoDictionaryWithFSRef: Image name: %@\n"), imageName);
                CFRelease(imageURL);
            } else {
                LogString(LogLevelWarn, CFSTR("CreateImageInfoDictionaryWithFSRef: Could not get URL for file!!!\n"));
                return NULL;
            }
            
            imageDict = CFDictionaryCreateMutable(kCFAllocatorDefault, 4, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
            
            short height = imageDesc->height;
            short width = imageDesc->width;
            float resolution = FixedToFloat(imageDesc->vRes);
            CFNumberRef imageHeight = CFNumberCreate(kCFAllocatorDefault, kCFNumberShortType, &height);
            CFNumberRef imageWidth = CFNumberCreate(kCFAllocatorDefault, kCFNumberShortType, &width);
            CFNumberRef imageResolution = CFNumberCreate(kCFAllocatorDefault, kCFNumberFloatType, &resolution);
            
            CFDictionarySetValue (imageDict, IMAGE_NAME_KEY, imageName);
            CFDictionarySetValue (imageDict, IMAGE_HEIGHT_KEY, imageHeight);
            CFDictionarySetValue (imageDict, IMAGE_WIDTH_KEY, imageWidth);
            CFDictionarySetValue (imageDict, IMAGE_RESOLUTION_KEY, imageResolution);
            
            CFRelease(imageName);
            CFRelease(imageHeight);
            CFRelease(imageWidth);
            CFRelease(imageResolution);
        } else {
            LogString(LogLevelInfo, CFSTR("CreateImageInfoDictionaryWithFSRef: Could not get ImageDescription for FSRef!\n"));
        }
    } else {
        LogString(LogLevelInfo, CFSTR("CreateImageInfoDictionaryWithFSRef: Could not get FileSpec for FSRef!\n"));
    }
    
    return imageDict;
} /* CreateImageInfoDictionaryWithFSRef */


/*****************************************************
 *
 * Routine:  CreateStringFromImageInfoDictWithOutputFormat(imageInfoDict, format)
 *
 * Purpose:  Creates a CFStringRef in the desired format from the imageInfoDict.
 *
 * Inputs:   imageInfoDict - CFDictionaryRef containing information about an image
 *           format - OutputFormat of the string
 *
 * Returns:  CFStringRef - the formatted CFStringRef in the desired format
 */
static CFStringRef CreateStringFromImageInfoDictWithOutputFormat(CFDictionaryRef imageInfoDict, enum OutputFormat format) {
    CFStringRef resultString = NULL;
    short height = 0;
    short width = 0;
    float resolution = 0.0f;
    
    LogString(LogLevelDebug, CFSTR("CreateStringFromImageInfoDictWithOutputFormat: creating output string with format: %d"), format);
    
    if (CFNumberGetValue(CFDictionaryGetValue(imageInfoDict, IMAGE_HEIGHT_KEY), kCFNumberShortType, &height) &&
        CFNumberGetValue(CFDictionaryGetValue(imageInfoDict, IMAGE_WIDTH_KEY), kCFNumberShortType, &width) &&
        CFNumberGetValue(CFDictionaryGetValue(imageInfoDict, IMAGE_RESOLUTION_KEY), kCFNumberFloatType, &resolution)) {
        CFStringRef fileName = CFDictionaryGetValue(imageInfoDict, IMAGE_NAME_KEY);
        
        switch (format) {
            case Menu:
                resultString = CFStringCreateWithFormat(kCFAllocatorDefault, 
                                                        NULL, 
                                                        CFSTR(MENU_FORMAT_STRING), 
                                                        fileName,
                                                        height,
                                                        width,
                                                        resolution);
                break;
            case HTML:
                resultString = CFStringCreateWithFormat(kCFAllocatorDefault, 
                                                        NULL, 
                                                        CFSTR(HTML_FORMAT_STRING), 
                                                        fileName,
                                                        height,
                                                        width);
                break;
            case CSS:
                resultString = CFStringCreateWithFormat(kCFAllocatorDefault, 
                                                        NULL, 
                                                        CFSTR(CSS_FORMAT_STRING), 
                                                        fileName,
                                                        height,
                                                        width);
                break;
        }
        
    }
    
    return resultString;
} /* CreateStringFromImageInfoDictWithOutputFormat */


/*****************************************************
 *
 * Routine:  CreateCustomOutputFormatStringFromImageInfoDictWithOutputType(imageInfoDict, isMainType)
 *
 * Purpose:  Creates a CFStringRef in the custom format defined in the preferences from the imageInfoDict.
 *
 * Inputs:   imageInfoDict - CFDictionaryRef containing information about an image
 *           isMainType - indicates whether we should construct the output using the primary or secondary
 *           custom output preference
 *
 * Returns:  CFStringRef - the formatted CFStringRef in the desired format
 */
static CFStringRef CreateCustomOutputFormatStringFromImageInfoDictWithOutputType(CFDictionaryRef imageInfoDict, Boolean isMainType) {
    CFStringRef resultString = NULL;
    short height = 0;
    short width = 0;
    
    LogString(LogLevelDebug, CFSTR("CreateCustomOutputFormatStringFromImageInfoDictWithOutputType: creating custom output string...\n"));
    
    if (CFNumberGetValue(CFDictionaryGetValue(imageInfoDict, IMAGE_HEIGHT_KEY), kCFNumberShortType, &height) &&
        CFNumberGetValue(CFDictionaryGetValue(imageInfoDict, IMAGE_WIDTH_KEY), kCFNumberShortType, &width)) {
        CFStringRef fileName = CFDictionaryGetValue(imageInfoDict, IMAGE_NAME_KEY);
        
        // Make sure we always have the latest values.
        CFPreferencesAppSynchronize(kDimensionizerCMPBundleIdentifier);
    
        // Need the array from prefs and need to know if it's primary or secondary in order to get it.
        CFArrayRef customFormatParts;

        if (isMainType) {
            CFPropertyListRef value = CFPreferencesCopyAppValue(PRIMARY_CUSTOM_FORMAT, kDimensionizerCMPBundleIdentifier);
            if (value && CFGetTypeID(value) == CFArrayGetTypeID()) {
                customFormatParts = CFArrayCreateCopy(kCFAllocatorDefault, value);
                CFRelease(value);
            }
        } else {
            CFPropertyListRef value = CFPreferencesCopyAppValue(SECONDARY_CUSTOM_FORMAT, kDimensionizerCMPBundleIdentifier);
            if (value && CFGetTypeID(value) == CFArrayGetTypeID()) {
                customFormatParts = CFArrayCreateCopy(kCFAllocatorDefault, value);
                CFRelease(value);
            }
        }
        
        if (customFormatParts) {
            LogString(LogLevelDebug, CFSTR("CreateCustomOutputFormatStringFromImageInfoDictWithOutputType: custom format from preferences: %@\n"), customFormatParts);
            
            // Enumerate through the array adding pieces & substituting the params if they are equal to the tokens.
            CFMutableStringRef mutableString = CFStringCreateMutable(kCFAllocatorDefault, 0);
            if (mutableString) {
                CFIndex count = CFArrayGetCount(customFormatParts);
                CFIndex i = 0;
                for (i = 0; i < count; i++) {
                    CFStringRef part = CFArrayGetValueAtIndex(customFormatParts, i);
                    if (CFStringCompare(part, WIDTH_TOKEN, 0) == kCFCompareEqualTo) {
                        CFStringAppendFormat(mutableString, NULL, CFSTR("%d"), width);
                    } else if (CFStringCompare(part, HEIGHT_TOKEN, 0) == kCFCompareEqualTo) {
                        CFStringAppendFormat(mutableString, NULL, CFSTR("%d"), height);
                    } else if (CFStringCompare(part, NAME_TOKEN, 0) == kCFCompareEqualTo) {
                        CFStringAppend(mutableString, fileName);
                    } else {
                        CFStringAppend(mutableString, part);
                    }
                }
                resultString = CFStringCreateCopy(kCFAllocatorDefault, mutableString);
                CFRelease(mutableString);
            }
            CFRelease(customFormatParts);
        } else {
            LogString(LogLevelWarn, CFSTR("CreateCustomOutputFormatStringFromImageInfoDictWithOutputType: Could not get custom format from preferences.\n"));
        }
    }
    
    return resultString;
} /* CreateCustomOutputFormatStringFromImageInfoDictWithOutputType */


/*****************************************************
 *
 * Routine:  InsertCommandIntoCommandListWithSubmenu(commandName, commandID, commands, submenuToAttach)
 *
 * Purpose:  Add one command to a list of commands. The list of commands
 *           may be a submenu or it may be the main contextual menu.
 *           commandID may be NULL -- it will be NULL when this menu item has a submenu.
 *           submenuToAttach may be NULL -- it will be non-NULL when this menu item
 *           has a submenu.
 *
 * Inputs:   commandName - CFString containing the command name to show in the menu
 *           commandID - SInt32 commandID for the menu command
 *           commands - AEDesList of commands into which to insert the new command
 *           submenuToAttach - A submenu of commands to attach to the commands list
 *
 * Returns:  Boolean - true if able to insert the command
 */
static Boolean InsertCommandIntoCommandListWithOptionsSubmenu(CFStringRef commandName, SInt32 commandID, AEDescList* commands, MenuItemAttributes attributes, UInt32 modifiers, AEDescList *submenuToAttach) {
    OSStatus err = noErr;
	AERecord commandRecord = {typeNull, NULL};
	Boolean success = false;
    
    CFStringRef tempCFStringRef = CFStringCreateCopy(kCFAllocatorDefault, commandName);
	
	/* Create an apple event record for command. */
    LogString(LogLevelDebug, CFSTR("InsertCommandIntoCommandListWithOptionsSubmenu: Creating an apple event record for the menu item: %@"), tempCFStringRef);
	err = AECreateList(NULL, 0, true, &commandRecord);
	require_noerr(err, InsertCommandIntoCommandListWithOptionsSubmenu_fail);
	
	/* Add command name. */
    CFRetain(tempCFStringRef);
    LogString(LogLevelDebug, CFSTR("InsertCommandIntoCommandListWithOptionsSubmenu: Adding command name to apple event record: %@"), tempCFStringRef);
    err = AEPutKeyPtr(&commandRecord, keyContextualMenuName, typeCFStringRef, &tempCFStringRef, sizeof(CFStringRef));
    require_noerr(err, InsertCommandIntoCommandListWithOptionsSubmenu_fail);
	
	/* Add the command ID (if there is one). */
	if (commandID != (SInt32)NULL) {
        LogString(LogLevelDebug, CFSTR("InsertCommandIntoCommandListWithOptionsSubmenu: Adding command id to apple event record: %d"), commandID);
		err = AEPutKeyPtr(&commandRecord, keyContextualMenuCommandID, typeLongInteger, &commandID, sizeof(commandID));
		require_noerr(err, InsertCommandIntoCommandListWithOptionsSubmenu_fail);
    }
    
    if ( gHasAttributeAndModifierKeys ) {
        // Stick the attributes into the AERecord.
        if (attributes != (MenuItemAttributes)NULL && attributes != 0) {
            LogString(LogLevelVerbose, CFSTR("InsertCommandIntoCommandListWithOptionsSubmenu: Adding attributes to apple event record."));
            err = AEPutKeyPtr(&commandRecord, keyContextualMenuAttributes, typeSInt32, &attributes, sizeof(attributes));
            require_noerr(err, InsertCommandIntoCommandListWithOptionsSubmenu_fail );
        }
        
        // Stick the modifiers into the AERecord.
        if (modifiers != (UInt32)NULL && modifiers != 0) {
            LogString(LogLevelVerbose, CFSTR("InsertCommandIntoCommandListWithOptionsSubmenu: Adding modifiers to apple event record."));
            err = AEPutKeyPtr(&commandRecord, keyContextualMenuModifiers, typeSInt32, &modifiers, sizeof(modifiers));
            require_noerr(err, InsertCommandIntoCommandListWithOptionsSubmenu_fail);
        }
    }
	
	/* Attach submenu (if there is one). */
	if (submenuToAttach != NULL) {
		err = AEPutKeyDesc(&commandRecord, keyContextualMenuSubmenu, submenuToAttach);
		require_noerr(err, InsertCommandIntoCommandListWithOptionsSubmenu_fail);
    }
    
	/* Add the command to list of commands. */
    LogString(LogLevelVerbose, CFSTR("InsertCommandIntoCommandListWithOptionsSubmenu: Inserting command to list of commands."));
	err = AEPutDesc(commands, 0, &commandRecord);
    
    if (gLogLevel == LogLevelVerbose) {
        LogContext(commands);
    }
	
	if (err == noErr) {
		success = true;
    }
    
InsertCommandIntoCommandListWithOptionsSubmenu_fail:
	AEDisposeDesc(&commandRecord);
    
    if (tempCFStringRef) {
        CFRelease(tempCFStringRef);
    }
	
	return success;
} /* InsertCommandIntoCommandListWithSubmenu */


/*****************************************************
 *
 * Routine:  AddStringToPasteboard(pasteboard, theString)
 *
 * Purpose:  Places the input string onto the pasteboard.
 *
 * Inputs:   pasteboard - PasteboardRef of the pasteboard on which we will place theString
 *           theString - CFStringRef to the string which will be placed on the pasteboard
 *
 * Returns:  OSStatus - the error code
 */
static OSStatus AddStringToPasteboard(PasteboardRef pasteboard, CFStringRef theString) {
	OSStatus err = noErr;
	PasteboardSyncFlags syncFlags;
	CFDataRef textData = NULL;
    
    LogString(LogLevelDebug, CFSTR("AddStringToPasteboard input string: %@"), theString);
    if (!theString) {
        LogString(LogLevelWarn, CFSTR("AddStringToPasteboard input string is null.  Cannot add to pasteboard."));
        return 1;
    }
	
	err = PasteboardClear(pasteboard);
	require_noerr(err, CantClearPasteboard);
    
	syncFlags = PasteboardSynchronize(pasteboard);
	require_action(!(syncFlags&kPasteboardModified), PasteboardNotSynchedAfterClear, err = badPasteboardSyncErr);
	require_action((syncFlags&kPasteboardClientIsOwner), ClientNotPasteboardOwner, err = notPasteboardOwnerErr);
    
    textData = CFStringCreateExternalRepresentation(kCFAllocatorDefault, theString, kCFStringEncodingUTF16, 0);
	require_action(textData != NULL, CantCreateTextData, err = memFullErr);
    
    LogString(LogLevelVerbose, CFSTR("Putting data on pasteboard.\n"));
    
	err = PasteboardPutItemFlavor(pasteboard, (PasteboardItemID)1, CFSTR("public.utf16-plain-text"), textData, 0);
	require_noerr(err, CantPutTextData);
    
CantPutTextData:
CantCreateTextData:
CantGetDataFromTextObject:
CantSetPromiseKeeper:
ClientNotPasteboardOwner:
PasteboardNotSynchedAfterClear:
CantClearPasteboard:
    
    if (textData) CFRelease(textData);
    
	return err;
} /* AddStringToPasteboard */



static int OutputFormatPreferenceForKey(const CFStringRef key) {
    int outputType = -1;
    
    // Look for the preference.
    CFPropertyListRef outputTypePref = CFPreferencesCopyAppValue(key, kDimensionizerCMPBundleIdentifier);

    // If the preference exists,  read it.
    if (outputTypePref) {
        if (CFGetTypeID(outputTypePref) == CFNumberGetTypeID()) {
            LogString(LogLevelDebug, CFSTR("OutputFormatPreferenceForKey: Output type preference for key: %@ is: %d.\n"), key, outputType);
            
            if (!CFNumberGetValue(outputTypePref, kCFNumberIntType, &outputType)) {
                outputType = -1;
            }
            
            CFRelease(outputTypePref);
        }
    } else {
        LogString(LogLevelInfo, CFSTR("OutputFormatPreferenceForKey: Could not get output type preference for key: %@.\n"), key);
    }
    
    
    return outputType;
} /* OutputTypePreferenceForKey */









// -----------------------------------------------------------------------------
//  Utility functions
// -----------------------------------------------------------------------------

/*****************************************************
 *
 * Routine:  DescIsOfTypeOrCanBeCoercedToType(desc, desiredType)
 *
 * Purpose:  Tests if the AEDesc desc is either of the desired type or can succesfully
 *           be coerced to the desired type.
 *
 * Inputs:   desc - pointer to an AEDesc to test against the desired type
 *           desiredType - OSType of the desired type
 *
 * Returns:  Boolean - true if the desc is of or can be coerced to the desired type
 */
static Boolean DescIsOfTypeOrCanBeCoercedToType(const AEDesc *desc, OSType desiredType) {
    AEDesc tempdesc = {typeNull, NULL};
    if (desc->descriptorType == desiredType) {
        return true;
    }
    
    if (AECoerceDesc(desc, desiredType, &tempdesc) == noErr) {
        AEDisposeDesc(&tempdesc);
        return true;
    }
    
    return false;
} /* DescIsOfTypeOrCanBeCoercedToType */


/*****************************************************
 *
 * Routine:  QTUtil_IsImageFile(fileSpec)
 *
 * Purpose:  Determines whether the file that the fileSpec points to is an image by
 *           testing whether QuickTime can work with it.
 *
 * Inputs:   fileSpec - FSSpecPtr to a file.
 *
 * Returns:  Boolean - true if the fileSpec points to an image
 */
static Boolean QTUtil_IsImageFile(const FSSpecPtr fileSpec) {
	Boolean isImage = false;    
	CanQuickTimeOpenFile(fileSpec,
						 0,
						 0,
						 &isImage,
						 NULL,
						 NULL,
						 0);
	return isImage;
} /* QTUtil_IsImageFile */


/*****************************************************
 *
 * Routine:  QTUtil_GetImageDescription(fileSpec)
 *
 * Purpose:  Gets the image description struct for the file which the fileSpec points to.
 *
 * Inputs:   fileSpec - FSSpecPtr to a file.  It's presumed here that the file has already
 *           been qualified as an image that QuickTime understands.
 *
 * Returns:  ImageDescriptionPtr - pointer to the image description struct
 */
static ImageDescriptionPtr QTUtil_GetImageDescription(const FSSpecPtr fileSpec) {
	GraphicsImportComponent gi;
	GetGraphicsImporterForFile(fileSpec, &gi);
	
	ImageDescriptionHandle imageDesc = (ImageDescriptionHandle)(NewHandle(sizeof(ImageDescription)));
	if (GraphicsImportGetImageDescription(gi, &imageDesc) == noErr) {
		return (*imageDesc);
	}
	
	return NULL;
} /* QTUtil_GetImageDescription */


/*****************************************************
 *
 * Routine:  GetCString(cStringToGet, cfString, encoding)
 *
 * Purpose:  Get a C string from a CFString. Caller is responsible for
 *           freeing the C string if the function returns true.
 *
 * Inputs:   cStringToGet - The C string
 *           cfString - The CFString to turn into a C string
 *           encoding - The desired encoding for the C string
 *
 * Returns:  Boolean - true if able to get the C string
 */
static Boolean GetCString(char **cStringToGet, CFStringRef cfString, CFStringEncoding encoding) {
	UInt32 lenText = (sizeof(UniChar) * CFStringGetLength(cfString)) + 1;
	*cStringToGet = malloc(lenText);
	
	if (*cStringToGet == NULL) {
		return false;
    }
	
	if (!CFStringGetCString(cfString, *cStringToGet, lenText, encoding)) {
		free(*cStringToGet);
		*cStringToGet = NULL;
		return false;
    }
    
	return true;
} /*GetCString*/


/*****************************************************
 *
 * Routine:  LogString(logLevel, formatString, ...)
 *
 * Purpose:  Prints a CFString and it list of format args to stdout.
 *
 * Inputs:   logLevel - the allowed logging level
 *           formatString - The CFString with the output format.
 *           ... - The argument list for the format string.
 *
 * Returns:  void
 */
static void LogString(const enum LogLevel logLevel, CFStringRef formatString, ...) {
    if (logLevel < gLogLevel) {
        return;
    }
    
	CFMutableStringRef resultString = CFStringCreateMutable(kCFAllocatorDefault, 0);
	CFDataRef data;
	va_list argList;
    
    switch(logLevel) {
        case 0:
            CFStringAppend(resultString, CFSTR("VERBOSE: "));
            break;
        case 1:
            CFStringAppend(resultString, CFSTR("DEBUG: "));
            break;
        case 2:
            CFStringAppend(resultString, CFSTR("INFO: "));
            break;
        case 3:
            CFStringAppend(resultString, CFSTR("WARNING: "));
            break;
        case 4:
            CFStringAppend(resultString, CFSTR("ERROR: "));
            break;
    }
	
	va_start(argList, formatString);
	CFStringAppend(resultString, CFStringCreateWithFormatAndArguments(NULL, NULL, formatString, argList));
	va_end(argList);
	
	data = CFStringCreateExternalRepresentation(NULL, resultString, CFStringGetSystemEncoding(), '?');
	
	if (data != NULL) {
        if (logLevel == LogLevelError) {
            fprintf(stderr, "%.*s\n\n", (int)CFDataGetLength(data), CFDataGetBytePtr(data));
        } else {
            printf("%.*s\n\n", (int)CFDataGetLength(data), CFDataGetBytePtr(data)); fflush(stdout);
        }
		CFRelease(data);
	}
    
	CFRelease(resultString);
} /* LogString */


/*****************************************************
 *
 * Routine:  LogContext(context)
 *
 * Purpose:  Prints a description of the context to stdout.
 *
 * Inputs:   context - AEDescList of commands to print out.
 *
 * Returns:  void
 */
static void LogContext(const AEDescList *context) {
    AEDesc tempdesc = {typeNull, NULL};
    if (AECoerceDesc(context, typeAEList, &tempdesc) == noErr) {
        Handle strHdl;
        OSStatus result = AEPrintDescToHandle(&tempdesc, &strHdl);
        if (noErr == result) {
            char nul = '\0';
            PtrAndHand(&nul, strHdl, 1);
            printf("LogContext: context: \"%s\".\n", *strHdl); fflush(stdout);
            DisposeHandle(strHdl);
        }
        
        AEDisposeDesc(&tempdesc);
    }
} /* LogContext */





//fss2path takes the FSSpec of a file, folder or volume and returns its path 
void fss2path(char *path, FSSpec *fss)
{
    int l;             //fss->name contains name of last item in path
    for(l=0; l<(fss->name[0]); l++) path[l] = fss->name[l + 1]; 
    path[l] = 0;
    
    if(fss->parID != fsRtParID) //path is more than just a volume name
    { 
        int i, len;
        CInfoPBRec pb;
        
        pb.dirInfo.ioNamePtr = fss->name;
        pb.dirInfo.ioVRefNum = fss->vRefNum;
        pb.dirInfo.ioDrParID = fss->parID;
        do
        {
            pb.dirInfo.ioFDirIndex = -1;  //get parent directory name
            pb.dirInfo.ioDrDirID = pb.dirInfo.ioDrParID;   
            if(PBGetCatInfoSync(&pb) != noErr) break;
            
            len = fss->name[0] + 1;
            for(i=l; i>=0;  i--) path[i + len] = path[i];
            for(i=1; i<len; i++) path[i - 1] = fss->name[i]; //add to start of path
            path[i - 1] = ':';
            l += len;
        } while(pb.dirInfo.ioDrDirID != fsRtDirID); //while more directory levels
    }
}
