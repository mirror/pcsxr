//
//  main.c
//  Psx-Memcard
//
//  Created by C.W. Betts on 6/6/14.
//
//

#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFPlugInCOM.h>
#include <CoreServices/CoreServices.h>
#include "GetMetadataForFile.h"

// -----------------------------------------------------------------------------
//	constants
// -----------------------------------------------------------------------------


#define PLUGIN_ID "E6CB501C-9D5C-4CAF-A9DA-A6940F261514"

//
// Below is the generic glue code for all plug-ins.
//

// -----------------------------------------------------------------------------
//	typedefs
// -----------------------------------------------------------------------------

// The layout for an instance of MetaDataImporterPlugIn 
typedef struct __MetadataImporterPluginType {
    MDImporterInterfaceStruct *conduitInterface;
    CFUUIDRef                 factoryID;
    UInt32                    refCount;
} MDImporterType;

// -----------------------------------------------------------------------------
//	prototypes
// -----------------------------------------------------------------------------
//	Forward declaration for the IUnknown implementation.
//

static MDImporterType	*AllocMetadataImporterPluginType(CFUUIDRef inFactoryID);
static void				DeallocMetadataImporterPluginType(MDImporterType *thisInstance);
static HRESULT			MetadataImporterQueryInterface(void *thisInstance,REFIID iid,LPVOID *ppv);
extern void				*MetadataImporterPluginFactory(CFAllocatorRef allocator,CFUUIDRef typeID);
static ULONG			MetadataImporterPluginAddRef(void *thisInstance);
static ULONG			MetadataImporterPluginRelease(void *thisInstance);
// -----------------------------------------------------------------------------
//	testInterfaceFtbl	definition
// -----------------------------------------------------------------------------
//	The TestInterface function table.
//

static MDImporterInterfaceStruct testInterfaceFtbl = {
	NULL,
	MetadataImporterQueryInterface,
	MetadataImporterPluginAddRef,
	MetadataImporterPluginRelease,
	GetMetadataForFile
};


// -----------------------------------------------------------------------------
//	AllocMetadataImporterPluginType
// -----------------------------------------------------------------------------
//	Utility function that allocates a new instance.
//      You can do some initial setup for the importer here if you wish
//      like allocating globals etc...
//
MDImporterType *AllocMetadataImporterPluginType(CFUUIDRef inFactoryID)
{
	MDImporterType *theNewInstance = (MDImporterType *)calloc(sizeof(MDImporterType), 1);
	
	/* Point to the function table */
	theNewInstance->conduitInterface = &testInterfaceFtbl;
	
	/*  Retain and keep an open instance refcount for each factory. */
	theNewInstance->factoryID = CFRetain(inFactoryID);
	CFPlugInAddInstanceForFactory(inFactoryID);
	
	/* This function returns the IUnknown interface so set the refCount to one. */
	theNewInstance->refCount = 1;
	return theNewInstance;
}

// -----------------------------------------------------------------------------
//	DeallocPsx_MemcardMDImporterPluginType
// -----------------------------------------------------------------------------
//	Utility function that deallocates the instance when
//	the refCount goes to zero.
//      In the current implementation importer interfaces are never deallocated
//      but implement this as this might change in the future
//
void DeallocMetadataImporterPluginType(MDImporterType *thisInstance)
{
	CFUUIDRef theFactoryID = thisInstance->factoryID;
	
	free(thisInstance);
	if (theFactoryID){
		CFPlugInRemoveInstanceForFactory(theFactoryID);
		CFRelease(theFactoryID);
	}
}

// -----------------------------------------------------------------------------
//	MetadataImporterQueryInterface
// -----------------------------------------------------------------------------
//	Implementation of the IUnknown QueryInterface function.
//
HRESULT MetadataImporterQueryInterface(void *thisInstance, REFIID iid, LPVOID *ppv)
{
	CFUUIDRef interfaceID = CFUUIDCreateFromUUIDBytes(kCFAllocatorDefault, iid);
	
	if (CFEqual(interfaceID, kMDImporterInterfaceID)) {
		/* If the Right interface was requested, bump the ref count,
		 * set the ppv parameter equal to the instance, and
		 * return good status.
		 */
		((MDImporterType*)thisInstance)->conduitInterface->AddRef(thisInstance);
		*ppv = thisInstance;
		CFRelease(interfaceID);
		return S_OK;
	} else if (CFEqual(interfaceID, IUnknownUUID)) {
		/* If the IUnknown interface was requested, same as above. */
		((MDImporterType*)thisInstance )->conduitInterface->AddRef(thisInstance);
		*ppv = thisInstance;
		CFRelease(interfaceID);
		return S_OK;
	} else {
		/* Requested interface unknown, bail with error. */
		*ppv = NULL;
		CFRelease(interfaceID);
		return E_NOINTERFACE;
	}
}

// -----------------------------------------------------------------------------
//	MetadataImporterPluginAddRef
// -----------------------------------------------------------------------------
//	Implementation of reference counting for this type. Whenever an interface
//	is requested, bump the refCount for the instance. NOTE: returning the
//	refcount is a convention but is not required so don't rely on it.
//
ULONG MetadataImporterPluginAddRef(void *thisInstance)
{
	return ++((MDImporterType*) thisInstance)->refCount;
}

// -----------------------------------------------------------------------------
// SampleCMPluginRelease
// -----------------------------------------------------------------------------
//	When an interface is released, decrement the refCount.
//	If the refCount goes to zero, deallocate the instance.
//
ULONG MetadataImporterPluginRelease(void *thisInstance)
{
	((MDImporterType*)thisInstance)->refCount -= 1;
	if (((MDImporterType*)thisInstance)->refCount == 0){
		DeallocMetadataImporterPluginType((MDImporterType*)thisInstance);
		return 0;
	}else{
		return ((MDImporterType*) thisInstance )->refCount;
	}
}

// -----------------------------------------------------------------------------
//	Psx_MemcardMDImporterPluginFactory
// -----------------------------------------------------------------------------
//	Implementation of the factory function for this type.
//
void *MetadataImporterPluginFactory(CFAllocatorRef allocator,CFUUIDRef typeID)
{
	MDImporterType	*result;
	CFUUIDRef		uuid;
	
	/* If correct type is being requested, allocate an
	 * instance of TestType and return the IUnknown interface.
	 */
	if (CFEqual(typeID,kMDImporterTypeID)){
		uuid = CFUUIDCreateFromString(kCFAllocatorDefault,CFSTR(PLUGIN_ID));
		result = AllocMetadataImporterPluginType(uuid);
		CFRelease(uuid);
		return result;
	}
	/* If the requested type is incorrect, return NULL. */
	return NULL;
}
