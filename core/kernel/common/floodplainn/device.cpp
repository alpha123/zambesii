
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kclib/stdlib.h>
#include <__kstdlib/__kcxxlib/memory>
#include <__kclasses/debugPipe.h>
#include <kernel/common/floodplainn/device.h>


/**	EXPLANATION:
 * Global array of device classes recognized by the kernel. This array is used
 * to initialize the device classes in the by-class tree ofthe floodplainn VFS.
 *
 * The second array below maps UDI metalanguage names to kernel device class
 * names. We use it to detect what classes of functionality a device exports.
 **/
utf8Char		*driverClasses[] =
{
	CC"unknown",		// 0
	CC"unrecognized",	// 1
	CC"bus",		// 2
	CC"io-controller",	// 3
	CC"generic-io",		// 4
	CC"network",		// 5
	CC"audio-output",	// 6
	CC"audio-input",	// 7
	CC"video-output",	// 8
	CC"video-input",	// 9
	CC"storage",		// 10
	CC"character-input",	// 11
	CC"coordinate-input",	// 12
	CC"battery",		// 13
	CC"gpio",		// 14
	CC"biometric-input",	// 15
	CC"printer",		// 16
	CC"scanner",		// 17
	CC"irq-controller",	// 18
	CC"timer",		// 19
	CC"clock",		// 20
	CC"card-reader",	// 21
	NULL
};

driverClassMapEntryS	driverClassMap[] =
{
	{ CC"udi_bridge", 2 },
	{ CC"udi_nic", 5 },
	{ CC"udi_gio", 4 },
	{ CC"udi_scsi", 3 },
	{ NULL, 0 }
};

error_t fplainn::driverC::preallocateModules(uarch_t nModules)
{
	if (nModules == 0) { return ERROR_SUCCESS; };
	modules = new moduleS[nModules];
	if (modules == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nModules = nModules;
	return ERROR_SUCCESS;
}

error_t fplainn::driverC::preallocateRegions(uarch_t nRegions)
{
	if (nRegions == 0) { return ERROR_SUCCESS; };
	regions = new regionS[nRegions];
	if (regions == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nRegions = nRegions;
	return ERROR_SUCCESS;
}

error_t fplainn::driverC::preallocateRequirements(uarch_t nRequirements)
{
	if (nRequirements == 0) { return ERROR_SUCCESS; };
	requirements = new requirementS[nRequirements];
	if (requirements == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nRequirements = nRequirements;
	return ERROR_SUCCESS;
}

error_t fplainn::driverC::preallocateMetalanguages(uarch_t nMetalanguages)
{
	if (nMetalanguages == 0) { return ERROR_SUCCESS; };
	metalanguages = new metalanguageS[nMetalanguages];
	if (metalanguages == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nMetalanguages = nMetalanguages;
	return ERROR_SUCCESS;
}

error_t fplainn::driverC::preallocateChildBops(uarch_t nChildBops)
{
	if (nChildBops == 0) { return ERROR_SUCCESS; };
	childBops = new childBopS[nChildBops];
	if (childBops == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nChildBops = nChildBops;
	return ERROR_SUCCESS;
}

error_t fplainn::driverC::preallocateParentBops(uarch_t nParentBops)
{
	if (nParentBops == 0) { return ERROR_SUCCESS; };
	parentBops = new parentBopS[nParentBops];
	if (parentBops == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nParentBops = nParentBops;
	return ERROR_SUCCESS;
}

error_t fplainn::driverC::preallocateInternalBops(uarch_t nInternalBops)
{
	if (nInternalBops == 0) { return ERROR_SUCCESS; };
	internalBops = new internalBopS[nInternalBops];
	if (internalBops == NULL) { return ERROR_MEMORY_NOMEM; };
	this->nInternalBops = nInternalBops;
	return ERROR_SUCCESS;
}

static inline void noop(void) { }

static sbit8 fillInClass(
	utf8Char (*classes)[DRIVER_CLASS_MAXLEN], ubit16 insertionIndex,
	utf8Char *className
	)
{
	for (uarch_t i=0; i<insertionIndex; i++)
	{
		// Check for duplicate classes.
		if (strcmp8(classes[i], className) != 0) { continue; };
		// If duplicate, don't allow nClasses to be incremented.
		return 0;
	};

	strcpy8(classes[insertionIndex], className);
	return 1;
}

error_t fplainn::driverC::detectClasses(void)
{
	uarch_t		nRecognizedClasses=0;
	metalanguageS	*metalanguage;

	/* Making 2 passes. First pass establishes the number of metalanguages
	 * the kernel recognizes; second pass allocates the array of classes
	 * for the driver, and fills it in.
	 **/
	for (ubit8 pass=1; pass <= 2; pass++)
	{
		for (uarch_t i=0; i<nChildBops; i++)
		{
			ubit16		currMetaIndex;

			currMetaIndex = childBops[i].metaIndex;
			metalanguage = getMetalanguage(currMetaIndex);
			if (metalanguage == NULL)
			{
				// Don't repeat the warning on the 2nd pass.
				(pass == 1)
				? printf(WARNING"driverC::detectClasses: drv "
					"%s/%s:\n",
					"\tMeta index %d in child bops doesn't "
					"map to any meta declaration.\n",
					basePath, shortName, currMetaIndex)
				: noop();

				continue;
			};

			/* Search through all the metalanguages that the driver
			 * exports as child_bind_ops, and see if the kernel can
			 * recognize any of them.
			 **/
			for (driverClassMapEntryS *tmp=driverClassMap;
				tmp->metaName != NULL;
				tmp++)
			{
				if (strcmp8(metalanguage->name, tmp->metaName))
					{ continue; };

				if (pass == 1) { nRecognizedClasses++; }
				else
				{
					nClasses += fillInClass(
						classes, nClasses,
						driverClasses[tmp->classIndex]);
				};

				break;
			};
		};

		// Don't execute anything beyond here on 2nd pass.
		if (pass == 2) { break; };

		/* If we couldn't recognize any of the metalanguage interfaces
		 * it exports, just class it as "unknown".
		 **/
		if (nRecognizedClasses == 0)
		{
			classes = new utf8Char[1][DRIVER_CLASS_MAXLEN];
			if (classes == NULL) { return ERROR_MEMORY_NOMEM; };

			// driver class 1 is set in stone as "unrecognized".
			strcpy8(classes[0], driverClasses[1]);
			nClasses = 1;
			return ERROR_SUCCESS;
		};

		classes = new utf8Char[nRecognizedClasses][DRIVER_CLASS_MAXLEN];
		if (classes == NULL) { return ERROR_MEMORY_NOMEM; };
	};

	return ERROR_SUCCESS;
}

error_t fplainn::driverInstanceC::initialize(void)
{
	if (driver->nChildBops > 0)
	{
		childBopVectors = new childBopS[driver->nChildBops];
		if (childBopVectors == NULL) { return ERROR_MEMORY_NOMEM; };

		memset(
			childBopVectors, 0,
			sizeof(*childBopVectors) * driver->nChildBops);

		for (uarch_t i=0; i<driver->nChildBops; i++)
		{
			childBopVectors[i].metaIndex =
				driver->childBops[i].metaIndex;
		};
	};

	return ERROR_SUCCESS;
}

error_t fplainn::driverInstanceC::addHostedDevice(utf8Char *path)
{
	heapPtrC<utf8Char*>	tmp;
	utf8Char		**old;
	heapPtrC<utf8Char>	str;
	uarch_t			len;

	for (uarch_t i=0; i<nHostedDevices; i++) {
		if (!strcmp8(hostedDevices[i], path)) { return ERROR_SUCCESS; };
	};

	len = strlen8(path);

	tmp = new utf8Char*[nHostedDevices + 1];
	str = new utf8Char[len + 1];
	tmp.useArrayDelete = str.useArrayDelete = 1;

	if (tmp == NULL || str == NULL) { return ERROR_MEMORY_NOMEM; };
	strcpy8(str.get(), path);

	if (nHostedDevices > 0)
	{
		memcpy(tmp.get(), hostedDevices,
			sizeof(*hostedDevices) * nHostedDevices);
	};

	tmp[nHostedDevices] = str.release();
	old = hostedDevices;
	hostedDevices = tmp.release();
	nHostedDevices++;

	delete[] old;
	return ERROR_SUCCESS;
}

void fplainn::driverInstanceC::removeHostedDevice(utf8Char *path)
{
	utf8Char	*tmp;

	for (uarch_t i=0; i<nHostedDevices; i++)
	{
		if (strcmp8(hostedDevices[i], path) != 0) { continue; };

		tmp = hostedDevices[i];
		memmove(
			&hostedDevices[i], &hostedDevices[i+1],
			sizeof(*hostedDevices) * (nHostedDevices - i - 1));

		nHostedDevices--;
		delete[] tmp;
		return;
	};
}

error_t fplainn::deviceInstanceC::initialize(void)
{
	regions = new regionS[device->driverInstance->driver->nRegions];
	if (regions == NULL) { return ERROR_MEMORY_NOMEM; };

	for (uarch_t i=0; i<device->driverInstance->driver->nRegions; i++)
	{
		regions[i].index = device->driverInstance->driver->regions[i]
			.index;
	};

	return ERROR_SUCCESS;
}

error_t fplainn::deviceInstanceC::addChannel(channelS *newChan)
{
	channelS		**tmp, **old;

	tmp = new channelS*[nChannels + 1];
	if (tmp == NULL) { return ERROR_MEMORY_NOMEM; };

	if (nChannels > 0)
		{ memcpy(tmp, channels, sizeof(*channels) * nChannels); };

	tmp[nChannels] = newChan;
	old = channels;
	channels = tmp;
	nChannels++;
	return ERROR_SUCCESS;
}

void fplainn::deviceInstanceC::removeChannel(channelS *chan)
{
	for (uarch_t i=0; i<nChannels; i++)
	{
		if (channels[i] != chan) { continue; };

		memmove(
			&channels[i], &channels[i+1],
			sizeof(*channels) * (nChannels - i - 1));

		nChannels--;
		return;
	};
}

error_t fplainn::deviceC::addParentTag(fvfs::tagC *tag, ubit16 *newId)
{
	parentTagS		*tmp, *old;

	tmp = new parentTagS[nParentTags + 1];
	if (tmp == NULL) { return ERROR_MEMORY_NOMEM; };

	if (nParentTags > 0)
		{ memcpy(tmp, parentTags, sizeof(*parentTags) * nParentTags); };

	new (&tmp[nParentTags]) parentTagS(parentTagCounter++, tag);
	*newId = tmp[nParentTags].id;
	old = parentTags;
	parentTags = tmp;
	nParentTags++;
	return ERROR_SUCCESS;
}

void fplainn::deviceC::dumpEnumerationAttributes(void)
{
	utf8Char		*fmtChar;

	printf(NOTICE"Device: @0x%p, bid %d, %d enum attrs.\n\tlongname %s.\n",
		this, bankId, nEnumerationAttrs, longName);

	for (uarch_t i=0; i<nEnumerationAttrs; i++)
	{
		switch (enumerationAttrs[i]->attr_type)
		{
		case UDI_ATTR_STRING: fmtChar = CC"%s"; break;
		case UDI_ATTR_UBIT32: fmtChar = CC"0x%x"; break;
		case UDI_ATTR_ARRAY8: fmtChar = CC"%x"; break;
		case UDI_ATTR_BOOLEAN: fmtChar = CC"%x"; break;
		default:
			printf(ERROR"Unknown device attr type %d.\n",
				enumerationAttrs[i]->attr_type);
			return;
		};

		// This recursive %s feature is actually pretty nice.
		printf(NOTICE"Attr: name %s, type %d, val %r.\n",
			enumerationAttrs[i]->attr_name, enumerationAttrs[i]->attr_type,
			fmtChar,
			(!strcmp8(fmtChar, CC"%s"))
				? (void *)enumerationAttrs[i]->attr_value
				: (void *)(uintptr_t)UDI_ATTR32_GET(
					enumerationAttrs[i]->attr_value));
	};
}

error_t fplainn::deviceC::getEnumerationAttribute(
	utf8Char *name, udi_instance_attr_list_t *attrib
	)
{
	// Simple search for an attribute with the name supplied.
	for (uarch_t i=0; i<nEnumerationAttrs; i++)
	{
		if (strcmp8(CC enumerationAttrs[i]->attr_name, CC name) == 0)
		{
			*attrib = *enumerationAttrs[i];
			return ERROR_SUCCESS;
		};
	};

	return ERROR_NOT_FOUND;
}

error_t fplainn::deviceC::addEnumerationAttribute(
	udi_instance_attr_list_t *attrib
	)
{
	error_t					ret;
	udi_instance_attr_list_t		**attrArrayMem, **tmp;
	heapPtrC<udi_instance_attr_list_t>	destAttrMem;
	sarch_t					releaseAttrMem=0;

	if (attrib == NULL) { return ERROR_INVALID_ARG; };

	destAttrMem = new udi_instance_attr_list_t;
	if (destAttrMem == NULL) { return ERROR_MEMORY_NOMEM; };

	// Can do other checks, such as checks on supplied attr's "type" etc.

	// Check to see if the attrib already exists.
	ret = getEnumerationAttribute(CC attrib->attr_name, destAttrMem.get());
	if (ret != ERROR_SUCCESS)
	{
		// Allocate mem for the new array of pointers to attribs.
		attrArrayMem = new udi_instance_attr_list_t *[
			nEnumerationAttrs + 1];

		if (attrArrayMem == NULL) { return ERROR_MEMORY_NOMEM; };

		if (nEnumerationAttrs > 0)
		{
			memcpy(
				attrArrayMem, enumerationAttrs,
				sizeof(*enumerationAttrs) * nEnumerationAttrs);
		};

		attrArrayMem[nEnumerationAttrs] = destAttrMem.get();
		/* Since we have to use the allocated memory to store the new
		 * attribute permanently, we have to call release() on the
		 * pointer management object later on. Set this bool to indicate
		 * that.
		 **/
		releaseAttrMem = 1;
		tmp = enumerationAttrs;
		enumerationAttrs = attrArrayMem;
		nEnumerationAttrs++;

		delete[] tmp;
	};

	memcpy(destAttrMem.get(), attrib, sizeof(*attrib));
	if (releaseAttrMem) { destAttrMem.release(); };
	return ERROR_SUCCESS;
}

error_t fplainn::driverC::moduleS::addAttachedRegion(ubit16 regionIndex)
{
	ubit16		*old;

	old = regionIndexes;
	regionIndexes = new ubit16[nAttachedRegions + 1];
	if (regionIndexes == NULL) { return ERROR_MEMORY_NOMEM; };
	if (nAttachedRegions > 0)
	{
		memcpy(
			regionIndexes, old,
			nAttachedRegions * sizeof(*regionIndexes));
	};

	delete[] old;

	regionIndexes[nAttachedRegions++] = regionIndex;
	return ERROR_SUCCESS;
}

error_t fplainn::driverC::addInstance(numaBankId_t bid, processId_t pid)
{
	driverInstanceC		*old, *newInstance;
	error_t			ret;

	newInstance = getInstance(bid);
	if (newInstance == NULL)
	{
		old = instances;
		instances = new driverInstanceC[nInstances + 1];
		if (instances == NULL) { return ERROR_MEMORY_NOMEM; };
		if (nInstances > 0) {
			memcpy(instances, old, nInstances * sizeof(*instances));
		};

		delete[] old;

		newInstance = &instances[nInstances];
	};

	new (newInstance) driverInstanceC(this, bid, pid);
	ret = newInstance->initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	nInstances++;
	return ERROR_SUCCESS;
}

void fplainn::driverC::dump(void)
{
	printf(NOTICE"Driver: index %d: %s/%s\n\t(%s).\n\tSupplier %s; "
		"Contact %s.\n"
		"%d mods, %d rgns, %d req's, %d metas, %d cbops, %d pbops, "
		"%d ibops.\n",
		basePath, shortName, longName, supplier, supplierContact,
		nModules, nRegions, nRequirements, nMetalanguages,
		nChildBops, nParentBops, nInternalBops);
}

