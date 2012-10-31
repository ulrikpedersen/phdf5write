/*
 * layoutxml.cpp
 *
 *  Created on: 26 Jan 2012
 *      Author: up45
 */

#include <cstdlib>
#include <algorithm>
#include <string>
#include <iostream>

using namespace std;

#include <libxml/xmlreader.h>
#include "layout.h"
#include "layoutxml.h"

#define XML_ATTR_ELEMENT_NAME "name"
#define XML_ATTR_GROUP "group"
#define XML_ATTR_DATASET "dataset"
#define XML_ATTR_ATTRIBUTE "attribute"

#define XML_ATTR_SOURCE "source"
#define XML_ATTR_SRC_DETECTOR "detector"
#define XML_ATTR_SRC_NDATTR "ndattribute"
#define XML_ATTR_SRC_CONST "constant"
#define XML_ATTR_SRC_CONST_VALUE "value"
#define XML_ATTR_SRC_CONST_TYPE "type"
#define XML_ATTR_GRP_NDATTR_DEFAULT "ndattr_default"

LayoutXML::LayoutXML() :
ptr_tree(NULL), ptr_curr_element(NULL)
{

    /* Initialize the libxml2 library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used. */
    LIBXML_TEST_VERSION
    this->xmlreader = NULL;
}

LayoutXML::~LayoutXML()
{
    delete this->ptr_tree;
}

int LayoutXML::load_xml(const char * filename)
{
    int ret = 0;

    this->xmlreader = xmlReaderForFile(filename, NULL, 0);
    if (this->xmlreader == NULL) {
        cerr << "Unable to open XML file: " << filename << endl;
        this->xmlreader = NULL;
        return -1;
    }

    cout << "Loading HDF5 layout XML file: " << filename << endl;
    while ( (ret = xmlTextReaderRead(this->xmlreader)) == 1) {
        this->process_node();
    }
    xmlFreeTextReader(this->xmlreader);
    if (ret != 0) {
        cerr << "Failed to parse XML file: "<< filename << endl;
    }

    // If no elements were created then we've failed...
    if (this->ptr_tree == NULL) return -1;

    //cout << "Tree shape: " << this->ptr_tree->_str_() << endl;
    return ret;
}

HdfRoot* LayoutXML::get_hdftree()
{
    return this->ptr_tree;
}

/** Process one XML node and create the necessary HDF5 element if necessary
 *
 */
void LayoutXML::process_node()
{
    xmlReaderTypes type = (xmlReaderTypes)xmlTextReaderNodeType(this->xmlreader);
    int ret = 0;
    const xmlChar* xmlname = NULL;
    string name;

    xmlname = xmlTextReaderConstName(this->xmlreader);
    if (xmlname == NULL) return;
    name.clear();
    name.append((const char*)xmlname);

    switch( type )
    {
        // Elements can be either 'group', 'dataset' or 'attribute'
        case XML_READER_TYPE_ELEMENT:
            //cout << "process_node: \'" << name << "\' (" << xmlname << ")" << endl;
            if ( name == XML_ATTR_GROUP )
                ret = this->new_group();
            else if ( name == XML_ATTR_DATASET )
                ret = this->new_dataset();
            else if ( name == XML_ATTR_ATTRIBUTE )
                ret = this->new_attribute();
            if (ret != 0)
                cerr << "Warning: adding new node: " << name << " failed..." << endl;
            break;

        // Parser callback at the end of an element.
        case XML_READER_TYPE_END_ELEMENT:
            if (this->ptr_tree == NULL) break;

            if (name == XML_ATTR_GROUP || name == XML_ATTR_DATASET)
            {
                //cout << "END ELEMENT name: " << name << " curr: " << this->ptr_curr_element->get_full_name() << endl;
                if (this->ptr_curr_element != NULL)
                    this->ptr_curr_element = this->ptr_curr_element->get_parent();

                if (this->ptr_curr_element == NULL)
                    this->ptr_curr_element = this->ptr_tree;
            }
            break;
        default:
            break;
    }
}

/** Process a datasets XML attributes
 * to work out the source of the attribute value: either detector data, NDAttribute or constant.
 */
int LayoutXML::process_dset_xml_attribute(HdfDataSource& out)
{
    int ret = -1;
    if (not xmlTextReaderHasAttributes(this->xmlreader) ) return ret;

    xmlChar *attr_src = NULL;
    string str_attr_src;

    attr_src = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar*)XML_ATTR_SOURCE);
    if (attr_src == NULL) return ret;
    str_attr_src = (char*)attr_src;

    if (str_attr_src == XML_ATTR_SRC_DETECTOR) {
        out = HdfDataSource( phdf_detector );
        ret = 0;
    }
    if (str_attr_src == XML_ATTR_SRC_NDATTR) {
        out = HdfDataSource( phdf_ndattribute );
        ret = 0;
    }
    if (str_attr_src == XML_ATTR_SRC_CONST) {
        out = HdfDataSource( phdf_constant );
        ret = 0;
    }

    return ret;
}

int LayoutXML::process_attribute_xml_attribute(HdfAttribute& out)
{
    int ret = -1;
    if (not xmlTextReaderHasAttributes(this->xmlreader) ) return ret;

    xmlChar *attr_src = NULL;
    string str_attr_src;

    attr_src = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar*)XML_ATTR_SOURCE);
    if (attr_src == NULL) return ret;
    str_attr_src = (char*)attr_src;

    string str_attr_val = "";
    xmlChar *attr_val = NULL;
    string str_attr_type = "";
    xmlChar *attr_type = NULL;

    // If the tag is: source="ndattribute"
    // Then setup the data source as a phdf_ndattribute with the name of the NDAttribute as argument value
    if (str_attr_src == XML_ATTR_SRC_NDATTR) {
    	attr_val = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar*)XML_ATTR_SRC_NDATTR);
    	if (attr_val != NULL) str_attr_val = (char*)attr_val;
    	out.source = HdfDataSource( phdf_ndattribute, str_attr_val );
    }
    // On the other hand if source="constant"
    // Then setup the data source as a phdf_constant with a string value.
    // Todo: a constant currently is only supported as a string type. This is
    // probably Good Enough (TM) for most use but should really be made work for
    // at least a int and float as well.
    else if (str_attr_src == XML_ATTR_SRC_CONST) {
    	attr_val = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar*)XML_ATTR_SRC_CONST_VALUE);
    	if (attr_val != NULL) str_attr_val = (char*)attr_val;
    	out.source = HdfDataSource( phdf_constant, str_attr_val );
    	attr_type = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar*)XML_ATTR_SRC_CONST_TYPE);
    	if (attr_type != NULL) {
    		str_attr_type = (char*)attr_type;
    		PHDF_DataType_t dtype = phdf_string;
    		if (str_attr_type == "int") dtype = phdf_int32;
    		else if (str_attr_type == "float") dtype = phdf_float64;
    		else if (str_attr_type == "string") dtype = phdf_string;
    		out.source.set_const_value(dtype, str_attr_val);
    	}
    }
    ret = 0;

    return ret;
}


int LayoutXML::new_group()
{
    // First check the basics
    if (not xmlTextReaderHasAttributes(this->xmlreader) ) return -1;

    xmlChar * group_name = NULL;
    group_name = xmlTextReaderGetAttribute(this->xmlreader,
                                           (const xmlChar *)XML_ATTR_ELEMENT_NAME);
    //cout << "  new_group: " << group_name << endl;
    if (group_name == NULL) return -1;

    string str_group_name((char*)group_name);
    free(group_name);

    // Initialise the tree if it has not already been done.
    if (this->ptr_tree == NULL) {
        this->ptr_tree = new HdfRoot(str_group_name);
        //cout << "  Initialised the root of the tree: " << *this->ptr_tree << endl;
        this->ptr_curr_element = this->ptr_tree;
    } else {
        HdfGroup *parent = static_cast<HdfGroup *>(this->ptr_curr_element);
        HdfGroup *new_group = NULL;
        new_group = parent->new_group(str_group_name);
        if (new_group == NULL) return -1;
        this->ptr_curr_element = new_group;
    }

    if (this->ptr_curr_element != NULL)
    {
    	// check whether this group is tagged as the default group to place
    	// NDAttribute meta-data in
    	xmlChar * ndattr_default;
    	ndattr_default = xmlTextReaderGetAttribute(this->xmlreader,
                                                  (const xmlChar *)XML_ATTR_GRP_NDATTR_DEFAULT);
    	if (ndattr_default != NULL)
    	{
        	string str_ndattr_default((char*)ndattr_default);
        	free(ndattr_default);
        	// if the group has tag: ndattr_default="true" (true in lower case)
        	// then set the group as the default container for NDAttributes.
        	if (str_ndattr_default == "true")
        	{
        		HdfGroup * new_group = static_cast<HdfGroup*>(this->ptr_curr_element);
        		new_group->set_default_ndattr_group();
        	}
    	}
    }
    return 0;
}

int LayoutXML::new_dataset()
{
    // First check the basics
    if (not xmlTextReaderHasAttributes(this->xmlreader) ) return -1;
    if (this->ptr_curr_element == NULL) return -1;

    xmlChar *dset_name = NULL;
    dset_name = xmlTextReaderGetAttribute(this->xmlreader,
                                          (const xmlChar *)XML_ATTR_ELEMENT_NAME);
    //cout << "  new_dataset: " << dset_name << endl;
    if (dset_name == NULL) return -1;

    string str_dset_name((char*)dset_name);

    HdfGroup *parent = (HdfGroup *)this->ptr_curr_element;
    HdfDataset *dset = NULL;
    dset = parent->new_dset(str_dset_name);
    if (dset == NULL) return -1;

    this->ptr_curr_element = dset;

    HdfDataSource attrval;
    this->process_dset_xml_attribute(attrval);
    dset->set_data_source(attrval);
    return 0;
}

int LayoutXML::new_attribute()
{
    int ret = 0;
    // First check the basics
    if (not xmlTextReaderHasAttributes(this->xmlreader) ) return -1;
    if (this->ptr_curr_element == NULL) return -1;

    xmlChar *ndattr_name = NULL;
    ndattr_name = xmlTextReaderGetAttribute(this->xmlreader,
                                            (const xmlChar*)XML_ATTR_ELEMENT_NAME);
    //cout << "  new_attribute: " << ndattr_name << " attached to: " << this->ptr_curr_element->get_full_name() << endl;
    if (ndattr_name == NULL) return -1;

    string str_ndattr_name((char*)ndattr_name);
    HdfAttribute ndattr(str_ndattr_name);
    this->process_attribute_xml_attribute(ndattr);

    ret = this->ptr_curr_element->add_attribute(ndattr);
    return ret;
}


