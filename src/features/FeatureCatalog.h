#pragma once

#include "features/IFeature.h"

#include <vector>

namespace er {

const std::vector<FeatureInfo>& featureCatalog();
const char* groupTitleRu(FeatureGroup group);
const char* groupTitleEn(FeatureGroup group);
const char* hotkeySectionRu(bool ctrl, bool alt);

}  // namespace er
