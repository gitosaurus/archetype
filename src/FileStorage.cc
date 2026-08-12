//
//  FileStorage.cc
//  archetype
//
//  Created by Derek Jones on 9/2/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <filesystem>
#include <format>
#include <iostream>
#include <span>
#include <string>
#include <system_error>

#include "FileStorage.hh"
#include "Serialization.hh"
#include "Universe.hh"

using namespace std;

namespace archetype {
    InFileStorage::InFileStorage(const filesystem::path& filename)
    {
        stream_.open(filename, ios::in | ios::binary);
        stream_.seekg(0, ios::end);
        remaining_ = static_cast<int>(stream_.tellg());
        stream_.seekg(0, ios::beg);
    }

    bool InFileStorage::ok() const {
        return stream_.is_open();
    }

    int InFileStorage::remaining() const {
        return remaining_;
    }

    int InFileStorage::read(span<Byte> buf) {
        stream_.read(reinterpret_cast<char*>(buf.data()), static_cast<streamsize>(buf.size()));
        int bytes_read = static_cast<int>(stream_.gcount());
        remaining_ -= bytes_read;
        return bytes_read;
    }

    void InFileStorage::write(span<const Byte>) {
    }

    int InFileStorage::peek(span<Byte> buf) {
        int bytes_read = read(buf);
        // A short read at the end of the file sets eofbit, and seekg does
        // nothing at all while a stream is in a failed state, so the flags have
        // to come off before the position can be put back.
        stream_.clear();
        stream_.seekg(-bytes_read, ios::cur);
        remaining_ += bytes_read;
        return bytes_read;
    }

    OutFileStorage::OutFileStorage(const filesystem::path& filename):
    failed_{false}
    {
        stream_.open(filename, ios::out | ios::binary);
    }

    bool OutFileStorage::ok() const {
        return stream_.is_open() and not failed_;
    }

    bool OutFileStorage::commit() {
        if (not stream_.is_open()) {
            return false;
        }
        stream_.flush();
        stream_.close();
        // close() sets failbit if the flush it performs does not succeed, so
        // this is the last chance to notice a disk that filled up.
        if (not stream_) {
            failed_ = true;
        }
        return not failed_;
    }

    int OutFileStorage::remaining() const {
        return 0;
    }

    int OutFileStorage::read(span<Byte>) {
        return 0;
    }

    int OutFileStorage::peek(span<Byte>) {
        return 0;
    }

    void OutFileStorage::write(span<const Byte> buf) {
        stream_.write(reinterpret_cast<const char*>(buf.data()), static_cast<streamsize>(buf.size()));
        if (not stream_) {
            failed_ = true;
        }
    }

    bool writeBytesAtomically(const filesystem::path& path,
                              span<const Storage::Byte> bytes,
                              bool keep_backup,
                              string& error_out)
    {
        filesystem::path temp_path = path;
        temp_path += ".tmp";
        {
            OutFileStorage temp_file(temp_path);
            if (not temp_file.ok()) {
                error_out = format("cannot create temporary file {}", temp_path.string());
                return false;
            }
            temp_file.write(bytes);
            if (not temp_file.commit()) {
                error_out = format("failed while writing {}", temp_path.string());
                error_code ignored;
                filesystem::remove(temp_path, ignored);
                return false;
            }
        }
        error_code ec;
        if (keep_backup and filesystem::exists(path, ec)) {
            filesystem::path backup_path = path;
            backup_path += ".bak";
            // Rotate rather than copy: a crash between the two renames leaves
            // the previous state in the .bak, so nothing is ever lost.
            filesystem::rename(path, backup_path, ec);
            if (ec) {
                error_out = format("cannot rename {} to {}: {}",
                                   path.string(), backup_path.string(), ec.message());
                error_code ignored;
                filesystem::remove(temp_path, ignored);
                return false;
            }
        }
        filesystem::rename(temp_path, path, ec);
        if (ec) {
            error_out = format("cannot rename {} to {}: {}",
                               temp_path.string(), path.string(), ec.message());
            error_code ignored;
            filesystem::remove(temp_path, ignored);
            return false;
        }
        return true;
    }

    bool writeUniverseAtomically(const filesystem::path& path,
                                 bool keep_backup,
                                 string& error_out)
    {
        MemoryStorage snapshot;
        snapshot << Universe::instance();
        return writeBytesAtomically(path, snapshot.bytes(), keep_backup, error_out);
    }
}
