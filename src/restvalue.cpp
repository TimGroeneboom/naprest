#include "restvalue.h"

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::RestBaseValue)
    RTTI_PROPERTY("Name", &nap::RestBaseValue::mName, nap::rtti::EPropertyMetaData::Default)
    RTTI_PROPERTY("Required", &nap::RestBaseValue::mRequired, nap::rtti::EPropertyMetaData::Default)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::RestValueInt)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::RestValueFloat)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::RestValueString)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::RestValueBool)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::RestValueDouble)
RTTI_END_CLASS

RTTI_BEGIN_CLASS(nap::RestValueLong)
RTTI_END_CLASS