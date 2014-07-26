
#include <__kstdlib/__kcxxlib/memory>
#include <kernel/common/thread.h>
#include <kernel/common/process.h>
#include <kernel/common/zudiIndexServer.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/floodplainn/floodplainn.h>


error_t zuiServer::detectDriverReq(
	utf8Char *devicePath, indexE index,
	void *privateData, ubit32 flags
	)
{
	heapObjC<zuiServer::indexMsgS>	request;
	Task				*currTask;

	if (strnlen8(devicePath, FVFS_PATH_MAXLEN) >= FVFS_PATH_MAXLEN)
		{ return ERROR_LIMIT_OVERFLOWED; };

	currTask = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask();

	// Allocate and queue the new request.
	request = new zuiServer::indexMsgS(
		ZUISERVER_DETECTDRIVER_REQ, devicePath, index);

	if (request == NULL) { return ERROR_MEMORY_NOMEM; };

	return currTask->parent->zasyncStream.send(
		floodplainn.zudiIndexServerTid, request.get(), sizeof(*request),
		ipc::METHOD_BUFFER, flags, privateData);
}

error_t zuiServer::loadDriverReq(utf8Char *devicePath, void *privateData)
{
	heapObjC<zuiServer::indexMsgS>	request;
	Task				*currTask;

	if (strnlen8(devicePath, FVFS_PATH_MAXLEN) >= FVFS_PATH_MAXLEN)
		{ return ERROR_LIMIT_OVERFLOWED; };

	currTask = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask();

	request = new zuiServer::indexMsgS(
		ZUISERVER_LOADDRIVER_REQ, devicePath,
		zuiServer::INDEX_KERNEL);

	if (request == NULL) { return ERROR_MEMORY_NOMEM; };

	return currTask->parent->zasyncStream.send(
		floodplainn.zudiIndexServerTid, request.get(), sizeof(*request),
		ipc::METHOD_BUFFER, 0, privateData);
}

void zuiServer::loadDriverRequirementsReq(void *privateData)
{
	heapObjC<zuiServer::indexMsgS>	request;
	Task				*currTask;

	currTask = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask();
	if (currTask->parent->getType() != processStreamC::DRIVER)
		{ return; };

	request = new zuiServer::indexMsgS(
		ZUISERVER_LOAD_REQUIREMENTS_REQ,
		currTask->parent->fullName,
		zuiServer::INDEX_KERNEL);

	if (request == NULL) { return; };

	currTask->parent->zasyncStream.send(
		floodplainn.zudiIndexServerTid, request.get(), sizeof(*request),
		ipc::METHOD_BUFFER, 0, privateData);
}

void zuiServer::setNewDeviceActionReq(
	newDeviceActionE action, void *privateData
	)
{
	heapObjC<zuiServer::indexMsgS>	request;
	Task			*currTask;

	currTask = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask();

	request = new zuiServer::indexMsgS(
		ZUISERVER_SET_NEWDEVICE_ACTION_REQ, NULL,
		zuiServer::INDEX_KERNEL, action);

	if (request == NULL) { return; };

	currTask->parent->zasyncStream.send(
		floodplainn.zudiIndexServerTid, request.get(), sizeof(*request),
		ipc::METHOD_BUFFER, 0, privateData);
}

void zuiServer::getNewDeviceActionReq(void *privateData)
{
	heapObjC<zuiServer::indexMsgS>	request;
	Task				*currTask;

	currTask = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask();

	request = new zuiServer::indexMsgS(
		ZUISERVER_GET_NEWDEVICE_ACTION_REQ, NULL,
		zuiServer::INDEX_KERNEL);

	if (request == NULL) { return; };

	currTask->parent->zasyncStream.send(
		floodplainn.zudiIndexServerTid, request.get(), sizeof(*request),
		ipc::METHOD_BUFFER, 0, privateData);
}

void zuiServer::newDeviceInd(
	utf8Char *path, indexE index, void *privateData
	)
{
	heapObjC<zuiServer::indexMsgS>	request;
	Task				*currTask;

	if (strnlen8(path, FVFS_PATH_MAXLEN) >= FVFS_PATH_MAXLEN)
	{
		printf(ERROR FPLAINN"newDeviceInd: device name too long.\n");
		return;
	};

	currTask = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentTask();

	request = new zuiServer::indexMsgS(
		ZUISERVER_NEWDEVICE_IND, path, index);

	if (request == NULL) { return; };

	currTask->parent->zasyncStream.send(
		floodplainn.zudiIndexServerTid, request.get(), sizeof(*request),
		ipc::METHOD_BUFFER, 0, privateData);
}

