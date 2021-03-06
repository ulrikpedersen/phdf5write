/*
 * layoutxml.h
 *
 *  Created on: 26 Jan 2012
 *      Author: up45
 */

#ifndef LAYOUTXML_H_
#define LAYOUTXML_H_

#include <libxml/xmlreader.h>
#include <log4cxx/logger.h>

// forward declarations
class HdfGroup;
class HdfRoot;
class HdfDataSource;
class HdfAttribute;
class HdfElement;

int main_xml(const char *fname);


class LayoutXML {
public:
    LayoutXML();
    ~LayoutXML();

    int load_xml(std::string& filename){ return this->load_xml(filename.c_str()); };
    int load_xml(const char* filename);

    HdfRoot* get_hdftree();


private:
    void process_node();

    int process_dset_xml_attribute(HdfDataSource& out);
    int process_attribute_xml_attribute(HdfAttribute& out);

    int new_group();
    int new_dataset();
    int new_attribute();

    log4cxx::LoggerPtr log;
    HdfRoot* ptr_tree;
    HdfElement *ptr_curr_element;
    xmlTextReaderPtr xmlreader;
};

#endif /* LAYOUTXML_H_ */
