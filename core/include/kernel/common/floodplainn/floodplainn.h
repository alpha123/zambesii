#ifndef _FLOODPLAINN_H
	#define _FLOODPLAINN_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string8.h>
	#include <__kclasses/ptrList.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/messageStream.h>
	#include <kernel/common/floodplainn/floodplainn.h>
	#include <kernel/common/floodplainn/device.h>
	#include <kernel/common/floodplainn/index.h>
	#include <kernel/common/vfsTrib/commonOperations.h>

#define DRIVERINDEX_REQUEST_DEVNAME_MAXLEN		(128)

#define FPLAINN						"Fplainn: "
#define FPLAINNIDX					"FplainnIndex: "

#define FPLAINN_DETECTDRIVER_FLAGS_CPU_TARGET	MSGSTREAM_FLAGS_CPU_TARGET

class floodplainnC
:
public tributaryC//, public vfs::directoryOperationsC
{
public:
	floodplainnC(void)
	{
		new (&zudiIndexes[0]) zudiIndexC(zudiIndexC::SOURCE_KERNEL);
		new (&zudiIndexes[1]) zudiIndexC(zudiIndexC::SOURCE_RAMDISK);
		new (&zudiIndexes[2]) zudiIndexC(zudiIndexC::SOURCE_EXTERNAL);
	}

	typedef void (initializeReqCallF)(error_t ret);
	error_t initializeReq(initializeReqCallF *callback);

	~floodplainnC(void) {}

public:
	/**	EXPLANATION:
	 * The driver indexer will search several levels of drivers in
	 * this order:
	 * Chipset List:
	 *	The list of drivers that are embedded in the kernel's
	 *	image. These take precedence, and are searched first.
	 * MRU List:
	 *	Across boots, the kernel will maintain a list of drivers
	 *	that were actually loaded and used. This is the MRU
	 *	(most-recently-used) List. This index is read from the
	 *	disk and absorbed into the index.
	 * Disk:
	 *	After searching among those drivers which are in the
	 *	kernel image, and those in the MRU list, if a suitable
	 *	match isn't found, the kernel will just search all the
	 *	drivers that are available on disk. This is the slowest
	 *	level of searching that can be done.
	 **/
	enum indexLevelE {
		INDEX_KERNEL=zudiIndexC::SOURCE_KERNEL,
		INDEX_RAMDISK=zudiIndexC::SOURCE_RAMDISK,
		INDEX_EXTERNAL=zudiIndexC::SOURCE_EXTERNAL };

	/* Creates a child device under a given parent and returns it to the
	 * caller.
	 **/
	error_t createDevice(
		utf8Char *parentId, ubit16 childId, ubit32 flags,
		fplainn::deviceC **device);

	// Removes a given child from a given parent.
	error_t removeDevice(utf8Char *parentId, ubit32 childId, ubit32 flags);
	// Removes a tree of children from a given parent.
	error_t removeDeviceAndChildren(
		utf8Char *parentId, ubit32 childId, ubit32 flags);

	/* Retrieves a device by its path. This may be a by-id, by-name,
	 * by-class or by-path path.
	 **/
	error_t getDevice(utf8Char *path, fplainn::deviceC **device);

	/**	EXPLANATION:
	 * These functions take a device path argument and attempt to perform
	 * a particular driver-loading related operation on them. Generally to
	 * instantiate and use a device, one first calls:
	 *	floodplainn.loadDriver("myDevice", ...);
	 * 	processTrib.spawnDriver("myDevice", ...);
	 *
	 * loadDriver() takes a device path and attempts to determine the best
	 * driver to use to instantiate that device. If loadDriver() was not
	 * previously called on a device, such that that device has no driver
	 * associated with it, spawnDriver() will fail.
	 *
	 *	DETAILS:
	 * error_t loadDriver(path, ubit32 flags);
	 *	Walks the kernel's device VFS searching for the device node that
	 *	"path" refers to and reads the device's enumeration attributes.
	 *	These are then compared against an index of drivers in memory.
	 *
	 *	If no driver is found in the current set of pre-fetched drivers
	 *	in memory, the kernel then seeks for the on-disk driver-DB, and
	 *	uses it to expand the current in-memory index, before searching
	 *	the index again.
	 *
	 *	If this check fails, the kernel assumes that no suitable driver
	 *	could be found and returns error.
	 **/
	#define MSGSTREAM_FPLAINN_DETECTDRIVER_REQ		(0)
	error_t detectDriverReq(
		utf8Char *devicePath, indexLevelE indexLevel,
		processId_t targetId, void *privateData, ubit32 flags);

	#define MSGSTREAM_FPLAINN_LOADDRIVER_REQ		(3)
	error_t loadDriverReq(utf8Char *devicePath, void *privateData);

	#define MSGSTREAM_FPLAINN_NEWDEVICE_IND			(4)
	void newDeviceInd(utf8Char *devicePath, void *privateData);

	enum newDeviceActionE {
		NDACTION_NOTHING=0, NDACTION_DETECT_DRIVER,
		NDACTION_LOAD_DRIVER, NDACTION_INSTANTIATE };

	#define MSGSTREAM_FPLAINN_SET_NEWDEVICE_ACTION_REQ	(1)
	void setNewDeviceActionReq(newDeviceActionE action, void *privateData);
	#define MSGSTREAM_FPLAINN_GET_NEWDEVICE_ACTION_REQ	(2)
	void getNewDeviceActionReq(void *privateData);

	/* FVFS listing functions. These allow the device tree to be treated
	 * like a VFS with "files" that can be listed.
	 *
	 * FVFS does not support open(), read(), write(), stat() etc on nodes
	 * however; only directory listing (node listing). To "read" the
	 * attributes of a device, use "getDeviceMetalanguageAttributes()" in
	 * conjunction with "getDeviceBaseAttributes()".
	 **/
	//error_t openDirectory(utf8Char *deviceId, void **handle);
	//error_t readDirectory(void *handle, void **tagInfo);
	//void closeDirectory(void *handle);

	/* These aren't really used by the kernel, and are mostly provided for
	 * userspace's benefit. The kernel will generally directly use
	 * "getDevice()" and just read the attributes it desires directly from
	 * the device's metadata object.
	 *
	 * getDeviceBaseAttributes():
	 *	Fills out a structure with the most basic information common
	 *	to every device: its ID, name info, vendor info, the driver
	 *	that has been chosen to instantiate the device (if any),
	 *	the instantiation status of the device, the device's
	 *	enumeration attributes (key-value pairs), the device's
	 *	registered device-categories (if any), and the device's
	 *	metalanguages, as well as the number of metalanguages it has
	 *	(if known).
	 *
	 * getDeviceMetalanguageAttributes():
	 *	This is used to get further information about the device. The
	 *	base information doesn't state anything about the class/category
	 *	of the device, its class-specific properties (e.g.: for an NIC,
	 *	these might include link speed, media type, connection status,
	 *	etc.).
	 *
	 *	Using the information about the device's metalanguages (given by
	 *	getDeviceBaseAttributes()), the caller can then query each of
	 *	the metalanguage-specific attributes for the device.
	 *
	 *	E.g.: If a device exports a "printer" metalanguage interface and
	 *	a "scanner" metalanguage interface, each of those metalanguages
	 *	would specify different attributes which describe properties of
	 *	that sub-function of the device.
	 *
	 *	By asking for the per-metalanguage attributes of the device
	 *	for its "printer" and "scanner" metalanguage-specific attributes
	 *	one can get all of the attributes the device exports.
	 **/
	error_t getDeviceBaseAttributes(utf8Char *deviceName, void *outputMem);
	error_t getDeviceMetalanguageAttributes(
		utf8Char *deviceName, utf8Char *metalanguageName,
		void *outputMem);

	typedef void (createRootDeviceReqCallF)(error_t);
	error_t createRootDeviceReq(createRootDeviceReqCallF *callback);

	static void __kdriverEntry(void);
	static void driverIndexerEntry(void);

public:
	struct driverIndexMsgS
	{
		driverIndexMsgS(
			processId_t targetPid, ubit16 subsystem, ubit16 function,
			uarch_t size, uarch_t flags, void *privateData)
		:
		header(targetPid, subsystem, function, size, flags, privateData)
		{}

		messageStreamC::headerS	header;
		indexLevelE		indexLevel;
		utf8Char		deviceName[
			DRIVERINDEX_REQUEST_DEVNAME_MAXLEN];
	};

	struct newDeviceActionMsgS
	{
		newDeviceActionMsgS(
			processId_t targetPid, ubit16 subsystem, ubit16 function,
			uarch_t size, uarch_t flags, void *privateData)
		:
		header(targetPid, subsystem, function, size, flags, privateData)
		{}

		messageStreamC::headerS	header;
		newDeviceActionE	action;
	};

	struct newDeviceMsgS
	{
		newDeviceMsgS(
			processId_t targetPid, ubit16 subsystem, ubit16 function,
			uarch_t size, uarch_t flags, void *privateData)
		:
		header(targetPid, subsystem, function, size, flags, privateData)
		{}

		messageStreamC::headerS	header;
		newDeviceActionE	lastCompletedAction;
		utf8Char		deviceName[
			DRIVERINDEX_REQUEST_DEVNAME_MAXLEN];
	};

	processId_t			indexerThreadId;
	ubit16				indexerQueueId;

	ptrListC<fplainn::driverC>	driverList;

	zudiIndexC			zudiIndexes[3];

private:
	static syscallbackFuncF initializeReq1;
};

extern floodplainnC		floodplainn;

#endif
