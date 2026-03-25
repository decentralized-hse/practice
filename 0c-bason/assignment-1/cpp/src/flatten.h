#pragma once

#include "bason_codec.h"

#include <vector>

////////////////////////////////////////////////////////////////////////////////

namespace NBason {

////////////////////////////////////////////////////////////////////////////////

// Convert a nested BASON record into a sequence of flat path-keyed leaf records
std::vector<TBasonRecord> FlattenBason(const TBasonRecord& root);

// Convert a sequence of flat path-keyed leaf records into a nested BASON record tree
TBasonRecord UnflattenBason(const std::vector<TBasonRecord>& records);

////////////////////////////////////////////////////////////////////////////////

} // namespace NBason
