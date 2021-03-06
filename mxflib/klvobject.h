/*! \file	klvobject.h
 *	\brief	Definition of classes that define basic KLV objects
 *
 *			Class KLVObject holds info about a KLV object
 *
 *	\version $Id: klvobject.h,v 1.7 2011/01/10 10:42:09 matt-beard Exp $
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
#ifndef MXFLIB__KLVOBJECT_H
#define MXFLIB__KLVOBJECT_H


// STL Includes
#include <string>
#include <list>
#include <map>


// Define some enums
namespace mxflib
{
	enum KeyFormat {
		KEY_NONE = 0,
		KEY_1_BYTE = 1,
		KEY_2_BYTE = 2,
		KEY_4_BYTE = 4,
		KEY_AUTO = 3
	};

	enum LenFormat
	{
		LEN_NONE = 0,
		LEN_1_BYTE = 1,
		LEN_2_BYTE = 2,
		LEN_4_BYTE = 4,
		LEN_BER = 3
	};
}


namespace mxflib
{
	// Forward declare so the class can include pointers to itself
	class KLVObject;

	//! A smart pointer to a KLVObject object
	typedef SmartPtr<KLVObject> KLVObjectPtr;

	//! A list of smart pointers to KLVObject objects
	typedef std::list<KLVObjectPtr> KLVObjectList;

	typedef std::map<std::string, KLVObjectPtr> KLVObjectMap;
}


namespace mxflib
{
	//! Base class for KLVObject Reader handlers
	/*! \note Classes derived from this class <b>must not</b> include their own RefCount<> derivation
	 */
	class KLVReadHandler_Base : public RefCount<KLVReadHandler_Base>
	{
	public:
		//! Base destructor
		virtual ~KLVReadHandler_Base();

		//! Read data from the source into the KLVObject
		/*! \param Buffer Reference to a buffer to receive the data
		 *  \param Object KLVObject which is requesting the data
		 *  \param Start Offset from the start of the KLV value to start reading
		 *  \param Size Number of bytes to read, if -1 all available bytes will be read (which could be billions!)
		 *  \return The count of bytes read (may be less than Size if less available)
		 *  \note A call to ReadData must replace the current contents of the KLVObject's DataChunk
		 *        with the new data - no original data should be preserved
		 */
		virtual size_t ReadData(DataChunk &Buffer, KLVObjectPtr Object, Position Start = 0, size_t Size = static_cast<size_t>(-1)) = 0;

//		//! Read the key and length of the KLVObject
//		virtual Int32 ReadKL(KLVObjectPtr Object) { return -1;}
	};

	//! Smart pointer for the base KLVObject read handler
	typedef SmartPtr<KLVReadHandler_Base> KLVReadHandlerPtr;


//	//! Base class for KLVObject write handlers
//	/*! \note Classes derived from this class <b>must not</b> include their own RefCount<> derivation
//	 */
//	class KLVWriteHandler_Base : public RefCount<KLVWriteHandler_Base>
//	{
//	public:
//		//! Base destructor
//		virtual ~KLVReadHandler_Base();
//
//		//! Write data from the KLVObject to the destination
//		/*! \param Object KLVObject that is the data source
//		 *  \param Buffer Location of the data to write
//		 *  \param Start Offset from the start of the KLV value of the first byte to be written
//		 *  \param Size Number of bytes to be written
//		 *  \return The count of bytes written
//		 */
//		virtual size_t WriteData(KLVObjectPtr Object, const UInt8 *Buffer, Position Start = 0, size_t Size = static_cast<size_t>(-1)) = 0;
//	};
//
//	//! Smart pointer for the base KLVObject read handler
//	typedef SmartPtr<KLVWriteHandler_Base> KLVWriteHandlerPtr;


	//! KLV Object class
	/*! This class gives access to single KLV items within an MXFfile.
	 *  The normal use for this class is handling of essence data. Huge values can be safely 
	 *  handled by loading them a "chunk" at a time.  Data is also available to identify the
	 *  location of the value in an MXFFile so that MXFFile::Read() and MXFFile::Write can
	 *  be used for efficient access.
	 *  \note This class does <b>not</b> provide any interlock mechanism to ensure safe
	 *        concurrent access. So if modified data is held in the object's DataChunk,
	 *        but not yet written to the file, calls to KLVObject::ReadData() or
	 *        MXFFile::Read() will return the <b>unmodifief</b> data.
	 */
	class KLVObject : public RefCount<KLVObject>
	{
	protected:
		class KLVInfo
		{
		private:
			KLVInfo(KLVInfo &);				//!< Prevent copy-construction

		public:
			MXFFilePtr File;				//!< Source or destination file
			Position Offset;				//!< Offset of the first byte of the <b>key</b> as an offset into the file (-1 if not available)
			Length OuterLength;				//!< The length of the entire readable value space - in basic KLV types this is always ValueLength, derived types may add some hidden overhead
			Int32 KLSize;					//!< Size of this object's KL in the source or destination file (-1 if not known)
			bool Valid;						//!< Set to true once the data is "set"

			KLVInfo()
			{
				Valid = false;
				Offset = -1;
				OuterLength = 0;
				KLSize = -1;
			}
			
			KLVInfo &operator=(const KLVInfo &RHS)
			{
				Valid = RHS.Valid;
				File = RHS.File;
				Offset = RHS.Offset;
				OuterLength = RHS.OuterLength;
				KLSize = RHS.KLSize;

				return *this;
			}
		};

		KLVInfo Source;						//!< Info on the source file
		KLVInfo Dest;						//!< Info on the destination file

		ULPtr TheUL;						//!< The UL for this object (if known)
		Length ValueLength;					//!< Length of the value field

		DataChunk Data;						//!< The raw data for this item (if available)
		Position DataBase;					//!< The offset of the first byte in the DataChunk from the start of the KLV value field

		KLVReadHandlerPtr ReadHandler;		//!< A read-handler to supply data in response to read requests. If NULL data will be read from SourceFile (if available)
//		KLVWriteHandlerPtr WriteHandler;	//!< A read-handler to supply data in response to read requests. If NULL data will be read from SourceFile (if available)

		//## DRAGONS: Ensure any new properties are copied by the KLVObject --> KLVEObject copy constructor ##

		//@@@ Is this another MSVC bug?  KLVEObject can't access protected KLVObject properties from KLVEObject constructor!!
		friend class KLVEObject;

	public:
		KLVObject(ULPtr ObjectUL = NULL);
		virtual void Init(void);
		virtual ~KLVObject() {};		//!< Virtual to allow sub-classing and polymorphic pointers

		//! Set the source details when an object has been read from a file
		/*! \param File The source file of this KLVObject
		 *  \param Location The byte offset of the start of the <b>key</b> of the KLV from the start of the file (current position if -1)
		 */
		virtual void SetSource(MXFFilePtr File, Position Location = -1)
		{
			Source.Valid = true;
			Source.File = File;
			if(Location < 0) Source.Offset = File->Tell();
			else Source.Offset = Location;

			// If we don't have a destination file assume it is the same as the source file
			if(!Dest.Valid)
			{
				Dest = Source;
			}
		}

		//! Set the destination details for the object to be written to a file
		/*! \param File The destination file of this KLVObject
		 *  \param Location The byte offset of the start of the <b>key</b> of the KLV from the start of the file, if omitted (or -1) the current position in that file will be used
		 */
		virtual void SetDestination(MXFFilePtr File, Position Location = -1)
		{
			Dest.Valid = true;
			Dest.File = File;

			if(Location < 0) Dest.Offset = File->Tell();
			else Dest.Offset = Location;
		}

		//! Get the object's UL
		virtual ULPtr GetUL(void) { return TheUL; }

		//! Set the object's UL
		virtual void SetUL(ULPtr NewUL) { TheUL = NewUL; }

		//! Get the location within the ultimate parent
		virtual Position GetLocation(void) { return Source.Offset; }

		//! Get text that describes where this item came from
		virtual std::string GetSource(void);

		//! Get text that describes exactly where this item came from
		std::string GetSourceLocation(void) 
		{
			if(!Source.File) return std::string("KLVObject created in memory");
			return std::string("0x") + Int64toHexString(GetLocation(),8) + std::string(" in ") + GetSource();
		}

		//! Get the size of the key and length (not of the value)
		virtual Int32 GetKLSize(void) { return Source.KLSize >= 0 ? Source.KLSize : Dest.KLSize; }

		//! Set the size of the key and length (not of the value)
		/*! This will be used when writing to the destination (if possible) - you cannot change the "source" KLSize 
		 */
		virtual void SetKLSize(Int32 NewKLSize) { Dest.KLSize = NewKLSize; }

		//! Get a GCElementKind structure
		virtual GCElementKind GetGCElementKind(void) { return mxflib::GetGCElementKind(TheUL); }

		//! Determine if this is a system item
		virtual bool IsGCSystemItem(void) { return mxflib::IsGCSystemItem(TheUL); }

		//! Get the track number of this KLVObject (if it is a GC KLV, else 0)
		virtual UInt32 GetGCTrackNumber(void) { return mxflib::GetGCTrackNumber(TheUL); };

		//! Get the position of the first byte in the DataChunk as an offset into the file
		/*! \return -1 if the data has not been read from a file (or the offset cannot be determined) 
		 */
		virtual Position GetDataBase(void) { return DataBase; };

		//! Set the position of the first byte in the DataChunk as an offset into the file
		/*! \note This function must be used with great care as data may will be written to this location
		 */
		virtual void SetDataBase(Position NewBase) { DataBase = NewBase; };

		//! Read the key and length for this KLVObject from the current source
		/*! \return The number of bytes read (i.e. KLSize)
		 */
		virtual Int32 ReadKL(void) { return Base_ReadKL(); }

		//! Base verion: Read the key and length for this KLVObject from the current source
		/*! \return The number of bytes read (i.e. KLSize)
		 *
		 *  DRAGONS: This base function may be called from derived class objects to get base behaviour.
		 *           It is therefore vital that the function does not call any "virtual" KLVObject
		 *           functions, directly or indirectly.
		 */
		Int32 Base_ReadKL(void);

		//! Read data from the start of the KLV value into the current DataChunk
		/*! \param Size Number of bytes to read, if -1 all available bytes will be read (which could be billions!)
		 *  \return The number of bytes read
		 */
		virtual size_t ReadData(size_t Size = static_cast<size_t>(-1)) { return Base_ReadDataFrom(0, Size); }

		//! Read data from a specified position in the KLV value field into the DataChunk
		/*! \param Offset Offset from the start of the KLV value from which to start reading
		 *  \param Size Number of bytes to read, if -1 all available bytes will be read (which could be billions!)
		 *  \return The number of bytes read
		 */
		virtual size_t ReadDataFrom(Position Offset, size_t Size = static_cast<size_t>(-1)) { return Base_ReadDataFrom(Offset, Size); }

		//! Base verion: Read data from a specified position in the KLV value field into the DataChunk
		/*! \param Offset Offset from the start of the KLV value from which to start reading
		 *  \param Size Number of bytes to read, if -1 all available bytes will be read (which could be billions!)
		 *  \return The number of bytes read
		 *
		 *  DRAGONS: This base function may be called from derived class objects to get base behaviour.
		 *           It is therefore vital that the function does not call any "virtual" KLVObject
		 *           functions, directly or indirectly.
		 */
		inline size_t Base_ReadDataFrom(Position Offset, size_t Size = static_cast<size_t>(-1)) { return Base_ReadDataFrom(Data, Offset, Size); }

		//! Base verion: Read data from a specified position in the KLV value field into the DataChunk
		/*! \param Offset Offset from the start of the KLV value from which to start reading
		 *  \param Size Number of bytes to read, if -1 all available bytes will be read (which could be billions!)
		 *  \return The number of bytes read
		 *
		 *  \note This function can write to a buffer other than the KLVObject's main buffer if required, 
		 *        however the file pointer will be updated so care must be used when mixing reads
		 *
		 *  DRAGONS: This base function may be called from derived class objects to get base behaviour.
		 *           It is therefore vital that the function does not call any "virtual" KLVObject
		 *           functions, directly or indirectly.
		 */
		size_t Base_ReadDataFrom(DataChunk &Buffer, Position Offset, size_t Size = static_cast<size_t>(-1));

		//! Write the key and length of the current DataChunk to the destination file
		/*! The key and length will be written to the source file as set by SetSource.
		 *  If LenSize is zero the length will be formatted to match KLSize (if possible!)
		 */
		virtual Int32 WriteKL(Int32 LenSize = 0) { return Base_WriteKL(LenSize); }

		//! Base verion: Write the key and length of the current DataChunk to the destination file
		/*! The key and length will be written to the source file as set by SetSource.
		 *  If LenSize is zero the length will be formatted to match KLSize (if possible!).
		 *  The length written can be overridden by using parameter NewLength
		 *
		 *  DRAGONS: This base function may be called from derived class objects to get base behaviour.
		 *           It is therefore vital that the function does not call any "virtual" KLVObject
		 *           functions, directly or indirectly.
		 */
		Int32 Base_WriteKL(Int32 LenSize = 0, Length NewLength = -1);


		//! Write (some of) the current data to the same location in the destination file
		/*! \param Size The number of bytes to write, if -1 all available bytes will be written
		 *  \return The number of bytes written
		 */
		virtual size_t WriteData(size_t Size = static_cast<size_t>(-1)) { return WriteDataFromTo(0, 0, Size); }

		//! Write (some of) the current data to the same location in the destination file
		/*! \param Start The offset within the current DataChunk of the first byte to write
		 *  \param Size The number of bytes to write, if -1 all available bytes will be written
		 *  \return The number of bytes written
		 */
		virtual size_t WriteDataFrom(Position Start, size_t Size = static_cast<size_t>(-1)) { return WriteDataFromTo(0, Start, Size); }

		//! Write (some of) the current data to a different location in the destination file
		/*! \param Offset The offset within the KLV value field of the first byte to write
		 *  \param Size The number of bytes to write, if <= 0 all available bytes will be written
		 *  \return The number of bytes written
		 */
		virtual size_t WriteDataTo(Position Offset, size_t Size = static_cast<size_t>(-1)) { return WriteDataFromTo(Offset, 0, Size); }

		//! Write (some of) the current data to the same location in the destination file
		/*! \param Offset The offset within the KLV value field of the first byte to write
		 *  \param Start The offset within the current DataChunk of the first byte to write
		 *  \param Size The number of bytes to write, if -1 all available bytes will be written
		 *  \return The number of bytes written
		 */
		virtual size_t WriteDataFromTo(Position Offset, Position Start, size_t Size = static_cast<size_t>(-1))
		{
			// Calculate default number of bytes to write
			Length BytesToWrite = Data.Size - Start;

			// Write the requested size (if valid)
			if((Size > 0) && ((Length)Size < BytesToWrite)) BytesToWrite = Size;

			// Sanity check the size of this chunk
			if((sizeof(size_t) < 8) && (BytesToWrite > 0xffffffff))
			{
				error("Tried to write > 4GBytes, but this platform can only handle <= 4GByte chunks\n");
				return 0;
			}

			return Base_WriteDataTo(&Data.Data[Start], Offset, static_cast<size_t>(BytesToWrite));
		}

		//! Write data from a given buffer to a given location in the destination file
		/*! \param Buffer Pointer to data to be written
		 *  \param Offset The offset within the KLV value field of the first byte to write
		 *  \param Size The number of bytes to write
		 *  \return The number of bytes written
		 *  \note As there may be a need for the implementation to know where within the value field
		 *        this data lives, there is no WriteData(Buffer, Size) function.
		 */
		virtual size_t WriteDataTo(const UInt8 *Buffer, Position Offset, size_t Size) { return Base_WriteDataTo(Buffer, Offset, Size); }

		//! Base verion: Write data from a given buffer to a given location in the destination file
		/*! \param Buffer Pointer to data to be written
		 *  \param Offset The offset within the KLV value field of the first byte to write
		 *  \param Size The number of bytes to write
		 *  \return The number of bytes written
		 *
 		 *  DRAGONS: This base function may be called from derived class objects to get base behaviour.
		 *           It is therefore vital that the function does not call any "virtual" KLVObject
		 *           functions, directly or indirectly.
		 */
		size_t Base_WriteDataTo(const UInt8 *Buffer, Position Offset, size_t Size);


		//! Set a handler to supply data when a read is performed
		/*! \note If not set it will be read from the source file (if available) or cause an error message
		 */
		virtual void SetReadHandler(KLVReadHandlerPtr Handler) { ReadHandler = Handler; }

//		//! Set a handler to supply data when a write is performed
//		/*! \note If not set it will be written to destination file (if available) or cause an error message
//		 */
//		virtual void SetWriteHandler(KLVWriteHandlerPtr Handler) { WriteHandler = Handler; }

		//! Get the length of the value field
		virtual Length GetLength(void) { return ValueLength; }

		//! Set the length of the value field
		virtual void SetLength(Length NewLength) { ValueLength = Dest.OuterLength = Source.OuterLength = NewLength; }

		//! Get a reference to the data chunk
		virtual DataChunk& GetData(void) { return Data; }
	};
}

#endif // MXFLIB__KLVOBJECT_H
