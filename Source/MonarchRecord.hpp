#ifndef MONARCHRECORD_HPP_
#define MONARCHRECORD_HPP_

#include "MonarchTypes.hpp"

namespace Monarch
{

    struct MonarchRecord
    {
            AcquisitionIdType fAcquisitionId;
            RecordIdType fRecordId;
            TimeType fTime;
            DataType fData[];
    };

}

#endif
