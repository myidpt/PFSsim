//
// Generated file, do not edit! Created by opp_msgc 4.0 from packet/GPacket.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#include <iostream>
#include <sstream>
#include "GPacket_m.h"

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




Register_Class(gPacket);

gPacket::gPacket(const char *name, int kind) : cPacket(name,kind)
{
    this->id_var = 0;
    this->risetime_var = 0;
    this->submittime_var = 0;
    this->arrivaltime_var = 0;
    this->dispatchtime_var = 0;
    this->finishtime_var = 0;
    this->returntime_var = 0;
    this->qos_delay_var = 0;
    this->highoffset_var = 0;
    this->lowoffset_var = 0;
    this->size_var = 0;
    this->read_var = 0;
    this->decision_var = -1;
    this->app_var = 0;
}

gPacket::gPacket(const gPacket& other) : cPacket()
{
    setName(other.getName());
    operator=(other);
}

gPacket::~gPacket()
{
}

gPacket& gPacket::operator=(const gPacket& other)
{
    if (this==&other) return *this;
    cPacket::operator=(other);
    this->id_var = other.id_var;
    this->risetime_var = other.risetime_var;
    this->submittime_var = other.submittime_var;
    this->arrivaltime_var = other.arrivaltime_var;
    this->dispatchtime_var = other.dispatchtime_var;
    this->finishtime_var = other.finishtime_var;
    this->returntime_var = other.returntime_var;
    this->qos_delay_var = other.qos_delay_var;
    this->highoffset_var = other.highoffset_var;
    this->lowoffset_var = other.lowoffset_var;
    this->size_var = other.size_var;
    this->read_var = other.read_var;
    this->decision_var = other.decision_var;
    this->app_var = other.app_var;
    return *this;
}

void gPacket::parsimPack(cCommBuffer *b)
{
    cPacket::parsimPack(b);
    doPacking(b,this->id_var);
    doPacking(b,this->risetime_var);
    doPacking(b,this->submittime_var);
    doPacking(b,this->arrivaltime_var);
    doPacking(b,this->dispatchtime_var);
    doPacking(b,this->finishtime_var);
    doPacking(b,this->returntime_var);
    doPacking(b,this->qos_delay_var);
    doPacking(b,this->highoffset_var);
    doPacking(b,this->lowoffset_var);
    doPacking(b,this->size_var);
    doPacking(b,this->read_var);
    doPacking(b,this->decision_var);
    doPacking(b,this->app_var);
}

void gPacket::parsimUnpack(cCommBuffer *b)
{
    cPacket::parsimUnpack(b);
    doUnpacking(b,this->id_var);
    doUnpacking(b,this->risetime_var);
    doUnpacking(b,this->submittime_var);
    doUnpacking(b,this->arrivaltime_var);
    doUnpacking(b,this->dispatchtime_var);
    doUnpacking(b,this->finishtime_var);
    doUnpacking(b,this->returntime_var);
    doUnpacking(b,this->qos_delay_var);
    doUnpacking(b,this->highoffset_var);
    doUnpacking(b,this->lowoffset_var);
    doUnpacking(b,this->size_var);
    doUnpacking(b,this->read_var);
    doUnpacking(b,this->decision_var);
    doUnpacking(b,this->app_var);
}

long gPacket::getId() const
{
    return id_var;
}

void gPacket::setId(long id_var)
{
    this->id_var = id_var;
}

double gPacket::getRisetime() const
{
    return risetime_var;
}

void gPacket::setRisetime(double risetime_var)
{
    this->risetime_var = risetime_var;
}

double gPacket::getSubmittime() const
{
    return submittime_var;
}

void gPacket::setSubmittime(double submittime_var)
{
    this->submittime_var = submittime_var;
}

double gPacket::getArrivaltime() const
{
    return arrivaltime_var;
}

void gPacket::setArrivaltime(double arrivaltime_var)
{
    this->arrivaltime_var = arrivaltime_var;
}

double gPacket::getDispatchtime() const
{
    return dispatchtime_var;
}

void gPacket::setDispatchtime(double dispatchtime_var)
{
    this->dispatchtime_var = dispatchtime_var;
}

double gPacket::getFinishtime() const
{
    return finishtime_var;
}

void gPacket::setFinishtime(double finishtime_var)
{
    this->finishtime_var = finishtime_var;
}

double gPacket::getReturntime() const
{
    return returntime_var;
}

void gPacket::setReturntime(double returntime_var)
{
    this->returntime_var = returntime_var;
}

double gPacket::getQos_delay() const
{
    return qos_delay_var;
}

void gPacket::setQos_delay(double qos_delay_var)
{
    this->qos_delay_var = qos_delay_var;
}

long gPacket::getHighoffset() const
{
    return highoffset_var;
}

void gPacket::setHighoffset(long highoffset_var)
{
    this->highoffset_var = highoffset_var;
}

long gPacket::getLowoffset() const
{
    return lowoffset_var;
}

void gPacket::setLowoffset(long lowoffset_var)
{
    this->lowoffset_var = lowoffset_var;
}

int gPacket::getSize() const
{
    return size_var;
}

void gPacket::setSize(int size_var)
{
    this->size_var = size_var;
}

int gPacket::getRead() const
{
    return read_var;
}

void gPacket::setRead(int read_var)
{
    this->read_var = read_var;
}

int gPacket::getDecision() const
{
    return decision_var;
}

void gPacket::setDecision(int decision_var)
{
    this->decision_var = decision_var;
}

int gPacket::getApp() const
{
    return app_var;
}

void gPacket::setApp(int app_var)
{
    this->app_var = app_var;
}

class gPacketDescriptor : public cClassDescriptor
{
  public:
    gPacketDescriptor();
    virtual ~gPacketDescriptor();

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

Register_ClassDescriptor(gPacketDescriptor);

gPacketDescriptor::gPacketDescriptor() : cClassDescriptor("gPacket", "cPacket")
{
}

gPacketDescriptor::~gPacketDescriptor()
{
}

bool gPacketDescriptor::doesSupport(cObject *obj) const
{
    return dynamic_cast<gPacket *>(obj)!=NULL;
}

const char *gPacketDescriptor::getProperty(const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : NULL;
}

int gPacketDescriptor::getFieldCount(void *object) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 14+basedesc->getFieldCount(object) : 14;
}

unsigned int gPacketDescriptor::getFieldTypeFlags(void *object, int field) const
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
        case 3: return FD_ISEDITABLE;
        case 4: return FD_ISEDITABLE;
        case 5: return FD_ISEDITABLE;
        case 6: return FD_ISEDITABLE;
        case 7: return FD_ISEDITABLE;
        case 8: return FD_ISEDITABLE;
        case 9: return FD_ISEDITABLE;
        case 10: return FD_ISEDITABLE;
        case 11: return FD_ISEDITABLE;
        case 12: return FD_ISEDITABLE;
        case 13: return FD_ISEDITABLE;
        default: return 0;
    }
}

const char *gPacketDescriptor::getFieldName(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldName(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 0: return "id";
        case 1: return "risetime";
        case 2: return "submittime";
        case 3: return "arrivaltime";
        case 4: return "dispatchtime";
        case 5: return "finishtime";
        case 6: return "returntime";
        case 7: return "qos_delay";
        case 8: return "highoffset";
        case 9: return "lowoffset";
        case 10: return "size";
        case 11: return "read";
        case 12: return "decision";
        case 13: return "app";
        default: return NULL;
    }
}

const char *gPacketDescriptor::getFieldTypeString(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldTypeString(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 0: return "long";
        case 1: return "double";
        case 2: return "double";
        case 3: return "double";
        case 4: return "double";
        case 5: return "double";
        case 6: return "double";
        case 7: return "double";
        case 8: return "long";
        case 9: return "long";
        case 10: return "int";
        case 11: return "int";
        case 12: return "int";
        case 13: return "int";
        default: return NULL;
    }
}

const char *gPacketDescriptor::getFieldProperty(void *object, int field, const char *propertyname) const
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

int gPacketDescriptor::getArraySize(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getArraySize(object, field);
        field -= basedesc->getFieldCount(object);
    }
    gPacket *pp = (gPacket *)object; (void)pp;
    switch (field) {
        default: return 0;
    }
}

bool gPacketDescriptor::getFieldAsString(void *object, int field, int i, char *resultbuf, int bufsize) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldAsString(object,field,i,resultbuf,bufsize);
        field -= basedesc->getFieldCount(object);
    }
    gPacket *pp = (gPacket *)object; (void)pp;
    switch (field) {
        case 0: long2string(pp->getId(),resultbuf,bufsize); return true;
        case 1: double2string(pp->getRisetime(),resultbuf,bufsize); return true;
        case 2: double2string(pp->getSubmittime(),resultbuf,bufsize); return true;
        case 3: double2string(pp->getArrivaltime(),resultbuf,bufsize); return true;
        case 4: double2string(pp->getDispatchtime(),resultbuf,bufsize); return true;
        case 5: double2string(pp->getFinishtime(),resultbuf,bufsize); return true;
        case 6: double2string(pp->getReturntime(),resultbuf,bufsize); return true;
        case 7: double2string(pp->getQos_delay(),resultbuf,bufsize); return true;
        case 8: long2string(pp->getHighoffset(),resultbuf,bufsize); return true;
        case 9: long2string(pp->getLowoffset(),resultbuf,bufsize); return true;
        case 10: long2string(pp->getSize(),resultbuf,bufsize); return true;
        case 11: long2string(pp->getRead(),resultbuf,bufsize); return true;
        case 12: long2string(pp->getDecision(),resultbuf,bufsize); return true;
        case 13: long2string(pp->getApp(),resultbuf,bufsize); return true;
        default: return false;
    }
}

bool gPacketDescriptor::setFieldAsString(void *object, int field, int i, const char *value) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->setFieldAsString(object,field,i,value);
        field -= basedesc->getFieldCount(object);
    }
    gPacket *pp = (gPacket *)object; (void)pp;
    switch (field) {
        case 0: pp->setId(string2long(value)); return true;
        case 1: pp->setRisetime(string2double(value)); return true;
        case 2: pp->setSubmittime(string2double(value)); return true;
        case 3: pp->setArrivaltime(string2double(value)); return true;
        case 4: pp->setDispatchtime(string2double(value)); return true;
        case 5: pp->setFinishtime(string2double(value)); return true;
        case 6: pp->setReturntime(string2double(value)); return true;
        case 7: pp->setQos_delay(string2double(value)); return true;
        case 8: pp->setHighoffset(string2long(value)); return true;
        case 9: pp->setLowoffset(string2long(value)); return true;
        case 10: pp->setSize(string2long(value)); return true;
        case 11: pp->setRead(string2long(value)); return true;
        case 12: pp->setDecision(string2long(value)); return true;
        case 13: pp->setApp(string2long(value)); return true;
        default: return false;
    }
}

const char *gPacketDescriptor::getFieldStructName(void *object, int field) const
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

void *gPacketDescriptor::getFieldStructPointer(void *object, int field, int i) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldStructPointer(object, field, i);
        field -= basedesc->getFieldCount(object);
    }
    gPacket *pp = (gPacket *)object; (void)pp;
    switch (field) {
        default: return NULL;
    }
}


