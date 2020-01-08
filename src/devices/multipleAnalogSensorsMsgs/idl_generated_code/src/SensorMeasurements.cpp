/*
 * Copyright (C) 2006-2020 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */

// Autogenerated by Thrift Compiler (0.12.0-yarped)
//
// This is an automatically generated file.
// It could get re-generated if the ALLOW_IDL_GENERATION flag is on.

#include <SensorMeasurements.h>

// Default constructor
SensorMeasurements::SensorMeasurements() :
        WirePortable(),
        measurements()
{
}

// Constructor with field values
SensorMeasurements::SensorMeasurements(const std::vector<SensorMeasurement>& measurements) :
        WirePortable(),
        measurements(measurements)
{
}

// Read structure on a Wire
bool SensorMeasurements::read(yarp::os::idl::WireReader& reader)
{
    if (!read_measurements(reader)) {
        return false;
    }
    return !reader.isError();
}

// Read structure on a Connection
bool SensorMeasurements::read(yarp::os::ConnectionReader& connection)
{
    yarp::os::idl::WireReader reader(connection);
    if (!reader.readListHeader(1)) {
        return false;
    }
    return read(reader);
}

// Write structure on a Wire
bool SensorMeasurements::write(const yarp::os::idl::WireWriter& writer) const
{
    if (!write_measurements(writer)) {
        return false;
    }
    return !writer.isError();
}

// Write structure on a Connection
bool SensorMeasurements::write(yarp::os::ConnectionWriter& connection) const
{
    yarp::os::idl::WireWriter writer(connection);
    if (!writer.writeListHeader(1)) {
        return false;
    }
    return write(writer);
}

// Convert to a printable string
std::string SensorMeasurements::toString() const
{
    yarp::os::Bottle b;
    b.read(*this);
    return b.toString();
}

// Editor: default constructor
SensorMeasurements::Editor::Editor()
{
    group = 0;
    obj_owned = true;
    obj = new SensorMeasurements;
    dirty_flags(false);
    yarp().setOwner(*this);
}

// Editor: constructor with base class
SensorMeasurements::Editor::Editor(SensorMeasurements& obj)
{
    group = 0;
    obj_owned = false;
    edit(obj, false);
    yarp().setOwner(*this);
}

// Editor: destructor
SensorMeasurements::Editor::~Editor()
{
    if (obj_owned) {
        delete obj;
    }
}

// Editor: edit
bool SensorMeasurements::Editor::edit(SensorMeasurements& obj, bool dirty)
{
    if (obj_owned) {
        delete this->obj;
    }
    this->obj = &obj;
    obj_owned = false;
    dirty_flags(dirty);
    return true;
}

// Editor: validity check
bool SensorMeasurements::Editor::isValid() const
{
    return obj != nullptr;
}

// Editor: state
SensorMeasurements& SensorMeasurements::Editor::state()
{
    return *obj;
}

// Editor: grouping begin
void SensorMeasurements::Editor::start_editing()
{
    group++;
}

// Editor: grouping end
void SensorMeasurements::Editor::stop_editing()
{
    group--;
    if (group == 0 && is_dirty) {
        communicate();
    }
}
// Editor: measurements setter
void SensorMeasurements::Editor::set_measurements(const std::vector<SensorMeasurement>& measurements)
{
    will_set_measurements();
    obj->measurements = measurements;
    mark_dirty_measurements();
    communicate();
    did_set_measurements();
}

// Editor: measurements setter (list)
void SensorMeasurements::Editor::set_measurements(size_t index, const SensorMeasurement& elem)
{
    will_set_measurements();
    obj->measurements[index] = elem;
    mark_dirty_measurements();
    communicate();
    did_set_measurements();
}

// Editor: measurements getter
const std::vector<SensorMeasurement>& SensorMeasurements::Editor::get_measurements() const
{
    return obj->measurements;
}

// Editor: measurements will_set
bool SensorMeasurements::Editor::will_set_measurements()
{
    return true;
}

// Editor: measurements did_set
bool SensorMeasurements::Editor::did_set_measurements()
{
    return true;
}

// Editor: clean
void SensorMeasurements::Editor::clean()
{
    dirty_flags(false);
}

// Editor: read
bool SensorMeasurements::Editor::read(yarp::os::ConnectionReader& connection)
{
    if (!isValid()) {
        return false;
    }
    yarp::os::idl::WireReader reader(connection);
    reader.expectAccept();
    if (!reader.readListHeader()) {
        return false;
    }
    int len = reader.getLength();
    if (len == 0) {
        yarp::os::idl::WireWriter writer(reader);
        if (writer.isNull()) {
            return true;
        }
        if (!writer.writeListHeader(1)) {
            return false;
        }
        writer.writeString("send: 'help' or 'patch (param1 val1) (param2 val2)'");
        return true;
    }
    std::string tag;
    if (!reader.readString(tag)) {
        return false;
    }
    if (tag == "help") {
        yarp::os::idl::WireWriter writer(reader);
        if (writer.isNull()) {
            return true;
        }
        if (!writer.writeListHeader(2)) {
            return false;
        }
        if (!writer.writeTag("many", 1, 0)) {
            return false;
        }
        if (reader.getLength() > 0) {
            std::string field;
            if (!reader.readString(field)) {
                return false;
            }
            if (field == "measurements") {
                if (!writer.writeListHeader(1)) {
                    return false;
                }
                if (!writer.writeString("std::vector<SensorMeasurement> measurements")) {
                    return false;
                }
            }
        }
        if (!writer.writeListHeader(2)) {
            return false;
        }
        writer.writeString("*** Available fields:");
        writer.writeString("measurements");
        return true;
    }
    bool nested = true;
    bool have_act = false;
    if (tag != "patch") {
        if (((len - 1) % 2) != 0) {
            return false;
        }
        len = 1 + ((len - 1) / 2);
        nested = false;
        have_act = true;
    }
    for (int i = 1; i < len; ++i) {
        if (nested && !reader.readListHeader(3)) {
            return false;
        }
        std::string act;
        std::string key;
        if (have_act) {
            act = tag;
        } else if (!reader.readString(act)) {
            return false;
        }
        if (!reader.readString(key)) {
            return false;
        }
        if (key == "measurements") {
            will_set_measurements();
            if (!obj->nested_read_measurements(reader)) {
                return false;
            }
            did_set_measurements();
        } else {
            // would be useful to have a fallback here
        }
    }
    reader.accept();
    yarp::os::idl::WireWriter writer(reader);
    if (writer.isNull()) {
        return true;
    }
    writer.writeListHeader(1);
    writer.writeVocab(yarp::os::createVocab('o', 'k'));
    return true;
}

// Editor: write
bool SensorMeasurements::Editor::write(yarp::os::ConnectionWriter& connection) const
{
    if (!isValid()) {
        return false;
    }
    yarp::os::idl::WireWriter writer(connection);
    if (!writer.writeListHeader(dirty_count + 1)) {
        return false;
    }
    if (!writer.writeString("patch")) {
        return false;
    }
    if (is_dirty_measurements) {
        if (!writer.writeListHeader(3)) {
            return false;
        }
        if (!writer.writeString("set")) {
            return false;
        }
        if (!writer.writeString("measurements")) {
            return false;
        }
        if (!obj->nested_write_measurements(writer)) {
            return false;
        }
    }
    return !writer.isError();
}

// Editor: send if possible
void SensorMeasurements::Editor::communicate()
{
    if (group != 0) {
        return;
    }
    if (yarp().canWrite()) {
        yarp().write(*this);
        clean();
    }
}

// Editor: mark dirty overall
void SensorMeasurements::Editor::mark_dirty()
{
    is_dirty = true;
}

// Editor: measurements mark_dirty
void SensorMeasurements::Editor::mark_dirty_measurements()
{
    if (is_dirty_measurements) {
        return;
    }
    dirty_count++;
    is_dirty_measurements = true;
    mark_dirty();
}

// Editor: dirty_flags
void SensorMeasurements::Editor::dirty_flags(bool flag)
{
    is_dirty = flag;
    is_dirty_measurements = flag;
    dirty_count = flag ? 1 : 0;
}

// read measurements field
bool SensorMeasurements::read_measurements(yarp::os::idl::WireReader& reader)
{
    measurements.clear();
    uint32_t _size0;
    yarp::os::idl::WireState _etype3;
    reader.readListBegin(_etype3, _size0);
    measurements.resize(_size0);
    for (size_t _i4 = 0; _i4 < _size0; ++_i4) {
        if (!reader.readNested(measurements[_i4])) {
            reader.fail();
            return false;
        }
    }
    reader.readListEnd();
    return true;
}

// write measurements field
bool SensorMeasurements::write_measurements(const yarp::os::idl::WireWriter& writer) const
{
    if (!writer.writeListBegin(BOTTLE_TAG_LIST, static_cast<uint32_t>(measurements.size()))) {
        return false;
    }
    for (const auto& _item5 : measurements) {
        if (!writer.writeNested(_item5)) {
            return false;
        }
    }
    if (!writer.writeListEnd()) {
        return false;
    }
    return true;
}

// read (nested) measurements field
bool SensorMeasurements::nested_read_measurements(yarp::os::idl::WireReader& reader)
{
    measurements.clear();
    uint32_t _size6;
    yarp::os::idl::WireState _etype9;
    reader.readListBegin(_etype9, _size6);
    measurements.resize(_size6);
    for (size_t _i10 = 0; _i10 < _size6; ++_i10) {
        if (!reader.readNested(measurements[_i10])) {
            reader.fail();
            return false;
        }
    }
    reader.readListEnd();
    return true;
}

// write (nested) measurements field
bool SensorMeasurements::nested_write_measurements(const yarp::os::idl::WireWriter& writer) const
{
    if (!writer.writeListBegin(BOTTLE_TAG_LIST, static_cast<uint32_t>(measurements.size()))) {
        return false;
    }
    for (const auto& _item11 : measurements) {
        if (!writer.writeNested(_item11)) {
            return false;
        }
    }
    if (!writer.writeListEnd()) {
        return false;
    }
    return true;
}
