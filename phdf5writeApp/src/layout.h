/*
 * layout.h
 *
 *  Created on: 23 Jan 2012
 *      Author: up45
 */

#ifndef LAYOUT_H_
#define LAYOUT_H_

#include <string>
#include <vector>
#include <map>

enum HdfDataSrc_t { notset, detector, ndattribute, constant };

class HdfAttrSource {
public:
	// Default constructor
    HdfAttrSource();
    // Initialising constructor
    HdfAttrSource( HdfDataSrc_t srctype, const std::string& val);
    HdfAttrSource( HdfDataSrc_t srctype);
    // Copy constructor
    HdfAttrSource( const HdfAttrSource& src);
    // Assignment operator
    HdfAttrSource& operator=(const HdfAttrSource& src);
    ~HdfAttrSource(){};
    bool is_src_detector();
    bool is_src_ndattribute();
    bool is_src_constant();

    std::string get_src_def(); /** return the string that define the source: either name of NDAttribute or constant value */

private:
    HdfDataSrc_t data_src_type;
    std::string val;
};


class HdfAttribute {
public:
    HdfAttribute(){};
    HdfAttribute(const HdfAttribute& src); // Copy constructor
    HdfAttribute(std::string& name);
    ~HdfAttribute(){};
    HdfAttribute& operator=(const HdfAttribute& src);
    std::string get_name();

    HdfAttrSource source;
private:
    void _copy(const HdfAttribute& src){this->name = src.name; this->source = src.source;};
    std::string name;
};

/** Describe a generic HDF5 structure element.
 * An element can contain a number of attributes and
 * be a subset of a number of parents.
 */
class HdfElement {
public:
    HdfElement();
    HdfElement(const HdfElement& src);
    HdfElement(const std::string& name);
    ~HdfElement(){};
    HdfElement& operator=(const HdfElement& src);

    const std::string& get_name();
    std::string get_full_name();
    std::string get_path(bool trailing_slash=false);
    int add_attribute(HdfAttribute& attr);
    bool has_attribute(const std::string& attr_name);
    int tree_level();
    HdfElement *get_parent();

protected:
    void _copy(const HdfElement& src);
    std::map<std::string, HdfAttribute> attributes;
    std::string name;
public:
    friend class HdfGroup;
private:
    HdfElement *parent;
};


class HdfDataset: public HdfElement {
public:
    HdfDataset() : HdfElement(){};
    HdfDataset(const std::string& name) : HdfElement(name){};
    HdfDataset(const HdfDataset& src);
    HdfDataset& operator=(const HdfDataset& src);
    ~HdfDataset(){};

    //void is_detector_data();

    /** Stream operator: use to prints a string representation of this class */
    inline friend std::ostream& operator<<(std::ostream& out, HdfDataset& dset)
    { out << dset._str_(); return out; }
    std::string _str_();  /** Return a string representation of the object */

    int set_data_source(HdfAttrSource& src);
private:
    void _copy(const HdfDataset& src);
    std::string ndattr_name;
    HdfAttrSource datasource;
};

/** Describe a HDF5 group element.
 * A group is like a directory in a file system. It can contain
 * other groups and datasets (like files).
 */
class HdfGroup: public HdfElement {
public:
    HdfGroup() : HdfElement(){};
    HdfGroup(const std::string& name) : HdfElement(name){};
    HdfGroup(const char * name) : HdfElement( std::string(name)) {};
    HdfGroup(const HdfGroup& src);
    ~HdfGroup();
    HdfGroup& operator=(const HdfGroup& src);

    HdfDataset* new_dset(const std::string& name);
    HdfDataset* new_dset(const char * name);
    HdfGroup* new_group(const std::string& name);
    HdfGroup* new_group(const char * name);
    int find_dset_ndattr(const std::string& ndattr_name, HdfDataset** dset); /** << Find and return a reference to the dataset for a given NDAttribute */
    int find_dset( std::string& dsetname, HdfDataset** dest);
    int num_groups();
    int num_datasets();

    /** Stream operator: use to prints a string representation of this class */
    inline friend std::ostream& operator<<(std::ostream& out, HdfGroup& grp)
    { out << grp._str_(); return out; }
    std::string _str_();  /** Return a string representation of the object */

    typedef std::map<std::string, HdfGroup*> MapGroups_t;
    typedef std::map<std::string, HdfDataset*> MapDatasets_t;
    MapGroups_t& get_groups();
    MapDatasets_t& get_datasets();

private:
    void _copy(const HdfGroup& src);
    bool name_exist(const std::string& name);

    std::map<std::string, HdfDataset*> datasets;
    std::map<std::string, HdfGroup*> groups;
};



#endif /* LAYOUT_H_ */
