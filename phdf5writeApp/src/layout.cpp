/*
 * layout.cpp
 *
 *  Created on: 23 Jan 2012
 *      Author: up45
 */

#include <iostream>
#include <string>
#include <algorithm>

using namespace std;

#include "layout.h"

/* ================== HdfElement Class public methods ==================== */
HdfElement::HdfElement()
{
    this->name = "entry";
    this->path = "/";
}

HdfElement::HdfElement(const string& name)
{
    cout << "HdfElement string name: " << name << endl;
    this->name = name;
}

string HdfElement::get_full_name()
{
    string fname = this->path;
    fname += this->name;
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

/* ================== HdfElement Class protected methods ==================== */
void HdfElement::_copy(const HdfElement& src)
{
    this->name = src.name;
    this->path = src.path;
    this->attributes = src.attributes;
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
    cout << "HdfGroup destructor! " << this->datasets.size() << endl;
    if (this->datasets.size() > 0)
        for_each(this->datasets.begin(), this->datasets.end(), _delete_obj<HdfDataset>);
    if (this->groups.size() > 0)
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

int HdfGroup::insert_dset(HdfDataset* dset)
{
    // First check that a dataset or a group with this name doesn't already exist
    if ( this->datasets.count(dset->get_name()) > 0 )
        return -1;
    if ( this->groups.count(dset->get_name()) > 0 )
        return -1;

    // Insert the string, HdfDataset pointer pair in the datasets map.
    // Check for successful insertion.
    pair<map<string,HdfDataset*>::iterator,bool> ret;
    ret = this->datasets.insert( pair<string, HdfDataset*>(dset->get_name(), dset) );
    if (ret.second == false) return -2;
    dset->path += this->get_full_name();
    dset->path += "/";
    return 0;
}

int HdfGroup::new_dset(std::string& name)
{
    HdfDataset* ds = new HdfDataset(name);
    return this->insert_dset(ds);
}

int HdfGroup::insert_group(HdfGroup* group)
{
    // First check that a dataset or a group with this name doesn't already exist
    if ( this->datasets.count(group->get_name()) > 0 )
        return -1;
    if ( this->groups.count(group->get_name()) > 0 )
        return -1;

    // Insert the string, HdfDataset pointer pair in the datasets map.
    // Check for successful insertion.
    pair<map<string,HdfGroup*>::iterator,bool> ret;
    ret = this->groups.insert( pair<string, HdfGroup*>(group->get_name(), group) );
    if (ret.second == false) return -2;
    group->path += this->get_full_name();
    group->path += "/";
    return 0;
}

int HdfGroup::new_group(std::string& name)
{
    HdfGroup* grp = new HdfGroup(name);
    return this->insert_group(grp);
}

int HdfGroup::find_dset_ndattr(std::string ndattr_name)
{

}

int HdfGroup::find_dset( std::string& dsetname, HdfDataset& dest )
{
    map<string, HdfDataset*>::const_iterator it_dsets;
    it_dsets = this->datasets.find(dsetname);
    if (it_dsets != this->datasets.end())
    {
        dest = *(it_dsets->second);
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

/* ================== HdfGroup Class private methods ==================== */
void HdfGroup::_copy(const HdfGroup& src)
{
    HdfElement::_copy(src);
    this->datasets = src.datasets;
    this->groups = src.groups;
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

/* ================== HdfDataset Class private methods ==================== */
void HdfDataset::_copy(const HdfDataset& src)
{
    HdfElement::_copy(src);
    this->ndattr_name = src.ndattr_name;
}
