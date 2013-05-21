// Copyright (C) 2012  Internet Systems Consortium, Inc. ("ISC")
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
// OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <datasrc/memory/zone_writer.h>
#include <datasrc/memory/zone_data.h>
#include <datasrc/memory/zone_table_segment.h>

#include <datasrc/exceptions.h>

#include <memory>

using std::auto_ptr;

namespace isc {
namespace datasrc {
namespace memory {

ZoneWriter::ZoneWriter(ZoneTableSegment& segment,
                       const LoadAction& load_action,
                       const dns::Name& origin,
                       const dns::RRClass& rrclass,
                       bool throw_on_load_error) :
    segment_(segment),
    load_action_(load_action),
    origin_(origin),
    rrclass_(rrclass),
    zone_data_(NULL),
    state_(ZW_UNUSED),
    catch_load_error_(throw_on_load_error)
{
    if (!segment.isWritable()) {
        isc_throw(isc::InvalidOperation,
                  "Attempt to construct ZoneWriter for a read-only segment");
    }
}

ZoneWriter::~ZoneWriter() {
    // Clean up everything there might be left if someone forgot, just
    // in case.
    cleanup();
}

void
ZoneWriter::load(std::string* error_msg) {
    if (state_ != ZW_UNUSED) {
        isc_throw(isc::InvalidOperation, "Trying to load twice");
    }

    try {
        zone_data_ = load_action_(segment_.getMemorySegment());
        segment_.resetHeader();

        if (!zone_data_) {
            // Bug inside load_action_.
            isc_throw(isc::InvalidOperation,
                      "No data returned from load action");
        }
    } catch (const ZoneLoaderException& ex) {
        if (!catch_load_error_) {
            throw;
        }
        if (error_msg) {
            *error_msg = ex.what();
        }
        segment_.resetHeader();
    }

    state_ = ZW_LOADED;
}

void
ZoneWriter::install() {
    if (state_ != ZW_LOADED) {
        isc_throw(isc::InvalidOperation, "No data to install");
    }

    // Check the internal integrity assumption: we should have non NULL
    // zone data or we've allowed load error to create an empty zone.
    assert(zone_data_ || catch_load_error_);

    // FIXME: This retry is currently untested, as there seems to be no
    // reasonable way to create a zone table segment with non-local memory
    // segment. Once there is, we should provide the test.
    while (state_ != ZW_INSTALLED) {
        try {
            ZoneTable* table(segment_.getHeader().getTable());
            if (table == NULL) {
                isc_throw(isc::Unexpected, "No zone table present");
            }
            const ZoneTable::AddResult result(
                zone_data_ ? table->addZone(segment_.getMemorySegment(),
                                            rrclass_, origin_, zone_data_) :
                table->addEmptyZone(segment_.getMemorySegment(), origin_));
            state_ = ZW_INSTALLED;
            zone_data_ = result.zone_data;
        } catch (const isc::util::MemorySegmentGrown&) {}

        segment_.resetHeader();
    }
}

void
ZoneWriter::cleanup() {
    // We eat the data (if any) now.

    if (zone_data_ != NULL) {
        ZoneData::destroy(segment_.getMemorySegment(), zone_data_, rrclass_);
        zone_data_ = NULL;
        state_ = ZW_CLEANED;
    }
}

}
}
}
