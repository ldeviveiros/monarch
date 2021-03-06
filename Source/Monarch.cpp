#include "Monarch.hpp"
#include "MonarchException.hpp"

namespace monarch
{

    Monarch::Monarch() :
                fState( eClosed ),
                fIO( NULL ),
                fHeader( NULL ),
                fDataTypeSize( 1 ),
                fDataNBytes( 0 ),
                fDataSize( 0 ),
                fInterleavedRecordNBytes( 0 ),
                fRecordInterleaved( NULL ),
                fRecordInterleavedBytes( NULL ),
                fSeparateRecordNBytes( 0 ),
                fRecordSeparateOne( NULL ),
                fRecordSeparateOneBytes( NULL ),
                fRecordSeparateTwo( NULL ),
                fRecordSeparateTwoBytes( NULL ),
                fReadFunction( &Monarch::InterleavedFromInterleaved ),
                fWriteFunction( &Monarch::InterleavedToInterleaved )
    {
    }
    Monarch::~Monarch()
    {
        if( fIO != NULL )
        {
            delete fIO;
            fIO = NULL;
        }

        if( fHeader != NULL )
        {
            delete fHeader;
            fHeader = NULL;
        }

        if( fRecordInterleavedBytes != NULL )
        {
            fRecordInterleaved->~MonarchRecordBytes();
            delete[] fRecordInterleavedBytes;
            fRecordInterleavedBytes = NULL;
        }

        if( fRecordSeparateOneBytes != NULL )
        {
            fRecordSeparateOne->~MonarchRecordBytes();
            delete[] fRecordSeparateOneBytes;
            fRecordSeparateOneBytes = NULL;
        }

        if( fRecordSeparateTwoBytes != NULL )
        {
            fRecordSeparateTwo->~MonarchRecordBytes();
            delete[] fRecordSeparateTwoBytes;
            fRecordSeparateTwoBytes = NULL;
        }
    }

    const Monarch* Monarch::OpenForReading( const string& aFilename )
    {
        Monarch* tMonarch = new Monarch();

        tMonarch->fIO = new MonarchIO( sAccessRead );
        if( tMonarch->fIO->Open( aFilename ) == false )
        {
            delete tMonarch;
            throw MonarchException() << "could not open <" << aFilename << "> for reading";
            return NULL;
        }

        tMonarch->fHeader = new MonarchHeader();
        tMonarch->fHeader->SetFilename( aFilename );

        tMonarch->fState = eOpen;

        return tMonarch;
    }

    Monarch* Monarch::OpenForWriting( const string& aFilename )
    {
        Monarch* tMonarch = new Monarch();

        tMonarch->fIO = new MonarchIO( sAccessWrite );
        if( tMonarch->fIO->Open( aFilename ) == false )
        {
            delete tMonarch;
            throw MonarchException() << "could not open <" << aFilename << "> for writing";
            return NULL;
        }

        tMonarch->fHeader = new MonarchHeader();
        tMonarch->fHeader->SetFilename( aFilename );

        tMonarch->fState = eOpen;

        return tMonarch;
    }

    void Monarch::ReadHeader() const
    {
        PreludeType tPrelude = 0;
        if( fIO->Read( &tPrelude ) == false )
        {
            throw MonarchException() << "prelude was not read properly";
            return;
        }

        char* tHeaderBuffer = new char[ tPrelude ];
        if( fIO->Read( tHeaderBuffer, tPrelude ) == false )
        {
            delete[] tHeaderBuffer;
            throw MonarchException() << "header was not read properly";
            return;
        }
        if( fHeader->DemarshalFromArray( tHeaderBuffer, tPrelude ) == false )
        {
            delete[] tHeaderBuffer;
            throw MonarchException() << "header was not demarshalled properly";
            return;
        }
        delete[] tHeaderBuffer;

        fDataTypeSize = fHeader->GetDataTypeSize();

        if( fHeader->GetAcquisitionMode() == 1 /* the FormatMode is ignored for single-channel data */ )
        {
            fDataSize = fHeader->GetRecordSize();
            fDataNBytes = fDataSize * fDataTypeSize;

            fInterleavedRecordNBytes = sizeof(AcquisitionIdType) + sizeof(RecordIdType) + sizeof(TimeType) + fDataNBytes;
            fRecordInterleavedBytes = new byte_type[ fInterleavedRecordNBytes ];
            fRecordInterleaved = new ( fRecordInterleavedBytes ) MonarchRecordBytes();

            fSeparateRecordNBytes = sizeof(AcquisitionIdType) + sizeof(RecordIdType) + sizeof(TimeType) + fDataNBytes;
            fRecordSeparateOneBytes = new byte_type[ fSeparateRecordNBytes ];
            fRecordSeparateOne = new ( fRecordSeparateOneBytes ) MonarchRecordBytes();

            //cout << "  *format is <" << sFormatSingle << ">" << endl;
            //cout << "  *data size is <" << fDataSize << ">" << endl;
            //cout << "  *interleaved size is <" << fInterleavedRecordSize << ">" << endl;
            //cout << "  *separate size is <" << fSeparateRecordSize << ">" << endl;

            fReadFunction = &Monarch::SeparateFromSingle;
        }
        else if( fHeader->GetAcquisitionMode() == 2 && fHeader->GetFormatMode() == sFormatMultiSeparate )
        {
            fDataSize = fHeader->GetRecordSize();
            fDataNBytes = fDataSize * fDataTypeSize;

            fInterleavedRecordNBytes = sizeof(AcquisitionIdType) + sizeof(RecordIdType) + sizeof(TimeType) + 2 * fDataNBytes;
            fRecordInterleavedBytes = new byte_type[ fInterleavedRecordNBytes ];
            fRecordInterleaved = new ( fRecordInterleavedBytes ) MonarchRecordBytes();

            fSeparateRecordNBytes = sizeof(AcquisitionIdType) + sizeof(RecordIdType) + sizeof(TimeType) + (size_t)fDataNBytes;
            fRecordSeparateOneBytes = new byte_type[ fSeparateRecordNBytes ];
            fRecordSeparateOne = new ( fRecordSeparateOneBytes ) MonarchRecordBytes();
            fRecordSeparateTwoBytes = new byte_type[ fSeparateRecordNBytes ];
            fRecordSeparateTwo = new ( fRecordSeparateTwoBytes ) MonarchRecordBytes();

            //cout << "  *format is <" << sFormatSeparateDual << ">" << endl;
            //cout << "  *data size is <" << fDataSize << ">" << endl;
            //cout << "  *interleaved size is <" << fInterleavedRecordSize << ">" << endl;
            //cout << "  *separate size is <" << fSeparateRecordSize << ">" << endl;

            fReadFunction = &Monarch::SeparateFromSeparate;
        }
        else if( fHeader->GetAcquisitionMode() == 2 && fHeader->GetFormatMode() == sFormatMultiInterleaved )
        {
            fDataSize = fHeader->GetRecordSize();
            fDataNBytes = fDataSize * fDataTypeSize;

            fInterleavedRecordNBytes = sizeof(AcquisitionIdType) + sizeof(RecordIdType) + sizeof(TimeType) + 2 * fDataNBytes;
            fRecordInterleavedBytes = new byte_type[ fInterleavedRecordNBytes ];
            fRecordInterleaved = new ( fRecordInterleavedBytes ) MonarchRecordBytes();

            fSeparateRecordNBytes = sizeof(AcquisitionIdType) + sizeof(RecordIdType) + sizeof(TimeType) + fDataNBytes;
            fRecordSeparateOneBytes = new byte_type[ fSeparateRecordNBytes ];
            fRecordSeparateOne = new ( fRecordSeparateOneBytes ) MonarchRecordBytes();
            fRecordSeparateTwoBytes = new byte_type[ fSeparateRecordNBytes ];
            fRecordSeparateTwo = new ( fRecordSeparateTwoBytes ) MonarchRecordBytes();

            //cout << "  *format is <" << sFormatInterleavedDual << ">" << endl;
            //cout << "  *data size is <" << fDataSize << ">" << endl;
            //cout << "  *interleaved size is <" << fInterleavedRecordSize << ">" << endl;
            //cout << "  *separate size is <" << fSeparateRecordSize << ">" << endl;

            fReadFunction = &Monarch::SeparateFromInterleaved;
        }
        else
        {
            throw MonarchException() << "Unable to read a header with acquisition mode <" << fHeader->GetAcquisitionMode() << "> and format mode <" << fHeader->GetFormatMode() << ">";
            return;
        }

        fState = eReady;
        return;
    }

    void Monarch::WriteHeader()
    {
        PreludeType tPrelude = fHeader->ByteSize();
        if( fIO->Write( &tPrelude ) == false )
        {
            throw MonarchException() << "prelude was not written properly";
            return;
        }

        char* tHeaderBuffer = new char[ tPrelude ];
        if( fHeader->MarshalToArray( tHeaderBuffer, tPrelude ) == false )
        {
            delete[] tHeaderBuffer;
            throw MonarchException() << "header was not marshalled properly";
            return;
        }
        if( fIO->Write( tHeaderBuffer, tPrelude ) == false )
        {
            delete[] tHeaderBuffer;
            throw MonarchException() << "header was not written properly";
            return;
        }
        delete[] tHeaderBuffer;

        fDataTypeSize = fHeader->GetDataTypeSize();

        if( fHeader->GetAcquisitionMode() == 1 /* the FormatMode is ignored for single-channel data */ )
        {
            fDataSize = fHeader->GetRecordSize();
            fDataNBytes = fDataSize * fDataTypeSize;

            fInterleavedRecordNBytes = sizeof(AcquisitionIdType) + sizeof(RecordIdType) + sizeof(TimeType) + fDataNBytes;
            fRecordInterleavedBytes = new byte_type[ fInterleavedRecordNBytes ];
            fRecordInterleaved = new ( fRecordInterleavedBytes ) MonarchRecordBytes();

            fSeparateRecordNBytes = sizeof(AcquisitionIdType) + sizeof(RecordIdType) + sizeof(TimeType) + fDataNBytes;
            fRecordSeparateOneBytes = new byte_type[ fSeparateRecordNBytes ];
            fRecordSeparateOne = new ( fRecordSeparateOneBytes ) MonarchRecordBytes();

            //cout << "  *format is <" << sFormatSingle << ">" << endl;
            //cout << "  *data type size is <" << fDataTypeSize << ">" << endl;
            //cout << "  *data size is <" << fDataSize << "> and # of data bytes is <" << fDataNBytes << ">" << endl;
            //cout << "  *interleaved # of bytes is <" << fInterleavedRecordNBytes << ">" << endl;
            //cout << "  *separate # of bytes is <" << fSeparateRecordNBytes << ">" << endl;

            fWriteFunction = &Monarch::SeparateToSingle;
        }
        else if( fHeader->GetAcquisitionMode() == 2 && fHeader->GetFormatMode() == sFormatMultiSeparate )
        {
            fDataSize = fHeader->GetRecordSize();
            fDataNBytes = fDataSize * fDataTypeSize;

            fInterleavedRecordNBytes = sizeof(AcquisitionIdType) + sizeof(RecordIdType) + sizeof(TimeType) + 2 * fDataNBytes;
            fRecordInterleavedBytes = new byte_type[ fInterleavedRecordNBytes ];
            fRecordInterleaved = new ( fRecordInterleavedBytes ) MonarchRecordBytes();

            fSeparateRecordNBytes = sizeof(AcquisitionIdType) + sizeof(RecordIdType) + sizeof(TimeType) + fDataNBytes;
            fRecordSeparateOneBytes = new byte_type[ fSeparateRecordNBytes ];
            fRecordSeparateOne = new ( fRecordSeparateOneBytes ) MonarchRecordBytes();
            fRecordSeparateTwoBytes = new byte_type[ fSeparateRecordNBytes ];
            fRecordSeparateTwo = new ( fRecordSeparateTwoBytes ) MonarchRecordBytes();

            //cout << "  *format is <" << sFormatMultiSeparate << ">" << endl;
            //cout << "  *data size is <" << fDataSize << "> and # of data bytes is <" << fDataNBytes << ">" << endl;
            //cout << "  *interleaved # of bytes is <" << fInterleavedRecordNBytes << ">" << endl;
            //cout << "  *separate # of bytes is <" << fSeparateRecordNBytes << ">" << endl;

            fWriteFunction = &Monarch::SeparateToSeparate;
        }
        else if( fHeader->GetAcquisitionMode() == 2 && fHeader->GetFormatMode() == sFormatMultiInterleaved )
        {
            fDataSize = fHeader->GetRecordSize();
            fDataNBytes = fDataSize * fDataTypeSize;

            fInterleavedRecordNBytes = sizeof(AcquisitionIdType) + sizeof(RecordIdType) + sizeof(TimeType) + 2 * fDataNBytes;
            fRecordInterleavedBytes = new byte_type[ fInterleavedRecordNBytes ];
            fRecordInterleaved = new ( fRecordInterleavedBytes ) MonarchRecordBytes();

            fSeparateRecordNBytes = sizeof(AcquisitionIdType) + sizeof(RecordIdType) + sizeof(TimeType) + fDataNBytes;
            fRecordSeparateOneBytes = new byte_type[ fSeparateRecordNBytes ];
            fRecordSeparateOne = new ( fRecordSeparateOneBytes ) MonarchRecordBytes();
            fRecordSeparateTwoBytes = new byte_type[ fSeparateRecordNBytes ];
            fRecordSeparateTwo = new ( fRecordSeparateTwoBytes ) MonarchRecordBytes();

            //cout << "  *format is <" << sFormatMultiInterleaved << ">" << endl;
            //cout << "  *data size is <" << fDataSize << "> and # of data bytes is <" << fDataNBytes << ">" << endl;
            //cout << "  *interleaved # of bytes is <" << fInterleavedRecordNBytes << ">" << endl;
            //cout << "  *separate # of bytes is <" << fSeparateRecordNBytes << ">" << endl;

            fWriteFunction = &Monarch::InterleavedToInterleaved;
        }
        else
        {
            throw MonarchException() << "unable to write a header with acquisition mode <" << fHeader->GetAcquisitionMode() << "> and format mode <" << fHeader->GetFormatMode() << ">";
            return;
        }

        fState = eReady;
        return;
    }

    void Monarch::SetInterface( InterfaceModeType aMode ) const
    {
        if( aMode == sInterfaceInterleaved )
        {
            if( fHeader->GetAcquisitionMode() == 1 /* the FormatMode is ignored for single-channel data */ )
            {
                fReadFunction = &Monarch::InterleavedFromSingle;
            }
            else if( fHeader->GetAcquisitionMode() == 2 && fHeader->GetFormatMode() == sFormatMultiInterleaved )
            {
                fReadFunction = &Monarch::InterleavedFromInterleaved;
            }
            else if( fHeader->GetAcquisitionMode() == 2 && fHeader->GetFormatMode() == sFormatMultiSeparate )
            {
                fReadFunction = &Monarch::InterleavedFromSeparate;
            }
        }
        if( aMode == sInterfaceSeparate )
        {
            if( fHeader->GetAcquisitionMode() == 1 /* the FormatMode is ignored for single-channel data */ )
            {
                fReadFunction = &Monarch::SeparateFromSingle;
            }
            else if( fHeader->GetAcquisitionMode() == 2 && fHeader->GetFormatMode() == sFormatMultiInterleaved )
            {
                fReadFunction = &Monarch::SeparateFromInterleaved;
            }
            else if( fHeader->GetAcquisitionMode() == 2 && fHeader->GetFormatMode() == sFormatMultiSeparate )
            {
                fReadFunction = &Monarch::SeparateFromSeparate;
            }
        }
        return;
    }

    void Monarch::SetInterface( InterfaceModeType aMode )
    {
        if( aMode == sInterfaceInterleaved )
        {
            if( fHeader->GetAcquisitionMode() == 1 /* the FormatMode is ignored for single-channel data */ )
            {
                fWriteFunction = &Monarch::InterleavedToSingle;
            }
            else if( fHeader->GetAcquisitionMode() == 2 && fHeader->GetFormatMode() == sFormatMultiInterleaved )
            {
                fWriteFunction = &Monarch::InterleavedToInterleaved;
            }
            else if( fHeader->GetAcquisitionMode() == 2 && fHeader->GetFormatMode() == sFormatMultiSeparate )
            {
                fWriteFunction = &Monarch::InterleavedToSeparate;
            }
        }
        if( aMode == sInterfaceSeparate )
        {
            if( fHeader->GetAcquisitionMode() == 1 /* the FormatMode is ignored for single-channel data */ )
            {
                fWriteFunction = &Monarch::SeparateToSingle;
            }
            else if( fHeader->GetAcquisitionMode() == 2 && fHeader->GetFormatMode() == sFormatMultiInterleaved )
            {
                fWriteFunction = &Monarch::SeparateToInterleaved;
            }
            else if( fHeader->GetAcquisitionMode() == 2 && fHeader->GetFormatMode() == sFormatMultiSeparate )
            {
                fWriteFunction = &Monarch::SeparateToSeparate;
            }
        }
        return;
    }

    bool Monarch::ReadRecord( int anOffset ) const
    {
        return (this->*fReadFunction)( anOffset );
    }

    bool Monarch::InterleavedFromSingle( int anOffset ) const
    {
        if( anOffset != 0 )
        {
            long int aByteOffset = anOffset * fInterleavedRecordNBytes;
            if( fIO->Seek( aByteOffset ) == false )
            {
                if( fIO->Done() != true )
                {
                    cout << "could not seek to requested position" << endl;
                }
                return false;
            }
        }

        if( fIO->Read( fRecordInterleavedBytes, fInterleavedRecordNBytes ) == false )
        {
            if( fIO->Done() != true )
            {
                throw MonarchException() << "could not read next single record";
            }
            return false;
        }

        return true;
    }

    bool Monarch::InterleavedFromSeparate( int anOffset ) const
    {
        if( anOffset != 0 )
        {
            long int aByteOffset = anOffset * 2 * fSeparateRecordNBytes;
            if( fIO->Seek( aByteOffset ) == false )
            {
                if( fIO->Done() != true )
                {
                    cout << "could not seek to requested position" << endl;
                }
                return false;
            }
        }

        if( fIO->Read( fRecordSeparateOneBytes, fSeparateRecordNBytes ) == false )
        {
            if( fIO->Done() != true )
            {
                throw MonarchException() << "could not read next channel one record";
            }
            return false;
        }

        if( fIO->Read( fRecordSeparateTwoBytes, fSeparateRecordNBytes ) == false )
        {
            if( fIO->Done() != true )
            {
                throw MonarchException() << "could not read next channel two record";
            }
            return false;
        }

        fRecordInterleaved->fTime = fRecordSeparateOne->fTime;
        fRecordInterleaved->fRecordId = fRecordSeparateOne->fRecordId;
        fRecordInterleaved->fTime = fRecordSeparateOne->fTime;
        Zip( fDataSize, fDataTypeSize, fRecordSeparateOne->fData, fRecordSeparateTwo->fData, fRecordInterleaved->fData );

        return true;
    }

    bool Monarch::InterleavedFromInterleaved( int anOffset ) const
    {
        if( anOffset != 0 )
        {
            long int aByteOffset = anOffset * fInterleavedRecordNBytes;
            if( fIO->Seek( aByteOffset ) == false )
            {
                if( fIO->Done() != true )
                {
                    cout << "could not seek to requested position" << endl;
                }
                return false;
            }
        }

        if( fIO->Read( fRecordInterleavedBytes, fInterleavedRecordNBytes ) == false )
        {
            if( fIO->Done() != true )
            {
                throw MonarchException() << "could not read next interleaved record";
            }
            return false;
        }

        return true;
    }

    bool Monarch::SeparateFromSingle( int anOffset ) const
    {
        if( anOffset != 0 )
        {
            long int aByteOffset = anOffset * fSeparateRecordNBytes;
            if( fIO->Seek( aByteOffset ) == false )
            {
                if( fIO->Done() != true )
                {
                    cout << "could not seek to requested position" << endl;
                }
                return false;
            }
        }

        if( fIO->Read( fRecordSeparateOneBytes, fSeparateRecordNBytes ) == false )
        {
            if( fIO->Done() != true )
            {
                throw MonarchException() << "could not read next channel one record";
            }
            return false;
        }

        return true;
    }

    bool Monarch::SeparateFromSeparate( int anOffset ) const
    {
        if( anOffset != 0 )
        {
            long int aByteOffset = anOffset * 2 * fSeparateRecordNBytes;
            if( fIO->Seek( aByteOffset ) == false )
            {
                if( fIO->Done() != true )
                {
                    cout << "could not seek to requested position" << endl;
                }
                return false;
            }
        }

        if( fIO->Read( fRecordSeparateOneBytes, fSeparateRecordNBytes ) == false )
        {
            if( fIO->Done() != true )
            {
                throw MonarchException() << "could not read next channel one record";
            }
            return false;
        }

        if( fIO->Read( fRecordSeparateTwoBytes, fSeparateRecordNBytes ) == false )
        {
            if( fIO->Done() != true )
            {
                throw MonarchException() << "could not read next channel two record";
            }
            return false;
        }

        return true;
    }

    bool Monarch::SeparateFromInterleaved( int anOffset ) const
    {
        if( anOffset != 0 )
        {
            long int aByteOffset = anOffset * fInterleavedRecordNBytes;
            if( fIO->Seek( aByteOffset ) == false )
            {
                if( fIO->Done() != true )
                {
                    cout << "could not seek to requested position" << endl;
                }
                return false;
            }
        }

        if( fIO->Read( fRecordInterleavedBytes, fInterleavedRecordNBytes ) == false )
        {
            if( fIO->Done() != true )
            {
                throw MonarchException() << "could not read next interleaved record";
            }
            return false;
        }

        fRecordSeparateOne->fTime = fRecordInterleaved->fTime;
        fRecordSeparateTwo->fTime = fRecordInterleaved->fTime;
        fRecordSeparateOne->fRecordId = fRecordInterleaved->fRecordId;
        fRecordSeparateTwo->fRecordId = fRecordInterleaved->fRecordId;
        fRecordSeparateOne->fTime = fRecordInterleaved->fTime;
        fRecordSeparateTwo->fTime = fRecordInterleaved->fTime;
        Unzip( fDataSize, fDataTypeSize, fRecordSeparateOne->fData, fRecordSeparateTwo->fData, fRecordInterleaved->fData );

        return true;
    }

    bool Monarch::WriteRecord()
    {
        return (this->*fWriteFunction)();
    }

    bool Monarch::InterleavedToSingle()
    {
        if( fIO->Write( fRecordInterleavedBytes, fInterleavedRecordNBytes ) == false )
        {
            throw MonarchException() << "could not write single record";
            return false;
        }

        return true;
    }

    bool Monarch::InterleavedToSeparate()
    {
        fRecordSeparateOne->fTime = fRecordInterleaved->fTime;
        fRecordSeparateTwo->fTime = fRecordInterleaved->fTime;
        fRecordSeparateOne->fRecordId = fRecordInterleaved->fRecordId;
        fRecordSeparateTwo->fRecordId = fRecordInterleaved->fRecordId;
        Unzip( fDataSize, fDataTypeSize, fRecordSeparateOne->fData, fRecordSeparateTwo->fData, fRecordInterleaved->fData );

        if( fIO->Write( fRecordSeparateOneBytes, fSeparateRecordNBytes ) == false )
        {
            throw MonarchException() << "could not write next channel one record";
            return false;
        }

        if( fIO->Write( fRecordSeparateTwoBytes, fSeparateRecordNBytes ) == false )
        {
            throw MonarchException() << "could not write next channel two record";
            return false;
        }

        return true;
    }

    bool Monarch::InterleavedToInterleaved()
    {
        if( fIO->Write( fRecordInterleavedBytes, fInterleavedRecordNBytes ) == false )
        {
            throw MonarchException() << "could not write interleaved record";
            return false;
        }

        return true;
    }

    bool Monarch::SeparateToSingle()
    {
        if( fIO->Write( fRecordSeparateOneBytes, fSeparateRecordNBytes ) == false )
        {
            throw MonarchException() << "could not write single record";
            return false;
        }

        return true;
    }

    bool Monarch::SeparateToSeparate()
    {
        if( fIO->Write( fRecordSeparateOneBytes, fSeparateRecordNBytes ) == false )
        {
            throw MonarchException() << "could not write next channel one record";
            return false;
        }

        if( fIO->Write( fRecordSeparateTwoBytes, fSeparateRecordNBytes ) == false )
        {
            throw MonarchException() << "could not write next channel two record";
            return false;
        }

        return true;
    }

    bool Monarch::SeparateToInterleaved()
    {
        fRecordInterleaved->fTime = fRecordSeparateOne->fTime;
        fRecordInterleaved->fRecordId = fRecordSeparateOne->fRecordId;
        fRecordInterleaved->fTime = fRecordSeparateOne->fTime;
        Zip( fDataSize, fDataTypeSize, fRecordSeparateOne->fData, fRecordSeparateTwo->fData, fRecordInterleaved->fData );

        if( fIO->Write( fRecordInterleavedBytes, fInterleavedRecordNBytes ) == false )
        {
            throw MonarchException() << "could not write interleaved record";
            return false;
        }

        return true;
    }

    void Monarch::Close() const
    {
        if( fIO->Close() == false )
        {
            throw MonarchException() << "could not close file";
        }
        return;
    }

    void Monarch::Close()
    {
        if( fIO->Close() == false )
        {
            throw MonarchException() << "could not close file";
        }
        return;
    }

}
