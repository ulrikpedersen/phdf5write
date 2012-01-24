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
    ~HdfAttribute(){};
    HdfAttribute& operator=(const HdfAttribute& src) {this->_copy(src); return *this;};

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

    std::string get_name(){ return this->name; };
    std::string get_full_name();
    int add_attribute(HdfAttribute& attr);

protected:
    void _copy(const HdfElement& src);
    std::vector<HdfAttribute> attributes;
    std::string path;
    std::string name;
public:
    friend class HdfGroup;
private:
};


class HdfDataset: public HdfElement {
public:
    HdfDataset() : HdfElement(){};
    HdfDataset(const std::string& name) : HdfElement(name){};
    HdfDataset(const HdfDataset& src);
    HdfDataset& operator=(const HdfDataset& src);
    ~HdfDataset(){};

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

    int insert_dset(HdfDataset* dset);
    int new_dset(std::string& name);
    int insert_group(HdfGroup* group);
    int new_group(std::string& name);
    int find_dset_ndattr(std::string ndattr_name); /** << Find and return a reference to the dataset for a given NDAttribute */
    int find_dset( std::string& dsetname, HdfDataset& dest);

private:
    void _copy(const HdfGroup& src);
    //template <typename T> void _delete_obj(T *obj) { delete obj->second; };
    std::map<std::string, HdfDataset*> datasets;
    std::map<std::string, HdfGroup*> groups;
};



#endif /* LAYOUT_H_ */
