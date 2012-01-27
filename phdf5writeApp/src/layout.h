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


class HdfAttribute {
public:
    HdfAttribute(){};
    HdfAttribute(const HdfAttribute& src) {this->_copy(src);};
    HdfAttribute(std::string& name){this->name = name;};
    ~HdfAttribute(){};
    HdfAttribute& operator=(const HdfAttribute& src) {this->_copy(src); return *this;};
    std::string get_name() {return this->name;};

private:
    void _copy(const HdfAttribute& src){};
    std::string name;
    // todo: type, value...
};

/** Describe a generic HDF5 structure element.
 * An element can contain a number of attributes and
 * be a subset of a number of parents.
 */
class HdfElement {
public:
    HdfElement();
    HdfElement(const HdfElement& src) {this->_copy(src);};
    HdfElement(const std::string& name);
    ~HdfElement(){};
    HdfElement& operator=(const HdfElement& src);

    const std::string& get_name(){ return this->name; };
    std::string get_full_name();
    int add_attribute(HdfAttribute& attr);
    bool has_attribute(const std::string& attr_name);
    int tree_level();
    HdfElement *get_parent() {return this->ptr_parent;};

protected:
    void _copy(const HdfElement& src);
    void build_full_path(HdfElement* new_child);
    std::map<std::string, HdfAttribute> attributes;
    std::string path;
    std::string name;
public:
    friend class HdfGroup;
private:
    HdfElement *ptr_parent;
};


class HdfDataset: public HdfElement {
public:
    HdfDataset() : HdfElement(){};
    HdfDataset(const std::string& name) : HdfElement(name){};
    HdfDataset(const HdfDataset& src);
    HdfDataset& operator=(const HdfDataset& src);
    ~HdfDataset(){};

    /** Stream operator: use to prints a string representation of this class */
    inline friend std::ostream& operator<<(std::ostream& out, HdfDataset& dset) {
        out << dset._str_();
        return out;
    }
    std::string _str_();  /** Return a string representation of the object */

private:
    void _copy(const HdfDataset& src);
    std::string ndattr_name;
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
    inline friend std::ostream& operator<<(std::ostream& out, HdfGroup& grp) {
        out << grp._str_();
        return out;
    }
    std::string _str_();  /** Return a string representation of the object */

private:
    void _copy(const HdfGroup& src);
    bool name_exist(const std::string& name);

    std::map<std::string, HdfDataset*> datasets;
    std::map<std::string, HdfGroup*> groups;
};



#endif /* LAYOUT_H_ */
