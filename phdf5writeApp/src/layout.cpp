/*
 * layout.cpp
 *
 *  Created on: 23 Jan 2012
 *      Author: up45
 */

#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>

using namespace std;

#include "layout.h"

/* ================== HdfAttribute Class public methods ==================== */
HdfAttribute::HdfAttribute(const HdfAttribute& src)
{
	this->_copy(src);
}
HdfAttribute::HdfAttribute(std::string& name)
: name(name) {}

/** Assignment operator */
HdfAttribute& HdfAttribute::operator=(const HdfAttribute& src)
{
	this->_copy(src);
	return *this;
}
string HdfAttribute::get_name() {return this->name;}


// constructors
HdfAttrSource::HdfAttrSource()
: data_src_type(notset), val(""){}
HdfAttrSource::HdfAttrSource( HdfDataSrc_t srctype, const std::string& val)
: data_src_type(srctype), val(val){}
HdfAttrSource::HdfAttrSource( HdfDataSrc_t srctype)
: data_src_type(srctype), val(""){}
HdfAttrSource::HdfAttrSource( const HdfAttrSource& src)
: data_src_type(src.data_src_type), val(src.val){}

/** Assignment operator
 * Copies the sources private data members to this object.
 */
HdfAttrSource& HdfAttrSource::operator=(const HdfAttrSource& src)
{
	this->data_src_type = src.data_src_type;
	this->val = src.val;
	return *this;
};

bool HdfAttrSource::is_src_constant() {
    return this->data_src_type == constant ? true : false;
}
bool HdfAttrSource::is_src_detector() {
    return this->data_src_type == detector ? true : false;
}
bool HdfAttrSource::is_src_ndattribute() {
    return this->data_src_type == ndattribute ? true : false;
}

std::string HdfAttrSource::get_src_def()
{
    return this->val;
}

/* ================== HdfElement Class public methods ==================== */
HdfElement::HdfElement()
{
    this->name = "entry";
    if (this->path.empty())
        this->path = "/";
    this->ptr_parent = NULL;
}

HdfElement::HdfElement(const HdfElement& src)
: ptr_parent(NULL)
{
	this->_copy(src);
}

HdfElement::HdfElement(const string& name)
{
    this->name = name;
    if (this->path.empty())
        this->path = "/";
    this->ptr_parent = NULL;
}

string HdfElement::get_full_name()
{
    //cout << "get_full_name: " << this->path << " name: " << this->name << endl;
    string fname;
    if (this->path.empty() or this->name.empty())
    {
        cerr << "Warning: missing path or name" << endl;
    } else {
        fname = this->path;
        fname += this->name;
    }
    return fname;
}

HdfElement& HdfElement::operator=(const HdfElement& src)
{
    // Check for self-assignment
    if (this == &src)
        return *this;
    this->_copy(src);
    return *this;
}

const string& HdfElement::get_name()
{
	return this->name;
}

HdfElement * HdfElement::get_parent()
{
	return this->ptr_parent;
}

int HdfElement::add_attribute(HdfAttribute& attr)
{
    if (this->attributes.count(attr.get_name()) != 0) return -1;
    pair<map<string,HdfAttribute>::iterator,bool> ret;
    ret = this->attributes.insert( pair<string, HdfAttribute>(attr.get_name(), attr) );
    // Check for successful insertion.
    if (ret.second == false) return -1;
    return 0;
}

bool HdfElement::has_attribute(const string& attr_name)
{
    return this->attributes.count(attr_name)>0 ? true : false;
}

int HdfElement::tree_level()
{
    int level = 0;
    size_t pos = 0;
    while( pos != string::npos ){
        pos = this->path.find('/', pos+1);
        level++;
    }
    return level;
}

/* ================== HdfElement Class protected methods ==================== */
void HdfElement::_copy(const HdfElement& src)
{
    this->name = src.name;
    this->path = src.path;
    this->attributes = src.attributes;
    this->ptr_parent = src.ptr_parent;
}

void HdfElement::build_full_path(HdfElement* new_child)
{
    new_child->path = this->get_full_name();
    new_child->path += "/";
}


/* ================== HdfGroup Class public methods ==================== */

HdfGroup::HdfGroup(const HdfGroup& src)
{
    this->_copy(src);
}

template <typename T>
void _delete_obj(std::pair<std::string, T*> pair )
{
    delete pair.second;
};

HdfGroup::~HdfGroup()
{
    for_each(this->datasets.begin(), this->datasets.end(), _delete_obj<HdfDataset>);
    for_each(this->groups.begin(), this->groups.end(), _delete_obj<HdfGroup>);

}

HdfGroup& HdfGroup::operator=(const HdfGroup& src)
{
    // Check for self-assignment
    if (this == &src)
        return *this;
    this->_copy(src);
    return *this;
}


HdfDataset* HdfGroup::new_dset(const std::string& name)
{
    HdfDataset* ds = NULL;

    // First check that a dataset or a group with this name doesn't already exist
    if ( this->name_exist(name) ) return NULL;

    // Create the object
    ds = new HdfDataset(name);
    ds->ptr_parent = this;

    // Insert the string, HdfDataset pointer pair in the datasets map.
    pair<map<string,HdfDataset*>::iterator,bool> ret;
    ret = this->datasets.insert( pair<string, HdfDataset*>(name, ds) );

    // Check for successful insertion.
    if (ret.second == false) return NULL;
    this->build_full_path(ds);
    return ds;
}

HdfDataset* HdfGroup::new_dset(const char* name)
{
    string str_name(name);
    return this->new_dset(str_name);
}


/** Create a new group, insert it into the group list, set the full path name,
 * and finally return a pointer to the newly created object.
 * Return NULL on error (group or dataset already exists or list insertion fails)
 */
HdfGroup* HdfGroup::new_group(const std::string& name)
{
    HdfGroup* grp = NULL;

    // First check that a dataset or a group with this name doesn't already exist
    if ( this->name_exist(name) ) return NULL;

    // Create the new group
    grp = new HdfGroup(name);
    grp->ptr_parent = this;

    // Insert the string, HdfDataset pointer pair in the datasets map.
    pair<map<string,HdfGroup*>::iterator,bool> ret;
    ret = this->groups.insert( pair<string, HdfGroup*>(name, grp) );
    // Check for successful insertion.
    if (ret.second == false) return NULL;
    this->build_full_path(grp);
    return grp;
}

HdfGroup* HdfGroup::new_group(const char * name)
{
    string str_name(name);
    return this->new_group(str_name);
}

int HdfGroup::find_dset_ndattr(const string& ndattr_name, HdfDataset** dset)
{
    // check first whether this group has a dataset with this attribute
    map<string, HdfDataset*>::iterator it_dsets;
    for (it_dsets = this->datasets.begin();
         it_dsets != this->datasets.end();
         ++it_dsets)
    {
        if ( it_dsets->second->has_attribute( ndattr_name ) )
        {
            *dset = it_dsets->second;
            return 0;
        }
    }

    // Now check if any children has a dataset with this attribute
    map<string, HdfGroup*>::iterator it_groups;
    for (it_groups = this->groups.begin();
         it_groups != this->groups.end();
         ++it_groups)
    {
        if (it_groups->second->find_dset_ndattr(ndattr_name, dset) == 0)
        {
            return 0;
        }
    }
    return -1;
}

int HdfGroup::find_dset( string& dsetname, HdfDataset** dest )
{
    map<string, HdfDataset*>::const_iterator it_dsets;
    it_dsets = this->datasets.find(dsetname);
    if (it_dsets != this->datasets.end())
    {
        *dest = it_dsets->second;
        return 0;
    }

    map<string, HdfGroup*>::iterator it_groups;
    int retcode = 0;
    for (it_groups = this->groups.begin();
            it_groups != this->groups.end();
            ++it_groups)
    {
        retcode = it_groups->second->find_dset(dsetname, dest);
        if (retcode == 0) return 0;
    }
    return -1;
}

int HdfGroup::num_datasets()
{
    return this->datasets.size();
}

int HdfGroup::num_groups()
{
    return this->groups.size();
}

string HdfGroup::_str_()
{
    stringstream out(stringstream::out);
    out << "< HdfGroup: \'" << this->get_full_name() << "\'";
    out << " groups=" << this->num_groups();
    out << " dsets=" << this->num_datasets();
    out << " attr=" << this->attributes.size();
    //out << " level=" << this->tree_level();
    out << " >";

    map<string, HdfGroup*>::iterator it_groups;
    for (it_groups = this->groups.begin();
            it_groups != this->groups.end();
            ++it_groups)
    {
        out << "\n";
        for (int i=0; i<this->tree_level(); i++) out << "  ";
        out << it_groups->second->_str_();
    }

    return out.str();
}

HdfGroup::MapDatasets_t& HdfGroup::get_datasets()
{
	return this->datasets;
}
HdfGroup::MapGroups_t& HdfGroup::get_groups()
{
	return this->groups;
}

/* ================== HdfGroup Class private methods ==================== */
void HdfGroup::_copy(const HdfGroup& src)
{
    HdfElement::_copy(src);
    this->datasets = src.datasets;
    this->groups = src.groups;
}



bool HdfGroup::name_exist(const std::string& name)
{
    // First check that a dataset or a group with this name doesn't already exist
    if ( this->datasets.count(name) > 0 )
        return true;
    if ( this->groups.count(name) > 0 )
        return true;
    return false;
}


/* ================== HdfDataset Class public methods ==================== */
HdfDataset::HdfDataset(const HdfDataset& src)
{
    this->_copy(src);
}

HdfDataset& HdfDataset::operator =(const HdfDataset& src)
{
    // Check for self-assignment
    if (this == &src)
        return *this;
    this->_copy(src);
    return *this;
}

string HdfDataset::_str_()
{
    stringstream out(stringstream::out);
    out << "< HdfDataset: \'" << this->get_full_name() << "\'";
    out << " NDAttribute: \'" << this->ndattr_name << "\' >";
    return out.str();
}

int HdfDataset::set_data_source(HdfAttrSource& src)
{
    int retval = -1;
    if (src.is_src_detector() || src.is_src_ndattribute()) {
        this->datasource = src;
        retval = 0;
    }
    return retval;
}



/* ================== HdfDataset Class private methods ==================== */
void HdfDataset::_copy(const HdfDataset& src)
{
    HdfElement::_copy(src);
    this->ndattr_name = src.ndattr_name;
    this->datasource = src.datasource;
}
