/* Copyright © 2017-2020 ABBYY Production LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
--------------------------------------------------------------------------------------------------------------*/

#pragma once

#include <NeoML/NeoMLDefs.h>

namespace NeoML {

// NeoML archive file
class NEOML_API CArchiveFile : public CBaseFile {
public:
	CArchiveFile() = default;
	// Creates an object and creates or opens a file
	// The same as calling a constructor without parameters and then the Open method
	CArchiveFile( const char* fileName, CArchive::TDirection direction, void* platformEnv = nullptr );
	~CArchiveFile() override { Abort(); }

	// Checks if the file is open
	bool IsOpen() const { return file != 0; }

	// Opens the file
	void Open( const char* fileName, CArchive::TDirection direction, void* platformEnv = nullptr );

	// CBaseFile class methods
#ifdef FINEOBJ_VERSION
	CUnicodeString GetFileName() const override { return fileName.CreateUnicodeString( CP_UTF8 ); }
#else
	const char* GetFileName() const override { return fileName; }
#endif
	int Read( void*, int bytesCount ) override;
	void Write( const void*, int bytesCount ) override;
	__int64 GetPosition() const override;
	__int64 Seek( __int64 offset, TSeekPosition from ) override;
	void SetLength( __int64 newLength ) override;
	__int64 GetLength() const override;
	void Abort() final;
	void Flush() override;
	void Close() override;
	bool IsEndOfFile() const override;

	void ReadRecord( void* buff, int size );
	unsigned char ReadByte();

private:
	void* file{}; // the base object
	CString fileName; // the file name (needed for the CBaseFile::GetFileName() method)
};

} // namespace NeoML
