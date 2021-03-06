/*! \file	partition.h
 *	\brief	Definition of Partition class
 *
 *			The Partition class holds data about a partition, either loaded 
 *          from a partition in the file or built in memory
 *
 *	\version $Id: partition.h,v 1.8 2011/01/10 10:42:09 matt-beard Exp $
 *
 */
/*
 *	Copyright (c) 2003, Matt Beard
 *	Portions Copyright (c) 2003, Metaglue Corporation
 *
 *	This software is provided 'as-is', without any express or implied warranty.
 *	In no event will the authors be held liable for any damages arising from
 *	the use of this software.
 *
 *	Permission is granted to anyone to use this software for any purpose,
 *	including commercial applications, and to alter it and redistribute it
 *	freely, subject to the following restrictions:
 *
 *	  1. The origin of this software must not be misrepresented; you must
 *	     not claim that you wrote the original software. If you use this
 *	     software in a product, an acknowledgment in the product
 *	     documentation would be appreciated but is not required.
 *	
 *	  2. Altered source versions must be plainly marked as such, and must
 *	     not be misrepresented as being the original software.
 *	
 *	  3. This notice may not be removed or altered from any source
 *	     distribution.
 */
#ifndef MXFLIB__PARTITION_H
#define MXFLIB__PARTITION_H

#include "mxflib/primer.h"

#include <list>


namespace mxflib
{
	//! Holds data relating to a single partition
	class Partition : public ObjectInterface, public RefCount<Partition>
	{
	public:
		/* DRAGONS: The Metadata class holds a smart pointer to this class.
		 *          This means that once the Metadata has been parsed it 'owns' the partition.
		 *          This is required to ensure that all metadata objects in the AllMetadata list live at least as long as the 'Metadata' object.
		 *          This means that this class can never include a smart pointer to the parsed Metadata object as this would be a loop!
		 */
		PrimerPtr PartitionPrimer;		//!< The Primer for this partition
										/*!< Or NULL if no primer pack active (only valid
										 *   if there is no header metadata in this partition
										 *   OR it has not yet been written)
										 */

		MDObjectList AllMetadata;		//!< List of all header metadata sets in the partition
		MDObjectList TopLevelMetadata;	//!< List of all metadata items in the partition not linked from another

	private:
		std::map<UUID, MDObjectPtr> RefTargets;				//!< Map of UUID of all reference targets to objects
		std::multimap<UUID, MDObjectPtr> UnmatchedRefs;		//!< Map of UUID of all strong or weak refs not yet linked

	public:
		Partition(const char *BaseType) { Object = new MDObject(BaseType); };
		Partition(MDOTypePtr BaseType) { Object = new MDObject(BaseType); };
		Partition(const UL &BaseUL) { Object = new MDObject(BaseUL); };
		Partition(ULPtr BaseUL) { Object = new MDObject(*BaseUL); };

		//! Reload the metadata tree - DRAGONS: not an ideal way of doing this
		void UpdateMetadata(ObjectInterface *NewObject) { ClearMetadata(); AddMetadata(NewObject->Object); };

		//! Reload the metadata tree - DRAGONS: not an ideal way of doing this
		void UpdateMetadata(MDObjectPtr NewObject) { ClearMetadata(); AddMetadata(NewObject); };

		//! Add a metadata object to the header metadata belonging to a partition
		/*! Note that any strongly linked objects are also added */
		void AddMetadata(ObjectInterface *NewObject) { AddMetadata(NewObject->Object); };
		void AddMetadata(MDObjectPtr NewObject, bool ForceFirst = false);

		//! Clear all header metadata for this partition (including the primer)
		void ClearMetadata(void)
		{
			PartitionPrimer = NULL;
			AllMetadata.clear();
			TopLevelMetadata.clear();
			RefTargets.clear();
			UnmatchedRefs.clear();
		}

		//! Read a full set of header metadata from this partition's source file (including primer)
		Length ReadMetadata(void);

		//! Read a full set of header metadata from a file (including primer)
		Length ReadMetadata(MXFFilePtr File, Length Size);

		//! Parse the current metadata sets into higher-level sets
		MetadataPtr ParseMetadata(void);

		//! Read any index table segments from this partition's source file
		MDObjectListPtr ReadIndex(void);

		//! Read any index table segments from a file
		MDObjectListPtr ReadIndex(MXFFilePtr File, UInt64 Size);

		//! Read any index segments from this partition's source file, and add them to a given table
		/*! \ret true if all OK
		 */
		bool ReadIndex(IndexTablePtr Table);

		//! Read raw index table data from this partition's source file
		DataChunkPtr ReadIndexChunk(void);

		//! Set the KAG for this partition
		void SetKAG(UInt64 KAG)
		{
			MDObjectPtr Ptr = Object[KAGSize_UL];
			mxflib_assert(Ptr);
			Ptr->SetUInt64(KAG);
		}

		// Access functions for the reference resolving properties
		// DRAGONS: These should be const, but can't make it work!
		std::map<UUID, MDObjectPtr>& GetRefTargets(void) { return RefTargets; };
		std::multimap<UUID, MDObjectPtr>& GetUnmatchedRefs(void) { return UnmatchedRefs; };

		//! Determine if the partition object is currently set as complete
		bool IsComplete(void);

		//! Determine if the partition object is currently set as closed
		bool IsClosed(void);

		//! Determine if the metadata in this partition is actually "complete"
		/*! \ret true if all required properties exist in the metadata sets and no best-effort property is set to its destinguished value
		 *  \ret false in all other cases, or if there is no metadata loaded
		 *  \note This is not a guarantee that the metadata is valid!
		 */
		bool IsMetadataComplete(void)
		{
			warning("Partition::IsMetadataComplete() not yet implemented\n");
			return false;
		}

		//! Locate start of Essence Container
		bool SeekEssence(void);

		//! Locate the set that refers to the given set (with a strong reference)
		MDObjectParent FindLinkParent(MDObjectPtr &Child);

		//! Locate the set that refers to the given set (with a strong reference)
		MDObjectParent FindLinkParent(MDObjectParent &Child) 
		{ 
			MDObjectPtr Obj = &(*Child);
			return FindLinkParent(Obj);
		}

	// Sequential access to the Elements of the Body

	public:
		// goto start of body...set the member variables _BodyLocation, _NextBodyLocation
		bool StartElements();
		// goto _NextBodyLocation
		KLVObjectPtr NextElement();
		// skip over a KLV packet

	protected:
		UInt64 Skip( UInt64 start );
		// skip over any KLVFill
		// DRAGONS: does not iterate - only copes with single KLVFill
		UInt64 SkipFill( UInt64 start );

		//! Load any metadictionaties that are in the list of currently loaded objects
		bool LoadMetadict(void);

		//! Satisfy, or record as un-matched, all outgoing references
		void ProcessChildRefs(MDObjectPtr ThisObject);

		//! Scan a metadata object for strong references in sub-objects and add those to this partition
		void AddMetadataSubs(MDObjectPtr &NewObject, bool ForceFirst);

	private:
		UInt64 _BodyLocation;				// file position for current Element
		UInt64 _NextBodyLocation;		// file position for Element after this
	};
}

// These simple inlines need to be defined after Partition
namespace mxflib
{
inline MDObjectPtr PartitionPtr::operator[](const char *ChildName) { return GetPtr()->Object[ChildName]; }
inline MDObjectPtr PartitionPtr::operator[](MDOTypePtr ChildType) { return GetPtr()->Object[ChildType]; }
inline MDObjectPtr PartitionPtr::operator[](const UL &ChildType) { return GetPtr()->Object[ChildType]; }
inline MDObjectPtr PartitionPtr::operator[](ULPtr &ChildType) { return GetPtr()->Object[*ChildType]; }
}

#endif // MXFLIB__PARTITION_H
