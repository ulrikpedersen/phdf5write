/** NDAttribute.h
 *
 * Mark Rivers
 * University of Chicago
 * October 18, 2013
 *
 */

#ifndef NDAttribute_H
#define NDAttribute_H

#include <cstdio>
#include <cstring>
#include <stdint.h>
#include <string>

#define MAX_ATTRIBUTE_STRING_SIZE 256

/** Success return code  */
#define ND_SUCCESS 0
/** Failure return code  */
#define ND_ERROR -1


/** Enumeration of NDArray data types */
typedef enum
{
    NDInt8,     /**< Signed 8-bit integer */
    NDUInt8,    /**< Unsigned 8-bit integer */
    NDInt16,    /**< Signed 16-bit integer */
    NDUInt16,   /**< Unsigned 16-bit integer */
    NDInt32,    /**< Signed 32-bit integer */
    NDUInt32,   /**< Unsigned 32-bit integer */
    NDFloat32,  /**< 32-bit float */
    NDFloat64   /**< 64-bit float */
} NDDataType_t;

/** Enumeration of NDAttribute attribute data types */
typedef enum
{
    NDAttrInt8    = NDInt8,     /**< Signed 8-bit integer */
    NDAttrUInt8   = NDUInt8,    /**< Unsigned 8-bit integer */
    NDAttrInt16   = NDInt16,    /**< Signed 16-bit integer */
    NDAttrUInt16  = NDUInt16,   /**< Unsigned 16-bit integer */
    NDAttrInt32   = NDInt32,    /**< Signed 32-bit integer */
    NDAttrUInt32  = NDUInt32,   /**< Unsigned 32-bit integer */
    NDAttrFloat32 = NDFloat32,  /**< 32-bit float */
    NDAttrFloat64 = NDFloat64,  /**< 64-bit float */
    NDAttrString,               /**< Dynamic length string */
    NDAttrUndefined             /**< Undefined data type */
} NDAttrDataType_t;

/** Enumeration of NDAttibute source types */
typedef enum
{
    NDAttrSourceDriver,    /**< Attribute is obtained directly from driver */
    NDAttrSourceParam,     /**< Attribute is obtained from parameter library */
    NDAttrSourceEPICSPV,   /**< Attribute is obtained from an EPICS PV */
    NDAttrSourceFunct      /**< Attribute is obtained from a user-specified function */
} NDAttrSource_t;

/** Union defining the values in an NDAttribute object */
typedef union {
    int8_t    i8;    /**< Signed 8-bit integer */
    uint8_t   ui8;   /**< Unsigned 8-bit integer */
    int16_t   i16;   /**< Signed 16-bit integer */
    uint16_t  ui16;  /**< Unsigned 16-bit integer */
    int32_t   i32;   /**< Signed 32-bit integer */
    uint32_t  ui32;  /**< Unsigned 32-bit integer */
    float f32;   /**< 32-bit float */
    double f64;   /**< 64-bit float */
} NDAttrValue;

/** NDAttribute class; an attribute has a name, description, source type, source string,
  * data type, and value.
  */
class NDAttribute {
public:
    /* Methods */
	NDAttribute(const std::string& pName, const std::string& pDescription,
	            NDAttrSource_t sourceType, const std::string& pSource,
	            NDAttrDataType_t dataType, void *pValue);
    NDAttribute(NDAttribute& attribute);
    virtual ~NDAttribute();
    virtual NDAttribute* copy(NDAttribute *pAttribute);
    virtual const char *getName();
    virtual const char *getDescription();
    virtual const char *getSource();
    virtual const char *getSourceInfo(NDAttrSource_t *pSourceType);
    virtual NDAttrDataType_t getDataType();
    virtual int getValueInfo(NDAttrDataType_t *pDataType, size_t *pDataSize);
    virtual int getValue(NDAttrDataType_t dataType, void *pValue, size_t dataSize=0);
    virtual int setDataType(NDAttrDataType_t dataType);
    virtual int setValue(const void *pValue);
    virtual int updateValue();
    virtual int report(FILE *fp, int details);
    friend class NDArray;

private:
    std::string pName;                   /**< Name string */
    std::string pDescription;            /**< Description string */
    NDAttrDataType_t dataType;     /**< Data type of attribute */
    NDAttrValue value;             /**< Value of attribute except for strings */
    std::string pString;                 /**< Value of attribute for strings */
    std::string pSource;                 /**< Source string - EPICS PV name or DRV_INFO string */
    NDAttrSource_t sourceType;     /**< Source type */
    std::string pSourceTypeString;       /**< Source type string */
};

#endif
