

#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <CoreFoundation/CFPlugInCOM.h>
#include <QuickTime/QuickTime.h>

// -----------------------------------------------------------------------------
//	constants
// -----------------------------------------------------------------------------

#define kDimensionizerFactoryID	( CFUUIDGetConstantUUIDWithBytes( NULL,		\
	0xD5, 0x7A, 0xA7, 0x59, 0x30, 0xC3, 0x46, 0x4A, 		\
	0xA0, 0x6C, 0xCD, 0x06, 0x5C, 0x08, 0xB7, 0x3E ) )
// D5 7A A7 59 - 30 C3 - 46 4A - A0 6C - CD 06 5C 08 B7 3E

#define scm_require(condition,location)		\
			if ( !(condition) )	\
				goto location;
#define scm_require_noerr(value,location)	\
			scm_require((value)==noErr,location)

// -----------------------------------------------------------------------------
//	typedefs
// -----------------------------------------------------------------------------

// The layout for an instance of DimensionizerType.
typedef struct DimensionizerType
{
	ContextualMenuInterfaceStruct	*cmInterface;
	CFUUIDRef						factoryID;
	UInt32							refCount;
 } DimensionizerType;


// -----------------------------------------------------------------------------
//	globals
// -----------------------------------------------------------------------------

int gNumCommandIDs = 0;
int gNumImages = 0;


// -----------------------------------------------------------------------------
//	prototypes
// -----------------------------------------------------------------------------
//	Forward declaration for the IUnknown implementation.
//
static void DeallocDimensionizerType(
		DimensionizerType	*thisInstance );

static OSStatus AddCommandToAEDescList(
		ConstStr255Param	inCommandString,
		TextEncoding		inEncoding,
		DescType			inDescType,
		SInt32				inCommandID,
		MenuItemAttributes	inAttributes,
		UInt32				inModifiers,
		AEDescList*			ioCommandList);

static OSStatus AddUnicodeCommandToAEDescList(CFStringRef inCommandCFStringRef,
											  SInt32 inCommandID,
											  AEDescList* ioCommandList);

static OSStatus CreateSampleSubmenu(
		AEDescList*			ioCommandList);

static OSStatus CreateSampleDynamicItems(
		AEDescList*			ioCommandList);

Boolean QTUtil_IsImageFile(FSSpec *fileSpec);
ImageDescription * QTUtil_GetImageDescription(FSSpec *fileSpec);

OSStatus AddDataToPasteboard( PasteboardRef inPasteboard, 
							  CFStringRef inString);

void show(CFStringRef formatString, ...);




// -----------------------------------------------------------------------------
//	DimensionizerQueryInterface
// -----------------------------------------------------------------------------
//	Implementation of the IUnknown QueryInterface function.
//
static HRESULT DimensionizerQueryInterface(
		void*		thisInstance,
		REFIID		iid,
		LPVOID*		ppv )
{
	// Create a CoreFoundation UUIDRef for the requested interface.
	CFUUIDRef	interfaceID = CFUUIDCreateFromUUIDBytes( NULL, iid );

	// Test the requested ID against the valid interfaces.
	if ( CFEqual( interfaceID, kContextualMenuInterfaceID ) )
	{
		// If the TestInterface was requested, bump the ref count,
		// set the ppv parameter equal to the instance, and
		// return good status.
		( ( DimensionizerType* ) thisInstance )->cmInterface->AddRef(
				thisInstance );
		*ppv = thisInstance;
		CFRelease( interfaceID );
		return S_OK;
	}
	else if ( CFEqual( interfaceID, IUnknownUUID ) )
	{
		// If the IUnknown interface was requested, same as above.
		( ( DimensionizerType* ) thisInstance )->cmInterface->AddRef(
			thisInstance );
		*ppv = thisInstance;
		CFRelease( interfaceID );
		return S_OK;
	}
	else
	{
		// Requested interface unknown, bail with error.
		*ppv = NULL;
		CFRelease( interfaceID );
		return E_NOINTERFACE;
	}
}






// -----------------------------------------------------------------------------
//	DimensionizerAddRef
// -----------------------------------------------------------------------------
//	Implementation of reference counting for this type. Whenever an interface
//	is requested, bump the refCount for the instance. NOTE: returning the
//	refcount is a convention but is not required so don't rely on it.
//
static ULONG DimensionizerAddRef( void *thisInstance )
{
	( ( DimensionizerType* ) thisInstance )->refCount += 1;
	return ( ( DimensionizerType* ) thisInstance)->refCount;
}






// -----------------------------------------------------------------------------
// DimensionizerRelease
// -----------------------------------------------------------------------------
//	When an interface is released, decrement the refCount.
//	If the refCount goes to zero, deallocate the instance.
//
static ULONG DimensionizerRelease( void *thisInstance )
{
	( ( DimensionizerType* ) thisInstance )->refCount -= 1;
	if ( ( ( DimensionizerType* ) thisInstance )->refCount == 0)
	{
		DeallocDimensionizerType(
				( DimensionizerType* ) thisInstance );
		return 0;
	}
	else
	{
		return ( ( DimensionizerType*) thisInstance )->refCount;
	}
}





// -----------------------------------------------------------------------------
//	DimensionizerExamineContext
// -----------------------------------------------------------------------------
//	The implementation of the ExamineContext test interface function.
//
static OSStatus DimensionizerExamineContext(
	void*				thisInstance,
	const AEDesc*		inContext,
	AEDescList*			outCommandPairs )
{
	// Sequence the command ids
	SInt32	theCommandID = 1;
	//SInt32	result;
		
	// make sure the descriptor isn't null
	if ( inContext != NULL )
	{
		AEDesc theFSRefDesc = { typeNull, NULL };
		//AEDesc* theFSRefDesc = (AEDesc *) NewPtr(sizeof(AEDesc));
		//Str15 theDescriptorType;
		OSErr err;
		
		long numImages = 0;
		long numItems = 0;
		err = AECountItems(inContext, &numItems);
		
		//printf("Number of selected items: %d\n", numItems);
		
		CFMutableArrayRef fileRefs = CFArrayCreateMutable(kCFAllocatorDefault, numItems, NULL);
		int i = 1; // 1-based count for accessing AEGetNthDesc.
		for (i = 1; i <= numItems; i++) {
			if (AEGetNthDesc(inContext, i, typeFSRef, NULL, &theFSRefDesc) == noErr) {
				// Get the file reference and see if it points to an image.
				// Allocates memory for an FSRef.  Will release later when done with it.
				FSRef *fileRefPtr = (FSRef *)NewPtr(sizeof(FSRef)); 
				if (AEGetDescData( &theFSRefDesc, (void *)( fileRefPtr ), sizeof(FSRef) ) == noErr) {
					FSSpec fileSpec;
					err = FSGetCatalogInfo( fileRefPtr, kFSCatInfoNone, NULL, NULL, &fileSpec, NULL );
					if (err == noErr) {
						//printf("Got FSSpec.\n");
						if (QTUtil_IsImageFile(&fileSpec)) {
							//printf("We got an image!\n");
							numImages++;
							CFArrayAppendValue(fileRefs, fileRefPtr);
						} else {
							if (nil != fileRefPtr) {
								// Not an image, so get release the memory.
								DisposePtr((Ptr) fileRefPtr);
							}
						}
					} else {
						//printf("Error code getting catalog info: %d\n", err);
						if (nil != fileRefPtr) {
							// Couldn't get the Catalog Info, so get release the memory.
							DisposePtr((Ptr) fileRefPtr);
						}
					}
				}
			}
		}
		
		numImages = CFArrayGetCount(fileRefs);
		//printf("Was able to convert %d of %d items to FSRefs\n", numImages, numItems);
		
		if (numImages == 1) {
			FSRef *fileRefPtr = (FSRef *)CFArrayGetValueAtIndex(fileRefs, 0);
			
			HFSUniStr255 fileNameStruct;
			FSSpec fileSpec;
			FSGetCatalogInfo( fileRefPtr, kFSCatInfoNone, NULL, &fileNameStruct, &fileSpec, NULL );
			ImageDescription *imageDesc = QTUtil_GetImageDescription(&fileSpec);
			if (imageDesc) {
				CFStringRef fileName = CFStringCreateWithCharacters(kCFAllocatorDefault,
																	fileNameStruct.unicode,
																	fileNameStruct.length);
				
				CFStringRef menuString = CFStringCreateWithFormat(kCFAllocatorDefault, 
																  NULL, 
																  CFSTR("%@ %d w x %d h (%0.0f dpi)"), 
																  fileName,
																  (*imageDesc).width,
																  (*imageDesc).height,
																  FixedToFloat((*imageDesc).vRes));
				
				
				AddUnicodeCommandToAEDescList( menuString, theCommandID++, outCommandPairs );
				
				CFRelease(fileName);
				CFRelease(menuString);
				}
			if (nil != fileRefPtr) {
				// After this, we won't need the memory for this item anymore.
				DisposePtr((Ptr) fileRefPtr);
			}
		} else if (numImages > 1) {
			// now, we need to create the supercommand which will "own" the
			// subcommands.  The supercommand lives in the root command list.
			// this looks very much like the AddCommandToAEDescList function,
			// except that instead of putting a command ID in the record,
			// we put in the subcommand list.
			
			// Create an apple event record for our supercommand
			AEDescList	theSubmenuCommands = { typeNull, NULL };
			AERecord	theSuperCommand = { typeNull, NULL };
			CFStringRef superCommandString = CFStringCreateWithFormat(kCFAllocatorDefault, 
																	  NULL, 
																	  CFSTR("%d images selected."), 
																	  numImages);
			
			// The first thing we should do is create an AEDescList of subcommands
			// Set up the AEDescList
			err = AECreateList( NULL, 0, false, &theSubmenuCommands );
			for (i = 0; i < numImages; i++) {
				FSRef *fileRefPtr = (FSRef *)CFArrayGetValueAtIndex(fileRefs, i);
				
				HFSUniStr255 fileNameStruct;
				FSSpec fileSpec;
				err = FSGetCatalogInfo( fileRefPtr, kFSCatInfoNone, NULL, &fileNameStruct, &fileSpec, NULL );
				
				ImageDescription *imageDesc = QTUtil_GetImageDescription(&fileSpec);
				if (imageDesc) {
					CFStringRef fileName = CFStringCreateWithCharacters(kCFAllocatorDefault,
																		fileNameStruct.unicode,
																		fileNameStruct.length);
					
					CFStringRef menuString = CFStringCreateWithFormat(kCFAllocatorDefault, 
																	  NULL, 
																	  CFSTR("%@ %d w x %d h (%0.0f dpi)"), 
																	  fileName,
																	  (*imageDesc).width,
																	  (*imageDesc).height,
																	  FixedToFloat((*imageDesc).vRes));
					
					AddUnicodeCommandToAEDescList( menuString, (1000 + i), &theSubmenuCommands );
					
					CFRelease(fileName);
					CFRelease(menuString);
				}
				if (nil != fileRefPtr) {
					// After this, we won't need the memory for this item anymore.
					DisposePtr((Ptr) fileRefPtr);
				}
			}			
			
			// Now, we need to create the supercommand which will "own" the
			// subcommands.  The supercommand lives in the root command list.
			// this looks very much like the AddCommandToAEDescList function,
			// except that instead of putting a command ID in the record,
			// we put in the subcommand list.
			
			// Create an apple event record for our supercommand
			err = AECreateList( NULL, 0, true, &theSuperCommand );
			//require_noerr( err, CreateSampleSubmenu_fail );
			
			CFIndex length = CFStringGetLength(superCommandString);
			const UniChar* tempPtr = (UniChar*) NewPtr(length * sizeof(UniChar));
			//scm_require((nil != tempPtr), AddCommandToAEDescList_fail);
			
			// Get the CFString characters (UTF-16 encoding)
			CFStringGetCharacters(superCommandString,
								  CFRangeMake(0, length), (UniChar*) tempPtr);
			
			
			// Stick the command text into the aerecord
			err = AEPutKeyPtr(&theSuperCommand, keyAEName, typeUnicodeText,
								   tempPtr, length  * sizeof(UniChar));
			//require_noerr( err, CreateSampleSubmenu_fail );
			
			// Stick the subcommands into into the AERecord.
			err = AEPutKeyDesc(&theSuperCommand, keyContextualMenuSubmenu, &theSubmenuCommands);
			
			// Stick the supercommand into the list of commands that we are
			// passing back to the CMM.
			err = AEPutDesc(outCommandPairs,		// the list we're putting our command into
							0,					// stick this command onto the end of our list
							&theSuperCommand);	// the command I'm putting into the list

			if (nil != tempPtr) {
				DisposePtr((Ptr) tempPtr);
			}
			CFRelease(superCommandString);
			
		}
		
		// Add the bottom menu separator if we had any images.
		if (numImages > 0) {
			AddCommandToAEDescList( NULL, 0, typeNull, 0, kMenuItemAttrSeparator, 0, outCommandPairs );
		}
		
		// Free up the resources we used.
		AEDisposeDesc( &theFSRefDesc );
		CFRelease(fileRefs);
	}

	return noErr;
}





// -----------------------------------------------------------------------------
//	HandleSelection
// -----------------------------------------------------------------------------
//	The implementation of the HandleSelection test interface function.
//
static OSStatus DimensionizerHandleSelection(
	void*				thisInstance,
	AEDesc*				inContext,
	SInt32				inCommandID )
{
	printf( "Dimensionizer->DimensionizerHandleSelection\n");
	
	
	
	
	// We can assume that the file ref points to an image if it's coming from 
	// our prescreened (in ...ExamineContext) list.
	//AEDesc theDesc = { typeNull, NULL };
	OSErr err;
	
	
	if (inContext != NULL) {
		if (inContext->descriptorType == typeAEList) {
			printf("Got a list.\n");
			FSRef anObjectRef;
			SInt32 numberOfObjects = 0;
			
			err = AECountItems (inContext, &numberOfObjects);
			
			if (err == noErr) {
				printf("List has %d items.\n", numberOfObjects);
				SInt32 i = 0;
				AEDesc anObjectDesc;
				AEKeyword aKeyword;
				
				for (i = 1; i <= numberOfObjects; ++i)
				{
					anObjectDesc.descriptorType = typeNull;
					anObjectDesc.dataHandle = NULL;
					
					err = AEGetNthDesc (inContext, i, typeWildCard,
										   &aKeyword, &anObjectDesc);
					
					if (err == noErr) {
						if (anObjectDesc.descriptorType == typeFSRef) {
							err = AEGetDescData (&anObjectDesc, &anObjectRef,
													sizeof (FSRef));
							
							if (err == noErr) {
								// Operate on FSRef
							}
								
						}
					}
				}
			}
		}	
	}
	
	// Allocates memory for an FSRef.  Will release later when done with it.
	/*
	printf( "Try to get the fileRef.\n");
	Size dataSize = AEGetDescDataSize(inContext);
	UInt8 *buffer = (UInt8 *)NewPtr(dataSize);
	if (AEGetDescData( &theDesc, (void *)( buffer ), dataSize ) == noErr) {
		CFStringRef inputString = CFStringCreateWithBytes(kCFAllocatorDefault,
														  buffer,
														  (CFIndex)dataSize,
														  kCFStringEncodingUnicode,
														  false);
		
		show(CFSTR("Got string: %@"), inputString);
		/*
		/*
		FSSpec fileSpec;
		printf( "Try to get the Catalog info.\n");
		err = FSGetCatalogInfo( fileRefPtr, kFSCatInfoNone, NULL, NULL, &fileSpec, NULL );
		
		HFSUniStr255 fileNameStruct;
		FSGetCatalogInfo( fileRefPtr, kFSCatInfoNone, NULL, &fileNameStruct, &fileSpec, NULL );
		
		ImageDescription *imageDesc = QTUtil_GetImageDescription(&fileSpec);
		if (imageDesc) {
			CFStringRef fileName = CFStringCreateWithCharacters(kCFAllocatorDefault,
																fileNameStruct.unicode,
																fileNameStruct.length);
			
			CFStringRef pbString = CFStringCreateWithFormat(kCFAllocatorDefault, 
															NULL, 
															CFSTR("<image src=\"%@\" height=\"%d\" width=\"%d\" alt="" />"), 
															fileName,
															(*imageDesc).height,
															(*imageDesc).width);
			
			show(CFSTR("pbString: %@"), pbString);
			PasteboardRef theClipboard;
			err = PasteboardCreate( kPasteboardClipboard, &theClipboard );
			if (err == noErr) {
				printf("Will try to add to pasteboard.\n");
				err = AddDataToPasteboard(theClipboard, pbString);
				printf("err adding to pasteboard: %d\n", err);
			}
			
			CFRelease(fileName);
			CFRelease(pbString);
		}
		 */
		
	//}
	
	//DisposePtr((Ptr)buffer);
	
	return noErr;
}





// -----------------------------------------------------------------------------
//	PostMenuCleanup
// -----------------------------------------------------------------------------
//	The implementation of the PostMenuCleanup test interface function.
//
static void DimensionizerPostMenuCleanup( void *thisInstance )
{
	// Nada.
}

// -----------------------------------------------------------------------------
//	testInterfaceFtbl	definition
// -----------------------------------------------------------------------------
//	The TestInterface function table.
//
static ContextualMenuInterfaceStruct testInterfaceFtbl =
			{ 
				// Required padding for COM
				NULL,
		
				// These three are the required COM functions
				DimensionizerQueryInterface,
				DimensionizerAddRef, 
				DimensionizerRelease, 
		
				// Interface implementation
				DimensionizerExamineContext,
				DimensionizerHandleSelection,
				DimensionizerPostMenuCleanup
			}; 





// -----------------------------------------------------------------------------
//	AllocDimensionizerType
// -----------------------------------------------------------------------------
//	Utility function that allocates a new instance.
//
static DimensionizerType* AllocDimensionizerType(
		CFUUIDRef		inFactoryID )
{
	
	// Allocate memory for the new instance.
	DimensionizerType *theNewInstance;
	theNewInstance = ( DimensionizerType* ) malloc(
			sizeof( DimensionizerType ) );

	// Point to the function table
	theNewInstance->cmInterface = &testInterfaceFtbl;

	// Retain and keep an open instance refcount<
	// for each factory.
	theNewInstance->factoryID = CFRetain( inFactoryID );
	CFPlugInAddInstanceForFactory( inFactoryID );

	// This function returns the IUnknown interface
	// so set the refCount to one.
	theNewInstance->refCount = 1;
	return theNewInstance;
}





// -----------------------------------------------------------------------------
//	DeallocDimensionizerType
// -----------------------------------------------------------------------------
//	Utility function that deallocates the instance when
//	the refCount goes to zero.
//
static void DeallocDimensionizerType( DimensionizerType* thisInstance )
{
	CFUUIDRef	theFactoryID = thisInstance->factoryID;
	free( thisInstance );
	if ( theFactoryID )
	{
		CFPlugInRemoveInstanceForFactory( theFactoryID );
		CFRelease( theFactoryID );
	}
}






// -----------------------------------------------------------------------------
//	DimensionizerFactory
// -----------------------------------------------------------------------------
//	Implementation of the factory function for this type.
//
void* DimensionizerFactory(
		CFAllocatorRef		allocator,
		CFUUIDRef			typeID )
{
	// If correct type is being requested, allocate an
	// instance of TestType and return the IUnknown interface.
	if ( CFEqual( typeID, kContextualMenuTypeID ) )
	{
		DimensionizerType *result;
		result = AllocDimensionizerType( kDimensionizerFactoryID );
		return result;
	}
	else
	{
		// If the requested type is incorrect, return NULL.
		return NULL;
	}
}






// -----------------------------------------------------------------------------
//	AddCommandToAEDescList
// -----------------------------------------------------------------------------
static OSStatus AddCommandToAEDescList(
	ConstStr255Param		inCommandString,
	TextEncoding			inEncoding,
	DescType				inDescType,
	SInt32					inCommandID,
	MenuItemAttributes		inAttributes,
	UInt32					inModifiers,
	AEDescList*				ioCommandList)
{
	OSStatus theError = noErr;
	
	AERecord theCommandRecord = { typeNull, NULL };
	
	//printf( "AddCommandToAEDescList: Trying to add an item.\n" );

	// create an apple event record for our command
	theError = AECreateList( NULL, kAEDescListFactorNone, true, &theCommandRecord );
	require_noerr( theError, AddCommandToAEDescList_fail );
	
	// stick the command text into the AERecord
	if ( inCommandString != NULL )
	{
		if ( inDescType == typeChar )
		{
			theError = AEPutKeyPtr( &theCommandRecord, keyAEName, typeChar,
				&inCommandString[1], StrLength( inCommandString ) );
			require_noerr( theError, AddCommandToAEDescList_fail );
		}
		else if ( inDescType == typeStyledText )
		{
			AERecord	textRecord;
			WritingCode	writingCode;
			AEDesc		textDesc;
			
			theError = AECreateList( NULL, kAEDescListFactorNone, true, &textRecord );
			require_noerr( theError, AddCommandToAEDescList_fail );
			
			theError = AEPutKeyPtr( &textRecord, keyAEText, typeChar,
				&inCommandString[1], StrLength( inCommandString ) );
			require_noerr( theError, AddCommandToAEDescList_fail );
			
			RevertTextEncodingToScriptInfo( inEncoding, &writingCode.theScriptCode,
				&writingCode.theLangCode, NULL );
			theError = AEPutKeyPtr( &textRecord, keyAEScriptTag, typeIntlWritingCode,
				&writingCode, sizeof( writingCode ) );
			require_noerr( theError, AddCommandToAEDescList_fail );

			theError = AECoerceDesc( &textRecord, typeStyledText, &textDesc );
			require_noerr( theError, AddCommandToAEDescList_fail );
			
			theError = AEPutKeyDesc( &theCommandRecord, keyAEName, &textDesc );
			require_noerr( theError, AddCommandToAEDescList_fail );
			
			AEDisposeDesc( &textRecord );
		}
		else if ( inDescType == typeIntlText )
		{
			IntlText*	intlText;
			ByteCount	size = sizeof( IntlText ) + StrLength( inCommandString ) - 1;
			
			// create an IntlText structure with the text and script
			intlText = (IntlText*) malloc( size );
			RevertTextEncodingToScriptInfo( inEncoding, &intlText->theScriptCode,
				&intlText->theLangCode, NULL );
			BlockMoveData( &inCommandString[1], &intlText->theText, StrLength( inCommandString ) );
			
			theError = AEPutKeyPtr( &theCommandRecord, keyAEName, typeIntlText, intlText, size );
			free( (char*) intlText );
			require_noerr( theError, AddCommandToAEDescList_fail );
		}
		else if ( inDescType == typeUnicodeText )
		{
			CFStringRef str = CFStringCreateWithPascalString( NULL, inCommandString, inEncoding );
			if ( str != NULL )
			{
				Boolean doFree = false;
				CFIndex sizeInChars = CFStringGetLength( str );
				CFIndex sizeInBytes = sizeInChars * sizeof( UniChar );
				const UniChar* unicode = CFStringGetCharactersPtr( str );
				if ( unicode == NULL )
				{
					doFree = true;
					unicode = (UniChar*) malloc( sizeInBytes );
					CFStringGetCharacters( str, CFRangeMake( 0, sizeInChars ), (UniChar*) unicode );
				}
				
				theError = AEPutKeyPtr( &theCommandRecord, keyAEName, typeUnicodeText, unicode, sizeInBytes );
					
				CFRelease( str );
				if ( doFree )
					free( (char*) unicode );
				
				require_noerr( theError, AddCommandToAEDescList_fail );
			}
		}
		else if ( inDescType == typeCFStringRef )
		{
			CFStringRef str = CFStringCreateWithPascalString( NULL, inCommandString, inEncoding );
			if ( str != NULL )
			{
				theError = AEPutKeyPtr( &theCommandRecord, keyAEName, typeCFStringRef, &str, sizeof( str ) );
				require_noerr( theError, AddCommandToAEDescList_fail );
				
				// do not release the string; the Contextual Menu Manager will release it for us
			}
		}
	}
		
	// stick the command ID into the AERecord
	if ( inCommandID != 0 )
	{
		theError = AEPutKeyPtr( &theCommandRecord, keyContextualMenuCommandID,
				typeLongInteger, &inCommandID, sizeof( inCommandID ) );
		require_noerr( theError, AddCommandToAEDescList_fail );
	}
	
	// stick the attributes into the AERecord
	if ( inAttributes != 0 )
	{
		theError = AEPutKeyPtr( &theCommandRecord, keyContextualMenuAttributes,
				typeLongInteger, &inAttributes, sizeof( inAttributes ) );
		require_noerr( theError, AddCommandToAEDescList_fail );
	}
	
	// stick the modifiers into the AERecord
	if ( inModifiers != 0 )
	{
		theError = AEPutKeyPtr( &theCommandRecord, keyContextualMenuModifiers,
				typeLongInteger, &inModifiers, sizeof( inModifiers ) );
		require_noerr( theError, AddCommandToAEDescList_fail );
	}
	
	// stick this record into the list of commands that we are
	// passing back to the CMM
	theError = AEPutDesc(ioCommandList, 		// the list we're putting our command into
						 0,						// stick this command onto the end of our list
						 &theCommandRecord );	// the command I'm putting into the list
	
	AddCommandToAEDescList_fail:
	// clean up after ourself; dispose of the AERecord
	AEDisposeDesc( &theCommandRecord );

    return theError;
    
} // AddCommandToAEDescList






static OSStatus AddUnicodeCommandToAEDescList(CFStringRef inCommandCFStringRef,
											  SInt32 inCommandID,
											  AEDescList* ioCommandList)
{
	OSStatus theError = noErr;
	AERecord theCommandRecord = { typeNull, NULL };
	CFIndex length = CFStringGetLength(inCommandCFStringRef);
	const UniChar* dataPtr = CFStringGetCharactersPtr(inCommandCFStringRef);
	const UniChar* tempPtr = nil;
	
	// printf( "AddCommandToAEDescList: Trying to add an item.\n" );
	
	if (dataPtr == NULL) // if CFStringGetCharactersPtr fails...
	{ // allocate a buffer
		tempPtr = (UniChar*) NewPtr(length * sizeof(UniChar));
		scm_require((nil != tempPtr), AddCommandToAEDescList_fail);
		
		// get the CFString characters (UTF-16 encoding)
		CFStringGetCharacters(inCommandCFStringRef,
							  CFRangeMake(0, length), (UniChar*) tempPtr);
		dataPtr = tempPtr;
	}
	scm_require((nil != dataPtr), AddCommandToAEDescList_fail);
	
	// create an apple event record for our command
	theError = AECreateList( NULL, 0, true, &theCommandRecord );
	require_noerr( theError, AddCommandToAEDescList_fail );
	
	// stick the command text into the AERecord
	theError = AEPutKeyPtr( &theCommandRecord, keyAEName,
							typeUnicodeText, dataPtr, length * sizeof(UniChar));
	require_noerr( theError, AddCommandToAEDescList_fail );
	
	// stick the command ID into the AERecord
	theError = AEPutKeyPtr( &theCommandRecord, keyContextualMenuCommandID,
							typeLongInteger, &inCommandID, sizeof(inCommandID));
	require_noerr( theError, AddCommandToAEDescList_fail );
	
	// stick this record into the list of commands that we are
	// passing back to the CMM
	theError = AEPutDesc(ioCommandList, // the list we're putting it into
						 0,				// put it onto the end of our list
						 &theCommandRecord); // what I'm putting into the list
	
	AddCommandToAEDescList_fail:
		
	// clean up after ourself; dispose of the AERecord
	AEDisposeDesc( &theCommandRecord );
	
	if (nil != tempPtr)
		DisposePtr((Ptr) tempPtr);
	
	return theError;
	
} // AddCommandToAEDescList







// -----------------------------------------------------------------------------
//	CreateSampleSubmenu
// -----------------------------------------------------------------------------
static OSStatus CreateSampleSubmenu(
	AEDescList*		ioCommandList)
{
	OSStatus	theError = noErr;
	
	AEDescList	theSubmenuCommands = { typeNull, NULL };
	AERecord	theSupercommand = { typeNull, NULL };
	Str255		theSupercommandText = "\pSubmenu Here";
	
	// the first thing we should do is create an AEDescList of
	// subcommands

	// set up the AEDescList
	theError = AECreateList( NULL, 0, false, &theSubmenuCommands );
	require_noerr( theError, CreateSampleSubmenu_Complete_fail );

	// stick some commands in this subcommand list
	theError = AddCommandToAEDescList( "\pSubcommand 1", kTextEncodingMacRoman, typeChar,
			1001, 0, 0, &theSubmenuCommands );
	require_noerr( theError, CreateSampleSubmenu_CreateDesc_fail );
	
	// another
	theError = AddCommandToAEDescList( "\pAnother Subcommand", kTextEncodingMacRoman, typeChar,
			1002, 0, 0, &theSubmenuCommands );
	require_noerr( theError, CreateSampleSubmenu_fail );
	
	// yet another
	theError = AddCommandToAEDescList( "\pLast One", kTextEncodingMacRoman, typeChar, 
			1003, 0, 0, &theSubmenuCommands);
	require_noerr( theError, CreateSampleSubmenu_fail );
		
	// now, we need to create the supercommand which will "own" the
	// subcommands.  The supercommand lives in the root command list.
	// this looks very much like the AddCommandToAEDescList function,
	// except that instead of putting a command ID in the record,
	// we put in the subcommand list.

	// create an apple event record for our supercommand
	theError = AECreateList( NULL, 0, true, &theSupercommand );
	require_noerr( theError, CreateSampleSubmenu_fail );
	
	// stick the command text into the aerecord
	theError = AEPutKeyPtr(&theSupercommand, keyAEName, typeChar,
		&theSupercommandText[1], StrLength( theSupercommandText ) );
	require_noerr( theError, CreateSampleSubmenu_fail );
	
	// stick the subcommands into into the AERecord
	theError = AEPutKeyDesc(&theSupercommand, keyContextualMenuSubmenu,
		&theSubmenuCommands);
	require_noerr( theError, CreateSampleSubmenu_fail );
	
	// stick the supercommand into the list of commands that we are
	// passing back to the CMM
	theError = AEPutDesc(
		ioCommandList,		// the list we're putting our command into
		0,					// stick this command onto the end of our list
		&theSupercommand);	// the command I'm putting into the list
	
	// clean up after ourself
CreateSampleSubmenu_fail:
	AEDisposeDesc(&theSubmenuCommands);

CreateSampleSubmenu_CreateDesc_fail:
	AEDisposeDesc(&theSupercommand);

CreateSampleSubmenu_Complete_fail:
    return theError;
    
} // CreateSampleSubmenu






// -----------------------------------------------------------------------------
//	CreateSampleDynamicItems
// -----------------------------------------------------------------------------
static OSStatus CreateSampleDynamicItems(
		AEDescList*			ioCommandList)
{
	OSStatus	theError = noErr;
	
	// add a command
	theError = AddCommandToAEDescList( "\pClose", 2001, kTextEncodingMacRoman, typeChar,
			kMenuItemAttrDynamic, 0, ioCommandList );
	require_noerr( theError, CreateSampleDynamicItems_fail );
	
	// another
	theError = AddCommandToAEDescList( "\pClose All", kTextEncodingMacRoman, typeChar,
			2002, kMenuItemAttrDynamic, kMenuOptionModifier, ioCommandList );
	require_noerr( theError, CreateSampleDynamicItems_fail );
	
	// yet another
	theError = AddCommandToAEDescList( "\pClose All Without Saving", kTextEncodingMacRoman, typeChar,
			2003, kMenuItemAttrDynamic, kMenuOptionModifier | kMenuShiftModifier, ioCommandList );
	require_noerr( theError, CreateSampleDynamicItems_fail );
	
CreateSampleDynamicItems_fail:
	return theError;
	
} // CreateSampleDynamicItems





Boolean QTUtil_IsImageFile(FSSpec *fileSpec) {
	Boolean isImage = FALSE;
	CanQuickTimeOpenFile(fileSpec,
						 0,
						 0,
						 &isImage,
						 NULL,
						 NULL,
						 0 );
	return isImage;
}



ImageDescription * QTUtil_GetImageDescription(FSSpec *fileSpec) {
	GraphicsImportComponent gi;
	GetGraphicsImporterForFile(fileSpec, &gi);
	
	ImageDescriptionHandle imageDesc = (ImageDescriptionHandle)(NewHandle(sizeof(ImageDescription)));
	if (GraphicsImportGetImageDescription(gi, &imageDesc) == noErr) {
		return (*imageDesc);
	}
	
	return NULL;
}



OSStatus AddDataToPasteboard( PasteboardRef inPasteboard, 
							  CFStringRef inString) {
	OSStatus            err = noErr;
	PasteboardSyncFlags syncFlags;
	Handle				buffer;
	CFDataRef           textData = NULL;
	
	show(CFSTR("instring: %@"), inString);
	
	err = PasteboardClear( inPasteboard );// 1
	require_noerr( err, CantClearPasteboard );
		
	syncFlags = PasteboardSynchronize( inPasteboard );
	require_action( !(syncFlags&kPasteboardModified), PasteboardNotSynchedAfterClear, err = badPasteboardSyncErr );
	require_action( (syncFlags&kPasteboardClientIsOwner), ClientNotPasteboardOwner, err = notPasteboardOwnerErr );
	
	int length = CFStringGetLength(inString);
	CFIndex dataLength;
	printf("trying to get bytes from string of length: %d.\n", length);
	CFStringGetBytes (inString,
					  CFRangeMake(0, length),
					  kCFStringEncodingUnicode,
					  0,
					  false,
					  (UInt8 *)&buffer,
					  0,
					  &dataLength
					  );
	printf("Creating CFData from bytes of string.\n");
	textData = CFDataCreate( kCFAllocatorDefault,
							 (UInt8 *)*buffer, dataLength );
	
	require_action( textData != NULL, CantCreateTextData, err = memFullErr );
	printf("Putting data on pasteboard.\n");
	err = PasteboardPutItemFlavor( inPasteboard, (PasteboardItemID)1,
									CFSTR("public.utf16-plain-text"),
									textData, 0 );
	require_noerr( err, CantPutTextData );
				
	CantPutTextData:
	CantCreateTextData:
	CantGetDataFromTextObject:
	CantSetPromiseKeeper:
	ClientNotPasteboardOwner:
	PasteboardNotSynchedAfterClear:
	CantClearPasteboard:

	return err;
}












void show(CFStringRef formatString, ...) {
	CFStringRef resultString;
	CFDataRef data;
	va_list argList;
	
	va_start(argList, formatString);
	resultString = CFStringCreateWithFormatAndArguments(NULL, NULL, 
														formatString, argList);
	va_end(argList);
	
	data = CFStringCreateExternalRepresentation(NULL, resultString, 
												CFStringGetSystemEncoding(), '?');
	
	if (data != NULL) {
		printf ("%.*s\n\n", (int)CFDataGetLength(data), 
				CFDataGetBytePtr(data));
		CFRelease(data);
	}
	CFRelease(resultString);
}

