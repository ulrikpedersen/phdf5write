/*
 * layoutxml.h
 *
 *  Created on: 26 Jan 2012
 *      Author: up45
 */

#ifndef LAYOUTXML_H_
#define LAYOUTXML_H_

#include <libxml/xmlreader.h>

// forward declarations
class HdfGroup;

int main_xml(const char *fname);


class LayoutXML {
public:
    LayoutXML();
    ~LayoutXML(){};

    int load_xml(std::string& filename){ return this->load_xml(filename.c_str()); };
    int load_xml(const char* filename);


private:
    int process_node();

    int new_group();
    int new_dataset();
    int new_ndattribute();

    HdfGroup* ptr_tree;
    HdfElement *ptr_curr_element;
    xmlTextReaderPtr xmlreader;
};

#endif /* LAYOUTXML_H_ */
