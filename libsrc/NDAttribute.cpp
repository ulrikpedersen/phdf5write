/** NDAttribute.cpp
 *
 * Mark Rivers
 * University of Chicago
 * October 18, 2013
 *
 */

#include <cstdlib>
#include <cstring>

#include "NDAttribute.h"

/** NDAttribute constructor
  * \param[in] pName The name of the attribute to be created. 
  * \param[in] sourceType The source type of the attribute (NDAttrSource_t).
  * \param[in] pSource The source string for the attribute.
  * \param[in] pDescription The description of the attribute.
  * \param[in] dataType The data type of the attribute (NDAttrDataType_t).
  * \param[in] pValue A pointer to the value for this attribute.
  */
  NDAttribute::NDAttribute(const std::string& pName, const std::string& pDescription,
                           NDAttrSource_t sourceType, const std::string& pSource,
                           NDAttrDataType_t dataType, void *pValue)
                           
  : dataType(NDAttrUndefined), sourceType(sourceType)
                           
{

  this->pName = pName;
  this->pDescription = pDescription;
  switch (sourceType) {
    case NDAttrSourceDriver:
      this->pSourceTypeString = "NDAttrSourceDriver";
      break;
    case NDAttrSourceEPICSPV:
      this->pSourceTypeString = "NDAttrSourceEPICSPV";
      break;
    case NDAttrSourceParam:
      this->pSourceTypeString = "NDAttrSourceParam";
      break;
    case NDAttrSourceFunct:
      this->pSourceTypeString = "NDAttrSourceFunct";
      break;
    default:
      this->pSourceTypeString = "Undefined";
  }
  this->pSource = pSource;
  this->pString = "";
  if (pValue) {
    this->setDataType(dataType);
    this->setValue(pValue);
  }
}

/** NDAttribute copy constructor
  * \param[in] attribute The attribute to copy from
  */
NDAttribute::NDAttribute(NDAttribute& attribute)
{
  const void *pValue;
  this->pName = attribute.pName;
  this->pDescription = attribute.pDescription;
  this->pSource = attribute.pSource;
  this->sourceType = attribute.sourceType;
  this->pSourceTypeString = attribute.pSourceTypeString;
  this->pString = "";
  this->dataType = attribute.dataType;
  if (attribute.dataType == NDAttrString) pValue = attribute.pString.c_str();
  else pValue = &attribute.value;
  this->setValue(pValue);
}


/** NDAttribute destructor 
  * Frees the strings for this attribute */
NDAttribute::~NDAttribute()
{
}

/** Copies properties from <b>this</b> to pOut.
  * \param[in] pOut A pointer to the output attribute
  *         If NULL the output attribute will be created using the copy constructor
  * Only the value is copied, all other fields are assumed to already be the same in pOut
  * \return  Returns a pointer to the copy
  */
NDAttribute* NDAttribute::copy(NDAttribute *pOut)
{
  const void *pValue;
  
  if (!pOut) 
    pOut = new NDAttribute(*this);
  else {
    if (this->dataType == NDAttrString) pValue = this->pString.c_str();
    else pValue = &this->value;
    pOut->setValue(pValue);
  }
  return pOut;
}

/** Returns the name of this attribute.
  */
const char *NDAttribute::getName()
{
  return pName.c_str();
}

/** Sets the data type of this attribute. This can only be called once.
  */
int NDAttribute::setDataType(NDAttrDataType_t type)
{
  // It is OK to set the data type to the same type as the existing type.
  // This will happen on channel access reconnects and if drivers create a parameter
  // and call this function every time.
  if (type == this->dataType) return ND_SUCCESS;
  if (this->dataType != NDAttrUndefined) {
    fprintf(stderr, "NDAttribute::setDataType, data type already defined = %d\n", this->dataType);
    return ND_ERROR;
  }
  if ((type < NDAttrInt8) || (type > NDAttrString)) {
    fprintf(stderr, "NDAttribute::setDataType, invalid data type = %d\n", type);
    return ND_ERROR;
  }
  this->dataType = type;
  return ND_SUCCESS;
}

/** Returns the data type of this attribute.
  */
NDAttrDataType_t NDAttribute::getDataType()
{
  return dataType;
}

/** Returns the description of this attribute.
  */
const char *NDAttribute::getDescription()
{
  return pDescription.c_str();
}

/** Returns the source string of this attribute.
  */
const char *NDAttribute::getSource()
{
  return pSource.c_str();
}

/** Returns the source information of this attribute.
  * \param[out] pSourceType Source type (NDAttrSource_t) of this attribute.
  * \return The source type string of this attribute
  */
const char *NDAttribute::getSourceInfo(NDAttrSource_t *pSourceType)
{
  *pSourceType = sourceType;
  return pSourceTypeString.c_str();
}

/** Sets the value for this attribute. 
  * \param[in] pValue Pointer to the value. */
int NDAttribute::setValue(const void *pValue)
{
  /* If any data type but undefined then pointer must be valid */
  if ((dataType != NDAttrUndefined) && !pValue) return ND_ERROR;

  /* Treat strings specially */
  if (dataType == NDAttrString) {
    /* If the previous value was the same string don't do anything, 
     * saves freeing and allocating memory.  
     * If not the same free the old string and copy new one. */
    if (this->pString != "") {
      if (this->pString == (char *)pValue) return ND_SUCCESS;
    }
    this->pString = (char *)pValue;
    return ND_SUCCESS;
  }
  if (this->pString != "") {
    this->pString = "";
  }
  switch (dataType) {
    case NDAttrInt8:
      this->value.i8 = *(int8_t *)pValue;
      break;
    case NDAttrUInt8:
      this->value.ui8 = *(uint8_t *)pValue;
      break;
    case NDAttrInt16:
      this->value.i16 = *(int16_t *)pValue;
      break;
    case NDAttrUInt16:
      this->value.ui16 = *(uint16_t *)pValue;
      break;
    case NDAttrInt32:
      this->value.i32 = *(int32_t*)pValue;
      break;
    case NDAttrUInt32:
      this->value.ui32 = *(uint32_t *)pValue;
      break;
    case NDAttrFloat32:
      this->value.f32 = *(float *)pValue;
      break;
    case NDAttrFloat64:
      this->value.f64 = *(double *)pValue;
      break;
    case NDAttrUndefined:
      break;
    default:
      return ND_ERROR;
      break;
  }
  return ND_SUCCESS;
}

/** Returns the data type and size of this attribute.
  * \param[out] pDataType Pointer to location to return the data type.
  * \param[out] pSize Pointer to location to return the data size; this is the
  *  data type size for all data types except NDAttrString, in which case it is the length of the
  * string including 0 terminator. */
int NDAttribute::getValueInfo(NDAttrDataType_t *pDataType, size_t *pSize)
{
  *pDataType = this->dataType;
  switch (this->dataType) {
    case NDAttrInt8:
      *pSize = sizeof(this->value.i8);
      break;
    case NDAttrUInt8:
      *pSize = sizeof(this->value.ui8);
      break;
    case NDAttrInt16:
      *pSize = sizeof(this->value.i16);
      break;
    case NDAttrUInt16:
      *pSize = sizeof(this->value.ui16);
      break;
    case NDAttrInt32:
      *pSize = sizeof(this->value.i32);
      break;
    case NDAttrUInt32:
      *pSize = sizeof(this->value.ui32);
      break;
    case NDAttrFloat32:
      *pSize = sizeof(this->value.f32);
      break;
    case NDAttrFloat64:
      *pSize = sizeof(this->value.f64);
      break;
    case NDAttrString:
      if (this->pString != "") *pSize = pString.size();
      else *pSize = 0;
      break;
    case NDAttrUndefined:
      *pSize = 0;
      break;
    default:
      return ND_ERROR;
      break;
  }
  return ND_SUCCESS;
}

/** Returns the value of this attribute.
  * \param[in] dataType Data type for the value.
  * \param[out] pValue Pointer to location to return the value.
  * \param[in] dataSize Size of the input data location; only used when dataType is NDAttrString.
  *
  * Currently the dataType parameter is only used to check that it matches the actual data type,
  * and ND_ERROR is returned if it does not.  In the future data type conversion may be added. */
int NDAttribute::getValue(NDAttrDataType_t dataType, void *pValue, size_t dataSize)
{
  if (dataType != this->dataType) return ND_ERROR;
  switch (this->dataType) {
    case NDAttrInt8:
      *(int8_t *)pValue = this->value.i8;
      break;
    case NDAttrUInt8:
       *(uint8_t *)pValue = this->value.ui8;
      break;
    case NDAttrInt16:
      *(int16_t *)pValue = this->value.i16;
      break;
    case NDAttrUInt16:
      *(uint16_t *)pValue = this->value.ui16;
      break;
    case NDAttrInt32:
      *(int32_t*)pValue = this->value.i32;
      break;
    case NDAttrUInt32:
      *(uint32_t *)pValue = this->value.ui32;
      break;
    case NDAttrFloat32:
      *(float *)pValue = this->value.f32;
      break;
    case NDAttrFloat64:
      *(double *)pValue = this->value.f64;
      break;
    case NDAttrString:
      if (this->pString != "") return (ND_ERROR);
      if (dataSize == 0) dataSize = this->pString.size();
      strncpy((char *)pValue, this->pString.c_str(), dataSize);
      break;
    case NDAttrUndefined:
    default:
      return ND_ERROR;
      break;
  }
  return ND_SUCCESS ;
}

/** Updates the current value of this attribute.
  * The base class does nothing, but derived classes may fetch the current value of the attribute,
  * for example from an EPICS PV or driver parameter library.
 */
int NDAttribute::updateValue()
{
  return ND_SUCCESS;
}

/** Reports on the properties of the attribute.
  * \param[in] fp File pointer for the report output.
  * \param[in] details Level of report details desired; currently does nothing
  */
int NDAttribute::report(FILE *fp, int details)
{
  
  fprintf(fp, "\n");
  fprintf(fp, "NDAttribute, address=%p:\n", this);
  fprintf(fp, "  name=%s\n", this->pName.c_str());
  fprintf(fp, "  description=%s\n", this->pDescription.c_str());
  fprintf(fp, "  source type=%s\n", this->pSourceTypeString.c_str());
  fprintf(fp, "  source=%s\n", this->pSource.c_str());
  switch (this->dataType) {
    case NDAttrInt8:
      fprintf(fp, "  dataType=NDAttrInt8\n");
      fprintf(fp, "  value=%d\n", this->value.i8);
      break;
    case NDAttrUInt8:
      fprintf(fp, "  dataType=NDAttrUInt8\n"); 
      fprintf(fp, "  value=%u\n", this->value.ui8);
      break;
    case NDAttrInt16:
      fprintf(fp, "  dataType=NDAttrInt16\n"); 
      fprintf(fp, "  value=%d\n", this->value.i16);
      break;
    case NDAttrUInt16:
      fprintf(fp, "  dataType=NDAttrUInt16\n"); 
      fprintf(fp, "  value=%d\n", this->value.ui16);
      break;
    case NDAttrInt32:
      fprintf(fp, "  dataType=NDAttrInt32\n"); 
      fprintf(fp, "  value=%d\n", this->value.i32);
      break;
    case NDAttrUInt32:
      fprintf(fp, "  dataType=NDAttrUInt32\n"); 
      fprintf(fp, "  value=%d\n", this->value.ui32);
      break;
    case NDAttrFloat32:
      fprintf(fp, "  dataType=NDAttrFloat32\n"); 
      fprintf(fp, "  value=%f\n", this->value.f32);
      break;
    case NDAttrFloat64:
      fprintf(fp, "  dataType=NDAttrFloat64\n"); 
      fprintf(fp, "  value=%f\n", this->value.f64);
      break;
    case NDAttrString:
      fprintf(fp, "  dataType=NDAttrString\n"); 
      fprintf(fp, "  value=%s\n", this->pString.c_str());
      break;
    case NDAttrUndefined:
      fprintf(fp, "  dataType=NDAttrUndefined\n");
      break;
    default:
      fprintf(fp, "  dataType=UNKNOWN\n");
      return ND_ERROR;
      break;
  }
  return ND_SUCCESS;
}


