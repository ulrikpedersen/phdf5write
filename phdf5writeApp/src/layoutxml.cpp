/*
 * layoutxml.cpp
 *
 *  Created on: 26 Jan 2012
 *      Author: up45
 */

/**
 * section: xmlReader
 * synopsis: Parse an XML file with an xmlReader
 * purpose: Demonstrate the use of xmlReaderForFile() to parse an XML file
 *          and dump the informations about the nodes found in the process.
 *          (Note that the XMLReader functions require libxml2 version later
 *          than 2.6.)
 * usage: reader1 <filename>
 * test: reader1 test2.xml > reader1.tmp ; diff reader1.tmp reader1.res ; rm reader1.tmp
 * author: Daniel Veillard
 * copy: see Copyright for the status of this software.
 */

//#include <cctype>
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
    int ret;

    this->xmlreader = xmlReaderForFile(filename, NULL, 0);
    if (this->xmlreader == NULL) {
        cerr << "Unable to open XML file: " << filename << endl;
        this->xmlreader = NULL;
        return -1;
    }

    //ret = xmlTextReaderRead(this->xmlreader);
    while ( (ret = xmlTextReaderRead(this->xmlreader)) == 1) {
        this->process_node();
        //ret = xmlTextReaderRead(this->xmlreader);
    }
    xmlFreeTextReader(this->xmlreader);
    if (ret != 0) {
        cerr << "Failed to parse XML file: "<< filename << endl;
    }

    // If no elements were created then we've failed...
    if (this->ptr_curr_element == NULL) return -1;

    cout << "Tree shape: " << this->ptr_tree->_str_();
    return ret;
}

int LayoutXML::process_node()
{
    xmlReaderTypes type = (xmlReaderTypes)xmlTextReaderNodeType(this->xmlreader);
    int ret = 0;
    const xmlChar* xmlname = NULL;
    string name;
    switch( type )
    {
        case XML_READER_TYPE_ELEMENT:
            xmlname = xmlTextReaderConstName(this->xmlreader);
            if (xmlname == NULL) break;
            name.clear();
            name.append((const char*)xmlname);
            cout << "process_node: " << name << endl;
            if ( name == XML_ATTR_GROUP )
                ret = this->new_group();
            else if ( name == XML_ATTR_DATASET )
                ret = this->new_dataset();
            else if ( name == XML_ATTR_ATTRIBUTE )
                ret = this->new_ndattribute();
            break;
        case XML_READER_TYPE_END_ELEMENT:
            if (this->ptr_curr_element == NULL)
                this->ptr_curr_element = this->ptr_tree;
            else
                this->ptr_curr_element = this->ptr_curr_element->get_parent();
            break;
        default:
            break;
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
    }

    HdfGroup *parent = (HdfGroup *)this->ptr_curr_element;
    HdfGroup *new_group = NULL;
    new_group = parent->new_group(str_group_name);
    if (new_group == NULL) return -1;

    this->ptr_curr_element = new_group;
    return 0;
}

int LayoutXML::new_dataset()
{
    // First check the basics
    if (not xmlTextReaderHasAttributes(this->xmlreader) ) return -1;
    if (this->ptr_curr_element == NULL) return -1;

    xmlChar *dset_name = NULL;
    dset_name = xmlTextReaderGetAttribute(this->xmlreader,
                                          (const xmlChar *)XML_ATTR_DATASET);
    cout << "new_dataset: " << dset_name << endl;
    if (dset_name == NULL) return -1;

    string str_dset_name((char*)dset_name);

    HdfGroup *parent = (HdfGroup *)this->ptr_curr_element;
    HdfDataset *dset = NULL;
    dset = parent->new_dset(str_dset_name);
    if (dset == NULL) return -1;

    this->ptr_curr_element = dset;
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
                                            (const xmlChar*)XML_ATTR_ATTRIBUTE);
    cout << "new_attribute: " << ndattr_name << endl;
    if (ndattr_name == NULL) return -1;

    string str_ndattr_name((char*)ndattr_name);
    HdfAttribute ndattr(str_ndattr_name);
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

