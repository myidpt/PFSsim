//
// Generated file, do not edit! Created by opp_msgc 4.0 from packet/SPacket.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#include <iostream>
#include <sstream>
#include "SPacket_m.h"

// Template rule which fires if a struct or class doesn't have operator<<
template<typename T>
std::ostream& operator<<(std::ostream& out,const T&) {return out;}

// Another default rule (prevents compiler from choosing base class' doPacking())
template<typename T>
void doPacking(cCommBuffer *, T& t) {
    throw cRuntimeError("Parsim error: no doPacking() function for type %s or its base class (check .msg and _m.cc/h files!)",opp_typename(typeid(t)));
}

template<typename T>
void doUnpacking(cCommBuffer *, T& t) {
    throw cRuntimeError("Parsim error: no doUnpacking() function for type %s or its base class (check .msg and _m.cc/h files!)",opp_typename(typeid(t)));
}




Register_Class(sPacket);

sPacket::sPacket(const char *name, int kind) : cPacket(name,kind)
{
    for (unsigned int i=0; i<256; i++)
        this->tags_var[i] = 0;
    this->forward_var = 0;
}

sPacket::sPacket(const sPacket& other) : cPacket()
{
    setName(other.getName());
    operator=(other);
}

sPacket::~sPacket()
{
}

sPacket& sPacket::operator=(const sPacket& other)
{
    if (this==&other) return *this;
    cPacket::operator=(other);
    for (unsigned int i=0; i<256; i++)
        this->tags_var[i] = other.tags_var[i];
    this->forward_var = other.forward_var;
    return *this;
}

void sPacket::parsimPack(cCommBuffer *b)
{
    cPacket::parsimPack(b);
    doPacking(b,this->tags_var,256);
    doPacking(b,this->forward_var);
}

void sPacket::parsimUnpack(cCommBuffer *b)
{
    cPacket::parsimUnpack(b);
    doUnpacking(b,this->tags_var,256);
    doUnpacking(b,this->forward_var);
}

unsigned int sPacket::getTagsArraySize() const
{
    return 256;
}

double sPacket::getTags(unsigned int k) const
{
    if (k>=256) throw cRuntimeError("Array of size 256 indexed by %d", k);
    return tags_var[k];
}

void sPacket::setTags(unsigned int k, double tags_var)
{
    if (k>=256) throw cRuntimeError("Array of size 256 indexed by %d", k);
    this->tags_var[k] = tags_var;
}

bool sPacket::getForward() const
{
    return forward_var;
}

void sPacket::setForward(bool forward_var)
{
    this->forward_var = forward_var;
}

class sPacketDescriptor : public cClassDescriptor
{
  public:
    sPacketDescriptor();
    virtual ~sPacketDescriptor();

    virtual bool doesSupport(cObject *obj) const;
    virtual const char *getProperty(const char *propertyname) const;
    virtual int getFieldCount(void *object) const;
    virtual const char *getFieldName(void *object, int field) const;
    virtual unsigned int getFieldTypeFlags(void *object, int field) const;
    virtual const char *getFieldTypeString(void *object, int field) const;
    virtual const char *getFieldProperty(void *object, int field, const char *propertyname) const;
    virtual int getArraySize(void *object, int field) const;

    virtual bool getFieldAsString(void *object, int field, int i, char *resultbuf, int bufsize) const;
    virtual bool setFieldAsString(void *object, int field, int i, const char *value) const;

    virtual const char *getFieldStructName(void *object, int field) const;
    virtual void *getFieldStructPointer(void *object, int field, int i) const;
};

Register_ClassDescriptor(sPacketDescriptor);

sPacketDescriptor::sPacketDescriptor() : cClassDescriptor("sPacket", "cPacket")
{
}

sPacketDescriptor::~sPacketDescriptor()
{
}

bool sPacketDescriptor::doesSupport(cObject *obj) const
{
    return dynamic_cast<sPacket *>(obj)!=NULL;
}

const char *sPacketDescriptor::getProperty(const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : NULL;
}

int sPacketDescriptor::getFieldCount(void *object) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 2+basedesc->getFieldCount(object) : 2;
}

unsigned int sPacketDescriptor::getFieldTypeFlags(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldTypeFlags(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 0: return FD_ISARRAY | FD_ISEDITABLE;
        case 1: return FD_ISEDITABLE;
        default: return 0;
    }
}

const char *sPacketDescriptor::getFieldName(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldName(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 0: return "tags";
        case 1: return "forward";
        default: return NULL;
    }
}

const char *sPacketDescriptor::getFieldTypeString(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldTypeString(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 0: return "double";
        case 1: return "bool";
        default: return NULL;
    }
}

const char *sPacketDescriptor::getFieldProperty(void *object, int field, const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldProperty(object, field, propertyname);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        default: return NULL;
    }
}

int sPacketDescriptor::getArraySize(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getArraySize(object, field);
        field -= basedesc->getFieldCount(object);
    }
    sPacket *pp = (sPacket *)object; (void)pp;
    switch (field) {
        case 0: return 256;
        default: return 0;
    }
}

bool sPacketDescriptor::getFieldAsString(void *object, int field, int i, char *resultbuf, int bufsize) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldAsString(object,field,i,resultbuf,bufsize);
        field -= basedesc->getFieldCount(object);
    }
    sPacket *pp = (sPacket *)object; (void)pp;
    switch (field) {
        case 0: double2string(pp->getTags(i),resultbuf,bufsize); return true;
        case 1: bool2string(pp->getForward(),resultbuf,bufsize); return true;
        default: return false;
    }
}

bool sPacketDescriptor::setFieldAsString(void *object, int field, int i, const char *value) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->setFieldAsString(object,field,i,value);
        field -= basedesc->getFieldCount(object);
    }
    sPacket *pp = (sPacket *)object; (void)pp;
    switch (field) {
        case 0: pp->setTags(i,string2double(value)); return true;
        case 1: pp->setForward(string2bool(value)); return true;
        default: return false;
    }
}

const char *sPacketDescriptor::getFieldStructName(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldStructName(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        default: return NULL;
    }
}

void *sPacketDescriptor::getFieldStructPointer(void *object, int field, int i) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldStructPointer(object, field, i);
        field -= basedesc->getFieldCount(object);
    }
    sPacket *pp = (sPacket *)object; (void)pp;
    switch (field) {
        default: return NULL;
    }
}


