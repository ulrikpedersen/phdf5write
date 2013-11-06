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

namespace phdf5 {

// forward declarations
class HdfGroup;
class HdfRoot;
class HdfDataSource;
class HdfAttribute;
class HdfElement;

int main_xml(const char *fname);


class LayoutXML {
public:

  static const std::string ATTR_ELEMENT_NAME;
  static const std::string ATTR_GROUP;
  static const std::string ATTR_DATASET;
  static const std::string ATTR_ATTRIBUTE;

  static const std::string ATTR_SOURCE;
  static const std::string ATTR_SRC_DETECTOR;
  static const std::string ATTR_SRC_NDATTR;
  static const std::string ATTR_SRC_CONST;
  static const std::string ATTR_SRC_CONST_VALUE;
  static const std::string ATTR_SRC_CONST_TYPE;
  static const std::string ATTR_GRP_NDATTR_DEFAULT;

    LayoutXML();
    ~LayoutXML();

//    int load_xml(std::string& filename){ return this->load_xml(filename.c_str()); };
    int load_xml(const std::string& filename);

    HdfRoot* get_hdftree();


private:
    int process_node();

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

} // phdf5

#endif /* LAYOUTXML_H_ */
