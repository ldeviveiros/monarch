/*
 * M3Stream.cc
 *
 *  Created on: Dec 26, 2014
 *      Author: nsoblath
 */

#include "M3Stream.hh"

#include "M3IToA.hh"
#include "M3Logger.hh"

/* Notes on reading:
 *
 * During reading, both fNRecordsInAcq and fRecordsCount are used, as well as both fAcquisitionId and fNAcquisitions;
 * All four variables will always be valid.
 *
 */

/* Notes on writing:
 *
 * As writing progresses, fRecordCountInAcq is incremented, and fNRecordsInAcq is ignored.
 * When the acquisition is finalized with FinalizeCurrentAcq(), fNRecordsInAcq is updated to be fRecordCountInAcq + 1.
 * Therefore fRecordCountInAcq is only valid for the last completed acquisition.
 *
 * As writing progresses, fAcquisitionId is incremented and fNAcquisitions is ignored.
 * When the stream is finalized with FinalizeStream(), fNAcquisitions is updated to be fAcquisitionId + 1.
 * Therefore fNAcquisitions is only valid after the stream is finalized.
 *
 */

namespace monarch3
{
    M3LOGGER( mlog, "M3Stream" );

    M3Stream::M3Stream( const M3StreamHeader& aHeader, H5::CommonFG* aH5StreamsLoc, MultiChannelFormatType aAccessFormat ) :
            fMode( kRead ),
            fAcquisitionId( 0 ),
            fNAcquisitions( 0 ),
            fIsInitialized( false ),
            fRecordsAccessed( false ),
            fDataTypeSize( aHeader.GetDataTypeSize() ),
            fStrRecNBytes( aHeader.GetNChannels() * aHeader.GetRecordSize() * aHeader.GetDataTypeSize() ),
            fStrRecSize( aHeader.GetNChannels() * aHeader.GetRecordSize() ),
            fChanRecNBytes( aHeader.GetRecordSize() * aHeader.GetDataTypeSize() ),
            fChanRecSize( aHeader.GetRecordSize() ),
            fStreamRecord(),
            fNChannels( aHeader.GetNChannels() ),
            fChannelRecords( new M3Record[ aHeader.GetNChannels() ] ),
            fRecordCountInAcq( 0 ),
            fNRecordsInAcq( 0 ),
            fDataInterleaved( aHeader.GetChannelFormat() == sInterleaved ),
            fAccessFormat( aAccessFormat ),
            fRecordIndex(),
            fNRecordsInFile( 0 ),
            fRecordCountInFile( 0 ),
            fDoReadRecord( NULL ),
            fDoWriteRecord( NULL ),
            fH5StreamParentLoc( new H5::Group( aH5StreamsLoc->openGroup( aHeader.GetLabel() ) ) ),
            fH5AcqLoc( NULL ),
            fH5CurrentAcqDataSet( NULL ),
            fH5DataSpaceUser( NULL )
    {
        M3DEBUG( mlog, "Creating stream for <" << aHeader.GetLabel() << ">" );

        if( aHeader.GetDataFormat() == sDigitized )
        {
            switch( fDataTypeSize )
            {
                case 1:
                    fDataTypeInFile = H5::PredType::STD_U8LE;
                    fDataTypeUser = H5::PredType::NATIVE_UINT8;
                   break;
                case 2:
                    fDataTypeInFile = H5::PredType::STD_U16LE;
                    fDataTypeUser = H5::PredType::NATIVE_UINT16;
                    break;
                case 4:
                    fDataTypeInFile = H5::PredType::STD_U32LE;
                    fDataTypeUser = H5::PredType::NATIVE_UINT32;
                    break;
                case 8:
                    fDataTypeInFile = H5::PredType::STD_U64LE;
                    fDataTypeUser = H5::PredType::NATIVE_UINT64;
                    break;
                default:
                    throw M3Exception() << "Unknown integer data type size: " << fDataTypeSize;
            }
        }
        else // aHeader.GetDataFormat() == sAnalog
        {
            switch( fDataTypeSize )
            {
                case 4:
                    fDataTypeInFile = H5::PredType::IEEE_F32LE;
                    fDataTypeUser = H5::PredType::NATIVE_FLOAT;
                    break;
                case 8:
                    fDataTypeInFile = H5::PredType::IEEE_F64LE ;
                    fDataTypeUser = H5::PredType::NATIVE_DOUBLE;
                    break;
                default:
                    throw M3Exception() << "Unknown floating-point data type size: " << fDataTypeSize;
            }
        }

        // Determine if we're in read or write mode
        // and get/create the acquisitions group
        // Nested exceptions are used so that the outer try block can be used to determine whether we're reading or writing
        try
        {
            fH5AcqLoc = new H5::Group( fH5StreamParentLoc->openGroup( "acquisitions" ) );
            M3DEBUG( mlog, "Opened acquisition group in <read> mode" );
            try
            {
                H5::Attribute tAttrNAcq( fH5StreamParentLoc->openAttribute( "n_acquisitions" ) );
                tAttrNAcq.read( tAttrNAcq.getDataType(), &fNAcquisitions );
                H5::Attribute tAttrNRec( fH5StreamParentLoc->openAttribute( "n_records" ) );
                tAttrNRec.read( tAttrNRec.getDataType(), &fNRecordsInFile );
                BuildIndex();
            }
            catch( H5::Exception& e2 )
            {
                throw;
            }
            M3DEBUG( mlog, "\tNumber of acquisitions found: " << fNAcquisitions << "; Number of records found: " << fNRecordsInFile );
            fMode = kRead;
        }
        catch( H5::Exception& e1 )
        {
            // if we ended up here, the acquisitions group doesn't exist, so we must be in write mode
            try
            {
                fH5AcqLoc = new H5::Group( fH5StreamParentLoc->createGroup( "acquisitions" ) );
                M3DEBUG( mlog, "Opened acquisition group in <write> mode" );
                fMode = kWrite;
            }
            catch( H5::Exception& e2 )
            {
                throw;
            }
        }

        Initialize();
    }

    M3Stream::~M3Stream()
    {
        delete fH5DataSpaceUser; fH5DataSpaceUser = NULL;
        delete fH5CurrentAcqDataSet; fH5CurrentAcqDataSet = NULL;
        delete fH5AcqLoc; fH5AcqLoc = NULL;
        delete fH5StreamParentLoc; fH5StreamParentLoc = NULL;

        delete [] fChannelRecords;
    }

    void M3Stream::Initialize() const
    {
        M3DEBUG( mlog, "Initializing stream" );
        fIsInitialized = false;

        // The case where the access format is separate, but the data in the file is interleaved is special.
        // In this case, the stream record memory is not used.
        // Reading and writing is done directly from the channel records using HDF5's interleaving capabilities.
        if( fAccessFormat == sSeparate && fDataInterleaved && fNChannels != 1 )
        {
            // no memory is allocated for the stream record
            fStreamRecord.SetData();

            // allocate memory for each channel record
            for( unsigned iChan = 0; iChan < fNChannels; ++iChan )
            {
                fChannelRecords[ iChan ].SetData( fChanRecNBytes );
            }

            // set the read/write functions to the special versions
            fDoReadRecord = &M3Stream::ReadRecordInterleavedToSeparate;
            fDoWriteRecord = &M3Stream::WriteRecordSeparateToInterleaved;

            // Arrays for HDF5 file reading/writing
            fStrDataDims[ 0 ] = 1;                 fStrDataDims[ 1 ] = fStrRecSize;
            fStrMaxDataDims[ 0 ] = H5S_UNLIMITED;  fStrMaxDataDims[ 1 ] = fStrRecSize;
            fStrDataChunkDims[ 0 ] = 1;            fStrDataChunkDims[ 1 ] = fStrRecSize;
            fDataDims1Rec[ 0 ] = 1;             fDataDims1Rec[ 1 ] = fChanRecSize;
            fDataOffset[ 0 ] = 0;               fDataOffset[ 1 ] = 0;
            fDataStride[ 0 ] = 1;               fDataStride[ 1 ] = 2;
            fDataBlock[ 0 ] = 1;                fDataBlock[ 1 ] = 1;

            // HDF5 object initialization
            delete fH5DataSpaceUser;
            fH5DataSpaceUser = new H5::DataSpace( N_DATA_DIMS, fDataDims1Rec, NULL );

            fIsInitialized = true;
            return;
        }

        // allocate stream record memory
        fStreamRecord.SetData( fStrRecNBytes );

        // channel records point to portions of the stream record and do not own their own data
        byte_type* tChanDataPtr = fStreamRecord.GetData();
        for( unsigned iChan = 0; iChan < fNChannels; ++iChan )
        {
            fChannelRecords[ iChan ].SetData( tChanDataPtr + fChanRecNBytes*iChan );
        }

        // set the read/write functions to the general versions
        fDoReadRecord = &M3Stream::ReadRecordAsIs;
        fDoWriteRecord = &M3Stream::WriteRecordAsIs;

        // Arrays for HDF5 file reading/writing
        fStrDataDims[ 0 ] = 1;                 fStrDataDims[ 1 ] = fStrRecSize;
        fStrMaxDataDims[ 0 ] = H5S_UNLIMITED;  fStrMaxDataDims[ 1 ] = fStrRecSize;
        fStrDataChunkDims[ 0 ] = 1;            fStrDataChunkDims[ 1 ] = fStrRecSize;
        fDataDims1Rec[ 0 ] = 1;             fDataDims1Rec[ 1 ] = fStrRecSize;
        fDataOffset[ 0 ] = 0;               fDataOffset[ 1 ] = 0;
        fDataStride[ 0 ] = 0;               fDataStride[ 1 ] = 0;
        fDataBlock[ 0 ] = 1;                fDataBlock[ 1 ] = fStrRecSize;

        // HDF5 object initialization
        delete fH5DataSpaceUser;
        fH5DataSpaceUser = new H5::DataSpace( N_DATA_DIMS, fDataDims1Rec, NULL );


        fIsInitialized = true;
        return;
    }

    const M3Record* M3Stream::GetStreamRecord() const
    {
        return &fStreamRecord;
    }

    const M3Record* M3Stream::GetChannelRecord( unsigned aChannel ) const
    {
        if( aChannel < fNChannels )
        {
            return &(fChannelRecords[ aChannel ]);
        }
        throw M3Exception() << "Channel <" << aChannel << "> requested; only " << fNChannels << " in this stream.";
    }

    bool M3Stream::ReadRecord( int anOffset ) const
    {
        if( ! fIsInitialized ) Initialize();

        anOffset += fRecordsAccessed;
        if( ( anOffset < 0 && abs( anOffset ) > fRecordCountInFile ) ||
            ( anOffset > 0 && fRecordCountInFile + anOffset >= fNRecordsInFile ) )
        {
            // either requested to go back before the beginning of the file, or past the end
            return false;
        }

        fRecordCountInFile = fRecordCountInFile + anOffset;
        unsigned nextAcq = fRecordIndex.at( fRecordCountInFile ).first;
        fRecordCountInAcq = fRecordIndex.at( fRecordCountInFile ).second;
        M3DEBUG( mlog, "Going to record " << fRecordCountInFile << " -- " << nextAcq << " -- " << fRecordCountInAcq );

        if( nextAcq != fAcquisitionId || ! fRecordsAccessed )
        {
            // go to a new acquisition
            fRecordsAccessed = true;
            fAcquisitionId = nextAcq;
            delete fH5CurrentAcqDataSet;
            u32toa( fAcquisitionId, fAcqNameBuffer );
            fH5CurrentAcqDataSet = new H5::DataSet( fH5AcqLoc->openDataSet( fAcqNameBuffer ) );
            H5::Attribute tAttr( fH5CurrentAcqDataSet->openAttribute( "n_records" ) );
            tAttr.read( tAttr.getDataType(), &fNRecordsInAcq );
        }

        fDataOffset[ 0 ] = fRecordCountInAcq;

        (this->*fDoReadRecord)();

        return true;
    }

    void M3Stream::Close() const
    {
        M3DEBUG( mlog, "const M3Stream::Close()" );

        delete fH5DataSpaceUser; fH5DataSpaceUser = NULL;
        delete fH5CurrentAcqDataSet; fH5CurrentAcqDataSet = NULL;
        delete fH5AcqLoc; fH5AcqLoc = NULL;
        delete fH5StreamParentLoc; fH5StreamParentLoc = NULL;

        return;
    }


    M3Record* M3Stream::GetStreamRecord()
    {
        return &fStreamRecord;
    }

    M3Record* M3Stream::GetChannelRecord( unsigned aChannel )
    {
        if( aChannel < fNChannels )
        {
            return &(fChannelRecords[ aChannel ]);
        }
        throw M3Exception() << "Channel <" << aChannel << "> requested; only " << fNChannels << " in this stream.";
    }

    bool M3Stream::WriteRecord( bool aIsNewAcquisition )
    {
        // note: fRecordCountInAcq is used to keep track of the number of records written in each acquisition;
        //       fNRecordsInAcq is only valid for the last completed acquisition.

        if( ! fIsInitialized ) Initialize();

        try
        {
            if( aIsNewAcquisition )
            {
                FinalizeCurrentAcq();

                if( fRecordsAccessed ) ++fAcquisitionId;
                else fRecordsAccessed = true;

                // Setup the new dataset
                fStrDataDims[ 0 ] = 1;
                H5::DSetCreatPropList tPropList;
                tPropList.setChunk( N_DATA_DIMS, fStrDataChunkDims );

                u32toa( fAcquisitionId, fAcqNameBuffer );
                fH5CurrentAcqDataSet = new H5::DataSet( fH5AcqLoc->createDataSet( fAcqNameBuffer, fDataTypeInFile, H5::DataSpace( N_DATA_DIMS, fStrDataDims, fStrMaxDataDims ), tPropList ) );
            }
            else
            {
                // Extend the current dataset
                fStrDataDims[ 0 ] = fStrDataDims[ 0 ] + 1;
                fH5CurrentAcqDataSet->extend( fStrDataDims );
            }

            M3DEBUG( mlog, "Writing acq. " << fAcquisitionId << ", record " << fRecordCountInAcq );

            fDataOffset[ 0 ] = fRecordCountInAcq;

            (this->*fDoWriteRecord)();

            ++fRecordCountInAcq;
            ++fRecordCountInFile;
            return true;
        }
        catch( H5::Exception& e )
        {
            M3ERROR( mlog, "HDF5 error while writing a record:\n\t" << e.getCDetailMsg() << " (function: " << e.getFuncName() << ")" );
        }
        catch( M3Exception& e )
        {
            M3ERROR( mlog, e.what() );
        }

        return false;
    }

    void M3Stream::Close()
    {
        M3DEBUG( mlog, "non-const M3Stream::Close()" );
        FinalizeStream();

        delete fH5DataSpaceUser; fH5DataSpaceUser = NULL;
        delete fH5CurrentAcqDataSet; fH5CurrentAcqDataSet = NULL;
        delete fH5AcqLoc; fH5AcqLoc = NULL;
        delete fH5StreamParentLoc; fH5StreamParentLoc = NULL;

        return;
    }

    void M3Stream::SetAccessFormat( MultiChannelFormatType aFormat ) const
    {
        fAccessFormat = aFormat;
        fIsInitialized = false;
        return;
    }

    void M3Stream::ReadRecordInterleavedToSeparate() const
    {
        H5::DataSpace tDataSpaceInFile = fH5CurrentAcqDataSet->getSpace();
        for( unsigned iChan = 0; iChan < fNChannels; ++iChan )
        {
            fDataOffset[ 1 ] = iChan;
            tDataSpaceInFile.selectHyperslab( H5S_SELECT_SET, fDataDims1Rec, fDataOffset, fDataStride, fDataBlock );
            fH5CurrentAcqDataSet->read( fChannelRecords[ iChan ].GetData(), fDataTypeUser, *fH5DataSpaceUser, tDataSpaceInFile );
        }
        return;
    }

    void M3Stream::ReadRecordAsIs() const
    {
        H5::DataSpace tDataSpaceInFile = fH5CurrentAcqDataSet->getSpace();
        tDataSpaceInFile.selectHyperslab( H5S_SELECT_SET, fDataDims1Rec, fDataOffset );
        fH5CurrentAcqDataSet->read( fStreamRecord.GetData(), fDataTypeUser, *fH5DataSpaceUser, tDataSpaceInFile );
        return;
    }

    void M3Stream::WriteRecordSeparateToInterleaved()
    {
        H5::DataSpace tDataSpaceInFile = fH5CurrentAcqDataSet->getSpace();
        for( unsigned iChan = 0; iChan < fNChannels; ++iChan )
        {
            fDataOffset[ 1 ] = iChan;
            tDataSpaceInFile.selectHyperslab( H5S_SELECT_SET, fDataDims1Rec, fDataOffset, fDataStride, fDataBlock );
            fH5CurrentAcqDataSet->write( fChannelRecords[ iChan ].GetData(), fDataTypeUser, *fH5DataSpaceUser, tDataSpaceInFile );
        }
        return;
    }

    void M3Stream::WriteRecordAsIs()
    {
        H5::DataSpace tDataSpaceInFile = fH5CurrentAcqDataSet->getSpace();
        tDataSpaceInFile.selectHyperslab( H5S_SELECT_SET, fDataDims1Rec, fDataOffset );
        fH5CurrentAcqDataSet->write( fStreamRecord.GetData(), fDataTypeUser, *fH5DataSpaceUser, tDataSpaceInFile );
        return;
    }

    void M3Stream::BuildIndex() const
    {
        fRecordIndex.resize( fNRecordsInFile );
        unsigned tNRecInAcq;
        unsigned iRecInFile = 0;
        for( unsigned iAcq = 0; iAcq < fNAcquisitions; ++iAcq )
        {
            u32toa( iAcq, fAcqNameBuffer );
            H5::Attribute tAttr( fH5AcqLoc->openDataSet( fAcqNameBuffer ).openAttribute( "n_records" ) );
            tAttr.read( tAttr.getDataType(), &tNRecInAcq );
            M3DEBUG( mlog, "Acquisition <" << fAcqNameBuffer << "> has " << tNRecInAcq << " records" );
            for( unsigned iRecInAcq = 0; iRecInAcq < tNRecInAcq; ++iRecInAcq )
            {
                fRecordIndex.at( iRecInFile ).first = iAcq;
                fRecordIndex.at( iRecInFile ).second = iRecInAcq;
                M3DEBUG( mlog, "Record index: " << iRecInFile << " -- " << iAcq << " -- " << iRecInAcq );
                ++iRecInFile;
            }
        }
        return;
    }

    void M3Stream::FinalizeCurrentAcq()
    {
        if( fH5CurrentAcqDataSet == NULL ) return;

        fNRecordsInAcq = fRecordCountInAcq;

        fH5CurrentAcqDataSet->createAttribute( "n_records", MH5Type< unsigned >::H5(), H5::DataSpace( H5S_SCALAR ) ).write( MH5Type< unsigned >::Native(), &fNRecordsInAcq );
        M3DEBUG( mlog, "Finalizing acq. " << fAcquisitionId << " with " << fNRecordsInAcq << " records" );

        fRecordCountInAcq = 0;
        delete fH5CurrentAcqDataSet;
        fH5CurrentAcqDataSet = NULL;

        return;
    }

    void M3Stream::FinalizeStream()
    {
        FinalizeCurrentAcq();

        if( fH5AcqLoc == NULL ) return;

        fNAcquisitions = ( fAcquisitionId + 1 ) * (unsigned)fRecordsAccessed;
        fH5StreamParentLoc->openAttribute( "n_acquisitions" ).write( MH5Type< unsigned >::Native(), &fNAcquisitions );
        fH5StreamParentLoc->openAttribute( "n_records" ).write( MH5Type< unsigned >::Native(), &fRecordCountInFile );
        M3DEBUG( mlog, "Finalizing stream with " << fNAcquisitions << " acquisitions and " << fRecordCountInFile << " records" );

        return;
    }

} /* namespace monarch */