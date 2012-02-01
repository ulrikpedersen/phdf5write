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

LayoutXML::LayoutXML() :
ptr_tree(NULL), ptr_curr_element(NULL)
{

    /* Initialize the libxml2 library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used. */
    LIBXML_TEST_VERSION
    this->xmlreader = NULL;
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

HdfGroup* LayoutXML::get_hdftree()
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
            cout << "process_node: \'" << name << "\' (" << xmlname << ")" << endl;
            if ( name == XML_ATTR_GROUP )
                ret = this->new_group();
            else if ( name == XML_ATTR_DATASET )
                ret = this->new_dataset();
            else if ( name == XML_ATTR_ATTRIBUTE )
                ret = this->new_ndattribute();
            if (ret != 0)
                cerr << "Warning: adding new node: " << name << " failed..." << endl;
            break;

        // Parser callback at the end of an element.
        case XML_READER_TYPE_END_ELEMENT:
            if (this->ptr_tree == NULL) break;

            if (name == XML_ATTR_GROUP || name == XML_ATTR_DATASET)
            {
                cout << "END ELEMENT name: " << name << " curr: " << this->ptr_curr_element->get_full_name() << endl;
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

/** Process the XML element's attributes
 * to work out the source of the attribute value: either detector data, NDAttribute or constant.
 */
int LayoutXML::process_attribute(HdfAttrValue& out)
{
    int ret = -1;
    if (not xmlTextReaderHasAttributes(this->xmlreader) ) return ret;

    xmlChar *attr_src = NULL;
    string str_attr_src;

    attr_src = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar*)XML_ATTR_SOURCE);
    if (attr_src == NULL) return ret;
    str_attr_src = (char*)attr_src;

    if (str_attr_src == XML_ATTR_SRC_DETECTOR) {
        out = HdfAttrValue( detector );
        ret = 0;
    }
    if (str_attr_src == XML_ATTR_SRC_NDATTR) {
        out = HdfAttrValue( ndattribute );
        ret = 0;
    }
    if (str_attr_src == XML_ATTR_SRC_CONST) {
        out = HdfAttrValue( constant );
        ret = 0;
    }

    return ret;
}

int LayoutXML::new_group()
{
    // First check the basics
    if (not xmlTextReaderHasAttributes(this->xmlreader) ) return -1;

    xmlChar * group_name = NULL;
    group_name = xmlTextReaderGetAttribute(this->xmlreader,
                                           (const xmlChar *)XML_ATTR_ELEMENT_NAME);
    cout << "new_group: " << group_name << endl;
    if (group_name == NULL) return -1;

    string str_group_name((char*)group_name);
    free(group_name);

    // Initialise the tree if it has not already been done.
    if (this->ptr_tree == NULL) {
        this->ptr_tree = new HdfGroup(str_group_name);
        cout << "Initialised the root of the tree: " << *this->ptr_tree << endl;
        this->ptr_curr_element = this->ptr_tree;
    } else {
        HdfGroup *parent = (HdfGroup *)this->ptr_curr_element;
        HdfGroup *new_group = NULL;
        new_group = parent->new_group(str_group_name);
        if (new_group == NULL) return -1;
        this->ptr_curr_element = new_group;
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
    cout << "new_dataset: " << dset_name << endl;
    if (dset_name == NULL) return -1;

    string str_dset_name((char*)dset_name);

    HdfGroup *parent = (HdfGroup *)this->ptr_curr_element;
    HdfDataset *dset = NULL;
    dset = parent->new_dset(str_dset_name);
    if (dset == NULL) return -1;

    this->ptr_curr_element = dset;

    HdfAttrValue attrval;
    this->process_attribute(attrval);
    if (dset->set_data_source(attrval) < 0)
    {
        cerr << "Warning: could not set datasource on " << dset->get_full_name() << endl;
    }
    return 0;
}

int LayoutXML::new_ndattribute()
{
    int ret = 0;
    // First check the basics
    if (not xmlTextReaderHasAttributes(this->xmlreader) ) return -1;
    if (this->ptr_curr_element == NULL) return -1;

    xmlChar *ndattr_name = NULL;
    ndattr_name = xmlTextReaderGetAttribute(this->xmlreader,
                                            (const xmlChar*)XML_ATTR_ELEMENT_NAME);
    cout << "new_attribute: " << ndattr_name << " attached to: " << this->ptr_curr_element->get_full_name() << endl;
    if (ndattr_name == NULL) return -1;

    string str_ndattr_name((char*)ndattr_name);
    HdfAttribute ndattr(str_ndattr_name);
    this->process_attribute(ndattr.value);

    ret = this->ptr_curr_element->add_attribute(ndattr);
    return ret;
}

/**
 * processNode:
 * @reader: the xmlReader
 *
 * Dump information about the current node
 */
static void
processNode(xmlTextReaderPtr reader) {
    const xmlChar *name, *value;

    name = xmlTextReaderConstName(reader);
    if (name == NULL) {
        printf("name is NULL\n");
        //name = BAD_CAST "--";
    }

    value = xmlTextReaderConstValue(reader);

    printf("%d %d %s %d %d %d",
        xmlTextReaderDepth(reader),
        xmlTextReaderNodeType(reader),
        name,
        xmlTextReaderIsEmptyElement(reader),
        xmlTextReaderHasValue(reader),
        xmlTextReaderAttributeCount(reader));
    if (value == NULL)
        printf(" --no value--\n");
    else {
        if (xmlStrlen(value) > 40)
            printf(" %.40s...\n", value);
        else
            printf("[[[%s]]]\n", value);
    }
    if (xmlTextReaderHasAttributes(reader))
    {
        int ret = xmlTextReaderMoveToFirstAttribute(reader);
        while( ret > 0 )
        {
            printf("  attr: %s %s\n", xmlTextReaderConstName(reader), xmlTextReaderConstValue(reader));
            ret = xmlTextReaderMoveToNextAttribute(reader);
        }
    }
}

/**
 * streamFile:
 * @filename: the file name to parse
 *
 * Parse and print information about an XML file.
 */
static void
streamFile(const char *filename) {
    xmlTextReaderPtr reader;
    int ret;

    reader = xmlReaderForFile(filename, NULL, 0);
    if (reader != NULL) {
        ret = xmlTextReaderRead(reader);
        while (ret == 1) {
            processNode(reader);
            ret = xmlTextReaderRead(reader);
        }
        xmlFreeTextReader(reader);
        if (ret != 0) {
            fprintf(stderr, "%s : failed to parse\n", filename);
        }
    } else {
        fprintf(stderr, "Unable to open %s\n", filename);
    }
}

int main_xml(const char *fname) {

    /*
     * this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     */
    LIBXML_TEST_VERSION

    streamFile(fname);

    /*
     * Cleanup function for the XML library.
     */
    xmlCleanupParser();
    /*
     * this is to debug memory for regression tests
     */
    xmlMemoryDump();
    return(0);
}

