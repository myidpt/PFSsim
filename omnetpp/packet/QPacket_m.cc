//
// Generated file, do not edit! Created by opp_msgc 4.0 from packet/QPacket.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#include <iostream>
#include <sstream>
#include "QPacket_m.h"

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




Register_Class(qPacket);

qPacket::qPacket(const char *name, int kind) : cPacket(name,kind)
{
    this->Id_var = 0;
    this->app_var = -1;
    this->dsNum_var = 0;
    for (unsigned int i=0; i<1024; i++)
        this->dsList_var[i] = 0;
}

qPacket::qPacket(const qPacket& other) : cPacket()
{
    setName(other.getName());
    operator=(other);
}

qPacket::~qPacket()
{
}

qPacket& qPacket::operator=(const qPacket& other)
{
    if (this==&other) return *this;
    cPacket::operator=(other);
    this->Id_var = other.Id_var;
    this->app_var = other.app_var;
    this->dsNum_var = other.dsNum_var;
    for (unsigned int i=0; i<1024; i++)
        this->dsList_var[i] = other.dsList_var[i];
    return *this;
}

void qPacket::parsimPack(cCommBuffer *b)
{
    cPacket::parsimPack(b);
    doPacking(b,this->Id_var);
    doPacking(b,this->app_var);
    doPacking(b,this->dsNum_var);
    doPacking(b,this->dsList_var,1024);
}

void qPacket::parsimUnpack(cCommBuffer *b)
{
    cPacket::parsimUnpack(b);
    doUnpacking(b,this->Id_var);
    doUnpacking(b,this->app_var);
    doUnpacking(b,this->dsNum_var);
    doUnpacking(b,this->dsList_var,1024);
}

int qPacket::getId() const
{
    return Id_var;
}

void qPacket::setId(int Id_var)
{
    this->Id_var = Id_var;
}

int qPacket::getApp() const
{
    return app_var;
}

void qPacket::setApp(int app_var)
{
    this->app_var = app_var;
}

int qPacket::getDsNum() const
{
    return dsNum_var;
}

void qPacket::setDsNum(int dsNum_var)
{
    this->dsNum_var = dsNum_var;
}

unsigned int qPacket::getDsListArraySize() const
{
    return 1024;
}

int qPacket::getDsList(unsigned int k) const
{
    if (k>=1024) throw cRuntimeError("Array of size 1024 indexed by %d", k);
    return dsList_var[k];
}

void qPacket::setDsList(unsigned int k, int dsList_var)
{
    if (k>=1024) throw cRuntimeError("Array of size 1024 indexed by %d", k);
    this->dsList_var[k] = dsList_var;
}

class qPacketDescriptor : public cClassDescriptor
{
  public:
    qPacketDescriptor();
    virtual ~qPacketDescriptor();

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

Register_ClassDescriptor(qPacketDescriptor);

qPacketDescriptor::qPacketDescriptor() : cClassDescriptor("qPacket", "cPacket")
{
}

qPacketDescriptor::~qPacketDescriptor()
{
}

bool qPacketDescriptor::doesSupport(cObject *obj) const
{
    return dynamic_cast<qPacket *>(obj)!=NULL;
}

const char *qPacketDescriptor::getProperty(const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : NULL;
}

int qPacketDescriptor::getFieldCount(void *object) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 4+basedesc->getFieldCount(object) : 4;
}

unsigned int qPacketDescriptor::getFieldTypeFlags(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldTypeFlags(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 0: return FD_ISEDITABLE;
        case 1: return FD_ISEDITABLE;
        case 2: return FD_ISEDITABLE;
        case 3: return FD_ISARRAY | FD_ISEDITABLE;
        default: return 0;
    }
}

const char *qPacketDescriptor::getFieldName(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldName(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 0: return "Id";
        case 1: return "app";
        case 2: return "dsNum";
        case 3: return "dsList";
        default: return NULL;
    }
}

const char *qPacketDescriptor::getFieldTypeString(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldTypeString(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 0: return "int";
        case 1: return "int";
        case 2: return "int";
        case 3: return "int";
        default: return NULL;
    }
}

const char *qPacketDescriptor::getFieldProperty(void *object, int field, const char *propertyname) const
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

int qPacketDescriptor::getArraySize(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getArraySize(object, field);
        field -= basedesc->getFieldCount(object);
    }
    qPacket *pp = (qPacket *)object; (void)pp;
    switch (field) {
        case 3: return 1024;
        default: return 0;
    }
}

bool qPacketDescriptor::getFieldAsString(void *object, int field, int i, char *resultbuf, int bufsize) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldAsString(object,field,i,resultbuf,bufsize);
        field -= basedesc->getFieldCount(object);
    }
    qPacket *pp = (qPacket *)object; (void)pp;
    switch (field) {
        case 0: long2string(pp->getId(),resultbuf,bufsize); return true;
        case 1: long2string(pp->getApp(),resultbuf,bufsize); return true;
        case 2: long2string(pp->getDsNum(),resultbuf,bufsize); return true;
        case 3: long2string(pp->getDsList(i),resultbuf,bufsize); return true;
        default: return false;
    }
}

bool qPacketDescriptor::setFieldAsString(void *object, int field, int i, const char *value) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->setFieldAsString(object,field,i,value);
        field -= basedesc->getFieldCount(object);
    }
    qPacket *pp = (qPacket *)object; (void)pp;
    switch (field) {
        case 0: pp->setId(string2long(value)); return true;
        case 1: pp->setApp(string2long(value)); return true;
        case 2: pp->setDsNum(string2long(value)); return true;
        case 3: pp->setDsList(i,string2long(value)); return true;
        default: return false;
    }
}

const char *qPacketDescriptor::getFieldStructName(void *object, int field) const
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

void *qPacketDescriptor::getFieldStructPointer(void *object, int field, int i) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldStructPointer(object, field, i);
        field -= basedesc->getFieldCount(object);
    }
    qPacket *pp = (qPacket *)object; (void)pp;
    switch (field) {
        default: return NULL;
    }
}


